"""
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

load("@com_nvidia_isaac_sdk//bzl:module.bzl", "isaac_app")

isaac_app(
    name = "ae400_multicam",
    app_json_file = "ae400_multicam.app.json",
    modules = [
        "ae400",
        "@com_nvidia_isaac_sdk//packages/rgbd_processing",
        "@com_nvidia_isaac_sdk//packages/sight",
        "@com_nvidia_isaac_sdk//packages/viewers",
    ],
)

