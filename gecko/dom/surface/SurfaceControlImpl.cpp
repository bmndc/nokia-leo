/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "SurfaceControlImpl.h"
#include "base/basictypes.h"
#include "mozilla/Assertions.h"
#include "mozilla/unused.h"
#include "nsPrintfCString.h"
#include "nsIWeakReferenceUtils.h"
#include "nsGlobalWindow.h"
#include "DeviceStorageFileDescriptor.h"
#include "SurfaceControlListener.h"

using namespace mozilla;

/* static */ StaticRefPtr<nsIThread> SurfaceControlImpl::sSurfaceThread;

SurfaceControlImpl::SurfaceControlImpl()
  : mListenerLock("mozilla::surface::SurfaceControlImpl.Listeners")
  , mPreviewState(SurfaceControlListener::kPreviewStopped)
  , mSurfaceState(SurfaceControlListener::kSurfaceUninitialized)
{
  class Delegate : public nsRunnable
  {
  public:
    NS_IMETHOD
    Run()
    {
      char stackBaseGuess;
      profiler_register_thread("SurfaceThread", &stackBaseGuess);
      return NS_OK;
    }
  };

  // reuse the same surface thread to conserve resources
  nsCOMPtr<nsIThread> ct = do_QueryInterface(sSurfaceThread);
  if (ct) {
    mSurfaceThread = ct.forget();
  } else {
    nsresult rv = NS_NewNamedThread("SurfaceThread", getter_AddRefs(mSurfaceThread));
    if (NS_FAILED(rv)) {
      MOZ_CRASH("Failed to create new Surface Thread");
    }
    mSurfaceThread->Dispatch(new Delegate(), NS_DISPATCH_NORMAL);
    sSurfaceThread = mSurfaceThread;
  }
}

SurfaceControlImpl::~SurfaceControlImpl()
{
}

void
SurfaceControlImpl::OnSurfaceStateChange(SurfaceControlListener::SurfaceState aNewState,
                                         nsresult aReason,
                                         android::sp<android::IGraphicBufferProducer> aProducer)
{
  // This callback can run on threads other than the Main Thread and
  //  the Surface Thread. On Gonk, it may be called from the surface's
  //  local binder thread, should the mediaserver process die.
  MutexAutoLock lock(mListenerLock);

  if (aNewState == mSurfaceState) {
    return;
  }

  MOZ_ASSERT(aNewState >= 0);
  mSurfaceState = aNewState;

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    SurfaceControlListener* l = mListeners[i];
    l->OnSurfaceStateChange(mSurfaceState, aReason, aProducer);
  }
}

void
SurfaceControlImpl::OnPreviewStateChange(SurfaceControlListener::PreviewState aNewState)
{
  // This callback runs on the Main Thread and the Camera Thread, and
  //  may run on the local binder thread, should the mediaserver
  //  process die.
  MutexAutoLock lock(mListenerLock);

  if (aNewState == mPreviewState) {
    return;
  }

  MOZ_ASSERT(aNewState >= 0);

  mPreviewState = aNewState;

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    SurfaceControlListener* l = mListeners[i];
    l->OnPreviewStateChange(mPreviewState);
  }
}

bool
SurfaceControlImpl::OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight)
{
  // This function runs on neither the Main Thread nor the Surface Thread.
  //  On Gonk, it is called from the surface driver's preview thread.
  MutexAutoLock lock(mListenerLock);

  bool consumed = false;

  for (uint32_t i = 0; i < mListeners.Length(); ++i) {
    SurfaceControlListener* l = mListeners[i];
    consumed = l->OnNewPreviewFrame(aImage, aWidth, aHeight) || consumed;
  }
  return consumed;
}

// Surface control asynchronous message; these are dispatched from
//  the Main Thread to the Surface Thread, where they are consumed.

class SurfaceControlImpl::ControlMessage : public nsRunnable
{
public:
  ControlMessage(SurfaceControlImpl* aSurfaceControl,
                 SurfaceControlListener::UserContext aContext)
    : mSurfaceControl(aSurfaceControl)
    , mContext(aContext)
  { }

  virtual nsresult RunImpl() = 0;

  NS_IMETHOD
  Run() override
  {
    MOZ_ASSERT(mSurfaceControl);
    MOZ_ASSERT(NS_GetCurrentThread() == mSurfaceControl->mSurfaceThread);

    nsresult rv = RunImpl();
    if (NS_FAILED(rv)) {
      nsPrintfCString msg("Surface control API(%d) failed with 0x%x", mContext, rv);
      NS_WARNING(msg.get());
      //mSurfaceControl->OnUserError(mContext, rv);
    }

    return NS_OK;
  }

protected:
  virtual ~ControlMessage() { }

  RefPtr<SurfaceControlImpl> mSurfaceControl;
  SurfaceControlListener::UserContext mContext;
};

