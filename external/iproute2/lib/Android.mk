LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
# clang cannot compile 'variable length array in structure' in ipxfrm.c
LOCAL_CLANG := false
LOCAL_SRC_FILES := utils.c rt_names.c ll_types.c ll_proto.c ll_addr.c inet_proto.c
LOCAL_MODULE := libiprouteutil
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include
LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DCONFDIR=\"/data/misc/net\" \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-Wno-pointer-arith \
	-Wno-sign-compare \
	-Wno-unused-parameter \
	-Werror

# This is a work around for b/18403920
LOCAL_LDFLAGS := -Wl,--no-gc-sections

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
# clang cannot compile 'variable length array in structure' in ipxfrm.c
LOCAL_CLANG := false
LOCAL_SRC_FILES := ll_map.c libnetlink.c
LOCAL_MODULE := libnetlink
LOCAL_SYSTEM_SHARED_LIBRARIES := libc
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include
LOCAL_CFLAGS := -O2 -g -W -Wall \
	-DHAVE_UNISTD_H \
	-DHAVE_ERRNO_H \
	-DHAVE_NETINET_IN_H \
	-DHAVE_SYS_IOCTL_H \
	-DHAVE_SYS_MMAN_H \
	-DHAVE_SYS_MOUNT_H \
	-DHAVE_SYS_PRCTL_H \
	-DHAVE_SYS_RESOURCE_H \
	-DHAVE_SYS_SELECT_H \
	-DHAVE_SYS_STAT_H \
	-DHAVE_SYS_TYPES_H \
	-DHAVE_STDLIB_H \
	-DHAVE_STRDUP \
	-DHAVE_MMAP \
	-DHAVE_UTIME_H \
	-DHAVE_GETPAGESIZE \
	-DHAVE_LSEEK64 \
	-DHAVE_LSEEK64_PROTOTYPE \
	-DHAVE_EXT2_IOCTLS \
	-DHAVE_LINUX_FD_H \
	-DHAVE_TYPE_SSIZE_T \
	-Wno-pointer-arith \
	-Wno-sign-compare \
	-Wno-unused-parameter \
	-Werror

include $(BUILD_SHARED_LIBRARY)
