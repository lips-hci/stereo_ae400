/*
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/
/*
Copyright (c) 2020, LIPS CORPORATION. All rights reserved.

Modify this driver to support LIPS AE400 Stereo Camera
*/
#include "Ae400CameraComp.hpp"

#include <memory>
#include <string>
#include <utility>

#include "engine/gems/geometry/pinhole.hpp"
#include "engine/gems/image/conversions.hpp"
#include "engine/gems/image/utils.hpp"
#include "messages/camera.hpp"

using namespace lips::ae400;

namespace isaac {

namespace lips {

// librealsense2 stream ids
const int kLeftIrStreamId = 1;
const int kRightIrStreamId = 2;
const int64_t kIrMaxDeltaTs = SecondsToNano(1.);

// Check the current firmware version vs. the recommended firmware version and
// log a warning if they do not match
void LogWarningIfNotRecommendedFirmwareVersion(const rs2::device& dev) {
  const std::string current = dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION);
  const std::string recommended = dev.get_info(RS2_CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION);

  // if firmware version is different from recommended version, log recommended version
  if (current != recommended) {
    LOG_WARNING(
        "Realsense recommended firmware version is %s, "
        "currently using firmware version %s",
        recommended.c_str(), current.c_str());
  }
}

// Set a sensor option
void SetSensorOption(const rs2_option& option, float value, rs2::sensor& sensor) {
  if (!sensor.supports(option)) {
    // we try to keep the sensors in sync, so it is normal that not all sensors
    // will support all options
    return;
  }

  try {
    // set the option
    sensor.set_option(option, value);
  } catch (const rs2::error& e) {
    // let the user know something went wrong
    const std::string sensorName = sensor.get_info(RS2_CAMERA_INFO_NAME);
    const std::string optionLabel = rs2_option_to_string(option);
    const rs2::option_range range = sensor.get_option_range(option);
    LOG_WARNING("Failed to set '%s' option '%s' to %f (min:%f, max:%f, default:%f, step:%f)",
                sensorName.c_str(), optionLabel.c_str(), value, range.min, range.max, range.def,
                range.step);
  }
}

// Converts a rs2::video_frame into a Image3ub
Image3ub ToColorImage(const rs2::video_frame& frame) {
    CpuBufferConstView image_buffer(reinterpret_cast<const byte*>(frame.get_data()),
                                    frame.get_height() * frame.get_stride_in_bytes());
    ImageConstView3ub color_image_view(image_buffer, frame.get_height(),
                                       frame.get_width());
    Image3ub color_image(color_image_view.dimensions());
    Copy(color_image_view, color_image);
    return color_image;
}

// Converts a rs2::video_frame into a Image1ub
Image1ub ToGreyImage(const rs2::video_frame& frame) {
    CpuBufferConstView image_buffer(reinterpret_cast<const byte*>(frame.get_data()),
                                    frame.get_height() * frame.get_stride_in_bytes());
    ImageConstView1ub grey_image_view(image_buffer, frame.get_height(), frame.get_width());
    Image1ub grey_image(grey_image_view.dimensions());
    Copy(grey_image_view, grey_image);
    return grey_image;
}

// Extracts camera intrinsics from a rs2::video_frame and converts them into a geometry::PinholeD
geometry::PinholeD ToPinhole(const rs2::video_frame& frame) {
  const auto intrinsics = frame.get_profile().as<rs2::video_stream_profile>().get_intrinsics();
  return geometry::PinholeD{
      Vector2i{intrinsics.height, intrinsics.width},
      Vector2d{intrinsics.fy, intrinsics.fx},
      Vector2d{intrinsics.ppy, intrinsics.ppx}};
}

// Converts rs2_extrinsics into a Pose3d
Pose3d ToPose(const rs2_extrinsics& extrinsics) {
  Matrix3f rotation;
  // Column-major 3x3 rotation matrix
  rotation << extrinsics.rotation[0], extrinsics.rotation[3], extrinsics.rotation[6],
              extrinsics.rotation[1], extrinsics.rotation[4], extrinsics.rotation[7],
              extrinsics.rotation[2], extrinsics.rotation[5], extrinsics.rotation[8];
  Quaternionf quaternion(rotation);
  Pose3f pose;
  pose.rotation = SO3f::FromQuaternion(quaternion);
  // Three-element translation vector, in meters
  pose.translation =
      Vector3f(extrinsics.translation[0], extrinsics.translation[1], extrinsics.translation[2]);
  return pose.cast<double>();
}

AE400Camera::AE400Camera() {}
AE400Camera::~AE400Camera() {}

// Stores the last timestamp reported by librealsense for a frame
struct AE400Camera::TimeStampInfo {
  // The difference between consecutive frame timestamps must be < -1s.
  int64_t last_ts = -kIrMaxDeltaTs-1;  // -1 * (1 second + 1 nanosecond)
  // The delta between Realsense and Isaac timestamps that could change at runtime.
  // The expectation is that the RS ISP uses same variable time epoch for all frame timestamps.
  int64_t frame_delta_time = 0;
};

// RealSense stream type
enum StreamType {
  kNone = 0,
  kColor = 1,
  kIr = 2,
  kDepth = 4
};

// Stores various Realsense options
struct AE400Camera::Impl {
  rs2::pipeline pipe;  // used to start the pipeline and wait for frames
  rs2::pipeline_profile profile;  // used to access Ir streams during the pipeline execution
  rs2::align align_to = rs2::align(RS2_STREAM_COLOR);  // align color and depth?
  rs2::device dev;                                     // Realsense device
  int active_streams = kNone;                          // streams enabled in the pipeline
  TimeStampInfo timestamp_info;                        // timestamp info for color and depth frames
  TimeStampInfo ir_timestamp;                          // timestamp info for IR frames
  bool auto_exposure_enabled;   // control auto exposure
  rs2::rates_printer printer;   // Declare rates printer for showing streaming rates
};

// The function returns the rs2::video_frame timestamp in nanoseconds adjusted to the difference
// between the ISAAC and RS clocks.
// The expectation is that the RS ISP uses the same variable time epoch for all frame timestamps.
int64_t AE400Camera::getAdjustedTimeStamp(int64_t camera_timestamp, TimeStampInfo& tsInfo) {
  const int64_t current_ts = static_cast<int64_t>(camera_timestamp * 1e6);
  const int64_t delta_t = current_ts - tsInfo.last_ts;
  tsInfo.last_ts = current_ts;
  // The RealSense camera ISP sometimes switches the frame timestamp epoch at runtime, so the delta
  // between the RS frame and Isaac timestamps should be adjusted in this case.
  if (0 > delta_t || kIrMaxDeltaTs < delta_t) {
    const int64_t isaac_ts{node()->clock()->timestamp()};
    tsInfo.frame_delta_time = isaac_ts - current_ts;
  }
  const int64_t acqtime = current_ts + tsInfo.frame_delta_time;
  return acqtime;
}

void AE400Camera::start() {
  try {
    impl_ = std::make_unique<Impl>();

    // get a list of realsense devices connected
    rs2::context ctx;
    rs2::device_list devices = ctx.query_devices();
    int num_devices = devices.size();

    // are any devices connected?
    if (num_devices == 0) {
      reportFailure("No device connected, please connect a RealSense device");
      return;
    }

    // Get the desired device using either the serial number or device index
    std::string serial_number;
    if (get_serial_number() != "") {
      serial_number = get_serial_number();

    // Go through each connected device, check the firmware, and configure
      for (const rs2::device& device : devices) {
        if (device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) == serial_number) {
          impl_->dev = device;
          break;
        }
      }
      if (!impl_->dev) {
        reportFailure("Device with serial number %s not found.", serial_number.c_str());
      }
    } else {
      // Check that the device index is valid
      if (get_dev_index() < 0 || get_dev_index() >= num_devices) {
        reportFailure("Please specify a valid device index between 0 and %d", num_devices - 1);
      }

      impl_->dev = devices[get_dev_index()];
      serial_number = impl_->dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
      set_serial_number(serial_number);
    }

    // Check the firmware, and configure
    LogWarningIfNotRecommendedFirmwareVersion(impl_->dev);
    initializeDeviceConfig(impl_->dev);

    // configure the pipeline, enable Ir, Depth and Color streams
    // The frame rate values set for IR and depth map streams should match.
    // It's the RS firmware limitation as the depth map is reconstructed from an IR stereo pair.
    // The color sensor framerate could be different from an IR sensor pair framerate.
    rs2::config cfg;
    cfg.enable_device(impl_->dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
    if (get_enable_ir_stereo()) {
      impl_->active_streams |= StreamType::kIr;
      cfg.enable_stream(RS2_STREAM_INFRARED, kLeftIrStreamId, get_cols(), get_rows(),
                        RS2_FORMAT_Y8, get_ir_framerate());
      cfg.enable_stream(RS2_STREAM_INFRARED, kRightIrStreamId, get_cols(), get_rows(),
                        RS2_FORMAT_Y8, get_ir_framerate());
    }
    if (get_enable_depth()) {
      impl_->active_streams |= StreamType::kDepth;
      cfg.enable_stream(RS2_STREAM_DEPTH, get_cols(), get_rows(), RS2_FORMAT_Z16,
                        get_depth_framerate());
    }
    if (get_enable_color()) {
      impl_->active_streams |= StreamType::kColor;
      cfg.enable_stream(RS2_STREAM_COLOR, get_cols(), get_rows(), RS2_FORMAT_RGB8,
                        get_color_framerate());
    }

    // start the pipeline
    impl_->profile = impl_->pipe.start(cfg);
  } catch (const rs2::error& e) {
    reportFailure("@%d RealSense error calling %s(%s): %s", __LINE__, e.get_failed_function().c_str(),
                  e.get_failed_args().c_str(), e.what());
    return;
  }

  // force settings into a state where they will update
  impl_->auto_exposure_enabled = !get_enable_auto_exposure();

  if (get_enable_ir_stereo()) {
    // Obtain and publish the fixed right-to-left IR camera tansform into the Pose Tree
    auto left_stream = impl_->profile.get_stream(RS2_STREAM_INFRARED, kLeftIrStreamId);
    auto right_stream = impl_->profile.get_stream(RS2_STREAM_INFRARED, kRightIrStreamId);
    const rs2_extrinsics extrinsics = right_stream.get_extrinsics_to(left_stream);
    // Camera extrinsics doesn't change with time
    set_left_ir_camera_T_right_ir_camera(ToPose(extrinsics), 0.0);
  }

  // Update device settings, now that the camera is started
  updateDeviceConfig(impl_->dev);

  tickBlocking();
}

void AE400Camera::tick() {
  try {
    //
    // HACK: comment updateDeviceConfig line below to improve performance,
    // but we can not adjust auto exposure dynamically
    //
    // check device settings, and update as needed
    updateDeviceConfig(impl_->dev);

    // wait for new frames
    // All published RealSense frames are rectified so distortion parameters are all 0
    rs2::frameset frames = impl_->pipe.wait_for_frames();

    if (get_rates_printer()) {
      frames.apply_filter(impl_->printer);
    }

    if (get_align_to_color()) {
      // spatially align the images
      frames = frames.apply_filter(impl_->align_to);
    }

    // The acqtime is calculated later as a camera frame timestamp + dT between
    // the Isaac and camera clocks. dT is variable as RS clock epoch changes at runtime.
    // The same acqtime is used for color and depth frames as a few codelets synchronize
    // messages between the depth and color channels, like DepthImageToPointCloud
    int64_t acqtime = 0;

    const bool color_on = impl_->active_streams & StreamType::kColor;
    const bool ir_on = impl_->active_streams & StreamType::kIr;
    const bool depth_on = impl_->active_streams & StreamType::kDepth;

    // color image
    if (color_on) {
      const rs2::video_frame color_frame = frames.get_color_frame();
      if (acqtime == 0) {
        acqtime = getAdjustedTimeStamp(color_frame.get_timestamp(), impl_->timestamp_info);
      }
      ToProto(std::move(ToColorImage(color_frame)), tx_color().initProto(), tx_color().buffers());
      ToProto(ToPinhole(color_frame), tx_color_intrinsics().initProto().initPinhole());
    }

    if (ir_on) {
      // Obtain the left ir frame
      const rs2::video_frame left_frame = frames.get_infrared_frame(kLeftIrStreamId);
      ToProto(std::move(ToGreyImage(left_frame)), tx_left_ir().initProto(), tx_left_ir().buffers());
      ToProto(ToPinhole(left_frame), tx_left_ir_intrinsics().initProto().initPinhole());

      // Obtain the right ir frame
      const rs2::video_frame right_frame = frames.get_infrared_frame(kRightIrStreamId);
      ToProto(std::move(ToGreyImage(right_frame)), tx_right_ir().initProto(),
              tx_right_ir().buffers());
      ToProto(ToPinhole(right_frame), tx_right_ir_intrinsics().initProto().initPinhole());

      // SVIO tracker needs to recieve the actual IR frame timestamps as a hint
      // for the prediction algorithm to understand the temporal relationship
      // between the current and previous frames.
      // The same timestamp is used for the left and right IR frames
      const int64_t ir_acqtime =
         getAdjustedTimeStamp(left_frame.get_timestamp(), impl_->ir_timestamp);
      tx_left_ir().publish(ir_acqtime);
      tx_left_ir_intrinsics().publish(ir_acqtime);
      tx_right_ir().publish(ir_acqtime);
      tx_right_ir_intrinsics().publish(ir_acqtime);
    }

    // Obtain and publish the depth image
    if (depth_on) {
      const rs2::depth_frame depth_frame = frames.get_depth_frame();
      if (acqtime == 0) {
        acqtime = getAdjustedTimeStamp(depth_frame.get_timestamp(), impl_->timestamp_info);
      }

      CpuBufferConstView depth_buffer(reinterpret_cast<const byte*>(depth_frame.get_data()),
                                      depth_frame.get_height() * depth_frame.get_stride_in_bytes());
      ImageConstView1ui16 depth_image_view(depth_buffer, depth_frame.get_height(),
                                          depth_frame.get_width());
      Image1f depth_image(depth_image_view.dimensions());
      ConvertUi16ToF32(depth_image_view, depth_image, 0.001);

      ToProto(ToPinhole(depth_frame), tx_depth_intrinsics().initProto().initPinhole());
      ToProto(std::move(depth_image), tx_depth().initProto(), tx_depth().buffers());
      tx_depth().publish(acqtime);
      tx_depth_intrinsics().publish(acqtime);
    }

    if (color_on) {
      tx_color().publish(acqtime);
      tx_color_intrinsics().publish(acqtime);
    }

    if(get_enable_imu()) {
      lips_ae400_imu imu_data = {0};
      if ( get_imu_data(0, &imu_data) == 0)
      {
        auto imu_datamsg = tx_imu_raw().initProto();
        double imu_acqtime = imu_data.timestamp * 0.001; //ms converts to sec

        // set accelerometer data
        imu_datamsg.setLinearAccelerationX(imu_data.accel_x);
        imu_datamsg.setLinearAccelerationY(imu_data.accel_y);
        imu_datamsg.setLinearAccelerationZ(imu_data.accel_z);

        // set gyroscope data
        imu_datamsg.setAngularVelocityX(imu_data.gyro_x);
        imu_datamsg.setAngularVelocityY(imu_data.gyro_y);
        imu_datamsg.setAngularVelocityZ(imu_data.gyro_z);

        tx_imu_raw().publish(imu_acqtime);
      }
    }
  } catch (const rs2::error& e) {
    reportFailure("@%d RealSense error calling %s(%s): %s", __LINE__, e.get_failed_function().c_str(),
                  e.get_failed_args().c_str(), e.what());
  }
}

void AE400Camera::stop() {
  try {
    // The pipeline should be stopped only if started. The profile is nullptr before pipe->start()
    if (impl_ && impl_->profile) {
      impl_->pipe.stop();
    }
    impl_.reset();
  } catch (const rs2::error& e) {
    reportFailure("@%d RealSense error calling %s(%s): %s", __LINE__, e.get_failed_function().c_str(),
                  e.get_failed_args().c_str(), e.what());
  }
}

// At the codelet startup, applies the default camera settings to RS ISP
void AE400Camera::initializeDeviceConfig(const rs2::device& dev) {
  // NOTE: this method is called before the pipeline is started, and not all
  // options can be configured before the sensor starts.
  for (auto& sensor : dev.query_sensors()) {
    SetSensorOption(RS2_OPTION_FRAMES_QUEUE_SIZE, get_frame_queue_size(), sensor);
    SetSensorOption(RS2_OPTION_AUTO_EXPOSURE_PRIORITY, get_auto_exposure_priority(), sensor);
    const bool enable_depth_laser = get_enable_depth_laser();
    SetSensorOption(RS2_OPTION_EMITTER_ENABLED, enable_depth_laser, sensor);
    if (enable_depth_laser) {
      SetSensorOption(RS2_OPTION_LASER_POWER, get_laser_power(), sensor);
    }
  }
}

// Applies the current user-selected camera settings to RS ISP
void AE400Camera::updateDeviceConfig(const rs2::device& dev) {
  for (auto& sensor : dev.query_sensors()) {
    if (impl_->auto_exposure_enabled != get_enable_auto_exposure()) {
      SetSensorOption(RS2_OPTION_ENABLE_AUTO_EXPOSURE, get_enable_auto_exposure(), sensor);
    }
  }

  impl_->auto_exposure_enabled = get_enable_auto_exposure();
}

}  // namespace lips
}  // namespace isaac
