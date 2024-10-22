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

#ifndef GONKDISPLAY_H
#define GONKDISPLAY_H

#include <system/window.h>
#include <utils/StrongPointer.h>
#include "mozilla/Types.h"

namespace android {
class DisplaySurface;
class IGraphicBufferProducer;
}

namespace mozilla {

typedef void * EGLDisplay;
typedef void * EGLSurface;

class MOZ_EXPORT GonkDisplay {
public:
   /**
    * This enum is for types of display. DISPLAY_PRIMARY refers to the default
    * built-in display, DISPLAY_EXTERNAL refers to displays connected with
    * HDMI, and DISPLAY_VIRTUAL are displays which makes composited output
    * available within the system. Currently, displays of external are detected
    * via the hotplug detection in HWC, and displays of virtual are connected
    * via Wifi Display.
    */
    enum DisplayType {
        DISPLAY_PRIMARY = 0,
        DISPLAY_EXTERNAL,
        DISPLAY_VIRTUAL,
        NUM_DISPLAY_TYPES
    };

    struct NativeData {
        android::sp<ANativeWindow> mNativeWindow;
#if ANDROID_VERSION >= 17
        android::sp<android::DisplaySurface> mDisplaySurface;
#endif
        float mXdpi;
        bool mComposer2DSupported;
        // True if platform is capable of notifying the system when a vsync
        // event occurs on this screen. We use it as a hint to create relative
        // VsyncSource::Display and VsyncScheduler.
        bool mVsyncSupported;
    };

    struct DisplayNativeData {
        DisplayNativeData()
            : mXdpi(0)
            , mSurfaceformat(0)
            , mWidth(0)
            , mHeight(0)
        {};

        float mXdpi;
        int32_t mSurfaceformat;
        uint32_t mWidth;
        uint32_t mHeight;
    };

    virtual void SetEnabled(bool enabled) = 0;

    virtual void SetExtEnabled(bool enabled) = 0;

    typedef void (*OnEnabledCallbackType)(bool enabled);

    virtual void OnEnabled(OnEnabledCallbackType callback) = 0;

    virtual void* GetHWCDevice() = 0;

    virtual bool IsExtFBDeviceEnabled() = 0;

    /**
     * Only GonkDisplayICS uses arguments.
     */
    virtual bool SwapBuffers(DisplayType aDisplayType) = 0;

    virtual ANativeWindowBuffer* DequeueBuffer(DisplayType dpy) = 0;

    virtual bool QueueBuffer(ANativeWindowBuffer* buf, DisplayType dpy) = 0;

    virtual void UpdateDispSurface(EGLDisplay dpy, EGLSurface sur) = 0;

    virtual NativeData GetNativeData(
        GonkDisplay::DisplayType aDisplayType,
        android::IGraphicBufferProducer* aSink = nullptr) = 0;

    virtual void NotifyBootAnimationStopped() = 0;

    virtual const DisplayNativeData& GetDispNativeData(
        GonkDisplay::DisplayType aDisplayType) {
        return mDispNativeData[aDisplayType];
    }

#ifdef ENABLE_TEE_SUI
    virtual int EnableSecureUI(bool enabled) = 0;

    virtual bool GetSecureUIState() = 0;
#endif

    virtual int TryLockScreen() = 0;

    virtual void UnlockScreen() = 0;

    virtual android::sp<ANativeWindow> GetSurface() = 0;

protected:
    DisplayNativeData mDispNativeData[NUM_DISPLAY_TYPES];
};

MOZ_EXPORT __attribute__ ((weak))
GonkDisplay* GetGonkDisplay();

}
#endif /* GONKDISPLAY_H */
