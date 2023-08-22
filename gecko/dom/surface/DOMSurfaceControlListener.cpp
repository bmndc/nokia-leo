/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DOMSurfaceControlListener.h"
#include "nsThreadUtils.h"
#include "DOMSurfaceControl.h"
#include "CameraPreviewMediaStream.h"
#include "nsQueryObject.h"

#include "mozilla/Logging.h"

static mozilla::LazyLogModule gSurfaceControlListenerLog("DOMSurfaceControlListener");
#define SURFACECONTROL_LISTENER_LOG(type, msg) MOZ_LOG(gSurfaceControlListenerLog, type, msg)

using namespace mozilla;
using namespace mozilla::dom;

DOMSurfaceControlListener::DOMSurfaceControlListener(nsDOMSurfaceControl* aDOMSurfaceControl,
                                                     CameraPreviewMediaStream* aStream)
  : mDOMSurfaceControl(
      new nsMainThreadPtrHolder<nsISupports>(static_cast<DOMMediaStream*>(aDOMSurfaceControl)))
  , mStream(aStream)
  , mFrameNum(0)
{
  SURFACECONTROL_LISTENER_LOG(LogLevel::Debug, ("%p constructor, DOMSurfaceControl: %p", 
                              this, aDOMSurfaceControl));
}

DOMSurfaceControlListener::~DOMSurfaceControlListener()
{
  SURFACECONTROL_LISTENER_LOG(LogLevel::Debug, ("%p destructor", this));
}

// Boilerplate callback runnable
class DOMSurfaceControlListener::DOMCallback : public nsRunnable
{
public:
  explicit DOMCallback(nsMainThreadPtrHandle<nsISupports> aDOMSurfaceControl)
    : mDOMSurfaceControl(aDOMSurfaceControl)
  {
    MOZ_COUNT_CTOR(DOMSurfaceControlListener::DOMCallback);
  }

protected:
  virtual ~DOMCallback()
  {
    MOZ_COUNT_DTOR(DOMSurfaceControlListener::DOMCallback);
  }

public:
  virtual void RunCallback(nsDOMSurfaceControl* aDOMSurfaceControl) = 0;

  NS_IMETHOD
  Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    RefPtr<nsDOMSurfaceControl> surface = do_QueryObject(mDOMSurfaceControl.get());
    if (!surface) {
      return NS_ERROR_INVALID_ARG;
    }
    RunCallback(surface);
    return NS_OK;
  }

protected:
  nsMainThreadPtrHandle<nsISupports> mDOMSurfaceControl;
};

void
DOMSurfaceControlListener::OnSurfaceStateChange(SurfaceState aState,
                                                nsresult aReason,
                                                android::sp<android::IGraphicBufferProducer> aProducer)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsISupports> aDOMSurfaceControl,
             SurfaceState aState,
             nsresult aReason,
             android::sp<android::IGraphicBufferProducer>& aProducer)
      : DOMCallback(aDOMSurfaceControl)
      , mState(aState)
      , mReason(aReason)
      , mProducer(aProducer)
    { }

    void
    RunCallback(nsDOMSurfaceControl* aDOMSurfaceControl) override
    {
      aDOMSurfaceControl->OnSurfaceStateChange(mState, mReason, mProducer);
    }

  protected:
    SurfaceState mState;
    nsresult mReason;
    android::sp<android::IGraphicBufferProducer> mProducer;
  };

  NS_DispatchToMainThread(new Callback(mDOMSurfaceControl, aState, aReason, aProducer));
}

void
DOMSurfaceControlListener::OnPreviewStateChange(PreviewState aState)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsISupports> aDOMSurfaceControl,
             PreviewState aState)
      : DOMCallback(aDOMSurfaceControl)
      , mState(aState)
    { }

    void
    RunCallback(nsDOMSurfaceControl* aDOMSurfaceControl) override
    {
      aDOMSurfaceControl->OnPreviewStateChange(mState);
    }

  protected:
    PreviewState mState;
  };

  switch (aState) {
    case kPreviewStopped:
      // Clear the current frame right away, without dispatching a
      //  runnable. This is an ugly coupling between the
      //  SurfaceTextureClient and the MediaStream/ImageContainer,
      //  but without it, the preview can fail to start.
      mStream->ClearCurrentFrame();
      break;

    case kPreviewPaused:
      // In the paused state, we still want to reflect the change
      //  in preview state, but we don't want to clear the current
      //  frame as above, since doing so seems to cause genlock
      //  problems when we restart the preview. See bug 957749.
      break;

    case kPreviewStarted:
      break;

    default:
      MOZ_ASSERT_UNREACHABLE("Invalid preview state");
      return;
  }

  if (aState == kPreviewStarted) {
    mStream->ClearCurrentFrame();
  }

  mStream->OnPreviewStateChange(aState == kPreviewStarted);
  NS_DispatchToMainThread(new Callback(mDOMSurfaceControl, aState));
}

bool
DOMSurfaceControlListener::OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight)
{
  mFrameNum ++;
  SURFACECONTROL_LISTENER_LOG(LogLevel::Debug, ("%p OnNewPreviewFrame Width:%d, Height:%d, frame_num:%lld", 
                              this, aWidth, aHeight, mFrameNum));
  mStream->SetCurrentFrame(gfx::IntSize(aWidth, aHeight), aImage);
  return true;
}

void
DOMSurfaceControlListener::OnUserError(UserContext aContext, nsresult aError)
{
  class Callback : public DOMCallback
  {
  public:
    Callback(nsMainThreadPtrHandle<nsISupports> aDOMSurfaceControl,
             UserContext aContext,
             nsresult aError)
      : DOMCallback(aDOMSurfaceControl)
      , mContext(aContext)
      , mError(aError)
    { }

    virtual void
    RunCallback(nsDOMSurfaceControl* aDOMSurfaceControl) override
    {
      aDOMSurfaceControl->OnUserError(mContext, mError);
    }

  protected:
    UserContext mContext;
    nsresult mError;
  };

  NS_DispatchToMainThread(new Callback(mDOMSurfaceControl, aContext, aError));
}