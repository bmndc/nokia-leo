/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include <curl/curl.h>
#include <cutils/properties.h>
#include "CEWidevineSessionManager.h"
#include "mozilla/CheckedInt.h"
#include "CEWidevineUtils.h"

const std::string kCpProductionProvisioningServerUrl =
  "https://www.googleapis.com/"
  "certificateprovisioning/v1/devicecertificates/create"
  "?key=AIzaSyB-5OLKTx2iU5mko18DfdwK5611JIjbUhE";

size_t
CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)
{
  size_t newLength = size * nmemb;
  s->append((char*)contents, newLength);
  return newLength;
}

bool
CEWidevineSessionManager::sendWidevinePostRequest(std::string &defaultUrl, std::string &request, std::string &result)
{
  CURL *curl;
  CURLcode res;
  struct curl_slist* headers = NULL;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();

  std::string strJson = "{";
  strJson += "\"signedRequest\" : \"";
  strJson += request;
  strJson += "\"";
  strJson += "}";

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, defaultUrl.c_str());

    headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1);//not zero means this is a 'POST' operation
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strJson.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
      WV_LOG(LogPriority::LOG_ERROR, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      return false;
    }

    curl_easy_cleanup(curl);
  } else {
    WV_LOG(LogPriority::LOG_ERROR, "curl_easy_init failed");
    return false;
  }

  curl_global_cleanup();
  return true;
}

CEWidevineSessionManager::CEWidevineSessionManager()
  : mCDM(NULL),
    mListener(NULL),
    mCEWidevineHost(NULL),
    mSessionCallback(NULL)
{
	WV_LOG(LogPriority::LOG_DEBUG, "CEWidevineSessionManager ctor %p", this);

  AddRef();

  if (GetPlatform()->createthread(&mThread) != GMPNoErr) {
    WV_LOG(LogPriority::LOG_ERROR, "failed to create thread in cewidevine cdm");
    mThread = nullptr;
    return;
  }
}

CEWidevineSessionManager::~CEWidevineSessionManager()
{
  WV_LOG(LogPriority::LOG_DEBUG, "CEWidevineSessionManager dtor %p", this);
  Reset();
}

void
CEWidevineSessionManager::Reset()
{
  if (mCDM) {
    delete mCDM;
    mCDM = NULL;
  }

  if (mListener) {
    delete mListener;
    mListener = NULL;
  }

  if (mCEWidevineHost) {
    delete mCEWidevineHost;
    mCEWidevineHost = NULL;
  }

  if (mSessionCallback) {
    delete mSessionCallback;
    mSessionCallback = NULL;
  }
}

void
CEWidevineSessionManager::Init(GMPDecryptorCallback* aCallback)
{
  WV_LOG(LogPriority::LOG_DEBUG, "%p %s", this, __func__);
  Reset();
  mCallback = aCallback;
  mCallback->SetCapabilities(GMP_EME_CAP_DECRYPT_AUDIO | GMP_EME_CAP_DECRYPT_VIDEO);

  widevine::Cdm::ClientInfo myClientInfo;
  // According to document
  // product_name = ro.product.name
  // company_name = ro.product.manufacturer
  // device_name  = ro.product.device
  // model_name   = ro.product.model
  // arch_name    = ro.product.cpu.abi
  // build_info   = ro.build.fingerprint
  //
  // Yet widevine thought we don't have such property, so they suggest us to use
  // company_name = "KAIOS"
  // model_name = "KAIOS"
  // however they also mentioned widevine are not enforcing it now.
  // So for now, use property mentioned in document
  char propval[PROPERTY_VALUE_MAX];
  property_get("ro.product.manufacturer", propval, "");
  myClientInfo.company_name = std::string(propval);
  property_get("ro.product.name", propval, "");
  myClientInfo.product_name = std::string(propval);
  property_get("ro.product.model", propval, "");
  myClientInfo.model_name = std::string(propval);

  mCEWidevineHost = new CEWidevineHost();

  widevine::Cdm::Status result = widevine::Cdm::initialize(widevine::Cdm::SecureOutputType::kNoSecureOutput, myClientInfo,
                                                           mCEWidevineHost, mCEWidevineHost, mCEWidevineHost, widevine::Cdm::LogLevel::kErrors);

  if (result != widevine::Cdm::Status::kSuccess) {
    WV_LOG(LogPriority::LOG_ERROR, "cdm init fail, result:%d", (int)result);
    Reset();
    return;
  }

  mSessionCallback = new CEWidevineSessionCallback(this);
  mListener = new EventListenerImpl(mSessionCallback);

  mCDM = widevine::Cdm::create(mListener, mCEWidevineHost, false);

  if (!mCDM) {
    WV_LOG(LogPriority::LOG_ERROR, "cdm create fail");
    Reset();
    return;
  }

  //step 1 get certificate
  std::string request;
  result = mCDM->getProvisioningRequest(&request);

  if (result != widevine::Cdm::Status::kSuccess) {
    WV_LOG(LogPriority::LOG_ERROR, "getProvisioningRequest fail. result:%d", result);
    Reset();
    return;
  }

  std::string message;
  std::string url = kCpProductionProvisioningServerUrl;

  if (!sendWidevinePostRequest(url, request, message)) {
    WV_LOG(LogPriority::LOG_ERROR, "sendWidevinePostRequest fail");
    Reset();
    return;
  }

  result = mCDM->handleProvisioningResponse(message);

  if (result != widevine::Cdm::Status::kSuccess) {
    WV_LOG(LogPriority::LOG_ERROR, "handleProvisioningResponse fail, result:%d", result);
    Reset();
    return;
  }
}

