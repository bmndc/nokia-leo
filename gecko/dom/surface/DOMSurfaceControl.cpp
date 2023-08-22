/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "DOMSurfaceControl.h"
#include "base/basictypes.h"
#include "nsCOMPtr.h"
#include "nsDOMClassInfo.h"
#include "nsHashPropertyBag.h"
#include "nsThread.h"
#include "DeviceStorage.h"
#include "DeviceStorageFileDescriptor.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/ipc/FileDescriptorUtils.h"
#include "mozilla/MediaManager.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "nsIAppsService.h"
#include "nsIObserverService.h"
#include "nsIDOMEventListener.h"
#include "nsIScriptSecurityManager.h"
#include "Navigator.h"
#include "nsXULAppAPI.h"
#include "DOMCameraManager.h"
#include "SurfaceCommon.h"
#include "nsGlobalWindow.h"
#include "CameraPreviewMediaStream.h"
#include "mozilla/dom/SurfaceControlBinding.h"
#include "mozilla/dom/VideoStreamTrack.h"
#include "mozilla/dom/BlobEvent.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsPrintfCString.h"
#include "mozilla/Logging.h"

static mozilla::LazyLogModule gSurfaceControlLog("nsDOMSurfaceControl");
#define SURFACECONTROL_LOG(type, msg) MOZ_LOG(gSurfaceControlLog, type, msg)

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::ipc;

class mozilla::SurfaceTrackCreatedListener : public MediaStreamListener
{
public:
  explicit SurfaceTrackCreatedListener(nsDOMSurfaceControl* aSurfaceControl)
    : mSurfaceControl(aSurfaceControl) {}

  void Forget() { mSurfaceControl = nullptr; }

  void DoNotifyTrackCreated(TrackID aTrackID)
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (!mSurfaceControl) {
      return;
    }

    mSurfaceControl->TrackCreated(aTrackID);
  }

  void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                StreamTime aTrackOffset, uint32_t aTrackEvents,
                                const MediaSegment& aQueuedMedia,
                                MediaStream* aInputStream,
                                TrackID aInputTrackID) override
  {
    if (aTrackEvents & TRACK_EVENT_CREATED) {
      nsCOMPtr<nsIRunnable> runnable =
        NS_NewRunnableMethodWithArgs<TrackID>(
          this, &SurfaceTrackCreatedListener::DoNotifyTrackCreated, aID);
      aGraph->DispatchToMainThreadAfterStreamStateUpdate(runnable.forget());
    }
  }

protected:
  ~SurfaceTrackCreatedListener() {}

  nsDOMSurfaceControl* mSurfaceControl;
};

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMSurfaceControl)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  // nsISupports is an ambiguous base of nsDOMSurfaceControl
  // so we have to work around that.
  if ( aIID.Equals(NS_GET_IID(nsDOMSurfaceControl)) )
    foundInterface = static_cast<nsISupports*>(static_cast<void*>(this));
  else
NS_INTERFACE_MAP_END_INHERITING(DOMMediaStream)

NS_IMPL_ADDREF_INHERITED(nsDOMSurfaceControl, DOMMediaStream)
NS_IMPL_RELEASE_INHERITED(nsDOMSurfaceControl, DOMMediaStream)

NS_IMPL_CYCLE_COLLECTION_INHERITED(nsDOMSurfaceControl, DOMMediaStream,
                                   mWindow,
                                   mGetSurfacePromise)

nsDOMSurfaceControl::DOMSurfaceConfiguration::DOMSurfaceConfiguration()
  : SurfaceConfiguration()
{
  MOZ_COUNT_CTOR(nsDOMSurfaceControl::DOMSurfaceConfiguration);
}

nsDOMSurfaceControl::DOMSurfaceConfiguration::DOMSurfaceConfiguration(const SurfaceConfiguration& aConfiguration)
  : SurfaceConfiguration(aConfiguration)
{
  MOZ_COUNT_CTOR(nsDOMSurfaceControl::DOMSurfaceConfiguration);
}

