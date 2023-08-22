/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_SpeakerManagerService_h__
#define mozilla_dom_SpeakerManagerService_h__

#include "nsAutoPtr.h"
#include "nsIObserver.h"
#include "nsTArray.h"
#include "SpeakerManager.h"
#include "nsIAudioManager.h"
#include "nsCheapSets.h"
#include "nsHashKeys.h"

namespace mozilla {
namespace dom {

class SpeakerManagerService : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  /*
   * Return null or instance which has been created.
   */
  static SpeakerManagerService* GetSpeakerManagerService();
  /*
   * Return SpeakerManagerService instance.
   * If SpeakerManagerService is not exist, create and return new one.
   */
  static SpeakerManagerService* GetOrCreateSpeakerManagerService();

  static PRLogModuleInfo* GetSpeakerManagerLog();

  virtual bool GetSpeakerStatus();
  virtual void SetAudioChannelActive(bool aIsActive);
  // Child ID of chrome process should be 0.
  virtual void ForceSpeaker(bool aEnable,
                            bool aVisible,
                            bool aAudioChannelActive,
                            uint64_t aWindowID,
                            uint64_t aChildID = 0);

  // Register the SpeakerManager to service for notify the speakerforcedchange event
  nsresult RegisterSpeakerManager(SpeakerManager* aSpeakerManager)
  {
    // One APP can only have one SpeakerManager, so return error if it already exists.
    if (mRegisteredSpeakerManagers.Contains(aSpeakerManager->WindowID())) {
      return NS_ERROR_FAILURE;
    }
    mRegisteredSpeakerManagers.Put(aSpeakerManager->WindowID(), aSpeakerManager);
    return NS_OK;
  }
  nsresult UnRegisterSpeakerManager(SpeakerManager* aSpeakerManager)
  {
    mRegisteredSpeakerManagers.Remove(aSpeakerManager->WindowID());
    return NS_OK;
  }

protected:
  struct SpeakerManagerData final
  {
    SpeakerManagerData(uint64_t aChildID, uint64_t aWindowID, bool aForceSpeaker)
      : mChildID(aChildID)
      , mWindowID(aWindowID)
      , mForceSpeaker(aForceSpeaker)
    {
    }

    uint64_t mChildID;
    uint64_t mWindowID;
    bool mForceSpeaker;
  };

  class SpeakerManagerList : protected nsTArray<SpeakerManagerData>
  {
  public:
    void InsertData(const SpeakerManagerData& aData);
    void RemoveData(const SpeakerManagerData& aData);
    void RemoveChild(uint64_t aChildID);

    using nsTArray<SpeakerManagerData>::IsEmpty;
    using nsTArray<SpeakerManagerData>::LastElement;
  };

  SpeakerManagerService();

  virtual ~SpeakerManagerService();
  // Notify to UA if device speaker status changed
  virtual void Notify();

  void TurnOnSpeaker(bool aEnable);
  // Update global speaker status
  void UpdateSpeakerStatus();

  /**
   * Shutdown the singleton.
   */
  static void Shutdown();
  // Hash map between window ID and registered SpeakerManager
  nsDataHashtable<nsUint64HashKey, RefPtr<SpeakerManager>> mRegisteredSpeakerManagers;
  // The Speaker status assign by UA
  bool mOrgSpeakerStatus;

  SpeakerManagerList mVisibleSpeakerManagers;
  SpeakerManagerList mActiveSpeakerManagers;
  // This is needed for IPC communication between
  // SpeakerManagerServiceChild and this class.
  friend class ContentParent;
  friend class ContentChild;
};

} // namespace dom
} // namespace mozilla

#endif
