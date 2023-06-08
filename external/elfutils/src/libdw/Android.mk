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

LIBDW_SRC_FILES := \
    cfi.c \
    cie.c \
    dwarf_abbrevhaschildren.c \
    dwarf_abbrev_hash.c \
    dwarf_addrdie.c \
    dwarf_aggregate_size.c \
    dwarf_arrayorder.c \
    dwarf_attr.c \
    dwarf_attr_integrate.c \
    dwarf_begin.c \
    dwarf_begin_elf.c \
    dwarf_bitoffset.c \
    dwarf_bitsize.c \
    dwarf_bytesize.c \
    dwarf_cfi_addrframe.c \
    dwarf_cfi_end.c \
    dwarf_child.c \
    dwarf_cu_die.c \
    dwarf_cu_getdwarf.c \
    dwarf_cuoffset.c \
    dwarf_decl_column.c \
    dwarf_decl_file.c \
    dwarf_decl_line.c \
    dwarf_diecu.c \
    dwarf_diename.c \
    dwarf_dieoffset.c \
    dwarf_end.c \
    dwarf_entry_breakpoints.c \
    dwarf_entrypc.c \
    dwarf_error.c \
    dwarf_filesrc.c \
    dwarf_formaddr.c \
    dwarf_formblock.c \
    dwarf_formflag.c \
    dwarf_formref.c \
    dwarf_formref_die.c \
    dwarf_formsdata.c \
    dwarf_formstring.c \
    dwarf_formudata.c \
    dwarf_frame_cfa.c \
    dwarf_frame_info.c \
    dwarf_frame_register.c \
    dwarf_func_inline.c \
    dwarf_getabbrevattr.c \
    dwarf_getabbrev.c \
    dwarf_getabbrevcode.c \
    dwarf_getabbrevtag.c \
    dwarf_getalt.c \
    dwarf_getarange_addr.c \
    dwarf_getarangeinfo.c \
    dwarf_getaranges.c \
    dwarf_getattrcnt.c \
    dwarf_getattrs.c \
    dwarf_getcfi.c \
    dwarf_getcfi_elf.c \
    dwarf_getelf.c \
    dwarf_getfuncs.c \
    dwarf_getlocation_attr.c \
    dwarf_getlocation.c \
    dwarf_getlocation_die.c \
    dwarf_getlocation_implicit_pointer.c \
    dwarf_getmacros.c \
    dwarf_getpubnames.c \
    dwarf_getscopes.c \
    dwarf_getscopes_die.c \
    dwarf_getscopevar.c \
    dwarf_getsrc_die.c \
    dwarf_getsrcdirs.c \
    dwarf_getsrc_file.c \
    dwarf_getsrcfiles.c \
    dwarf_getsrclines.c \
    dwarf_getstring.c \
    dwarf_hasattr.c \
    dwarf_hasattr_integrate.c \
    dwarf_haschildren.c \
    dwarf_hasform.c \
    dwarf_haspc.c \
    dwarf_highpc.c \
    dwarf_lineaddr.c \
    dwarf_linebeginstatement.c \
    dwarf_lineblock.c \
    dwarf_linecol.c \
    dwarf_linediscriminator.c \
    dwarf_lineendsequence.c \
    dwarf_lineepiloguebegin.c \
    dwarf_lineisa.c \
    dwarf_lineno.c \
    dwarf_lineop_index.c \
    dwarf_lineprologueend.c \
    dwarf_linesrc.c \
    dwarf_lowpc.c \
    dwarf_macro_getparamcnt.c \
    dwarf_macro_getsrcfiles.c \
    dwarf_macro_opcode.c \
    dwarf_macro_param1.c \
    dwarf_macro_param2.c \
    dwarf_macro_param.c \
    dwarf_next_cfi.c \
    dwarf_nextcu.c \
    dwarf_offabbrev.c \
    dwarf_offdie.c \
    dwarf_onearange.c \
    dwarf_onesrcline.c \
    dwarf_peel_type.c \
    dwarf_ranges.c \
    dwarf_setalt.c \
    dwarf_siblingof.c \
    dwarf_sig8_hash.c \
    dwarf_srclang.c \
    dwarf_tag.c \
    dwarf_whatattr.c \
    dwarf_whatform.c \
    fde.c \
    frame-cache.c \
    libdw_alloc.c \
    libdw_findcu.c \
    libdw_form.c \
    libdw_visit_scopes.c \


ifeq ($(HOST_OS),linux)

#
# host libdw
#

include $(CLEAR_VARS)

# Clang has no nested functions.
LOCAL_CLANG := false

LOCAL_SRC_FILES := $(LIBDW_SRC_FILES)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../libdw \
	$(LOCAL_PATH)/../libelf

LOCAL_CFLAGS += -DHAVE_CONFIG_H -std=gnu99 -D_GNU_SOURCE -D_BSD_SOURCE -DIS_LIBDW

# to suppress the "pointer of type ‘void *’ used in arithmetic" warning
LOCAL_CFLAGS += -Wno-pointer-arith

LOCAL_MODULE:= libdw

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_STATIC_LIBRARIES := libz

include $(BUILD_HOST_STATIC_LIBRARY)

endif # linux

#
# target libdw
#

include $(CLEAR_VARS)

# Clang has no nested functions.
LOCAL_CLANG := false

LOCAL_SRC_FILES := $(LIBDW_SRC_FILES)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../libdw \
	$(LOCAL_PATH)/../libelf

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../bionic-fixup

LOCAL_CFLAGS += -include $(LOCAL_PATH)/../../bionic-fixup/AndroidFixup.h

LOCAL_CFLAGS += -DHAVE_CONFIG_H -std=gnu99 -D_GNU_SOURCE -D_BSD_SOURCE -DIS_LIBDW -Werror

# to suppress the "pointer of type ‘void *’ used in arithmetic" warning
LOCAL_CFLAGS += -Wno-pointer-arith

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE:= libdw

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_STATIC_LIBRARIES := libz

include $(BUILD_STATIC_LIBRARY)
