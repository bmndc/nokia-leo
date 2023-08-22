DRM_GPU_DRIVERS := $(strip $(filter-out swrast, $(BOARD_GPU_DRIVERS)))

intel_drivers := i915 i965 i915g ilo
radeon_drivers := r300g r600g radeonsi
rockchip_drivers := rockchip
nouveau_drivers := nouveau
vmwgfx_drivers := vmwgfx
tegra_drivers := tegra

valid_drivers := \
	$(intel_drivers) \
	$(radeon_drivers) \
	$(rockchip_drivers) \
	$(nouveau_drivers) \
	$(vmwgfx_drivers) \
	$(tegra_drivers)

# warn about invalid drivers
invalid_drivers := $(filter-out $(valid_drivers), $(DRM_GPU_DRIVERS))
ifneq ($(invalid_drivers),)
$(warning invalid GPU drivers: $(invalid_drivers))
# tidy up
DRM_GPU_DRIVERS := $(filter-out $(invalid_drivers), $(DRM_GPU_DRIVERS))
endif

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/Makefile.sources

LOCAL_SRC_FILES := $(LIBKMS_FILES)

ifneq ($(filter $(vmwgfx_drivers), $(DRM_GPU_DRIVERS)),)
LOCAL_SRC_FILES += $(LIBKMS_VMWGFX_FILES)
endif

ifneq ($(filter $(intel_drivers), $(DRM_GPU_DRIVERS)),)
LOCAL_SRC_FILES += $(LIBKMS_INTEL_FILES)
endif

ifneq ($(filter $(nouveau_drivers), $(DRM_GPU_DRIVERS)),)
LOCAL_SRC_FILES += $(LIBKMS_NOUVEAU_FILES)
endif

ifneq ($(filter $(radeon_drivers), $(DRM_GPU_DRIVERS)),)
LOCAL_SRC_FILES += $(LIBKMS_RADEON_FILES)
endif

LOCAL_MODULE := libkms
LOCAL_SHARED_LIBRARIES := libdrm

LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/libdrm

LOCAL_COPY_HEADERS_TO := libdrm
LOCAL_COPY_HEADERS := $(LIBKMS_H_FILES)

include $(BUILD_SHARED_LIBRARY)
