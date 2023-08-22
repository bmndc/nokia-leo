# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)
eigen_SRC_FILES:= \
	single.cpp \
	double.cpp \
	complex_single.cpp \
	complex_double.cpp \
	xerbla.cpp \
	f2c/complexdots.c\
	f2c/srotm.c \
	f2c/srotmg.c \
	f2c/drotm.c \
	f2c/drotmg.c \
	f2c/lsame.c  \
	f2c/dspmv.c \
	f2c/ssbmv.c \
	f2c/chbmv.c  \
	f2c/sspmv.c \
	f2c/zhbmv.c  \
	f2c/chpmv.c \
	f2c/dsbmv.c \
	f2c/zhpmv.c \
	f2c/dtbmv.c \
	f2c/stbmv.c \
	f2c/ctbmv.c \
	f2c/ztbmv.c \
	f2c/d_cnjg.c \
	f2c/r_cnjg.c

LOCAL_CLANG := true
# EIGEN_ANDROID_SSE_WR is for "Eigen Android SSE Work Around"
# Will be removed after we understand it better.
LOCAL_CFLAGS += -DEIGEN_ANDROID_SSE_WR
LOCAL_MODULE := libF77blas

LOCAL_SRC_FILES := $(eigen_SRC_FILES)
LOCAL_C_INCLUDES += external/eigen/

include $(BUILD_STATIC_LIBRARY)


# Build Eigen using API 8 toolchain for RS Support lib.
include $(CLEAR_VARS)
LOCAL_CLANG := true
# EIGEN_ANDROID_SSE_WR is for "Eigen Android SSE Work Around"
# Will be removed after we understand it better.
LOCAL_CFLAGS += -DEIGEN_ANDROID_SSE_WR
# EIGEN_ANDROID_POSIX_MEMALIGN_WR is for "Eigen Android posix_memalign Work Around"
# Only used for build for low Android API(x86 target) without posix_memalign.
LOCAL_CFLAGS += -DEIGEN_ANDROID_POSIX_MEMALIGN_WR
LOCAL_MODULE := libF77blasV8
LOCAL_SDK_VERSION := 8
LOCAL_NDK_STL_VARIANT := stlport_static

LOCAL_SRC_FILES := $(eigen_SRC_FILES)
LOCAL_C_INCLUDES += external/eigen/

include $(BUILD_STATIC_LIBRARY)
