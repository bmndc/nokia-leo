/* vim: set sw=4 ts=4 et ft=cpp: tw=80: */
/* Copyright 2013 Mozilla Foundation and Mozilla contributors
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

#include "GonkDisplayJB.h"
#if ANDROID_VERSION == 17
#include <gui/SurfaceTextureClient.h>
#else
#include <gui/Surface.h>
#include <gui/GraphicBufferAlloc.h>
#endif

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <hardware/power.h>
#include <suspend/autosuspend.h>

#include "cutils/properties.h"
#if ANDROID_VERSION >= 19
#include "VirtualDisplaySurface.h"
#endif
#include "FramebufferSurface.h"
#if ANDROID_VERSION == 17
#include "GraphicBufferAlloc.h"
#endif
#include "mozilla/Assertions.h"

#define DEFAULT_XDPI 75.0
// This define should be passed from gonk-misc and depends on device config.
// #define GET_FRAMEBUFFER_FORMAT_FROM_HWC

using namespace android;

namespace mozilla {

static GonkDisplayJB* sGonkDisplay = nullptr;

GonkDisplayJB::GonkDisplayJB()
    : mModule(nullptr)
    , mFBModule(nullptr)
    , mHwc(nullptr)
    , mFBDevice(nullptr)
    , mExtFBDevice(nullptr)
    , mPowerModule(nullptr)
    , mList(nullptr)
    , mExtList(nullptr)
    , mEnabledCallback(nullptr)
    , mFBEnabled(true) // Initial value should sync with hal::GetScreenEnabled()
    , mExtFBEnabled(true) // Initial value should sync with hal::GetExtScreenEnabled()
#ifdef ENABLE_TEE_SUI
    , mStateSecureUI(false)
#endif
    , mExtDisplaySuppported(false)
{
    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &mFBModule);
	ALOGI("conqueror: GonkDisplayJB  graphic mFBModule=0x%x\n",mFBModule?mFBModule:0);

    ALOGW_IF(err, "%s module not found", GRALLOC_HARDWARE_MODULE_ID);
    if (!err) {
        err = framebuffer_open(mFBModule, &mFBDevice);
        ALOGW_IF(err, "could not open framebuffer");
    }

	ALOGI("conqueror: graphic mFBDevice=0x%x,err=%d\n",mFBDevice?mFBDevice:0,err);

    DisplayNativeData &dispData = mDispNativeData[DISPLAY_PRIMARY];
    if (!err && mFBDevice) {
        dispData.mWidth = mFBDevice->width;
        dispData.mHeight = mFBDevice->height;
        dispData.mXdpi = mFBDevice->xdpi;
        /* The emulator actually reports RGBA_8888, but EGL doesn't return
         * any matching configuration. We force RGBX here to fix it. */
        dispData.mSurfaceformat = HAL_PIXEL_FORMAT_RGBX_8888;
    }

    err = hw_get_module(HWC_HARDWARE_MODULE_ID, &mModule);
    ALOGW_IF(err, "%s module not found", HWC_HARDWARE_MODULE_ID);
    if (!err) {
		ALOGW("conqueror:GonkDisplayJB, will call hwc_Open_1 \n");
        err = hwc_open_1(mModule, &mHwc);
        ALOGE_IF(err, "%s device failed to initialize (%s)",
                 HWC_HARDWARE_COMPOSER, strerror(-err));
    }

	ALOGI("conqueror: HWC  mHwc=0x%x\n",mHwc?mHwc:0);
	ALOGI("conqueror: HWC  mHwc->common.version=0x%x\n",mHwc->common.version);
	ALOGI("conqueror: HWC  common.version_1_0=0x%x,1_5=0x%x\n",HWC_DEVICE_API_VERSION_1_0,HWC_DEVICE_API_VERSION_1_5);

    /* Fallback on the FB rendering path instead of trying to support HWC 1.0 */
    if (!err && mHwc->common.version == HWC_DEVICE_API_VERSION_1_0) {
        hwc_close_1(mHwc);
        mHwc = nullptr;
    }

    if (!err && mHwc) {
        if (mFBDevice) {
            framebuffer_close(mFBDevice);
            mFBDevice = nullptr;
        }

        int32_t values[4];
        const uint32_t attrs[] = {
            HWC_DISPLAY_WIDTH,
            HWC_DISPLAY_HEIGHT,
            HWC_DISPLAY_DPI_X,
#ifdef GET_FRAMEBUFFER_FORMAT_FROM_HWC
            HWC_DISPLAY_FBFORMAT,
#endif
            HWC_DISPLAY_NO_ATTRIBUTE
        };
        mHwc->getDisplayAttributes(mHwc, 0, 0, attrs, values);

        dispData.mWidth = values[0];
        dispData.mHeight = values[1];
        dispData.mXdpi = values[2] / 1000.0f;
#ifdef GET_FRAMEBUFFER_FORMAT_FROM_HWC
        dispData.mSurfaceformat = values[3];
#else
        dispData.mSurfaceformat = HAL_PIXEL_FORMAT_RGBA_8888;
#endif
    }

    err = hw_get_module(POWER_HARDWARE_MODULE_ID,
                        (hw_module_t const**)&mPowerModule);
    if (!err)
        mPowerModule->init(mPowerModule);
    ALOGW_IF(err, "Couldn't load %s module (%s)", POWER_HARDWARE_MODULE_ID, strerror(-err));

    mAlloc = new GraphicBufferAlloc();

    CreateFramebufferSurface(mSTClient,
                             mDispSurface,
                             dispData.mWidth,
                             dispData.mHeight,
                             dispData.mSurfaceformat);

    mList = (hwc_display_contents_1_t *)calloc(1, sizeof(*mList) + (sizeof(hwc_layer_1_t)*2));

    uint32_t usage = GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER;

    if (mHwc) {
        PowerOnDisplay(HWC_DISPLAY_PRIMARY);
        CreateFramebufferSurface(mBootAnimSTClient,
                                 mBootAnimDispSurface,
                                 dispData.mWidth,
                                 dispData.mHeight,
                                 dispData.mSurfaceformat);
    } else if (mFBDevice) {
        // If display uses fb, they can not use single buffer for boot animation
        mSTClient->perform(mSTClient.get(), NATIVE_WINDOW_SET_BUFFER_COUNT, 2);
        mSTClient->perform(mSTClient.get(), NATIVE_WINDOW_SET_USAGE, usage);
    }

    // Set this prop to turn on native framebuffer support for fb1,
    // such as external screen of flip phone.
    // ro.h5.display.fb1_backlightdev=__full_backlight_device_path__
    //
    // ex: Octans will set this prop in device/t2m/octans/octans.mk
    DisplayNativeData &extDispData = mDispNativeData[DISPLAY_EXTERNAL];
    mExtFBDevice = NativeFramebufferDevice::Create();

    if (mExtFBDevice) {
        if (mExtFBDevice->Open()) {
            extDispData.mWidth = mExtFBDevice->mWidth;
            extDispData.mHeight = mExtFBDevice->mHeight;
            extDispData.mSurfaceformat = mExtFBDevice->mSurfaceformat;
            extDispData.mXdpi = mExtFBDevice->mXdpi;

            mExtFBDevice->EnableScreen(true);
            CreateFramebufferSurface(mExtSTClient,
                                     mExtDispSurface,
                                     extDispData.mWidth,
                                     extDispData.mHeight,
                                     extDispData.mSurfaceformat);
            mExtSTClient->perform(mExtSTClient.get(), NATIVE_WINDOW_SET_BUFFER_COUNT, 2);
            mExtSTClient->perform(mExtSTClient.get(), NATIVE_WINDOW_SET_USAGE, usage);
            mExtDisplaySuppported = true;
        } else {
            delete mExtFBDevice;
            mExtFBDevice = nullptr;
        }
    } else if (mHwc) {
        int32_t values[4];
        const uint32_t attrs[] = {
            HWC_DISPLAY_WIDTH,
            HWC_DISPLAY_HEIGHT,
            HWC_DISPLAY_DPI_X,
#ifdef GET_FRAMEBUFFER_FORMAT_FROM_HWC
            HWC_DISPLAY_FBFORMAT,
#endif
            HWC_DISPLAY_NO_ATTRIBUTE
        };
        uint32_t config = 0;
        size_t numConfigs = 1;

        if ((mHwc->getDisplayConfigs(mHwc, HWC_DISPLAY_EXTERNAL, &config, &numConfigs) == 0)
          && (mHwc->getDisplayAttributes(mHwc, HWC_DISPLAY_EXTERNAL, config, attrs, values) == 0)) {
            extDispData.mWidth = values[0];
            extDispData.mHeight = values[1];
            extDispData.mXdpi = values[2] ? values[2] / 1000.f : DEFAULT_XDPI;
#ifdef GET_FRAMEBUFFER_FORMAT_FROM_HWC
            extDispData.mSurfaceformat = values[3];
#else
            extDispData.mSurfaceformat = HAL_PIXEL_FORMAT_RGBA_8888;
#endif
            mExtList = (hwc_display_contents_1_t *)calloc(1, sizeof(*mExtList) + (sizeof(hwc_layer_1_t)*2));
            PowerOnDisplay(HWC_DISPLAY_EXTERNAL);
            CreateFramebufferSurface(mExtSTClient,
                                     mExtDispSurface,
                                     extDispData.mWidth,
                                     extDispData.mHeight,
                                     extDispData.mSurfaceformat);
            mExtSTClient->perform(mExtSTClient.get(), NATIVE_WINDOW_SET_BUFFER_COUNT, 2);
            mExtSTClient->perform(mExtSTClient.get(), NATIVE_WINDOW_SET_USAGE, usage);
            mExtDisplaySuppported = true;

        }
    }

    if (!mExtDisplaySuppported) {
        // Set mExtFBEnabled to false if no support externl screen.
        mExtFBEnabled = false;
    }
}