void
CEWidevineSessionManager::CreateSession(uint32_t aCreateSessionToken,
                                        uint32_t aPromiseId,
                                        const char* aInitDataType,
                                        uint32_t aInitDataTypeSize,
                                        const uint8_t* aInitData,
                                        uint32_t aInitDataSize,
                                        GMPSessionType aSessionType)
{
  WV_LOG(LogPriority::LOG_DEBUG, "%p %s", this, __func__);

  if (!mCDM) {
    std::string message = "no cdm exist, create session fail";
    WV_LOG(LogPriority::LOG_ERROR, message.c_str());
    mCallback->RejectPromise(aPromiseId, kGMPAbortError, message.c_str(), message.size());
    return;
  }

  // step 1. create session
  std::string session_id;
  widevine::Cdm::Status result;

  switch (aSessionType) {
    case GMPSessionType::kGMPTemporySession:
      result = mCDM->createSession(widevine::Cdm::kTemporary, &session_id);
      break;
    case GMPSessionType::kGMPPersistentSession:
      result = mCDM->createSession(widevine::Cdm::kPersistentLicense, &session_id);
      break;
    default:
      std::string message = "don't support session type: " + std::to_string((int)aSessionType);
      WV_LOG(LogPriority::LOG_ERROR, message.c_str());
      mCallback->RejectPromise(aPromiseId, kGMPAbortError, message.c_str(), message.size());
      break;
  }

  if (result != widevine::Cdm::Status::kSuccess) {
    std::string message = "createSession fail. result: " + std::to_string((int)result);
    WV_LOG(LogPriority::LOG_ERROR, message.c_str());
    mCallback->RejectPromise(aPromiseId, kGMPAbortError, message.c_str(), message.size());
    return;
  }

  WV_LOG(LogPriority::LOG_DEBUG, "%p session_id: %s", this, session_id.c_str());

  mSessions[session_id] = std::set<std::string>();
  mCallback->SetSessionId(aCreateSessionToken, session_id.c_str(), session_id.length());

  // step 2. generate request string(send to APP, request for true decrypt key)
  // cdm will call EventListenerImpl::onMessage() when request string is ready
  // support cenc only
  std::string initDataType(aInitDataType, aInitDataType + aInitDataTypeSize);

  if (initDataType != "cenc") {
    std::string message = "'" + initDataType + "' is an initDataType unsupported by CEWidevine";
    WV_LOG(LogPriority::LOG_ERROR, message.c_str());
    mCallback->RejectPromise(aPromiseId, kGMPNotSupportedError, message.c_str(), message.size());
    return;
  }

  std::string init_data(aInitData, aInitData + aInitDataSize);
  result = mCDM->generateRequest(session_id, widevine::Cdm::InitDataType::kCenc, init_data);

  if (result != widevine::Cdm::Status::kSuccess) {
    std::string message = "generateRequest fail. result: " + std::to_string((int)result);
    WV_LOG(LogPriority::LOG_ERROR, message.c_str());
    mCallback->RejectPromise(aPromiseId, kGMPAbortError, message.c_str(), message.size());
    return;
  }
}

