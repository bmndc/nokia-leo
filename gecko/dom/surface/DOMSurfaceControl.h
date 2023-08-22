/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_SURFACE_DOMSURFACECONTROL_H
#define DOM_SURFACE_DOMSURFACECONTROL_H

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/SurfaceControlBinding.h"
#include "mozilla/dom/CameraManagerBinding.h"
#include "mozilla/dom/Promise.h"
#include "ISurfaceControl.h"
#include "DOMMediaStream.h"
#include "AudioChannelAgent.h"
#include "nsProxyRelease.h"
#include "nsHashPropertyBag.h"
#include "DeviceStorage.h"
#include "DOMSurfaceControlListener.h"
#include "IDOMSurfaceControlCallback.h"
#include "nsWeakReference.h"
#ifdef MOZ_WIDGET_GONK
#include "nsITimer.h"
#endif

class nsPIDOMWindowInner;

namespace mozilla {

class SurfaceTrackCreatedListener;
class CameraPreviewMediaStream;

#define NS_DOM_SURFACE_CONTROL_CID \
{ 0x2b84b0f1, 0x016b, 0x464e, \
  { 0xaf, 0x05, 0xc6, 0xee, 0x84, 0x26, 0x63, 0x6a } }

// Main surface control.
class nsDOMSurfaceControl final : public DOMMediaStream
                                , public nsSupportsWeakReference
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_DOM_SURFACE_CONTROL_CID)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMSurfaceControl, DOMMediaStream)
  NS_DECL_ISUPPORTS_INHERITED

  // Because this header's filename doesn't match its C++ or DOM-facing
  // classname, we can't rely on the [Func="..."] WebIDL tag to implicitly
  // include the right header for us; instead we must explicitly include a
  // HasSupport() method in each header. We can get rid of these with the
  // Great Renaming proposed in bug 983177.
  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  nsDOMSurfaceControl(const dom::SurfaceConfiguration& aInitialConfig,
                      dom::Promise* aPromise,
                      nsPIDOMWindowInner* aWindow,
                      IDOMSurfaceControlCallback* aCallback);

  void Shutdown();

  void SetDataSourceSize(uint32_t aWidth, uint32_t aHeight);
  dom::SurfaceConfiguration* GetConfiguration();
  
  nsPIDOMWindowInner* GetParentObject() const { return mWindow; }

  MediaStream* GetCameraStream() const override;

  // Called by SurfaceTrackCreatedListener when the underlying track has been created.
  // XXX Bug 1124630. This can be removed with SurfacePreviewMediaStream.
  void TrackCreated(TrackID aTrackID);

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  operator nsISupports*() { return static_cast<DOMMediaStream*>(this); }


protected:
  virtual ~nsDOMSurfaceControl();

  class DOMSurfaceConfiguration final : public dom::SurfaceConfiguration
  {
  public:
    NS_INLINE_DECL_REFCOUNTING(DOMSurfaceConfiguration)

    DOMSurfaceConfiguration();
    explicit DOMSurfaceConfiguration(const dom::SurfaceConfiguration& aConfiguration);

  private:
    // Private destructor, to discourage deletion outside of Release():
    ~DOMSurfaceConfiguration();
  };

  friend class DOMSurfaceControlListener;

  void OnGetSurfaceComplete();
  void OnSurfaceStateChange(DOMSurfaceControlListener::SurfaceState aState,
                            nsresult aReason,
                            android::sp<android::IGraphicBufferProducer> aProducer);
  void OnPreviewStateChange(DOMSurfaceControlListener::PreviewState aState);
  void OnUserError(SurfaceControlListener::UserContext aContext, nsresult aError);

  bool IsWindowStillActive();
  nsresult SelectPreviewSize(const dom::SurfaceSize& aRequestedPreviewSize, ISurfaceControl::Size& aSelectedPreviewSize);

  already_AddRefed<dom::Promise> CreatePromise(ErrorResult& aRv);
  void AbortPromise(RefPtr<dom::Promise>& aPromise);
  virtual void EventListenerAdded(nsIAtom* aType) override;
  void DispatchPreviewStateEvent(DOMSurfaceControlListener::PreviewState aState);

  RefPtr<ISurfaceControl> mSurfaceControl; // non-DOM surface control

  RefPtr<DOMSurfaceConfiguration>              mCurrentConfiguration;
  // surface control pending promises
  RefPtr<dom::Promise>                        mGetSurfacePromise;

  // Surface event listener; we only need this weak reference so that
  //  we can remove the listener from the surface when we're done
  //  with it.
  DOMSurfaceControlListener* mListener;

  //IDOMSurfaceControlCallback,
  //call back to client which wait for the IGraphicBufferProducer
  IDOMSurfaceControlCallback* mDOMSurfaceControlCallback;

  // our viewfinder stream
  RefPtr<CameraPreviewMediaStream> mInput;

  // A listener on mInput for adding tracks to the DOM side.
  RefPtr<SurfaceTrackCreatedListener> mSurfaceTrackCreatedListener;

  // set once when this object is created
  nsCOMPtr<nsPIDOMWindowInner>   mWindow;
  DOMSurfaceControlListener::PreviewState mPreviewState;

private:
  nsDOMSurfaceControl(const nsDOMSurfaceControl&) = delete;
  nsDOMSurfaceControl& operator=(const nsDOMSurfaceControl&) = delete;

  android::sp<android::IGraphicBufferProducer> mProducer;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsDOMSurfaceControl, NS_DOM_SURFACE_CONTROL_CID)

} // namespace mozilla

#endif // DOM_SURFACE_DOMSURFACECONTROL_H