GonkDisplayJB::~GonkDisplayJB()
{
    if (mHwc)
        hwc_close_1(mHwc);
    if (mFBDevice)
        framebuffer_close(mFBDevice);
    if (mExtFBDevice)
        delete mExtFBDevice;
    free(mList);
    if (mExtList)
        free(mExtList);
}

void
GonkDisplayJB::CreateFramebufferSurface(android::sp<ANativeWindow>& aNativeWindow,
                                        android::sp<android::DisplaySurface>& aDisplaySurface,
                                        uint32_t aWidth, uint32_t aHeight, int32_t aFormat)
{
#if ANDROID_VERSION >= 21
    sp<IGraphicBufferProducer> producer;
    sp<IGraphicBufferConsumer> consumer;
    BufferQueue::createBufferQueue(&producer, &consumer, mAlloc);
#elif ANDROID_VERSION >= 19
    sp<BufferQueue> consumer = new BufferQueue(mAlloc);
    sp<IGraphicBufferProducer> producer = consumer;
#elif ANDROID_VERSION >= 18
    sp<BufferQueue> consumer = new BufferQueue(true, mAlloc);
    sp<IGraphicBufferProducer> producer = consumer;
#else
    sp<BufferQueue> consumer = new BufferQueue(true, mAlloc);
#endif

    aDisplaySurface = new FramebufferSurface(0, aWidth, aHeight, aFormat, consumer);

#if ANDROID_VERSION == 17
    aNativeWindow = new SurfaceTextureClient(
        static_cast<sp<ISurfaceTexture>>(aDisplaySurface->getBufferQueue()));
#else
    aNativeWindow = new Surface(producer);
#endif
}

