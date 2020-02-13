"""
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""
# Description:
#   Realsense sdk

licenses(["notice"])  # Apache 2.0. See LICENSE file

exports_files([
    "LICENSE",
    "COPYING",
])

cc_import(
    name = "libis4",
    shared_library = select({
        "@com_nvidia_isaac//engine/build:platform_x86_64": "lib/x64/libis4.so",
        "@com_nvidia_isaac//engine/build:platform_jetpack43": "lib/aarch64/libis4.so",
    })
)

cc_library(
    name = "ae400cs",
    srcs = glob(
        ["src/**/*.cpp"],
        exclude = [
            "src/cuda/*",
            "src/libuvc/*",
            "src/tm2/*",
            "src/win/*",
            "src/win7/*",
        ],
    ),
    hdrs = glob([
        "include/librealsense2/**/*.h*",
        "src/**/*.h",
        "src/**/*.hpp",
    ]),
    copts = [
        # When preprocessing, do not shorten system header paths with canonicalization.
        "-fno-canonical-system-headers",
        # Disable all warnings.
        # librealsense2 produces a large number of warnings. Not all can blocked with -Wno- flags.
        # So we need to use the heavy handed approach of disabling all warnings.
        "-w",
        "-Wno-error",
    ],
    defines = [
        "RS2_USE_V4L2_BACKEND",
        "HWM_OVER_XU",
    ],
    includes = [
        "include",
        "src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":json",
        ":realsense-file",
        ":sqlite",
        "@libusb",
        ":libis4",
    ],
)

cc_library(
    name = "easylogging",
    srcs = ["third-party/easyloggingpp/src/easylogging++.cc"],
    hdrs = ["third-party/easyloggingpp/src/easylogging++.h"],
    includes = ["third-party/easyloggingpp/src"],
)

cc_library(
    name = "realsense-file",
    srcs = glob([
        "third-party/realsense-file/lz4/lz4.c",
        "third-party/realsense-file/rosbag/**/*.c",
        "third-party/realsense-file/rosbag/**/*.cpp",
    ]),
    hdrs = glob([
        "third-party/realsense-file/**/*.h",
        "third-party/realsense-file/**/*.hpp",
    ]),
    copts = [
        # When preprocessing, do not shorten system header paths with canonicalization.
        "-fno-canonical-system-headers",
        "-Wno-unused-variable",
        "-Wno-misleading-indentation",
        "-Wno-deprecated",
    ],
    includes = [
        "third-party/realsense-file/boost",
        "third-party/realsense-file/lz4",
        "third-party/realsense-file/rosbag/console_bridge/include",
        "third-party/realsense-file/rosbag/cpp_common/include",
        "third-party/realsense-file/rosbag/msgs",
        "third-party/realsense-file/rosbag/rosbag_storage/include",
        "third-party/realsense-file/rosbag/roscpp_serialization/include",
        "third-party/realsense-file/rosbag/roscpp_traits/include",
        "third-party/realsense-file/rosbag/roslz4/include",
        "third-party/realsense-file/rosbag/rostime/include",
    ],
)

cc_library(
    name = "sqlite",
    srcs = ["third-party/sqlite/sqlite3.c"],
    hdrs = ["third-party/sqlite/sqlite3.h"],
)

cc_library(
    name = "json",
    hdrs = ["third-party/json.hpp"],
)
