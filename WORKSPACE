"""
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

workspace(name = "ae400_sample")

# Point following dependency to latest Isaac SDK Release
# downloaded from https://developer.nvidia.com/isaac/downloads (login required)
local_repository(
    name = "com_nvidia_isaac",
    path = "/home/jsm/Downloads/isaac",
)

load("@com_nvidia_isaac//engine/build:isaac.bzl", "isaac_git_repository", "isaac_http_archive")
load("@com_nvidia_isaac//third_party:engine.bzl", "isaac_engine_workspace")
load("@com_nvidia_isaac//third_party:packages.bzl", "isaac_packages_workspace")
load("@com_nvidia_isaac//third_party:ros.bzl", "isaac_ros_workspace")
load("@com_nvidia_isaac//third_party:zed.bzl", "isaac_zed_workspace")

isaac_engine_workspace()

isaac_packages_workspace()

isaac_ros_workspace()

isaac_zed_workspace()

####################################################################################################
# Load cartographer

# Loads before boost to override for aarch64 specific config
isaac_http_archive(
    name = "org_lzma_lzma",
    build_file = "@com_nvidia_isaac//third_party:lzma.BUILD",
    licenses = ["@org_lzma_lzma//:COPYING"],
    sha256 = "9717ae363760dedf573dad241420c5fea86256b65bc21d2cf71b2b12f0544f4b",
    strip_prefix = "xz-5.2.4",
    type = "tar.xz",
    url = "https://developer.nvidia.com/isaac/download/third_party/xz-5-2-4-tar-xz",
)

# Loads boost c++ library (https://www.boost.org/) and
# custom bazel build support (https://github.com/nelhage/rules_boost/)
# explicitly due to bazel bug: https://github.com/bazelbuild/bazel/issues/1550
isaac_http_archive(
    name = "com_github_nelhage_rules_boost",
    licenses = ["@com_github_nelhage_rules_boost//:LICENSE"],
    patches = ["@com_nvidia_isaac//third_party:rules_boost.patch"],
    sha256 = "1479f6a46d37c415b0f803186bacb7a78f76305331c556bba20d13247622752a",
    type = "tar.gz",
    url = "https://developer.nvidia.com/isaac/download/third_party/rules_boost-82ae1790cef07f3fd618592ad227fe2d66fe0b31-tar-gz",
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")

boost_deps()

isaac_http_archive(
    name = "com_github_googlecartographer_cartographer",
    licenses = ["@com_github_googlecartographer_cartographer//:LICENSE"],
    sha256 = "a52591e5f7cd2a4bb63c005addbcfaa751cd0b19a223c800b5e90afb5055d946",
    type = "tar.gz",
    url = "https://developer.nvidia.com/isaac/download/third_party/cartographer-bcd5486025df4f601c3977c44a5e00e9c80b4975-tar-gz",
)

load("@com_github_googlecartographer_cartographer//:bazel/repositories.bzl", "cartographer_repositories")

cartographer_repositories()

# Loads Google grpc C++ library (https://grpc.io/) explicitly for cartographer
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

bind(
    name = "zlib",
    actual = "@net_zlib_zlib//:zlib",
)

# Loads Prometheus Library (https://github.com/jupp0r/prometheus-cpp/) explicitly for cartographer
load("@com_github_jupp0r_prometheus_cpp//:repositories.bzl", "prometheus_cpp_repositories")

prometheus_cpp_repositories()

####################################################################################################
# Configures toolchain
load("@com_nvidia_isaac//engine/build/toolchain:toolchain.bzl", "toolchain_configure")

toolchain_configure(name = "toolchain")

####################################################################################################
# LIPS AE400 specific dependencies
load("//third_party:ae400.bzl", "ae400_workspace")

ae400_workspace()