void
GonkDisplayJB::CreateVirtualDisplaySurface(android::IGraphicBufferProducer* aSink,
                                           android::sp<ANativeWindow>& aNativeWindow,
                                           android::sp<android::DisplaySurface>& aDisplaySurface)
{
#if ANDROID_VERSION >= 21
    sp<IGraphicBufferProducer> producer;
    sp<IGraphicBufferConsumer> consumer;
    BufferQueue::createBufferQueue(&producer, &consumer, mAlloc);
#elif ANDROID_VERSION >= 19
    sp<BufferQueue> consumer = new BufferQueue(mAlloc);
    sp<IGraphicBufferProducer> producer = consumer;
#endif

#if ANDROID_VERSION >= 19
    sp<VirtualDisplaySurface> virtualDisplay;
    virtualDisplay = new VirtualDisplaySurface(-1, aSink, producer, consumer, String8("VirtualDisplaySurface"));
    aDisplaySurface = virtualDisplay;
    aNativeWindow = new Surface(virtualDisplay);
#endif
}

#ifdef ENABLE_TEE_SUI
int GonkDisplayJB::EnableSecureUI(bool enabled)
{
    Mutex::Autolock lock(mPrimaryScreenLock);
    if (mStateSecureUI == enabled) {
        ALOGE("Invalid state transition: from %d to %d", mStateSecureUI, enabled);
        return -1;
    }

    mStateSecureUI = enabled;
    if (enabled) {
        mHwc->setRegulatorOverride(mHwc, HWC_DISPLAY_PRIMARY, enabled);
        SetEnabled(!enabled);
        mHwc->setBacklightOverride(mHwc, HWC_DISPLAY_PRIMARY, enabled);
    } else {
        mHwc->setBacklightOverride(mHwc, HWC_DISPLAY_PRIMARY, enabled);
        SetEnabled(!enabled);
        mHwc->setRegulatorOverride(mHwc, HWC_DISPLAY_PRIMARY, enabled);
    }
    return 0;
}

