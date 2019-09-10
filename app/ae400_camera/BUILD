"""
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

load("@com_nvidia_isaac//engine/build:isaac.bzl", "isaac_app")

isaac_app(
    name = "ae400_camera",
    app_json_file = "ae400_camera.app.json",
    modules = [
        "ae400",
        "@com_nvidia_isaac//packages/rgbd_processing",
        "@com_nvidia_isaac//packages/viewers",
    ],
)
