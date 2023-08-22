LOCAL_PATH:= $(call my-dir)

# f2fs-tools depends on Linux kernel headers being in the system include path.
ifeq ($(HOST_OS),linux)

# The versions depend on $(LOCAL_PATH)/VERSION
version_CFLAGS := -DF2FS_MAJOR_VERSION=1 -DF2FS_MINOR_VERSION=4 -DF2FS_TOOLS_VERSION=\"1.4.0\" -DF2FS_TOOLS_DATE=\"2014-10-18\"

# external/e2fsprogs/lib is needed for uuid/uuid.h
common_C_INCLUDES := $(LOCAL_PATH)/include external/e2fsprogs/lib/

#----------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := libf2fs_fmt
LOCAL_SRC_FILES := \
	lib/libf2fs.c \
	mkfs/f2fs_format.c \
	mkfs/f2fs_format_utils.c \

LOCAL_C_INCLUDES := $(common_C_INCLUDES)
LOCAL_CFLAGS := $(version_CFLAGS)
LOCAL_EXPORT_CFLAGS := $(version_CFLAGS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/mkfs
include $(BUILD_STATIC_LIBRARY)

#----------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := libf2fs_fmt_host
LOCAL_SRC_FILES := \
	lib/libf2fs.c \
	mkfs/f2fs_format.c \
	mkfs/f2fs_format_utils.c \

LOCAL_C_INCLUDES := $(common_C_INCLUDES)
LOCAL_CFLAGS := $(version_CFLAGS)
LOCAL_EXPORT_CFLAGS := $(version_CFLAGS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/mkfs
include $(BUILD_HOST_STATIC_LIBRARY)

#----------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := libf2fs_fmt_host_dyn
LOCAL_SRC_FILES := \
	lib/libf2fs.c \
	mkfs/f2fs_format.c \

LOCAL_C_INCLUDES := $(common_C_INCLUDES)
LOCAL_CFLAGS := $(version_CFLAGS)
LOCAL_EXPORT_CFLAGS := $(version_CFLAGS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)/mkfs
LOCAL_STATIC_LIBRARIES := \
     libf2fs_ioutils_host \
     libext2_uuid_host \
     libsparse_host \
     libz
# LOCAL_LDLIBS := -ldl
include $(BUILD_HOST_SHARED_LIBRARY)

#----------------------------------------------------------
include $(CLEAR_VARS)
# The LOCAL_MODULE name is referenced by the code. Don't change it.
LOCAL_MODULE := mkfs.f2fs

# mkfs.f2fs is used in recovery: must be static.
LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin

LOCAL_SRC_FILES := \
	lib/libf2fs_io.c \
	mkfs/f2fs_format_main.c
LOCAL_C_INCLUDES := $(common_C_INCLUDES)
LOCAL_CFLAGS := $(version_CFLAGS)
LOCAL_STATIC_LIBRARIES := libc libf2fs_fmt libext2_uuid_static
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

#----------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := make_f2fs

LOCAL_SRC_FILES := \
	lib/libf2fs_io.c \
	mkfs/f2fs_format_main.c
LOCAL_C_INCLUDES := $(common_C_INCLUDES)
LOCAL_CFLAGS := $(version_CFLAGS)
LOCAL_STATIC_LIBRARIES := libf2fs_fmt
LOCAL_SHARED_LIBRARIES := libext2_uuid
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

#----------------------------------------------------------
include $(CLEAR_VARS)
# The LOCAL_MODULE name is referenced by the code. Don't change it.
LOCAL_MODULE := fsck.f2fs
LOCAL_SRC_FILES := \
	fsck/dump.c \
	fsck/fsck.c \
	fsck/main.c \
	fsck/mount.c \
	lib/libf2fs.c \
	lib/libf2fs_io.c \

LOCAL_C_INCLUDES := $(common_C_INCLUDES)
LOCAL_CFLAGS := $(version_CFLAGS)
LOCAL_SHARED_LIBRARIES := libext2_uuid
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

#----------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := fsck.f2fs
LOCAL_SRC_FILES := \
	fsck/dump.c \
	fsck/fsck.c \
	fsck/main.c \
	fsck/mount.c \
	lib/libf2fs.c \
	lib/libf2fs_io.c \

LOCAL_C_INCLUDES := $(common_C_INCLUDES)
LOCAL_CFLAGS := $(version_CFLAGS)
LOCAL_HOST_SHARED_LIBRARIES :=  libext2_uuid_host
include $(BUILD_HOST_EXECUTABLE)

endif
