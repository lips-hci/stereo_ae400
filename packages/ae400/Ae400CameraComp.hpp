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

#include "engine/alice/alice_codelet.hpp"
#include "engine/core/math/types.hpp"
#include "librealsense2/rs.hpp"
#include "messages/messages.hpp"

namespace isaac {

// RealsenseCamera is an Isaac codelet for the Realsense D435 camera that provides color and
// depth images. The sensor can also provide raw IR images, however this is currently not supported.
class RealsenseCamera : public alice::Codelet {
 public:
  RealsenseCamera();
  ~RealsenseCamera();

  void start() override;
  void tick() override;
  void stop() override;

  // A color camera image that can be Image3ub(for color) or Image1ui16 (for grayscale.)
  ISAAC_PROTO_TX(ColorCameraProto, color);
  // Depth image (in meters). This is in left Ir camera frame
  ISAAC_PROTO_TX(DepthCameraProto, depth);

  // The resolution of captured images. Valid choices are:
  //
  //  - 1280x720 (at most 30 Hz)
  //  - 848x480
  //  - 640x480
  //  - 640x360
  //  - 424x240
  //
  // The camera can also produce images also at 1920x1080, however this is currently
  // not supported as color and depth are set to the same resolution.
  // Number of pixels in the height dimension
  ISAAC_PARAM(int, rows, 360);
  // Number of pixels in the width dimension
  ISAAC_PARAM(int, cols, 640);
  // The framerate of the RGB camera acquisition. Valid choices are: 60, 30, 15, 6.
  ISAAC_PARAM(int, rgb_framerate, 30);
  // The framerate of the depth camera acquisition. Valid choices are: 90, 60, 30, 15, 6.
  ISAAC_PARAM(int, depth_framerate, 30);
  // If enabled, the depth image is spatially aligned to the color image to provide matching color
  // and depth values for every pixel. This is a CPU-intensive process and can reduce frame rates.
  ISAAC_PARAM(bool, align_to_color, true);
  // Max number of frames you can hold at a given time. Increasing this number reduces frame
  // drops but increase latency, and vice versa; ranges from 0 to 32.
  ISAAC_PARAM(int, frame_queue_size, 2);
  // Limit exposure time when auto-exposure is ON to preserve constant fps rate.
  ISAAC_PARAM(bool, auto_exposure_priority, false);
  // Amount of power used by the depth laser, in mW. Valid ranges are between 0 and 360, in
  // increments of 30.
  ISAAC_PARAM(int, laser_power, 150);
  // Enable auto exposure, disabling can reduce motion blur
  ISAAC_PARAM(bool, enable_auto_exposure, true);

 private:
  // Inital configuration of a realsense device
  void initializeDeviceConfig(const rs2::device& dev);

  // Update configuration of a realsense device
  void updateDeviceConfig(const rs2::device& dev);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace isaac

ISAAC_ALICE_REGISTER_CODELET(isaac::RealsenseCamera);
