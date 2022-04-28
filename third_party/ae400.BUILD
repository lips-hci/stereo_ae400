"""
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""
"""
Copyright (c) 2020, LIPS Corporation. All rights reserved.

Update History:
2020-03-10: first version to use ae400-realsense-sdk release v0.9.0.7
"""
# Description:
#   Support Isaac 2019.3 based on LIPS AE400 Realsense SDK

licenses(["notice"])  # Apache 2.0. See LICENSE file

exports_files([
    "LICENSE",
    "COPYING",
])

# backend-ethernet
# RS codebase now is v2.43.0
# v1.0.2.0 for RS codebase v2.43~v2.48
# v1.0.2.5 for RS codebase v2.49~v2.50
cc_library(
    name = "libbackend_ethernet",
    srcs = glob(
        [
            "third-party/easyloggingpp/src/easylogging++.cc",
        ]) + select({
            "@com_nvidia_isaac_engine//engine/build:platform_x86_64": [
                "third-party/lips/lib/v1.0.2.0/linux/amd64/libbackend-ethernet.a",
            ],
            "@com_nvidia_isaac_engine//engine/build:platform_jetpack45": [
                "third-party/lips/lib/v1.0.2.0/linux/arm64/libbackend-ethernet.a",
            ],
        }),
    hdrs = glob([
        "include/librealsense2/**/*.h*",
        "third-party/easyloggingpp/src/easylogging++.h",
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
    linkopts = [
        "-fPIC",
    ],
    defines = [
        "BUILD_EASYLOGGINGPP",
        "ELPP_NO_DEFAULT_LOG_FILE",
        "ELPP_THREAD_SAFE",
        "RS2_USE_V4L2_BACKEND",
        "DISABLE_UVC_METADATA",
        "ENABLE_AE400_IMU",
        "HWM_OVER_XU",
        "RASPBERRY_PI",
    ],
    includes = [
        "include",
        "src",
        "third-party/easyloggingpp/src",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@libusb",
    ],
)

cc_library(
    name = "ae400_realsense_sdk",
    srcs = glob(
        [
            "src/**/*.cpp",
            "common/utilities/time/l500/get-mfr-ww.cpp",
            "common/utilities/time/work_week.cpp",
        ],
        exclude = [
            "src/android/*",
            "src/android/jni/*",
            "src/android/fw-logger/*",
            "src/usbhost/*",
            "src/cuda/*",
            "src/fw/*",
            "src/gl/*",
            "src/libuvc/*",
            "src/mf/*",
            "src/tm2/*",
            "src/win/*",
            "src/win7/*",
            "src/winusb/*",
            "src/ethernet/*",
            "src/compression/*",
            "src/ipDeviceCommon/*",
        ],
    ) + [
        "common/fw/firmware-version.h",
        "common/parser.hpp",
        "common/utilities/time/l500/get-mfr-ww.h",
        "common/utilities/time/work_week.h",
        "third-party/stb_image.h",
        "common/decompress-huffman.h",
    ],
    hdrs = glob([
        "include/librealsense2/**/*.h*",
        "src/**/*.h",
        "src/**/*.hpp",
        "common/utilities/time/*.h",
    ]),
    alwayslink=True,
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
        "RASPBERRY_PI",
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
        ":rapidxml",
        ":libbackend_ethernet",
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
    hdrs = glob([
        "third-party/json.hpp",
        "third-party/rapidjson/*.h",
        "third-party/rapidjson/**/*.h",
    ]),
)

cc_library(
    name = "rapidxml",
    hdrs = [
        "third-party/rapidxml/rapidxml.hpp",
        "third-party/rapidxml/rapidxml_utils.hpp",
    ],
)
