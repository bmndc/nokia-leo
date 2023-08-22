/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "mozilla/dom/IMEConnect.h"
#include "mozilla/dom/IMEConnectBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "MainThreadUtils.h"

namespace mozilla {
namespace dom {


nsString IMEConnect::mCandidateWord;
uint16_t IMEConnect::mTotalWord;
uint16_t IMEConnect::mTotalGroup;
uint16_t IMEConnect::mActiveWordIndex;
uint16_t IMEConnect::mActiveGroupIndex;
uint8_t  IMEConnect::mActiveSelectionBar;
uint32_t IMEConnect::mCurrentLID;
uint8_t  IMEConnect::mInitStatus;
vector<wchar_t> IMEConnect::mKeyBuff;
CandidateCH IMEConnect::mCandVoca;
CandidateCH IMEConnect::mCandGroup;


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(IMEConnect, mWindow)
NS_IMPL_CYCLE_COLLECTING_ADDREF(IMEConnect)
NS_IMPL_CYCLE_COLLECTING_RELEASE(IMEConnect)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(IMEConnect)
NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

IMEConnect::IMEConnect(nsPIDOMWindowInner* aWindow): mWindow(aWindow)
{
  MOZ_ASSERT(mWindow);
}

already_AddRefed<IMEConnect>
IMEConnect::Constructor(const GlobalObject& aGlobal, ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsPIDOMWindowInner> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<IMEConnect> xt_object = new IMEConnect(window);
  aRv = xt_object->Init((eKeyboardT9 << KEYBOARD_ID_SHIFT) | eImeEnglishUs);

  return xt_object.forget();
}

already_AddRefed<IMEConnect>
IMEConnect::Constructor(const GlobalObject& aGlobal, uint32_t aLid, ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsPIDOMWindowInner> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<IMEConnect> xt_object = new IMEConnect(window);
  aRv = xt_object->Init(aLid);

  return xt_object.forget();
}

nsresult
IMEConnect::Init(uint32_t aLid)
{
  KIKA_LOGI("Init::version = %s", IQQI_Version());
  SetLanguage(aLid);
  return NS_OK;
}

wchar_t WcharToLower(wchar_t wch)
{
  if (wch <= 'Z' && wch >= 'A') {
    return wch - ('Z'-'z');
  }
  return wch;
}

wchar_t* WstrToLower(wchar_t *str)
{
  wchar_t *p = (wchar_t*)str;

  while (*p) {
    *p = WcharToLower(*p);
    p++;
  }

  return str;
}

void U16strToWstr(wchar_t *dest, const char16_t *src, unsigned int length)
{
  unsigned int i = 0;
  while (i<length) {
    dest[i] = (wchar_t)src[i];
    i++;
  }
  dest[length] = 0;
}

void PackCandidates()
{
  uint8_t imeId = IMEConnect::mCurrentLID & IQQI_IME_ID_MASK;
  uint8_t keyboardId = (IMEConnect::mCurrentLID & KEYBOARD_ID_MASK) >> KEYBOARD_ID_SHIFT;
  bool isGroupSupported = (imeId == eImeChineseCn || imeId == eImeChineseTw) && (keyboardId == eKeyboardT9);

  IMEConnect::mCandidateWord.AssignLiteral("");

  /* GroupBar Packing */
  if (isGroupSupported) {
    for (int i=0; i<IMEConnect::mTotalGroup; i++) {
      nsString jsWord;

      KIKA_LOGD("PackCandidates::Group candidate[%d] = %ls", i, IMEConnect::mCandGroup.record(i));

      if (i == IMEConnect::mActiveGroupIndex && IMEConnect::mActiveSelectionBar == eSelectionBarGroup) {
        jsWord.AppendLiteral(">");
      }

      for (int j=0; j<IMEConnect::mCandGroup.vlen(i); j++) {
        jsWord.Append((PRUnichar)IMEConnect::mCandGroup.record(i)[j]);
      }

      jsWord.AppendLiteral("|");

      if (i == IMEConnect::mTotalGroup - 1) {
        jsWord.AppendLiteral("|");
      }

      IMEConnect::mCandidateWord.Append(jsWord);
    }
  }

  /* WordBar Packing */
  for (int i=0; i<IMEConnect::mTotalWord; i++) {
    nsString jsWord;

    KIKA_LOGD("PackCandidates::candidate[%d] = %ls", i, IMEConnect::mCandVoca.record(i));

    if (i == IMEConnect::mActiveWordIndex && IMEConnect::mActiveSelectionBar == eSelectionBarWord) {
      jsWord.AppendLiteral(">");
    }

    for (int j=0; j<IMEConnect::mCandVoca.vlen(i); j++) {
      jsWord.Append((PRUnichar)IMEConnect::mCandVoca.record(i)[j]);
    }

    jsWord.AppendLiteral("|");

    IMEConnect::mCandidateWord.Append(jsWord);
  }
}

void FetchCandidates()
{
  uint8_t imeId = IMEConnect::mCurrentLID & IQQI_IME_ID_MASK;
  uint8_t keyboardId = (IMEConnect::mCurrentLID & KEYBOARD_ID_MASK) >> KEYBOARD_ID_SHIFT;
  bool isGroupSupported = (imeId == eImeChineseCn || imeId == eImeChineseTw) && (keyboardId == eKeyboardT9);
  bool isChinese = (imeId == eImeChineseCn || imeId == eImeChineseTw || imeId == eImeChineseHk);
  wstring wsKeyin;

  IMEConnect::mKeyBuff.push_back(0x0);
  wsKeyin = WstrToLower(reinterpret_cast<wchar_t*>(&IMEConnect::mKeyBuff[0]));
  IMEConnect::mKeyBuff.pop_back();

  if (isGroupSupported) {
    IMEConnect::mCandGroup.alloc(MAX_GROUP_COUNT, MAX_GROUP_LENGTH);
    IMEConnect::mTotalGroup = IQQI_GetGrouping(imeId, (wchar_t *)wsKeyin.c_str(), 0, MAX_GROUP_COUNT, IMEConnect::mCandGroup.pointer());

    if (IMEConnect::mTotalGroup > 0) {
      wsKeyin.append(L":");
      wsKeyin.append(IMEConnect::mCandGroup.record(IMEConnect::mActiveGroupIndex));

      IMEConnect::mCandVoca.alloc(MAX_WORD_ZH_COUNT, MAX_WORD_ZH_LENGTH);
      IQQI_GetCandidates(imeId, (wchar_t *)wsKeyin.c_str(), false, 3, 0, MAX_WORD_ZH_COUNT, IMEConnect::mCandVoca.pointer());
    }
  } else {
    int maxCount = isChinese ? MAX_WORD_ZH_COUNT : MAX_WORD_COUNT;
    int maxLength = isChinese ? MAX_WORD_ZH_LENGTH : MAX_WORD_LENGTH;

    IMEConnect::mCandVoca.alloc(maxCount, maxLength);
    IQQI_GetCandidates(imeId, (wchar_t *)wsKeyin.c_str(), false, 3, 0, maxCount, IMEConnect::mCandVoca.pointer());
  }

  IMEConnect::mTotalWord = IQQI_GetCandidateCount(0, (wchar_t *)wsKeyin.c_str(), false, 0);

  PackCandidates();
}

void
IMEConnect::SetLetterMultiTap(const unsigned long aKeyCode, const unsigned long aTapCount, unsigned short aPrevUnichar)
{
  uint8_t imeId = IMEConnect::mCurrentLID & IQQI_IME_ID_MASK;
  uint8_t keyboardId = (IMEConnect::mCurrentLID & KEYBOARD_ID_MASK) >> KEYBOARD_ID_SHIFT;

  if (mInitStatus == eInitStatusError) {
    KIKA_LOGE("SetLetterMultiTap::init failed mInitStatus=%d", mInitStatus);
    return;
  }

  if (keyboardId != eKeyboardT9) {
    KIKA_LOGW("SetLetterMultiTap::Not support in current keyboardId = 0x%x", keyboardId);
    return;
  }

  if (!((aKeyCode >= 0x30 && aKeyCode <= 0x39) || aKeyCode == 0x2A)) { // Except 0~9, *
    KIKA_LOGW("SetLetterMultiTap::Invalid aKeyCode = 0x%lx", aKeyCode);
    return;
  }

  KIKA_LOGD("SetLetterMultiTap::aKeyCode = 0x%lx, aPrevUnichar = 0x%x, count = %ld", aKeyCode, aPrevUnichar, aTapCount);

  wchar_t *keyin = IQQI_MultiTap_Input(imeId, (int)aKeyCode, (int)aPrevUnichar, (int)aTapCount);

  if (keyin == 0) {
    KIKA_LOGW("SetLetterMultiTap::MultiTap invalid keyin");
    return;
  }

  IMEConnect::mCandidateWord.AssignLiteral("");

  KIKA_LOGD("SetLetterMultiTap::keyin = %ls", keyin);

  nsString jsWord;
  for (int i=0; keyin[i]!=0; i++) {
    mKeyBuff.push_back(keyin[i]);
    jsWord.Append((PRUnichar)keyin[i]);
  }
  IMEConnect::mCandidateWord.Append(jsWord);
}

void
IMEConnect::SetLetter(const unsigned long aHexPrefix, const unsigned long aHexLetter, ErrorResult& aRv)
{
  uint8_t imeId = IMEConnect::mCurrentLID & IQQI_IME_ID_MASK;
  uint8_t keyboardId = (IMEConnect::mCurrentLID & KEYBOARD_ID_MASK) >> KEYBOARD_ID_SHIFT;
  bool isGroupSupported = (imeId == eImeChineseCn || imeId == eImeChineseTw) && (keyboardId == eKeyboardT9);
  bool shouldRefetch = false;

  if (mInitStatus != eInitStatusSuccess) {
    KIKA_LOGE("SetLetter::init failed mInitStatus=%d", mInitStatus);
    return;
  }

  KIKA_LOGD("SetLetter::aHexPrefix = 0x%x, aHexLetter = 0x%x", (int)aHexPrefix, (int)aHexLetter);

  switch (aHexLetter) {
    case 0x26: // up arrow
    case 0x28: // down arrow
      if (isGroupSupported) {
        mActiveSelectionBar = (mActiveSelectionBar == eSelectionBarWord) ? eSelectionBarGroup : eSelectionBarWord;
        PackCandidates();
      }
      break;
    case 0x25: // left arrow
      if (isGroupSupported && (mActiveSelectionBar == eSelectionBarGroup)) {
        mActiveGroupIndex = (mActiveGroupIndex > 0 ? mActiveGroupIndex - 1 : mTotalGroup - 1);
        mActiveWordIndex = 0;
        shouldRefetch = true;
      } else {
        mActiveWordIndex = (mActiveWordIndex > 0 ? mActiveWordIndex - 1 : mTotalWord - 1);
        PackCandidates();
      }
      break;
    case 0x27: // right arrow
      if (isGroupSupported && (mActiveSelectionBar == eSelectionBarGroup)) {
        mActiveGroupIndex = (mActiveGroupIndex < mTotalGroup - 1 ? mActiveGroupIndex + 1 : 0);
        mActiveWordIndex = 0;
        shouldRefetch = true;
      } else {
        mActiveWordIndex = (mActiveWordIndex < mTotalWord - 1 ? mActiveWordIndex + 1 : 0);
        PackCandidates();
      }
      break;
    case 0x0D: // enter
    case 0x20: // space
      mKeyBuff.clear();
      IMEConnect::mCandidateWord.AssignLiteral("");
      break;
    case 0x08: // backspace
      if (mKeyBuff.size() > 0) {
        mKeyBuff.pop_back();
        mActiveWordIndex = 0;
        if (mKeyBuff.size() != 0) {
          shouldRefetch = true;
          if (isGroupSupported) {
            mActiveGroupIndex = 0;
            mActiveSelectionBar = eSelectionBarGroup;
          }
        } else {
          mKeyBuff.clear();
          IMEConnect::mCandidateWord.AssignLiteral("");
        }
      }
      break;
    default:
      if (keyboardId == eKeyboardT9) {
        if ((aHexLetter >= 0x30 && aHexLetter <= 0x39) || aHexLetter == 0x2A) { // 0~9, *
          KIKA_LOGD("SetLetter::push aHexLetter = 0x%x", (int)aHexLetter);
          mKeyBuff.push_back(aHexLetter);
          mActiveWordIndex = 0;
          shouldRefetch = true;
          if (isGroupSupported) {
            mActiveGroupIndex = 0;
            mActiveSelectionBar = eSelectionBarGroup;
          }
        }
      } else if (keyboardId == eKeyboardQwerty) {
        if (aHexLetter >= 0x41 && aHexLetter <= 0x5A) { // A~Z
          KIKA_LOGD("SetLetter::push aHexLetter = 0x%x", (int)aHexLetter);
          mKeyBuff.push_back(aHexLetter);
          mActiveWordIndex = 0;
          shouldRefetch = true;
          if (isGroupSupported) {
            mActiveGroupIndex = 0;
            mActiveSelectionBar = eSelectionBarGroup;
          }
        }
      }
      break;
  }

  if (shouldRefetch) {
    FetchCandidates();
  }
}

void
IMEConnect::GetNextWordCandidates(const nsAString& aWord, nsAString& aRetval)
{
  uint8_t imeId = IMEConnect::mCurrentLID & IQQI_IME_ID_MASK;
  bool isChinese = (imeId == eImeChineseCn || imeId == eImeChineseTw || imeId == eImeChineseHk);
  bool isKorean = (imeId == eImeKorean);
  int maxCount = isChinese ? MAX_NEXTWORD_ZH_COUNT : MAX_NEXTWORD_COUNT;
  int maxLength = isChinese ? MAX_NEXTWORD_ZH_LENGTH : MAX_NEXTWORD_LENGTH;
  int total;
  nsString currentWord, nextWordCandidates;
  CandidateCH candNextWord;

  if (mInitStatus != eInitStatusSuccess) {
    KIKA_LOGE("GetNextWordCandidates::init failed mInitStatus=%d", mInitStatus);
    return;
  }

  currentWord = nsString(aWord);
  wchar_t wsWord[currentWord.Length() + 1];
  U16strToWstr(wsWord, currentWord.get(), currentWord.Length());

  candNextWord.alloc(maxCount, maxLength);

  if (!isKorean) {
    total = IQQI_GetNextWordCandidates(imeId, (wchar_t*)wsWord, 0, maxCount, candNextWord.pointer());
  } else {
    total = IQQI_GetCandidates(imeId, (wchar_t *)wsWord, false, 0, 0, maxCount, candNextWord.pointer());
  }

  KIKA_LOGD("GetNextWordCandidates::total = %d, wsWord = %ls", total, wsWord);

  for (int i=0; i<total; i++) {
    for (int j=0; j<candNextWord.vlen(i); j++) {
      nextWordCandidates.Append((PRUnichar)candNextWord.record(i)[j]);
    }
    nextWordCandidates.AppendLiteral("|");
  }
  candNextWord.empty();

  KIKA_LOGD("GetNextWordCandidates::nextWordCandidates = %s", NS_ConvertUTF16toUTF8(nextWordCandidates).get());

  aRetval = nextWordCandidates;
}

void
IMEConnect::GetComposingWords(const nsAString& aLetters, const long aIndicator, nsAString& aRetval)
{
  uint8_t imeId = mCurrentLID & IQQI_IME_ID_MASK;
  bool isKorean = (imeId == eImeKorean);
  nsString words;

  if (isKorean) {
    wchar_t letters[aLetters.Length() + 1];
    wchar_t* wsWords;

    U16strToWstr(letters, ((nsString)aLetters).get(), aLetters.Length());
    wsWords = IQQI_GetComposingText(imeId, letters, 0, (int)aIndicator);

    size_t length = wcslen(wsWords);
    for (size_t i=0; i<length; i++) {
      words.Append((PRUnichar)wsWords[i]);
    }

    KIKA_LOGD("GetComposingWords::letters = %ls, words = %s", letters, NS_ConvertUTF16toUTF8(words).get());
  } else {
    KIKA_LOGW("GetComposingWords::unexpected imeId = 0x%x", imeId);
  }

  aRetval = words;
}

void
IMEConnect::ImportDictionary(Blob& aBlob, ErrorResult& aRv)
{
  uint64_t blobSize;
  uint32_t numRead;
  char *readBuf;
  int ret;
  uint8_t imeId = mCurrentLID & IQQI_IME_ID_MASK;

  if (mInitStatus != eInitStatusSuccess) {
    KIKA_LOGE("ImportDictionary::init failed mInitStatus=%d", mInitStatus);
    return;
  }

  blobSize = aBlob.GetSize(aRv);
  if (aRv.Failed()) {
    KIKA_LOGE("ImportDictionary::failed to get blob blobSize=%lld", blobSize);
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsCOMPtr<nsIInputStream> stream;
  aBlob.GetInternalStream(getter_AddRefs(stream), aRv);
  if (aRv.Failed()) {
    KIKA_LOGE("ImportDictionary::failed to get internal stream");
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  readBuf = (char*)malloc(blobSize);
  if (!readBuf) {
    KIKA_LOGE("ImportDictionary::failed allocate memory");
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  aRv = stream->Read(readBuf, blobSize, &numRead);
  if (aRv.Failed() || (numRead != blobSize)) {
    KIKA_LOGE("ImportDictionary::failed to read stream");
    free(readBuf);
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  ret = IQQI_LearnWordByToken(imeId, NULL, readBuf, blobSize);
  if (ret) {
    KIKA_LOGE("ImportDictionary::failed on IQQI_LearnWordByToken ret = %d", ret);
    free(readBuf);
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  free(readBuf);
  KIKA_LOGI("ImportDictionary::loading dictionary successfully");
}

uint32_t
IMEConnect::SetLanguage(const uint32_t aLid)
{
  int err;
  char IME_ErrorList[32] = {0};
  uint8_t imeId = aLid & IQQI_IME_ID_MASK;
  uint8_t keyboardId = (aLid & KEYBOARD_ID_MASK) >> KEYBOARD_ID_SHIFT;
  string dictPath = DICT_ROOT_PATH;

  mInitStatus = eInitStatusError;
  mCurrentLID = 0;

  mCandidateWord.AssignLiteral("");
  mKeyBuff.clear();
  mActiveWordIndex = 0;
  mActiveGroupIndex = 0;
  mActiveSelectionBar = eSelectionBarWord;
  mCandVoca.empty();
  mCandGroup.empty();

  if (imeId <= eImeStart || imeId >= eImeEnd) {
    KIKA_LOGW("SetLanguage::imeId = 0x%x not found", imeId);
    return mInitStatus;
  }

  if (keyboardId <= eKeyboardStart || keyboardId >= eKeyboardEnd) {
    KIKA_LOGW("SetLanguage::keyboardId = 0x%x not found", keyboardId);
    return mInitStatus;
  }

  KIKA_LOGD("SetLanguage::keyboardId = 0x%x, imeId = 0x%x", keyboardId, imeId);

  IQQI_SetOption(eOptionKeyboardMode, keyboardId);

  dictPath += DICT_TABLE[imeId];
  err = IQQI_Initial(imeId, (char*)dictPath.c_str(), NULL, NULL, IME_ErrorList);
  if (err != KIKA_OK) {
    KIKA_LOGW("SetLanguage::IQQI_initial() dictPath = %s, err = %d", dictPath.c_str(), err);
    mInitStatus = eInitStatusDictNotFound;
  } else {
    mInitStatus = eInitStatusSuccess;
  }

  mCurrentLID = aLid;

  KIKA_LOGI("SetLanguage::mCurrentLID = 0x%x, mInitStatus = %d" , mCurrentLID, mInitStatus);

  return mInitStatus;
}

JSObject*
IMEConnect::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return mozilla::dom::IMEConnectBinding::Wrap(aCx, this, aGivenProto);
}

IMEConnect::~IMEConnect()
{
  IQQI_Free();
}


} // namespace dom
} // namespace mozilla
