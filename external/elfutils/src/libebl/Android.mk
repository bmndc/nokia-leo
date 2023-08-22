# Copyright (C) 2013 The Android Open Source Project
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

LIBEBL_SRC_FILES := \
eblabicfi.c \
eblauxvinfo.c \
eblbackendname.c \
eblbsspltp.c \
eblcheckobjattr.c \
eblcheckreloctargettype.c \
ebl_check_special_section.c \
ebl_check_special_symbol.c \
eblclosebackend.c \
eblcopyrelocp.c \
eblcorenote.c \
eblcorenotetypename.c \
ebldebugscnp.c \
ebldwarftoregno.c \
ebldynamictagcheck.c \
ebldynamictagname.c \
eblelfclass.c \
eblelfdata.c \
eblelfmachine.c \
eblgotpcreloccheck.c \
eblgstrtab.c \
eblinitreg.c \
eblmachineflagcheck.c \
eblmachineflagname.c \
eblmachinesectionflagcheck.c \
eblnonerelocp.c \
eblnormalizepc.c \
eblobjecttypename.c \
eblobjnote.c \
eblobjnotetypename.c \
eblopenbackend.c \
eblosabiname.c \
eblreginfo.c \
eblrelativerelocp.c \
eblrelocsimpletype.c \
eblreloctypecheck.c \
eblreloctypename.c \
eblrelocvaliduse.c \
eblresolvesym.c \
eblretval.c \
eblsectionname.c \
eblsectionstripp.c \
eblsectiontypename.c \
eblsegmenttypename.c \
eblshflagscombine.c \
eblstother.c \
eblstrtab.c \
eblsymbolbindingname.c \
eblsymboltypename.c \
ebl_syscall_abi.c \
eblsysvhashentrysize.c \
eblunwind.c \
eblwstrtab.c \


ifeq ($(HOST_OS),linux)

#
# host libebl
#
#
include $(CLEAR_VARS)

# Disable clang like other elfutils libraries.
LOCAL_CLANG := false

LOCAL_SRC_FILES := $(LIBEBL_SRC_FILES)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../libebl \
	$(LOCAL_PATH)/../libasm \
	$(LOCAL_PATH)/../libelf \
	$(LOCAL_PATH)/../libdw

LOCAL_CFLAGS += -DHAVE_CONFIG_H -std=gnu99 -D_GNU_SOURCE

LOCAL_CLANG := false

LOCAL_MODULE:= libebl

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

include $(BUILD_HOST_STATIC_LIBRARY)

endif # linux

#
# target libebl
#

include $(CLEAR_VARS)

# Disable clang like other elfutils libraries.
LOCAL_CLANG := false

LOCAL_SRC_FILES := $(LIBEBL_SRC_FILES)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../libebl \
	$(LOCAL_PATH)/../libasm \
	$(LOCAL_PATH)/../libelf \
	$(LOCAL_PATH)/../libdw

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../bionic-fixup

LOCAL_CFLAGS += -include $(LOCAL_PATH)/../../bionic-fixup/AndroidFixup.h

LOCAL_CFLAGS += -DHAVE_CONFIG_H -std=gnu99 -D_GNU_SOURCE -Werror -Wno-error=pointer-arith

LOCAL_MODULE:= libebl

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

include $(BUILD_STATIC_LIBRARY)
