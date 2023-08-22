/* Copyright (C) 2015 Acadine Technologies. All rights reserved.
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2010-2014 The Linux Foundation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "NativeFramebufferDevice.h"
#include "mozilla/FileUtils.h"
#include "utils/Log.h"
#include "cutils/properties.h"

#ifdef BUILD_ARM_NEON
#include "rgb8888_to_rgb565_neon.h"
#endif

#define DEFAULT_XDPI 75.0

namespace mozilla {

inline unsigned int roundUpToPageSize(unsigned int x) {
    return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
}

inline void Transform8888To565_Software(uint8_t* outbuf, const uint8_t* inbuf, int PixelNum)
{
    uint32_t bytes = PixelNum * 4;
    uint16_t *out = (uint16_t *)outbuf;
    uint8_t *in = (uint8_t *)inbuf;
    for (uint32_t i = 0; i < bytes; i += 4) {
        *out++ = ((in[i]     & 0xF8) << 8) |
                 ((in[i + 1] & 0xFC) << 3) |
                 ((in[i + 2]       ) >> 3);
    }
}

inline void Transform8888To565(uint8_t* outbuf, const uint8_t* inbuf, int PixelNum)
{
#ifdef BUILD_ARM_NEON
        Transform8888To565_NEON(outbuf, inbuf, PixelNum);
#else
        Transform8888To565_Software(outbuf, inbuf, PixelNum);
#endif
}

NativeFramebufferDevice::NativeFramebufferDevice(int aExtFbFd)
    : mWidth(320)
    , mHeight(480)
    , mSurfaceformat(HAL_PIXEL_FORMAT_RGBA_8888)
    , mXdpi(DEFAULT_XDPI)
    , mIsEnabled(false)
    , mFd(aExtFbFd)
    , mMappedAddr(nullptr)
    , mMemLength(0)
    , mGrmodule(nullptr)
{
}

NativeFramebufferDevice::~NativeFramebufferDevice()
{
    Close();
}

NativeFramebufferDevice*
NativeFramebufferDevice::Create()
{
    char propValue[PROPERTY_VALUE_MAX];

    // Check for dev node path of external screen's framebuffer;
    if (property_get("ro.kaios.display.ext_fb_dev", propValue, NULL) <= 0) {
        return nullptr;
    }

    char const *const device_template[] = {
            "/dev/graphics/%s",
            "/dev/%s",
            0 };

    int i = 0;
    char name[64];

    int fbFd = -1;
    while ((fbFd == -1) && device_template[i]) {
        snprintf(name, 64, device_template[i], propValue);
        fbFd = open(name, O_RDWR, 0);
        i++;
    }
    if (fbFd < 0) {
        ALOGE("Failed to open external framebuffer device %s", propValue);
        return nullptr;
    }

    return new NativeFramebufferDevice(fbFd);
}

bool
NativeFramebufferDevice::Open()
{
    if (ioctl(mFd, FBIOGET_FSCREENINFO, &mFInfo) == -1) {
        ALOGE("FBIOGET_FSCREENINFO failed");
        Close();
        return false;
    }

    if (ioctl(mFd, FBIOGET_VSCREENINFO, &mVInfo) == -1) {
        ALOGE("FBIOGET_VSCREENINFO: failed");
        Close();
        return false;
    }

    mVInfo.reserved[0] = 0;
    mVInfo.reserved[1] = 0;
    mVInfo.reserved[2] = 0;
    mVInfo.xoffset = 0;
    mVInfo.yoffset = 0;
    mVInfo.activate = FB_ACTIVATE_NOW;

    if(mVInfo.bits_per_pixel == 32) {

        // Explicitly request RGBA_8888
        mVInfo.bits_per_pixel = 32;
        mVInfo.red.offset     = 24;
        mVInfo.red.length     = 8;
        mVInfo.green.offset   = 16;
        mVInfo.green.length   = 8;
        mVInfo.blue.offset    = 8;
        mVInfo.blue.length    = 8;
        mVInfo.transp.offset  = 0;
        mVInfo.transp.length  = 8;

        mFBSurfaceformat = HAL_PIXEL_FORMAT_RGBX_8888;
    } else {

        // Explicitly request 5/6/5
        mVInfo.bits_per_pixel = 16;
        mVInfo.red.offset     = 11;
        mVInfo.red.length     = 5;
        mVInfo.green.offset   = 5;
        mVInfo.green.length   = 6;
        mVInfo.blue.offset    = 0;
        mVInfo.blue.length    = 5;
        mVInfo.transp.offset  = 0;
        mVInfo.transp.length  = 0;

        mFBSurfaceformat = HAL_PIXEL_FORMAT_RGB_565;
    }

    if (ioctl(mFd, FBIOPUT_VSCREENINFO, &mVInfo) == -1) {
        ALOGW("FBIOPUT_VSCREENINFO failed, update offset failed");
        Close();
        return false;
    }

    if (int(mVInfo.width) <= 0 || int(mVInfo.height) <= 0) {
        // the driver doesn't return that information
        // default to 160 dpi
        mVInfo.width  = ((mVInfo.xres * 25.4f)/160.0f + 0.5f);
        mVInfo.height = ((mVInfo.yres * 25.4f)/160.0f + 0.5f);
    }

    float xdpi = (mVInfo.xres * 25.4f) / mVInfo.width;
    float ydpi = (mVInfo.yres * 25.4f) / mVInfo.height;

    ALOGI(  "using (fd=%d)\n"
            "id           = %s\n"
            "xres         = %d px\n"
            "yres         = %d px\n"
            "xres_virtual = %d px\n"
            "yres_virtual = %d px\n"
            "bpp          = %d\n"
            "r            = %2u:%u\n"
            "g            = %2u:%u\n"
            "b            = %2u:%u\n",
            mFd,
            mFInfo.id,
            mVInfo.xres,
            mVInfo.yres,
            mVInfo.xres_virtual,
            mVInfo.yres_virtual,
            mVInfo.bits_per_pixel,
            mVInfo.red.offset, mVInfo.red.length,
            mVInfo.green.offset, mVInfo.green.length,
            mVInfo.blue.offset, mVInfo.blue.length
    );

    ALOGI(  "width        = %d mm (%f dpi)\n"
            "height       = %d mm (%f dpi)\n",
            mVInfo.width,  xdpi,
            mVInfo.height, ydpi
    );

    const hw_module_t *module = nullptr;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module)) {
        ALOGE("Could not get gralloc module");
        Close();
        return false;
    }

    mMemLength = roundUpToPageSize(mFInfo.line_length * mVInfo.yres_virtual);
    mMappedAddr = mmap(0, mMemLength, PROT_READ | PROT_WRITE, MAP_SHARED, mFd, 0);
    if (mMappedAddr == (void*)-1) {
        ALOGE("Error: failed to map framebuffer device to memory %d : %s", errno, strerror(errno));
        Close();
        return false;
    }

    mGrmodule = const_cast<gralloc_module_t *>(reinterpret_cast<const gralloc_module_t *>(module));

    mWidth = mVInfo.xres;
    mHeight = mVInfo.yres;
    mXdpi = xdpi;

    // ToDo: Not sure what kind of situation that surface format reported to
    //       gecko should be different then fb format.
    mSurfaceformat = mFBSurfaceformat;

    mIsEnabled = true;
    return true;
}

bool
NativeFramebufferDevice::Post(buffer_handle_t buf)
{
    android::Mutex::Autolock lock(mMutex);

    if (!mIsEnabled) {
      return false;
    }

    void *vaddr;
    if (mGrmodule->lock(mGrmodule, buf,
                        GRALLOC_USAGE_SW_READ_RARELY,
                        0, 0, mVInfo.xres, mVInfo.yres, &vaddr)) {
        ALOGE("Failed to lock buffer_handle_t");
        return false;
    }

    if (mFBSurfaceformat == HAL_PIXEL_FORMAT_RGB_565 &&
        mSurfaceformat == HAL_PIXEL_FORMAT_RGBA_8888) {
        Transform8888To565((uint8_t*)mMappedAddr, (uint8_t*)vaddr, mVInfo.xres * mVInfo.yres);
    } else {
        memcpy(mMappedAddr, vaddr, mFInfo.line_length * mVInfo.yres);
    }

    mGrmodule->unlock(mGrmodule, buf);

    mVInfo.activate = FB_ACTIVATE_VBL | FB_ACTIVATE_FORCE;

    // Please refer to KaiOS - Bug 307: FB driver needs fsync operation for
    // triggering the buffer update.
    fsync(mFd);
    if(0 > ioctl(mFd, FBIOPUT_VSCREENINFO, &mVInfo)) {
      ALOGE("FBIOPUT_VSCREENINFO failed : error on refresh");
    }

    return true;
}

bool
NativeFramebufferDevice::Close()
{
    android::Mutex::Autolock lock(mMutex);

    if (mMappedAddr) {
      munmap(mMappedAddr, mMemLength);
      mMemLength = 0;
      mMappedAddr = nullptr;
    }

    if (mFd != -1) {
        close(mFd);
        mFd = -1;
    }

    return true;
}

void
NativeFramebufferDevice::DrawSolidColorFrame()
{
    if (!mMappedAddr || mFd < 0) {
        return;
    }

    memset(mMappedAddr, 0, mMemLength);

    mVInfo.activate = FB_ACTIVATE_VBL;

    if(0 > ioctl(mFd, FBIOPUT_VSCREENINFO, &mVInfo)) {
      ALOGE("FBIOPUT_VSCREENINFO failed : error on refresh");
    }
}

bool
NativeFramebufferDevice::EnableScreen(int enabled)
{
    int mode = FB_BLANK_UNBLANK;
    bool ret = true;

    if (mFd < 0) {
        ALOGE("No framebuffer device.");
        return false;
    }

    if (!enabled) {
        mode = FB_BLANK_POWERDOWN;
    }

    {
      android::Mutex::Autolock lock(mMutex);
      mIsEnabled = enabled;
      if (!mIsEnabled) {
        DrawSolidColorFrame();
      }
    }

    if (ioctl(mFd, FBIOBLANK, mode) == -1) {
        ALOGE("FBIOBLANK failed.");
        ret = false;
    }

    return ret;
}

} // namespace mozilla