nsresult
SurfaceControlImpl::Dispatch(ControlMessage* aMessage)
{
  nsresult rv = mSurfaceThread->Dispatch(aMessage, NS_DISPATCH_NORMAL);
  if (NS_SUCCEEDED(rv)) {
    return NS_OK;
  }

  nsPrintfCString msg("Failed to dispatch surface control message (0x%x)", rv);
  NS_WARNING(msg.get());
  return NS_ERROR_FAILURE;
}

class SurfaceControlImpl::ListenerMessage : public SurfaceControlImpl::ControlMessage
{
public:
  ListenerMessage(SurfaceControlImpl* aSurfaceControl,
                  SurfaceControlListener* aListener)
    : ControlMessage(aSurfaceControl, SurfaceControlListener::kInUnspecified)
    , mListener(aListener)
  { }

protected:
  RefPtr<SurfaceControlListener> mListener;
};

void
SurfaceControlImpl::AddListenerImpl(already_AddRefed<SurfaceControlListener> aListener)
{
  MutexAutoLock lock(mListenerLock);

  *mListeners.AppendElement() = aListener;
}

void
SurfaceControlImpl::AddListener(SurfaceControlListener* aListener)
 {
  class Message : public ListenerMessage
  {
  public:
    Message(SurfaceControlImpl* aSurfaceControl,
            SurfaceControlListener* aListener)
      : ListenerMessage(aSurfaceControl, aListener)
    { }

    nsresult
    RunImpl() override
    {
      mSurfaceControl->AddListenerImpl(mListener.forget());
      return NS_OK;
    }
  };

  if (aListener) {
    Dispatch(new Message(this, aListener));
  }
}

void
SurfaceControlImpl::RemoveListenerImpl(SurfaceControlListener* aListener)
{
  MutexAutoLock lock(mListenerLock);

  RefPtr<SurfaceControlListener> l(aListener);
  mListeners.RemoveElement(l);
  // XXXmikeh - do we want to notify the listener that it has been removed?
}

void
SurfaceControlImpl::RemoveListener(SurfaceControlListener* aListener)
 {
  class Message : public ListenerMessage
  {
  public:
    Message(SurfaceControlImpl* aSurfaceControl, SurfaceControlListener* aListener)
      : ListenerMessage(aSurfaceControl, aListener)
    { }

    nsresult
    RunImpl() override
    {
      mSurfaceControl->RemoveListenerImpl(mListener);
      return NS_OK;
    }
  };

  if (aListener) {
    Dispatch(new Message(this, aListener));
  }
}


nsresult 
SurfaceControlImpl::SetDataSourceSize(const ISurfaceControl::Size& aSize)
{
  class Message : public ControlMessage
  {
  public:
    Message(SurfaceControlImpl* aSurfaceControl,
            SurfaceControlListener::UserContext aContext,
            const ISurfaceControl::Size& aSize)
      : ControlMessage(aSurfaceControl, aContext)
      , mSize(aSize)
    {
    }

    nsresult
    RunImpl() override
    {
      return mSurfaceControl->SetDataSourceSizeImpl(mSize);
    }

  protected:
    ISurfaceControl::Size mSize;
  };

  return Dispatch(new Message(this, SurfaceControlListener::kInSetDataSourceSize, aSize));
}

nsresult
SurfaceControlImpl::Start(const Configuration* aConfig)
{
  class Message : public ControlMessage
  {
  public:
    Message(SurfaceControlImpl* aSurfaceControl,
            SurfaceControlListener::UserContext aContext,
            const Configuration* aConfig)
      : ControlMessage(aSurfaceControl, aContext)
      , mHaveInitialConfig(false)
    {
      if (aConfig) {
        mConfig = *aConfig;
        mHaveInitialConfig = true;
      }
    }

    nsresult
    RunImpl() override
    {
      if (mHaveInitialConfig) {
        return mSurfaceControl->StartImpl(&mConfig);
      }
      return mSurfaceControl->StartImpl();
    }

  protected:
    bool mHaveInitialConfig;
    Configuration mConfig;
  };

  return Dispatch(new Message(this, SurfaceControlListener::kInCreateSurface, aConfig));
}

nsresult
SurfaceControlImpl::StartPreview()
{
  class Message : public ControlMessage
  {
  public:
    Message(SurfaceControlImpl* aSurfaceControl,
            SurfaceControlListener::UserContext aContext)
      : ControlMessage(aSurfaceControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mSurfaceControl->StartPreviewImpl();
    }
  };

  return Dispatch(new Message(this, SurfaceControlListener::kInStartPreview));
}

nsresult
SurfaceControlImpl::Stop()
{
  class Message : public ControlMessage
  {
  public:
    Message(SurfaceControlImpl* aSurfaceControl,
            SurfaceControlListener::UserContext aContext)
      : ControlMessage(aSurfaceControl, aContext)
    { }

    nsresult
    RunImpl() override
    {
      return mSurfaceControl->StopImpl();
    }
  };

  return Dispatch(new Message(this, SurfaceControlListener::kInDestroySurface));
}