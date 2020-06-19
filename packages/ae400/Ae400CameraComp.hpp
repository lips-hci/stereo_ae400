/*
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/
#pragma once

#include <memory>
#include <string>

#include "engine/alice/alice_codelet.hpp"
#include "engine/core/math/types.hpp"
#include "librealsense2/rs.hpp"
#include "messages/camera.capnp.h"

namespace isaac {
namespace lips {

// AE400Camera is an Isaac codelet for the LIPSedge™ AE400 Industrial 3D Camera
// powered by Intel® RealSense™ Technology and designed for industrial applications.
// It provides color and depth images. The sensor can also provide raw IR images.
//
// Please visit product page for more information and support.
// https://www.lips-hci.com/product-page/lipsedge-ae400-industrial-3d-camera
//
// The supported FPS and Resolutions of each camera are:
// Stereo Camera | Color(RGB) Camera
//  - N/A        | - 1920x1080
//  - 1280x720   | - 1280x720
//  - 960x540    | - 960x540
//  - 848x480    | - 848x480
//  - 640x480    | - 640x480
//  - 640x360    | - 640x360
//  - 480x270    | - N/A
//  - 424x240    | - 424x240
//  - N/A        | - 320x240
//  - N/A        | - 320x180
//
// Valid framerate for the color image are 60, 30, 15, 6 FPS. Valid framerate for the depth image
// are 90, 60, 30, 15, 6 FPS. The camera can also produce images at 1920x1080, however this is currently
// not supported as color and depth are set to the same resolution.
class AE400Camera : public alice::Codelet {
 public:
  AE400Camera();
  ~AE400Camera();

  void start() override;
  void tick() override;
  void stop() override;

  // The left IR camera image and intrinsics
  ISAAC_PROTO_TX(ColorCameraProto, left_ir);
  // The right IR camera image and intrinsics
  ISAAC_PROTO_TX(ColorCameraProto, right_ir);
  // The color camera image, which can be of type Image3ub for color or Image1ui16 for grayscale.
  ISAAC_PROTO_TX(ColorCameraProto, color);
  // The depth image (in meters) in the left IR camera frame
  ISAAC_PROTO_TX(DepthCameraProto, depth);
  // IR stereo camera extrinsics (the right_T_left IR camera transformation).
  // The camera extrinsics doesn't change with time.
  ISAAC_POSE3(left_ir_camera, right_ir_camera);
  // The vertical resolution for both color and depth images.
  ISAAC_PARAM(int, rows, 360);
  // The horizontal resolution for both color and depth images.
  ISAAC_PARAM(int, cols, 640);
  // The framerate of the left and right IR sensors. Valid values are 90, 60, 30, 25, 15, 6.
  // It should match the depth map framerate due to the RS 415 firmware constraints.
  ISAAC_PARAM(int, ir_framerate, 30);
  // The framerate of the RGB camera acquisition. Valid values are 60, 30, 15, 6.
  ISAAC_PARAM(int, color_framerate, 30);
  // The framerate of the depth map generation. Valid values are 90, 60, 30, 15, 6.
  // It should match the IR framerate due to the RS 415 firmware constraints.
  ISAAC_PARAM(int, depth_framerate, 30);
  // If enabled, the depth image is spatially aligned to the color image to provide matching color
  // and depth values for every pixel. This is a CPU-intensive process and can reduce frame rates.
  ISAAC_PARAM(bool, align_to_color, true);
  // Max number of frames you can hold at a given time. Increasing this number reduces frame
  // drops but increase latency, and vice versa; ranges from 0 to 32.
  ISAAC_PARAM(int, frame_queue_size, 2);
  // Limit exposure time when auto-exposure is ON to preserve a constant FPS rate.
  ISAAC_PARAM(bool, auto_exposure_priority, false);
  // Amount of power used by the depth laser, in mW. Valid values are between 0 and 360, in
  // increments of 30.
  ISAAC_PARAM(int, laser_power, 150);
  // Enable acquisition and publication of the IR stereo pair.
  // This setting can't be changed at runtime.
  ISAAC_PARAM(bool, enable_ir_stereo, false);
  // Enable acquisition and publication of the color frames.
  // This setting can't be changed at runtime.
  ISAAC_PARAM(bool, enable_color, true);
  // Enable depth map computation and publication. This setting can't be changed at runtime.
  ISAAC_PARAM(bool, enable_depth, true);
  // Enable the depth laser projector to improve the depth image accuracy.
  // Disabling it helps the visual odometry tracker by removing the dot pattern
  // from the IR stereo pair. This setting can't be changed at runtime.
  ISAAC_PARAM(bool, enable_depth_laser, true);
  // Enable auto exposure. Disabling it can reduce motion blur
  ISAAC_PARAM(bool, enable_auto_exposure, true);
  // The index of the Realsense device in the list of devices detected. This indexing is dependent
  // on the order the Realsense library detects the cameras, and may vary based on mounting order.
  // By default the first camera device in the list is chosen. This camera choice can be overridden
  // by the serial number parameter below.
  ISAAC_PARAM(int, dev_index, 0)
  // An alternative way to specify the desired device in a multicamera setup. The serial number of
  // the Realsense camera can be found printed on the device. If specified, this parameter will take
  // precedence over the dev_index parameter above.
  ISAAC_PARAM(std::string, serial_number, "")

 private:
  struct TimeStampInfo;
  struct Impl;

  // Inital configuration of a realsense device
  void initializeDeviceConfig(const rs2::device& dev);

  // Update configuration of a realsense device
  void updateDeviceConfig(const rs2::device& dev);

  // The function returns the rs2::video_frame timestamp in nanoseconds adjusted to the difference
  // between the ISAAC and RS clocks.
  // The expectation is that the RS ISP uses the same variable time epoch for all frame timestamps.
  int64_t getAdjustedTimeStamp(int64_t camera_timestamp, TimeStampInfo& tsInfo);

  std::unique_ptr<Impl> impl_;
};

}  // namespace lips
}  // namespace isaac

ISAAC_ALICE_REGISTER_CODELET(isaac::lips::AE400Camera);