bool
GonkDisplayJB::GetSecureUIState()
{
    return mStateSecureUI;
}
#endif

void
GonkDisplayJB::SetEnabled(bool enabled)
{
    if (enabled) {
        autosuspend_disable();
        mPowerModule->setInteractive(mPowerModule, true);
    }

    if (!enabled && mEnabledCallback) {
        mEnabledCallback(enabled);
    }

#if ANDROID_VERSION >= 21
    if (mHwc) {
        if (mHwc->common.version >= HWC_DEVICE_API_VERSION_1_4) {
            mHwc->setPowerMode(mHwc, HWC_DISPLAY_PRIMARY,
                (enabled ? HWC_POWER_MODE_NORMAL : HWC_POWER_MODE_OFF));
        } else {
            mHwc->blank(mHwc, HWC_DISPLAY_PRIMARY, !enabled);
        }
    } else if (mFBDevice && mFBDevice->enableScreen) {
        mFBDevice->enableScreen(mFBDevice, enabled);
    }
#else
    if (mHwc && mHwc->blank) {
        mHwc->blank(mHwc, HWC_DISPLAY_PRIMARY, !enabled);
    } else if (mFBDevice && mFBDevice->enableScreen) {
        mFBDevice->enableScreen(mFBDevice, enabled);
    }
#endif
    mFBEnabled = enabled;

    if (enabled && mEnabledCallback) {
        mEnabledCallback(enabled);
    }

    if (!enabled && !mExtFBEnabled) {
        autosuspend_enable();
        mPowerModule->setInteractive(mPowerModule, false);
    }
}

int GonkDisplayJB::TryLockScreen()
{
    int ret = mPrimaryScreenLock.tryLock();
    return ret;
}

void GonkDisplayJB::UnlockScreen()
{
    mPrimaryScreenLock.unlock();
}

void
GonkDisplayJB::SetExtEnabled(bool enabled)
{
    if (!mExtDisplaySuppported) {
        return;
    }

    if (enabled) {
        autosuspend_disable();
        mPowerModule->setInteractive(mPowerModule, true);
    }

    if (mExtFBDevice) {
        mExtFBDevice->EnableScreen(enabled);
    } else if (mHwc) {
#if ANDROID_VERSION >= 21
        if (mHwc->common.version >= HWC_DEVICE_API_VERSION_1_4) {
            mHwc->setPowerMode(mHwc, HWC_DISPLAY_EXTERNAL,
                              (enabled ? HWC_POWER_MODE_NORMAL : HWC_POWER_MODE_OFF));
        } else {
            mHwc->blank(mHwc, HWC_DISPLAY_EXTERNAL, !enabled);
        }
#else
        if (mHwc->blank) {
            mHwc->blank(mHwc, HWC_DISPLAY_EXTERNAL, !enabled);
        }
#endif
    }

    mExtFBEnabled = enabled;

    SwapBuffers(DISPLAY_EXTERNAL);

    if (!enabled && !mFBEnabled) {
        autosuspend_enable();
        mPowerModule->setInteractive(mPowerModule, false);
    }
}

void
GonkDisplayJB::OnEnabled(OnEnabledCallbackType callback)
{
    mEnabledCallback = callback;
}

