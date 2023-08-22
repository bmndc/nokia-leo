# Copyright (C) 2015 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

# squashfs-tools depends on Linux Kernel specific headers (e.g. sysinfo.h).
ifeq ($(HOST_OS),linux)

include $(CLEAR_VARS)

# The LOCAL_MODULE name is referenced by the code. Don't change it.
LOCAL_MODULE := mksquashfs

mksquashfs_files := mksquashfs.c squashfs_fs.h squashfs_swap.h mksquashfs.h \
                    sort.h pseudo.h compressor.h xattr.h action.h error.h progressbar.h \
                    info.h caches-queues-lists.h read_fs.h restore.h process_fragments.h

read_fs_files := read_fs.c squashfs_fs.h squashfs_swap.h compressor.h xattr.h \
                 error.h mksquashfs.h

sort_files := sort.c squashfs_fs.h mksquashfs.h sort.h error.h progressbar.h

swap_files := swap.c

pseudo_files := pseudo.c pseudo.h error.h progressbar.h

compressor_files := compressor.c compressor.h squashfs_fs.h

xattr_files := xattr.c squashfs_fs.h squashfs_swap.h mksquashfs.h xattr.h error.h \
               progressbar.h

read_xattrs_files := read_xattrs.c squashfs_fs.h squashfs_swap.h xattr.h error.h

action_files := action.c squashfs_fs.h mksquashfs.h action.h error.h

progressbar_files := progressbar.c error.h

read_file_files := read_file.c error.h

info_files := info.c squashfs_fs.h mksquashfs.h error.h progressbar.h \
              caches-queues-lists.h

restore_files := restore.c caches-queues-lists.h squashfs_fs.h mksquashfs.h error.h \
                 progressbar.h info.h

process_fragments_files := process_fragments.c process_fragments.h

caches_queues_lists_files := caches-queues-lists.c error.h caches-queues-lists.h

gzip_wrapper_files := gzip_wrapper.c squashfs_fs.h gzip_wrapper.h compressor.h

android_files := android.c android.h

lz4_wrapper_files := lz4_wrapper.c squashfs_fs.h lz4_wrapper.h compressor.h

LOCAL_SRC_FILES := $(mksquashfs_files) $(read_fs_files) $(action_files) $(swap_files) \
                   $(pseudo_files) $(compressor_files) $(sort_files) $(progressbar_files) \
                   $(read_file_files) $(info_files) $(restore_files) \
                   $(process_fragments_files) $(caches_queues_lists_files) $(xattr_files) \
                   $(read_xattrs_files) $(gzip_wrapper_files) $(android_files) \
                   $(lz4_wrapper_files)

LOCAL_CFLAGS := -I -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE -Wall \
                -DCOMP_DEFAULT="\"lz4\"" -DGZIP_SUPPORT -DLZ4_SUPPORT -DXATTR_SUPPORT -DXATTR_DEFAULT

LOCAL_LDLIBS := -lpthread -lm -lz

LOCAL_C_INCLUDES := external/lz4/lib

LOCAL_SHARED_LIBRARIES := libcutils libselinux
LOCAL_STATIC_LIBRARIES := liblz4

LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_EXECUTABLE)

endif