nsDOMSurfaceControl::DOMSurfaceConfiguration::~DOMSurfaceConfiguration()
{
  MOZ_COUNT_DTOR(nsDOMSurfaceControl::DOMSurfaceConfiguration);
}


/* static */
bool
nsDOMSurfaceControl::HasSupport(JSContext* aCx, JSObject* aGlobal)
{
  return true;
}

nsDOMSurfaceControl::nsDOMSurfaceControl(const SurfaceConfiguration& aInitialConfig,
                                         Promise* aPromise,
                                         nsPIDOMWindowInner* aWindow,
                                         IDOMSurfaceControlCallback* aCallback)
  : DOMMediaStream(aWindow, nullptr)
  , mSurfaceControl(nullptr)
  , mGetSurfacePromise(aPromise)
  , mListener(NULL)
  , mDOMSurfaceControlCallback(aCallback)
  , mWindow(aWindow)
  , mPreviewState(SurfaceControlListener::kPreviewStopped)
  , mProducer(NULL)
{
  SURFACECONTROL_LOG(LogLevel::Debug, ("%p Constructor", this));
  mInput = new CameraPreviewMediaStream(this);
  mOwnedStream = mInput;

  BindToOwner(aWindow);

  RefPtr<DOMSurfaceConfiguration> initialConfig =
    new DOMSurfaceConfiguration(aInitialConfig);

  ISurfaceControl::Configuration config;
  nsresult rv = SelectPreviewSize(aInitialConfig.mPreviewSize, config.mPreviewSize);
  if (NS_FAILED(rv)) {
    mListener->OnUserError(DOMSurfaceControlListener::kInCreateSurface, rv);
    return;
  }

  mSurfaceControl = ISurfaceControl::Create();

  mCurrentConfiguration = initialConfig.forget();

  // Register a SurfaceTrackCreatedListener directly on SurfacePreviewMediaStream
  // so we can know the TrackID of the video track.
  mSurfaceTrackCreatedListener = new SurfaceTrackCreatedListener(this);
  mInput->AddListener(mSurfaceTrackCreatedListener);

  // Register the playback listener directly on the surface input stream.
  // We want as low latency as possible for the surface, thus avoiding
  // MediaStreamGraph altogether. Don't do the regular InitStreamCommon()
  // to avoid initializing the Owned and Playback streams. This is OK since
  // we are not user/DOM facing anyway.
  CreateAndAddPlaybackStreamListener(mInput);

  // Register a listener for surface events.
  mListener = new DOMSurfaceControlListener(this, mInput);
  mSurfaceControl->AddListener(mListener);

  // Create the surface...
  rv = mSurfaceControl->Start(&config);
  if (NS_SUCCEEDED(rv)) {
    mSurfaceControl->StartPreview();
  }
}

nsDOMSurfaceControl::~nsDOMSurfaceControl()
{
  SURFACECONTROL_LOG(LogLevel::Debug, ("%p Destructor", this));
  /*invoke DOMMediaStream destroy*/
  Destroy();

  if (mInput) {
    mInput->Destroy();
    mInput = nullptr;
  }
  if (mSurfaceTrackCreatedListener) {
    mSurfaceTrackCreatedListener->Forget();
    mSurfaceTrackCreatedListener = nullptr;
  }
}

JSObject*
nsDOMSurfaceControl::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return SurfaceControlBinding::Wrap(aCx, this, aGivenProto);
}

bool
nsDOMSurfaceControl::IsWindowStillActive()
{
  return nsDOMCameraManager::IsWindowStillActive(mWindow->WindowID());
}