void*
GonkDisplayJB::GetHWCDevice()
{
    return mHwc;
}

bool
GonkDisplayJB::IsExtFBDeviceEnabled()
{
    return mExtDisplaySuppported;
}

bool
GonkDisplayJB::SwapBuffers(DisplayType aDisplayType)
{
    if (aDisplayType == DISPLAY_PRIMARY) {
        // Should be called when composition rendering is complete for a frame.
        // Only HWC v1.0 needs this call.
        // HWC > v1.0 case, do not call compositionComplete().
        // mFBDevice is present only when HWC is v1.0.
        if (mFBDevice && mFBDevice->compositionComplete) {
            mFBDevice->compositionComplete(mFBDevice);
        }
        return Post(mDispSurface->lastHandle,
                    mDispSurface->GetPrevDispAcquireFd(),
                    DISPLAY_PRIMARY);

    } else if (aDisplayType == DISPLAY_EXTERNAL) {
        if (mExtDisplaySuppported) {
            return Post(mExtDispSurface->lastHandle,
                        mExtDispSurface->GetPrevDispAcquireFd(),
                        DISPLAY_EXTERNAL);
        }

        return false;
    }

    return false;
}

bool
GonkDisplayJB::Post(buffer_handle_t buf, int fence, DisplayType aDisplayType)
{
    if (aDisplayType == DISPLAY_PRIMARY) {
        if (!mHwc) {
            if (fence >= 0) {
                android::sp<Fence> fenceObj = new Fence(fence);
                fenceObj->waitForever("GonkDisplayJB::Post");
            }
            return !mFBDevice->post(mFBDevice, buf);
        }
        return HwcPost(buf, fence, aDisplayType, mList);
    } else if (aDisplayType == DISPLAY_EXTERNAL) {
        // Only support fb1 for certain device, use hwc to control
        // external screen in general case.
        if (mExtFBDevice) {
            if (fence >= 0) {
                android::sp<Fence> fenceObj = new Fence(fence);
                fenceObj->waitForever("GonkDisplayJB::Post");
            }
            return mExtFBDevice->Post(buf);
        }
        return HwcPost(buf, fence, aDisplayType, mExtList);
    }

    return false;
}

ANativeWindowBuffer*
GonkDisplayJB::DequeueBuffer(DisplayType aDisplayType)
{
    // Check for bootAnim or normal display flow.
    sp<ANativeWindow> nativeWindow;
    if (aDisplayType == DISPLAY_PRIMARY) {
        nativeWindow =
            !mBootAnimSTClient.get() ? mSTClient : mBootAnimSTClient;
    } else if (aDisplayType == DISPLAY_EXTERNAL) {
        if (mExtDisplaySuppported) {
            nativeWindow = mExtSTClient;
        }
    }

    if (!nativeWindow.get()) {
        return nullptr;
    }

    ANativeWindowBuffer *buf;
    int fenceFd = -1;
    nativeWindow->dequeueBuffer(nativeWindow.get(), &buf, &fenceFd);
    sp<Fence> fence(new Fence(fenceFd));
#if ANDROID_VERSION == 17
    fence->waitForever(1000, "GonkDisplayJB_DequeueBuffer");
    // 1000 is what Android uses. It is a warning timeout in ms.
    // This timeout was removed in ANDROID_VERSION 18.
#else
    fence->waitForever("GonkDisplayJB_DequeueBuffer");
#endif
    return buf;
}

bool
GonkDisplayJB::QueueBuffer(ANativeWindowBuffer* buf, DisplayType aDisplayType)
{
    bool success = false;
    int error = DoQueueBuffer(buf, aDisplayType);

    sp<DisplaySurface> displaySurface;
    if (aDisplayType == DISPLAY_PRIMARY) {
        displaySurface =
            !mBootAnimSTClient.get() ? mDispSurface : mBootAnimDispSurface;
    } else if (aDisplayType == DISPLAY_EXTERNAL) {
        if (mExtDisplaySuppported) {
            displaySurface = mExtDispSurface;
        }
    }

    if (!displaySurface.get()) {
        return false;
    }

    success = Post(displaySurface->lastHandle,
                   displaySurface->GetPrevDispAcquireFd(),
                   aDisplayType);

    return error == 0 && success;
}