// Normally this is used for APP to update key id
// cdm will parse aResponse to get decrypt key.
// if success, cdm will call EventListenerImpl::onKeyStatusesChange()
void
CEWidevineSessionManager::UpdateSession(uint32_t aPromiseId,
                                        const char* aSessionId,
                                        uint32_t aSessionIdLength,
                                        const uint8_t* aResponse,
                                        uint32_t aResponseSize)
{
  std::string sessionId(aSessionId, aSessionId + aSessionIdLength);

  WV_LOG(LogPriority::LOG_DEBUG, "%p session: %s", this, sessionId.c_str());

  auto itr = mSessions.find(sessionId);
  if (itr == mSessions.end()) {
    std::string message = "Widevine CDM couldn't resolve session ID: " + sessionId +" in UpdateSession.";
    WV_LOG(LogPriority::LOG_ERROR, message.c_str());
    mCallback->RejectPromise(aPromiseId, kGMPNotFoundError, message.c_str(), message.size());
    return;
  }

  std::string response(aResponse, aResponse + aResponseSize);
  widevine::Cdm::Status result = mCDM->update(sessionId, response);

  if (result != widevine::Cdm::Status::kSuccess) {
    std::string message = "CDM update fail. result:" + std::to_string((int)result);
    WV_LOG(LogPriority::LOG_ERROR, message.c_str());
    mCallback->RejectPromise(aPromiseId, kGMPAbortError, message.c_str(), message.size());
    return;
  }
}

//TODO, deal with kLicenseRenewal and kLicenseRelease
void
CEWidevineSessionManager::onMessage(const std::string& session_id,
                                    widevine::Cdm::MessageType message_type,
                                    const std::string& message)
{
  WV_LOG(LogPriority::LOG_DEBUG, "sessionID: %s, message_type:%d", session_id.c_str(), (int)message_type);

  if (message_type != widevine::Cdm::MessageType::kLicenseRequest) {
    WV_LOG(LogPriority::LOG_ERROR, "%p only support License Request", this);
    return;
  }

  mCallback->SessionMessage(session_id.c_str(), session_id.length(),
                            kGMPLicenseRequest, reinterpret_cast<const uint8_t*>(&message[0]), message.length());
}

void
CEWidevineSessionManager::onKeyStatusesChange(const std::string& session_id, bool has_new_usable_key)
{
  auto itr = mSessions.find(session_id);
  if (itr == mSessions.end()) {
    WV_LOG(LogPriority::LOG_ERROR, "%s not found %s", session_id.c_str(), __func__);
    return;
  }

  // deal with usable key for now
  // TODO: deal with expire key
  if (!has_new_usable_key) {
    WV_LOG(LogPriority::LOG_ERROR, "no usable key");
    return;
  }

  // widevine only notify which session_id has new key
  // yet it does not return the new key id
  // we need to list all key id under the session_id
  widevine::Cdm::KeyStatusMap result;
  mCDM->getKeyStatuses(session_id, &result);

  for (auto it = result.begin(); it != result.end(); it++) {
    std::string keyID = it->first;
    if (it->second == widevine::Cdm::KeyStatus::kUsable && mSessions[session_id].count(keyID) == 0) {
      mSessions[session_id].insert(keyID);
      WV_LOG(LogPriority::LOG_DEBUG, "call mCallback->KeyStatusChanged(keyID:%s)", keyID.c_str());
      mCallback->KeyStatusChanged(session_id.c_str(), session_id.length(),
                                reinterpret_cast<const uint8_t*>(&keyID[0]), keyID.length(),
                                kGMPUsable);
    }
  }
}

// TODO
void
CEWidevineSessionManager::LoadSession(uint32_t aPromiseId,
                                      const char* aSessionId,
                                      uint32_t aSessionIdLength)
{
  std::string message = "widevine adapter don't support LoadSession";
  WV_LOG(LogPriority::LOG_ERROR, message.c_str());
  mCallback->RejectPromise(aPromiseId, kGMPNotSupportedError, message.c_str(), message.size());
}