nsresult
nsDOMSurfaceControl::SelectPreviewSize(const SurfaceSize& aRequestedPreviewSize, ISurfaceControl::Size& aSelectedPreviewSize)
{
  if (aRequestedPreviewSize.mWidth && aRequestedPreviewSize.mHeight) {
    aSelectedPreviewSize.width = aRequestedPreviewSize.mWidth;
    aSelectedPreviewSize.height = aRequestedPreviewSize.mHeight;
  } else {
    /* Use the window width and height if no preview size is provided.
       Note that the width and height are actually reversed from the
       surface perspective. */
    int32_t width = 0;
    int32_t height = 0;
    float ratio = 0.0;
    nsresult rv;

    rv = mWindow->GetDevicePixelRatio(&ratio);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = mWindow->GetInnerWidth(&height);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = mWindow->GetInnerHeight(&width);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    MOZ_ASSERT(width > 0);
    MOZ_ASSERT(height > 0);
    MOZ_ASSERT(ratio > 0.0);
    aSelectedPreviewSize.width = std::ceil(width * ratio);
    aSelectedPreviewSize.height = std::ceil(height * ratio);
  }

  return NS_OK;
}

MediaStream*
nsDOMSurfaceControl::GetCameraStream() const
{
  return mInput;
}

void
nsDOMSurfaceControl::Shutdown()
{
  SURFACECONTROL_LOG(LogLevel::Debug, ("%p Shutdown", this));
  // Remove any pending solicited event handlers; these
  // reference our window object, which in turn references
  // us. If we don't remove them, we can leak DOM objects.
  AbortPromise(mGetSurfacePromise);

  if (mSurfaceControl) {
    mSurfaceControl->Stop();
    mSurfaceControl = nullptr;
  }

  if (mDOMSurfaceControlCallback) {
    mDOMSurfaceControlCallback->OnProducerDestroyed();
    mDOMSurfaceControlCallback = NULL;
  }
}

void
nsDOMSurfaceControl::TrackCreated(TrackID aTrackID) {
  usleep(100000);
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_RELEASE_ASSERT(mWindow, "Shouldn't have been created with a null window!");
  nsIPrincipal* principal = mWindow->GetExtantDoc()
                          ? mWindow->GetExtantDoc()->NodePrincipal()
                          : nullptr;

  // This track is not connected through a port.
  MediaInputPort* inputPort = nullptr;
  dom::VideoStreamTrack* track =
    new dom::VideoStreamTrack(this, aTrackID, aTrackID,
                              new BasicUnstoppableTrackSource(principal));
  RefPtr<TrackPort> port =
    new TrackPort(inputPort, track,
                  TrackPort::InputPortOwnership::OWNED);
  mTracks.AppendElement(port.forget());
  NotifyTrackAdded(track);
}


void 
nsDOMSurfaceControl::SetDataSourceSize(uint32_t aWidth, uint32_t aHeight)
{
  SURFACECONTROL_LOG(LogLevel::Debug, ("%p SetDataSourceSize Width:%d, Height:%d", this, aWidth, aHeight));
  //Set the display size to ISurfaceControl.
  //Convert dom::SurfaceSize to ISurfaceControl::Size
  ISurfaceControl::Size surfaceControlSize;
  surfaceControlSize.width = aWidth;
  surfaceControlSize.height = aHeight;
  nsresult rv = mSurfaceControl->SetDataSourceSize(surfaceControlSize);
  if (NS_FAILED(rv)) {
    //Print warning log.
  }
}

#define THROW_IF_NO_SURFACECONTROL(...)                                          \
  do {                                                                          \
    if (!mSurfaceControl) {                                                      \
      aRv = NS_ERROR_NOT_AVAILABLE;                                             \
      return __VA_ARGS__;                                                       \
    }                                                                           \
  } while (0)

already_AddRefed<Promise>
nsDOMSurfaceControl::CreatePromise(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(mWindow);
  if (!global) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  return Promise::Create(global, aRv);
}

void
nsDOMSurfaceControl::AbortPromise(RefPtr<Promise>& aPromise)
{
  RefPtr<Promise> promise = aPromise.forget();
  if (promise) {
    promise->MaybeReject(NS_ERROR_NOT_AVAILABLE);
  }
}


void
nsDOMSurfaceControl::EventListenerAdded(nsIAtom* aType)
{
  if (aType == nsGkAtoms::onpreviewstatechange) {
    DispatchPreviewStateEvent(mPreviewState);
  }
}