int
GonkDisplayJB::DoQueueBuffer(ANativeWindowBuffer* buf, DisplayType aDisplayType)
{
    int error = 0;
    sp<ANativeWindow> nativeWindow;
    if (aDisplayType == DISPLAY_PRIMARY) {
        nativeWindow =
            !mBootAnimSTClient.get() ? mSTClient : mBootAnimSTClient;
    } else if (aDisplayType == DISPLAY_EXTERNAL) {
        if (mExtDisplaySuppported) {
            nativeWindow = mExtSTClient;
        }
    }

    if (!nativeWindow.get()) {
        return error;
    }

    error = mSTClient->queueBuffer(nativeWindow.get(), buf, -1);

    return error;
}

void
GonkDisplayJB::UpdateDispSurface(EGLDisplay dpy, EGLSurface sur)
{
    if (sur != EGL_NO_SURFACE) {
      eglSwapBuffers(dpy, sur);
    } else {
      // When BasicCompositor is used as Compositor,
      // EGLSurface does not exit.
      ANativeWindowBuffer* buf = DequeueBuffer(DISPLAY_PRIMARY);
      DoQueueBuffer(buf, DISPLAY_PRIMARY);
    }
}

void
GonkDisplayJB::NotifyBootAnimationStopped()
{
    if (mBootAnimSTClient.get()) {
        mBootAnimSTClient = nullptr;
        mBootAnimDispSurface = nullptr;
    }
}

int
GonkDisplayJB::HwcPost(buffer_handle_t buf, int fence, DisplayType aDisplayType, hwc_display_contents_1_t* aList)
{
    DisplayNativeData &dispData = mDispNativeData[aDisplayType];

    hwc_display_contents_1_t *displays[HWC_NUM_DISPLAY_TYPES] = {NULL};
    const hwc_rect_t r = { 0, 0, static_cast<int>(dispData.mWidth), static_cast<int>(dispData.mHeight) };
    displays[aDisplayType] = aList;
    aList->retireFenceFd = -1;
    aList->numHwLayers = 2;
    aList->flags = HWC_GEOMETRY_CHANGED;
#if ANDROID_VERSION >= 18
    aList->outbuf = nullptr;
    aList->outbufAcquireFenceFd = -1;
#endif
    aList->hwLayers[0].compositionType = HWC_FRAMEBUFFER;
    aList->hwLayers[0].hints = 0;
    /* Skip this layer so the hwc module doesn't complain about null handles */
    aList->hwLayers[0].flags = HWC_SKIP_LAYER;
    aList->hwLayers[0].backgroundColor = {0};
    aList->hwLayers[0].acquireFenceFd = -1;
    aList->hwLayers[0].releaseFenceFd = -1;
    /* hwc module checks displayFrame even though it shouldn't */
    aList->hwLayers[0].displayFrame = r;
    aList->hwLayers[1].compositionType = HWC_FRAMEBUFFER_TARGET;
    aList->hwLayers[1].hints = 0;
    aList->hwLayers[1].flags = 0;
    aList->hwLayers[1].handle = buf;
    aList->hwLayers[1].transform = 0;
    aList->hwLayers[1].blending = HWC_BLENDING_NONE;
#if ANDROID_VERSION >= 19
    if (mHwc->common.version >= HWC_DEVICE_API_VERSION_1_3) {
        aList->hwLayers[1].sourceCropf.left = 0;
        aList->hwLayers[1].sourceCropf.top = 0;
        aList->hwLayers[1].sourceCropf.right = dispData.mWidth;
        aList->hwLayers[1].sourceCropf.bottom = dispData.mHeight;
    } else {
        aList->hwLayers[1].sourceCrop = r;
    }
#else
    aList->hwLayers[1].sourceCrop = r;
#endif
    aList->hwLayers[1].displayFrame = r;
    aList->hwLayers[1].visibleRegionScreen.numRects = 1;
    aList->hwLayers[1].visibleRegionScreen.rects = &aList->hwLayers[1].displayFrame;
    aList->hwLayers[1].acquireFenceFd = fence;
    aList->hwLayers[1].releaseFenceFd = -1;
#if ANDROID_VERSION >= 18
    aList->hwLayers[1].planeAlpha = 0xFF;
#endif
    mHwc->prepare(mHwc, HWC_NUM_DISPLAY_TYPES, displays);
    int err = mHwc->set(mHwc, HWC_NUM_DISPLAY_TYPES, displays);
    if (aDisplayType == DISPLAY_PRIMARY) {
        if (!mBootAnimDispSurface.get()) {
            mDispSurface->setReleaseFenceFd(aList->hwLayers[1].releaseFenceFd);
        } else {
            mBootAnimDispSurface->setReleaseFenceFd(aList->hwLayers[1].releaseFenceFd);
        }
    }
    else {
        mExtDispSurface->setReleaseFenceFd(aList->hwLayers[1].releaseFenceFd);
    }

    if (aList->retireFenceFd >= 0)
        close(aList->retireFenceFd);
    return !err;
}

