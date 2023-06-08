LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libdrm_rockchip
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libdrm

LOCAL_SRC_FILES := rockchip_drm.c
LOCAL_EXPORT_C_INCLUDE_DIRS += \
	$(LOCAL_PATH)/rockchip

LOCAL_C_INCLUDES := \
	$(LIBDRM_TOP) \
	$(LIBDRM_TOP)/rockchip \
	$(LIBDRM_TOP)/include/drm

LOCAL_CFLAGS := \
	-DHAVE_LIBDRM_ATOMIC_PRIMITIVES=1

LOCAL_COPY_HEADERS := rockchip_drm.h rockchip_drmif.h
LOCAL_COPY_HEADERS_TO := libdrm

LOCAL_SHARED_LIBRARIES := \
	libdrm

include $(BUILD_SHARED_LIBRARY)