void
nsDOMSurfaceControl::DispatchPreviewStateEvent(SurfaceControlListener::PreviewState aState)
{
  nsString state;
  switch (aState) {
    case SurfaceControlListener::kPreviewStarted:
      state = NS_LITERAL_STRING("started");
      break;

    default:
      state = NS_LITERAL_STRING("stopped");
      break;
  }

  //DispatchStateEvent(NS_LITERAL_STRING("previewstatechange"), state);
}

void
nsDOMSurfaceControl::OnGetSurfaceComplete()
{
  SURFACECONTROL_LOG(LogLevel::Debug, ("%p OnGetSurfaceComplete", this));
  if (mDOMSurfaceControlCallback) {
    mDOMSurfaceControlCallback->OnProducerCreated(mProducer);
  }
  // The hardware is open, so we can return a surface to JS, even if
  // the preview hasn't started yet.
  RefPtr<Promise> promise = mGetSurfacePromise.forget();
  if (promise) {
    promise->MaybeResolve(this);
  }
}

// Surface Control event handlers--must only be called from the Main Thread!
void
nsDOMSurfaceControl::OnSurfaceStateChange(SurfaceControlListener::SurfaceState aState,
                                          nsresult aReason,
                                          android::sp<android::IGraphicBufferProducer> aProducer)
{
  MOZ_ASSERT(NS_IsMainThread());

  switch (aState) {
    case SurfaceControlListener::kSurfaceCreate:
      MOZ_ASSERT(aReason == NS_OK);
      mProducer = aProducer;
      // The surface is create, so we can return a MediaStream to JS.
      OnGetSurfaceComplete();

      break;

    case SurfaceControlListener::kSurfaceDestroyed:
      mProducer.clear();
      break;

    case SurfaceControlListener::kSurfaceCreateFailed:
      MOZ_ASSERT(aReason == NS_ERROR_NOT_AVAILABLE);
      mProducer.clear();
      OnUserError(DOMSurfaceControlListener::kInCreateSurface, NS_ERROR_NOT_AVAILABLE);
      break;

    case SurfaceControlListener::kSurfaceUninitialized:
      mProducer.clear();
      break;

    default:
      MOZ_ASSERT_UNREACHABLE("Unanticipated surface hardware state");
  }
}


void
nsDOMSurfaceControl::OnPreviewStateChange(SurfaceControlListener::PreviewState aState)
{
  MOZ_ASSERT(NS_IsMainThread());

  mPreviewState = aState;
  nsString state;
  switch (aState) {
    case SurfaceControlListener::kPreviewStarted:
      state = NS_LITERAL_STRING("started");
      break;

    default:
      state = NS_LITERAL_STRING("stopped");
      break;
  }

  DispatchPreviewStateEvent(aState);
}

void
nsDOMSurfaceControl::OnUserError(SurfaceControlListener::UserContext aContext, nsresult aError)
{
  MOZ_ASSERT(NS_IsMainThread());

  SURFACECONTROL_LOG(LogLevel::Error, ("%p OnUserError : %d", this, aContext));

  RefPtr<Promise> promise;

  switch (aContext) {
    case SurfaceControlListener::kInCreateSurface:
      promise = mGetSurfacePromise.forget();
      // If we failed to create the surface, we never actually provided a reference
      // for the application to release explicitly. Thus we must clear our handle
      // here to ensure everything is freed.
      mSurfaceControl = nullptr;
      break;

    default:
      {
        nsPrintfCString msg("Unhandled aContext=%u", aContext);
        NS_WARNING(msg.get());
      }
      MOZ_ASSERT_UNREACHABLE("Unhandled user error");
      return;
  }

  if (!promise) {
    return;
  }

  promise->MaybeReject(aError);
}

dom::SurfaceConfiguration* 
nsDOMSurfaceControl::GetConfiguration()
{
  return mCurrentConfiguration;
}
