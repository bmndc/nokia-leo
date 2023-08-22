/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef __CEWidevineSessionManager_h__
#define __CEWidevineSessionManager_h__

#include <set>
#include <map>
#include "cdm.h"
#include "gmp-decryption.h"
#include "EventListenerImpl.h"
#include "CEWidevineHost.h"
#include "RefCounted.h"

class CEWidevineSessionManager final : public GMPDecryptor
                                     , public RefCounted
{
public:
  CEWidevineSessionManager();
  ~CEWidevineSessionManager();

  virtual void Init(GMPDecryptorCallback* aCallback) override;

  virtual void CreateSession(uint32_t aCreateSessionToken,
                             uint32_t aPromiseId,
                             const char* aInitDataType,
                             uint32_t aInitDataTypeSize,
                             const uint8_t* aInitData,
                             uint32_t aInitDataSize,
                             GMPSessionType aSessionType) override;

  virtual void LoadSession(uint32_t aPromiseId,
                           const char* aSessionId,
                           uint32_t aSessionIdLength) override;

  virtual void UpdateSession(uint32_t aPromiseId,
                             const char* aSessionId,
                             uint32_t aSessionIdLength,
                             const uint8_t* aResponse,
                             uint32_t aResponseSize) override;

  virtual void CloseSession(uint32_t aPromiseId,
                            const char* aSessionId,
                            uint32_t aSessionIdLength) override;

  virtual void RemoveSession(uint32_t aPromiseId,
                             const char* aSessionId,
                             uint32_t aSessionIdLength) override;

  virtual void SetServerCertificate(uint32_t aPromiseId,
                                    const uint8_t* aServerCert,
                                    uint32_t aServerCertSize) override;

  virtual void Decrypt(GMPBuffer* aBuffer,
                       GMPEncryptedBufferMetadata* aMetadata) override;

  virtual void DecryptingComplete() override;

  void onMessage(const std::string&, widevine::Cdm::MessageType, const std::string&);

  void onKeyStatusesChange(const std::string&, bool);

private:
  void DoDecrypt(GMPBuffer* aBuffer, GMPEncryptedBufferMetadata* aMetadata);
	bool sendWidevinePostRequest(std::string &, std::string &, std::string &);
  void Shutdown();
  void Reset();

	widevine::Cdm *mCDM;
	EventListenerImpl *mListener;
	CEWidevineHost *mCEWidevineHost;
  GMPDecryptorCallback* mCallback;
  std::map<std::string, std::set<std::string>> mSessions;
  GMPThread* mThread;

  CEWidevineSessionCallback* mSessionCallback;
};

#endif  //__CEWidevineSessionManager_h__