void
CEWidevineSessionManager::CloseSession(uint32_t aPromiseId,
                                       const char* aSessionId,
                                       uint32_t aSessionIdLength)
{
  std::string sessionId(aSessionId, aSessionId + aSessionIdLength);
  WV_LOG(LogPriority::LOG_DEBUG, "%p aSessionId: %s close session", this, sessionId.c_str());

  auto itr = mSessions.find(sessionId);
  if (itr == mSessions.end()) {
    std::string message = sessionId + " session not found";
    WV_LOG(LogPriority::LOG_ERROR, message.c_str());
    mCallback->RejectPromise(aPromiseId, kGMPNotFoundError, message.c_str(), message.size());
    return;
  }

  mSessions.erase(sessionId);

  widevine::Cdm::Status result = mCDM->close(sessionId);

  if (result != widevine::Cdm::Status::kSuccess) {
    std::string message = "CDM close session fail. result: " + std::to_string((int)result);
    WV_LOG(LogPriority::LOG_ERROR, message.c_str());
    mCallback->RejectPromise(aPromiseId, kGMPAbortError, message.c_str(), message.size());
    return;
  }

  mCallback->ResolvePromise(aPromiseId);
  mCallback->SessionClosed(aSessionId, aSessionIdLength);
}

// TODO
void
CEWidevineSessionManager::RemoveSession(uint32_t aPromiseId,
                                        const char* aSessionId,
                                        uint32_t aSessionIdLength)
{
  std::string message = "widevine adapter don't support RemoveSession";
  WV_LOG(LogPriority::LOG_ERROR, message.c_str());
  mCallback->RejectPromise(aPromiseId, kGMPNotSupportedError, message.c_str(), message.size());
}

// TODO
void
CEWidevineSessionManager::SetServerCertificate(uint32_t aPromiseId,
                                               const uint8_t* aServerCert,
                                               uint32_t aServerCertSize)
{
  std::string message = "widevine adapter don't support SetServerCertificate";
  WV_LOG(LogPriority::LOG_ERROR, message.c_str());
  mCallback->RejectPromise(aPromiseId, kGMPNotSupportedError, message.c_str(), message.size());
}

void
CEWidevineSessionManager::Decrypt(GMPBuffer* aBuffer,
                                  GMPEncryptedBufferMetadata* aMetadata)
{
  if (!mThread) {
    WV_LOG(LogPriority::LOG_ERROR, "No decrypt thread %s", __func__);
    mCallback->Decrypted(aBuffer, GMPCryptoErr);
    return;
  }

  mThread->Post(WrapTaskRefCounted(this, &CEWidevineSessionManager::DoDecrypt, aBuffer, aMetadata));
}