void
GonkDisplayJB::PowerOnDisplay(int aDpy)
{
    MOZ_ASSERT(mHwc);
#if ANDROID_VERSION >= 21
    if (mHwc->common.version >= HWC_DEVICE_API_VERSION_1_4) {
		ALOGE("conqueror: GonkDisplayJBi call setPowerMode\n ");
        mHwc->setPowerMode(mHwc, aDpy, HWC_POWER_MODE_NORMAL);
    } else {
        mHwc->blank(mHwc, aDpy, 0);
    }
#else
    mHwc->blank(mHwc, aDpy, 0);
#endif
}

GonkDisplay::NativeData
GonkDisplayJB::GetNativeData(GonkDisplay::DisplayType aDisplayType,
                             android::IGraphicBufferProducer* aSink)
{
    NativeData data;

    if (aDisplayType == DISPLAY_PRIMARY) {
        data.mNativeWindow = mSTClient;
        data.mDisplaySurface = mDispSurface;
        data.mXdpi = mDispNativeData[DISPLAY_PRIMARY].mXdpi;
        data.mComposer2DSupported = true;
        data.mVsyncSupported = true;
    } else if (aDisplayType == DISPLAY_EXTERNAL) {
        if (mExtFBDevice) {
            data.mNativeWindow = mExtSTClient;
            data.mDisplaySurface = mExtDispSurface;
            data.mXdpi = mDispNativeData[DISPLAY_EXTERNAL].mXdpi;
            data.mComposer2DSupported = false;
            data.mVsyncSupported = false;
        } else {
            int32_t values[3];
            const uint32_t attrs[] = {
                HWC_DISPLAY_WIDTH,
                HWC_DISPLAY_HEIGHT,
                HWC_DISPLAY_DPI_X,
                HWC_DISPLAY_NO_ATTRIBUTE
            };
            uint32_t config = 0;
            size_t numConfigs = 1;

            mHwc->getDisplayConfigs(mHwc, aDisplayType, &config, &numConfigs);
            mHwc->getDisplayAttributes(mHwc, aDisplayType, config, attrs, values);
            int width = values[0];
            int height = values[1];
            // FIXME!! values[2] returns 0 for external display, which doesn't
            // sound right, Bug 1169176 is the follow-up bug for this issue.
            data.mXdpi = values[2] ? values[2] / 1000.f : DEFAULT_XDPI;
            data.mComposer2DSupported = true;
            data.mVsyncSupported = false;
            PowerOnDisplay(HWC_DISPLAY_EXTERNAL);
            CreateFramebufferSurface(data.mNativeWindow,
                                     data.mDisplaySurface,
                                     width,
                                     height,
                                     mDispNativeData[DISPLAY_PRIMARY].mSurfaceformat);
        }
    } else if (aDisplayType == DISPLAY_VIRTUAL) {
        data.mXdpi = mDispNativeData[DISPLAY_PRIMARY].mXdpi;
        CreateVirtualDisplaySurface(aSink,
                                    data.mNativeWindow,
                                    data.mDisplaySurface);
    }

    return data;
}

__attribute__ ((visibility ("default")))
GonkDisplay*
GetGonkDisplay()
{
    if (!sGonkDisplay)
        sGonkDisplay = new GonkDisplayJB();
    return sGonkDisplay;
}

} // namespace mozilla
