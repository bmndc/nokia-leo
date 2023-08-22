LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= ping.c ping_common.c
LOCAL_MODULE := ping
LOCAL_CFLAGS := -DWITHOUT_IFADDRS -Wno-sign-compare
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -DWITHOUT_IFADDRS -Wno-sign-compare
LOCAL_SRC_FILES := ping6.c ping_common.c
LOCAL_MODULE := ping6
LOCAL_SHARED_LIBRARIES := libcrypto
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -Wno-sign-compare
LOCAL_SRC_FILES := tracepath.c
LOCAL_MODULE := tracepath
LOCAL_MODULE_TAGS := debug
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -Wno-sign-compare
LOCAL_SRC_FILES := tracepath6.c
LOCAL_MODULE := tracepath6
LOCAL_MODULE_TAGS := debug
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -Wno-sign-compare
LOCAL_SRC_FILES := traceroute6.c
LOCAL_MODULE := traceroute6
LOCAL_MODULE_TAGS := debug
include $(BUILD_EXECUTABLE)