void
CEWidevineSessionManager::DoDecrypt(GMPBuffer* aBuffer,
                                    GMPEncryptedBufferMetadata* aMetadata)
{
  // Step 1
  // set up input metadata
  widevine::Cdm::InputBuffer input;

  // set key id
  input.key_id = aMetadata->KeyId();
  input.key_id_length = aMetadata->KeyIdSize();

  // set up IV
  // widevine must use length 16 IV
  // other wise it requires padding.
  #define WIDEVINE_IV_LENGTH 16

  if (aMetadata->IVSize() > WIDEVINE_IV_LENGTH) {
    WV_LOG(LogPriority::LOG_ERROR, "IV size error: %d", aMetadata->IVSize());
    mCallback->Decrypted(aBuffer, GMPCryptoErr);
    return;
  }

  uint8_t finalIV[WIDEVINE_IV_LENGTH] = {0};

  const uint8_t* tempIV = aMetadata->IV();
  for (uint32_t i = 0; i != aMetadata->IVSize(); i++) {
    finalIV[i] = tempIV[i];
  }
  input.iv_length = WIDEVINE_IV_LENGTH;
  input.iv = finalIV;

  // set up encryption scheme
  input.encryption_scheme = widevine::Cdm::kAesCtr;

  // is_video false -> no secure hardware to decode
  input.is_video = false;

  // Step 2
  // buffer may include clear and cipher byte
  // extract only cipher byte to decrypt.
  std::vector<uint8_t> tmp(aBuffer->Size());

  const uint16_t* aClearBytes = aMetadata->ClearBytes();
  const uint32_t* aCipherBytes = aMetadata->CipherBytes();

  if (aMetadata->NumSubsamples()) {
    mozilla::CheckedInt<uintptr_t> data = reinterpret_cast<uintptr_t>(aBuffer->Data());
    const uintptr_t endBuffer = reinterpret_cast<uintptr_t>(aBuffer->Data() + aBuffer->Size());
    uint8_t* iter = &tmp[0];
    for (size_t i = 0; i < aMetadata->NumSubsamples(); i++) {
      data += aClearBytes[i];

      if (!data.isValid() || data.value() > endBuffer) {
        WV_LOG(LogPriority::LOG_ERROR, "Trying to read past the end of the buffer! %d", __LINE__);
        mCallback->Decrypted(aBuffer, GMPCryptoErr);
        return;
      }

      const uint32_t& cipherBytes = aCipherBytes[i];
      mozilla::CheckedInt<uintptr_t> dataAfterCipher = data + cipherBytes;

      if (!dataAfterCipher.isValid() || dataAfterCipher.value() > endBuffer) {
        WV_LOG(LogPriority::LOG_ERROR, "Trying to read past the end of the buffer! %d", __LINE__);
        mCallback->Decrypted(aBuffer, GMPCryptoErr);
        return;
      }

      memcpy(iter, reinterpret_cast<uint8_t*>(data.value()), cipherBytes);

      data = dataAfterCipher;
      iter += cipherBytes;
    }
    tmp.resize((size_t)(iter - &tmp[0]));
    input.data_length = tmp.size();
  } else {
    memcpy(&tmp[0], aBuffer->Data(), aBuffer->Size());
    input.data_length = aBuffer->Size();
  }

  //if tmp is zero, no need to decrypt, just return data
  if (tmp.size() == 0) {
    WV_LOG(LogPriority::LOG_VERBOSE, "no need to decrypt, just return");
    mCallback->Decrypted(aBuffer, GMPNoErr);
    return;
  }

  input.data = reinterpret_cast<uint8_t*>(&tmp[0]);

  widevine::Cdm::OutputBuffer output;
  std::vector<uint8_t> output_buffer(input.data_length);
  output.data = &(output_buffer[0]);
  output.data_length = output_buffer.size();

  // Step 3
  // send data to cdm for decrypt
  widevine::Cdm::Status result = mCDM->decrypt(input, output);

  if (result != widevine::Cdm::Status::kSuccess) {
    WV_LOG(LogPriority::LOG_ERROR, "%p decrypt error, result:%d", this, (int)result);
    mCallback->Decrypted(aBuffer, GMPCryptoErr);
    return;
  }

  // Step 4
  // put decrypted data and original clear byte as one buffer
  if (aMetadata->NumSubsamples()) {
    // Take the decrypted buffer, split up into subsamples, and insert those
    // subsamples back into their original position in the original buffer.
    uint8_t* data = aBuffer->Data();
    uint8_t* iter = output.data;
    aCipherBytes = aMetadata->CipherBytes();
    aClearBytes = aMetadata->ClearBytes();
    for (size_t i = 0; i < aMetadata->NumSubsamples(); i++) {
      data += aClearBytes[i];
      uint32_t cipherBytes = aCipherBytes[i];

      memcpy(data, iter, cipherBytes);

      data += cipherBytes;
      iter += cipherBytes;
    }
  } else {
    memcpy(aBuffer->Data(), &tmp[0], aBuffer->Size());
  }

  mCallback->Decrypted(aBuffer, GMPNoErr);
}

void
CEWidevineSessionManager::Shutdown()
{
  WV_LOG(LogPriority::LOG_DEBUG, "%p %s", this, __func__);

  for (auto it = mSessions.begin(); it != mSessions.end(); it++) {
    widevine::Cdm::Status result = mCDM->close(it->first);
    if (result != widevine::Cdm::Status::kSuccess) {
      WV_LOG(LogPriority::LOG_ERROR, "session:%s close fail", it->first.c_str());
    }
  }
  mSessions.clear();
}

void
CEWidevineSessionManager::DecryptingComplete()
{
  WV_LOG(LogPriority::LOG_DEBUG, "%p %s", this, __func__);

  GMPThread* thread = mThread;
  thread->Join();

  Shutdown();
  Release();
}