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

LOCAL_PATH := $(call my-dir)

LIBDWELF_SRC_FILES := \
    dwelf_dwarf_gnu_debugaltlink.c \
    dwelf_elf_gnu_build_id.c \
    dwelf_elf_gnu_debuglink.c \


ifeq ($(HOST_OS),linux)

#
# host libdwelf
#

include $(CLEAR_VARS)

# Clang has no nested functions.
LOCAL_CLANG := false

LOCAL_SRC_FILES := $(LIBDWELF_SRC_FILES)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../libdw \
	$(LOCAL_PATH)/../libdwfl \
	$(LOCAL_PATH)/../libebl \
	$(LOCAL_PATH)/../libelf

LOCAL_CFLAGS += -DHAVE_CONFIG_H -std=gnu99 -D_GNU_SOURCE -D_BSD_SOURCE

# to suppress the "pointer of type ‘void *’ used in arithmetic" warning
LOCAL_CFLAGS += -Wno-pointer-arith

LOCAL_MODULE:= libdwelf

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_STATIC_LIBRARIES := libz

include $(BUILD_HOST_STATIC_LIBRARY)

endif # linux

#
# target libdwelf
#

include $(CLEAR_VARS)

# Clang has no nested functions.
LOCAL_CLANG := false

LOCAL_SRC_FILES := $(LIBDWELF_SRC_FILES)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../libdw \
	$(LOCAL_PATH)/../libdwfl \
	$(LOCAL_PATH)/../libebl \
	$(LOCAL_PATH)/../libelf

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../bionic-fixup

LOCAL_CFLAGS += -include $(LOCAL_PATH)/../../bionic-fixup/AndroidFixup.h

LOCAL_CFLAGS += -DHAVE_CONFIG_H -std=gnu99 -D_GNU_SOURCE -D_BSD_SOURCE -Werror

# to suppress the "pointer of type ‘void *’ used in arithmetic" warning
LOCAL_CFLAGS += -Wno-pointer-arith

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE:= libdwelf

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_STATIC_LIBRARIES := libz

include $(BUILD_STATIC_LIBRARY)
