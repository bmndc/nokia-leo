/* (c) 2019 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
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


#define U16STR_TO_U8CSTR(u16str)    NS_ConvertUTF16toUTF8(u16str.data()).get()


namespace mozilla {
namespace dom {


uint8_t  IMEConnect::mInitStatus;
uint8_t  IMEConnect::mImeId;
uint8_t  IMEConnect::mKeyboardId;
uint16_t IMEConnect::mTotalWord;
uint16_t IMEConnect::mTotalGroup;
uint16_t IMEConnect::mActiveWordIndex;
uint16_t IMEConnect::mActiveGroupIndex;
uint8_t  IMEConnect::mActiveSelectionBar;

nsString IMEConnect::mCandidateWord;
list<u16string> IMEConnect::mCandWord;
list<u16string> IMEConnect::mCandGroup;
vector<wchar_t> IMEConnect::mTypedKeycodes;
vector<u16string> IMEConnect::mHistoryWords;
bool IMEConnect::mHasPreciseCandidate;

map<ctunicode_t, vector<ctunicode_t>> IMEConnect::mKeyLetters;
struct DictFile IMEConnect::mDictFile;


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
  CT_LOGI("Init::++");
  SetLanguage(aLid);
  return NS_OK;
}

void
IMEConnect::SetLetter(const unsigned long aHexPrefix, const unsigned long aHexLetter,
  ErrorResult& aRv)
{
  bool isGroupSupported = (mImeId == eImeChinesePinyin || mImeId == eImeChineseZhuyin)
    && mKeyboardId == eKeyboardT9;
  bool shouldRefetch = false;

  if (mInitStatus != eInitStatusSuccess) {
    CT_LOGE("SetLetter::init failed mInitStatus=%d", mInitStatus);
    return;
  }

  CT_LOGD("SetLetter::aHexPrefix=0x%x, aHexLetter=0x%x", (int)aHexPrefix, (int)aHexLetter);

  switch (aHexLetter) {
    case 0x26: // up arrow
    case 0x28: // down arrow
      if (isGroupSupported) {
        mActiveSelectionBar =
          ((mActiveSelectionBar == eSelectionBarWord) ? eSelectionBarGroup : eSelectionBarWord);
        // TODO: remove unnecessary fetching operations and enhance performance
        FetchCandidates();
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
      mTypedKeycodes.clear();
      mCandidateWord.AssignLiteral("");
      break;
    case 0x08: // backspace
      if (mTypedKeycodes.size() > 0) {
        mTypedKeycodes.pop_back();
        mActiveWordIndex = 0;
        if (mTypedKeycodes.size() != 0) {
          shouldRefetch = true;
          if (isGroupSupported) {
            mActiveGroupIndex = 0;
            mActiveSelectionBar = eSelectionBarGroup;
          }
        } else {
          mTypedKeycodes.clear();
          mCandidateWord.AssignLiteral("");
        }
      }
      break;
    default:
      if ((mKeyboardId == eKeyboardT9 && ((aHexLetter >= 0x30 && aHexLetter <= 0x39) || aHexLetter == 0x2A)) // 0~9, *
        || mKeyboardId == eKeyboardQwerty) {
        mTypedKeycodes.push_back(aHexLetter);
        mActiveWordIndex = 0;
        shouldRefetch = true;
        if (isGroupSupported) {
          mActiveGroupIndex = 0;
          mActiveSelectionBar = eSelectionBarGroup;
        }
      }
      break;
  }

  if (shouldRefetch) {
    FetchCandidates();
  }
}

void
IMEConnect::SetLetterMultiTap(const unsigned long aKeyCode, const unsigned long aTapCount,
  unsigned short aPrevUnichar)
{
  if (mInitStatus == eInitStatusError) {
    CT_LOGE("SetLetterMultiTap::init failed mInitStatus=%d", mInitStatus);
    return;
  }

  if (mKeyboardId != eKeyboardT9) {
    CT_LOGE("SetLetterMultiTap::invalid mKeyboardId=0x%x", mKeyboardId);
    return;
  }

  if (!((aKeyCode >= 0x30 && aKeyCode <= 0x39) || aKeyCode == 0x2A)) {
    CT_LOGE("SetLetterMultiTap::invalid aKeyCode=0x%lx", aKeyCode);
    return;
  }

  u16string str;
  unsigned int len = mKeyLetters[aKeyCode].size();
  for (unsigned int i=0; i<len; i++) {
    if (i == ((aTapCount - 1) % len)) {
      str.append((char16_t*)">", 1);
    }
    str.append((char16_t*)&mKeyLetters[aKeyCode][i], 1);
    str.append((char16_t*)"|", 1);
  }

  CT_LOGD("SetLetterMultiTap::str=%s", U16STR_TO_U8CSTR(str));

  mCandidateWord = nsString(str.c_str(), str.length());
}

void
IMEConnect::GetWordCandidates(const nsAString& aLetters, const Sequence<nsString>& aContext, nsAString& aRetval)
{
  if (mInitStatus != eInitStatusSuccess) {
    CT_LOGE("GetWordCandidates::init failed mInitStatus=%d", mInitStatus);
    return;
  }

  mTypedKeycodes.clear();
  mHistoryWords.clear();

  u16string letters = u16string(((nsString)aLetters).get());
  for (unsigned int i=0; i<letters.length(); i++) {
    mTypedKeycodes.push_back((wchar_t)tolower(letters[i]));
  }

  for (unsigned int i=0; i<aContext.Length() && i<CUSTOM_HISTORY_SIZE; i++) {
    u16string context = u16string(((nsString)aContext[i]).get());
    mHistoryWords.push_back(context);
  }

  FetchCandidates();
  aRetval = mCandidateWord;
}

void
IMEConnect::GetNextWordCandidates(const nsAString& aWord, nsAString& aRetval)
{
  struct CT_InputContext input = {0};
  struct CT_SearchResult result = {0};
  struct CT_FilterList filterList = {0};
  struct CT_CandidateItem candItems[CT_DEFAULT_MAX_REQUEST] = {0};
  ctint32 ret;

  if (mInitStatus != eInitStatusSuccess) {
    CT_LOGE("GetNextWordCandidates::init failed mInitStatus=%d", mInitStatus);
    return;
  }

  // setup current word
  u16string curWord = u16string(((nsString)aWord).get());
  for (unsigned int i=0; i<curWord.length() && i<CT_MAX_WORD_ARRAY_SIZE; i++) {
    input.history.history_items[0].word[i] = curWord[i];
  }
  input.history.history_size++;

  // default settings
  input.request_size = CT_DEFAULT_MAX_REQUEST;
  input.filters_size = 0;

  result.filter_list = &filterList;
  result.candidate_items = candItems;

  // search nextword
  ret = CT_RetrieveNextWordCandidates(mDictFile.dic, &input.history, input.request_size, &result);
  if (ret) {
    CT_LOGW("GetNextWordCandidates::CT_RetrieveNextWordCandidates failed ret=%d", ret);
    return;
  }

  // pack nextword result
  u16string str;
  for (unsigned int i=0; i<result.candidate_items_size; i++) {
    str.append((char16_t*)result.candidate_items[i].word_item.word);
    str.append((char16_t*)"|", 1);
  }

  CT_LOGD("GetNextWordCandidates::str=%s", U16STR_TO_U8CSTR(str));
  aRetval = nsString(str.c_str(), str.length());
}

void
IMEConnect::ImportDictionary(Blob& aBlob, ErrorResult& aRv)
{
  uint64_t blobSize;
  uint32_t numRead;
  char *readBuf;
  int ret;

  if (mInitStatus != eInitStatusSuccess) {
    CT_LOGE("ImportDictionary::init failed mInitStatus=%d", mInitStatus);
    return;
  }

  CT_LOGI("ImportDictionary::mImeId=0x%x", mImeId);

  if (mImeId == eImeChinesePinyin || mImeId == eImeChineseBihua || mImeId == eImeChineseZhuyin) {
    CT_LOGE("ImportDictionary::invalid mImeId=0x%x", mImeId);
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  blobSize = aBlob.GetSize(aRv);
  if (aRv.Failed()) {
    CT_LOGE("ImportDictionary::blob GetSize failed");
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsCOMPtr<nsIInputStream> stream;
  aBlob.GetInternalStream(getter_AddRefs(stream), aRv);
  if (aRv.Failed()) {
    CT_LOGE("ImportDictionary::blob GetInternalStream failed");
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  readBuf = (char*)malloc(blobSize);
  if (!readBuf) {
    CT_LOGE("ImportDictionary::blob malloc failed");
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  aRv = stream->Read(readBuf, blobSize, &numRead);
  if (aRv.Failed() || (numRead != blobSize)) {
    CT_LOGE("ImportDictionary::blob read stream failed");
    free(readBuf);
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  ret = CT_ResetUsrCache(mDictFile.dic);
  if (ret) {
    CT_LOGE("ImportDictionary::CT_ResetUsrCache failed ret=%d", ret);
    free(readBuf);
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  string word;
  for (unsigned int i=0; i<blobSize; i++) {
    if (readBuf[i] == '|') {
      struct CT_WordItem word_item = {0};
      ctunicode_t evidence[CT_DISPLAY_EVIDENCE_ARRAY_SIZE] = {0};

      str_to_wstr(word_item.word, word.c_str());
      CT_LOGD("ImportDictionary::word=%s", word.c_str());

      ret = CT_AddWordToDictionary(mDictFile.dic, &word_item, evidence);
      if (ret < 0) {
        CT_LOGW("ImportDictionary::CT_AddWordToDictionary failed, ret=%d", ret);
        free(readBuf);
        aRv.Throw(NS_ERROR_FAILURE);
        return;
      }
      word.clear();
      continue;
    }
    word += readBuf[i];
  }

  free(readBuf);
}

uint32_t
IMEConnect::SetLanguage(const uint32_t aLid)
{
  int ret;
  uint8_t imeId = aLid & IME_ID_MASK;
  uint8_t keyboardId = (aLid & KEYBOARD_ID_MASK) >> KEYBOARD_ID_SHIFT;

  CT_LOGI("SetLanguage::aLid=0x%x", aLid);

  mInitStatus = eInitStatusError;
  mImeId = 0;
  mKeyboardId = 0;

  mCandidateWord.AssignLiteral("");
  mCandWord.clear();
  mCandGroup.clear();
  mTypedKeycodes.clear();
  mHistoryWords.clear();
  mTotalWord = 0;
  mTotalGroup = 0;
  mActiveWordIndex = 0;
  mActiveGroupIndex = 0;
  mActiveSelectionBar = eSelectionBarWord;

  if (imeId <= eImeStart || imeId >= eImeEnd) {
    CT_LOGE("SetLanguage::invalid imeId=0x%x", imeId);
    return mInitStatus;
  }

  if (keyboardId <= eKeyboardStart || keyboardId >= eKeyboardEnd) {
    CT_LOGE("SetLanguage::invalid keyboardId=0x%x", keyboardId);
    return mInitStatus;
  }

  if (mDictFile.dic != NULL) {
    ret = DeinitSysDictionary();
    if (ret < 0) {
      CT_LOGE("SetLanguage::DeinitSysDictionary failed ret=%d", ret);
      return mInitStatus;
    }
  }

  ret = InitKeyLayout(imeId, keyboardId);
  if (ret < 0) {
    CT_LOGE("SetLanguage::InitKeyLayout failed ret=%d", ret);
    return mInitStatus;
  }

  ret = InitSysDictionary(imeId);
  if (ret < 0) {
    CT_LOGE("SetLanguage::InitSysDictionary failed ret=%d", ret);
    mInitStatus = eInitStatusDictNotFound;
  } else {
    mInitStatus = eInitStatusSuccess;
  }

  mImeId = imeId;
  mKeyboardId = keyboardId;

  CT_LOGI("SetLanguage::mImeId=0x%x, mKeyboardId=0x%x, mInitStatus=%d", mImeId, mKeyboardId, mInitStatus);

  return mInitStatus;
}

int
IMEConnect::InitKeyLayout(uint32_t aImeId, uint32_t aKeyboardId)
{
  FILE *fd;
  int layoutsNum, keysNum;
  char layoutName[7];
  char buf[KEY_LAYOUT_MAX_BUF_SIZE];

  string keyboardType = "";
  if (aKeyboardId == eKeyboardT9) {
    keyboardType = "T9";
  } else if (aKeyboardId == eKeyboardQwerty) {
    keyboardType = "QWERTY";
  }

  mKeyLetters.clear();

  string path = DICT_ROOT_PATH + KEYLAYOUT_FILE[aImeId];
  fd = fopen(path.c_str(), "r");
  if (fd == NULL) {
    CT_LOGE("InitKeyLayout::fopen fail");
    return ERROR;
  }

  fscanf(fd, "%d", &layoutsNum);

  while (layoutsNum > 0) {
    fscanf(fd, "%s", layoutName);
    fscanf(fd, "%d", &keysNum);

    if (strcmp(layoutName, keyboardType.c_str())) {
      fgets(buf, sizeof(buf), fd);
      while (keysNum > 0) {
        fgets(buf, sizeof(buf), fd);
        keysNum--;
      }
    } else {
      break;
    }
    layoutsNum--;
  }

  if (layoutsNum <= 0) {
    fclose(fd);
    CT_LOGE("InitKeyLayout::layout not found");
    return ERROR;
  }

  fgets(buf, sizeof(buf), fd);
  for (int i=0; i<keysNum; i++) {
    fgets(buf, sizeof(buf), fd);
    if (buf[0]) {
      buf[strlen(buf) - 1] = 0;
      ctunicode_t letters[KEY_LAYOUT_MAX_LETTERS] = {0};
      str_to_wstr(letters, buf);
      vector<ctunicode_t> letterVec;
      for (int j=0; j<KEY_LAYOUT_MAX_LETTERS && letters[j]; j++) {
        letterVec.push_back(letters[j]);
      }

      if (aKeyboardId == eKeyboardT9) {
        if (i == 10) {
          mKeyLetters['*'] = letterVec;
        } else {
          mKeyLetters['0'+i] = letterVec;
        }
      } else if (aKeyboardId == eKeyboardQwerty) {
        mKeyLetters[letters[0]] = letterVec;
      }

    }
  }
  fclose(fd);

  CT_LOGD("InitKeyLayout::path=%s", path.c_str());
  return 0;
}

int
IMEConnect::InitSysDictionary(uint32_t aImeId) {
  string romPath;
  string usrPath;
  string extPath;
  CT_DictionaryEngineType engineType;
  ctint32 ret;

  romPath = DICT_ROOT_PATH + SYS_DICT_FILE[aImeId];
  switch (aImeId) {
    case eImeChinesePinyin:
      engineType = CT_ChinesePinYin;
      break;
    case eImeChineseBihua:
      engineType = CT_ChineseBiHua;
      extPath = DICT_ROOT_PATH + SYS_DICT_FILE[eImeChinesePinyin];
      break;
    case eImeChineseZhuyin:
      engineType = CT_ChineseZhuyin;
      break;
    default:
      engineType = CT_Western;
      usrPath = DICT_ROOT_PATH + WESTERN_USR_DICT_FILE;
      break;
  }

  // init system dictionary
  mDictFile.file_handles_base[mDictFile.file_size] =
    (struct CT_BaseImageDescriptor *)ct_image_file_descriptor_init(romPath.c_str(), CTO_RDONLY);
  if (mDictFile.file_handles_base[mDictFile.file_size] == NULL) {
    CT_LOGE("InitSysDictionary::init rom dict failed, romPath=%s", romPath.c_str());
    return ERROR;
  }
  mDictFile.file_handles_base[mDictFile.file_size]->image_type = CT_RomFile;
  mDictFile.images_list.sys_images[0].images[0] = mDictFile.file_handles_base[mDictFile.file_size];
  mDictFile.images_list.sys_images[0].n_images++;
  mDictFile.file_size++;

  // init extra system dictionay for special case
  if (!extPath.empty()) {
    mDictFile.file_handles_base[mDictFile.file_size] =
      (struct CT_BaseImageDescriptor *)ct_image_file_descriptor_init(extPath.c_str(), CTO_RDONLY);
    if (mDictFile.file_handles_base[mDictFile.file_size] == NULL) {
      CT_LOGE("InitSysDictionary::init ext dict failed, extPath=%s", extPath.c_str());
      return ERROR;
    }
    mDictFile.file_handles_base[mDictFile.file_size]->image_type = CT_RomFile;
    mDictFile.images_list.sys_images[0].images[1] = mDictFile.file_handles_base[mDictFile.file_size];
    mDictFile.images_list.sys_images[0].n_images++;
    mDictFile.file_size++;
  }

  // init user dictionary
  if (!usrPath.empty()) {
    mDictFile.file_handles_base[mDictFile.file_size] =
      (struct CT_BaseImageDescriptor *)ct_image_file_descriptor_init(usrPath.c_str(), CTO_RDONLY);
    if (mDictFile.file_handles_base[mDictFile.file_size] == NULL) {
      CT_LOGE("InitSysDictionary::init usr dict failed, usrPath=%s", usrPath.c_str());
      return ERROR;
    }
    mDictFile.file_handles_base[mDictFile.file_size]->image_type = CT_UsrFile;
    mDictFile.images_list.usr_image.images[0] = mDictFile.file_handles_base[mDictFile.file_size];
    mDictFile.images_list.usr_image.n_images++;
    mDictFile.file_size++;
  }

  mDictFile.images_list.n_languages++;

  mDictFile.dic = CT_InitializeDictionary(engineType, &mDictFile.images_list);
  if (mDictFile.dic == NULL) {
    CT_LOGE("InitSysDictionary::initialize dictionaries failed");
    return ERROR;
  }

  // reset cache of the memory-based user dictionary
  if (!usrPath.empty()) {
    ret = CT_ResetUsrCache(mDictFile.dic);
    if (ret) {
      CT_LOGE("InitSysDictionary::CT_ResetUsrCache failed ret=%d", ret);
      return ERROR;
    }
  }

  CT_LOGD("InitSysDictionary::romPath=%s", romPath.c_str());
  return 0;
}

int
IMEConnect::DeinitSysDictionary()
{
  ctint32 ret = CT_DeinitializeDictionary(mDictFile.dic);
  if (ret) {
    CT_LOGW("DeinitSysDictionary::CT_DeinitializeDictionary failed ret=%d", ret);
    return ERROR;
  }

  for (int i=0; i<mDictFile.file_size; i++) {
    ct_deinit_image_file_descriptor((struct CT_ImageFileDescriptor *)mDictFile.file_handles_base[i]);
  }

  memset(&mDictFile, 0, sizeof(mDictFile));

  return 0;
}

void
IMEConnect::FetchCandidates()
{
  bool isGroupSupported = (mImeId == eImeChinesePinyin || mImeId == eImeChineseZhuyin)
    && mKeyboardId == eKeyboardT9;
  bool isChinese = (mImeId == eImeChinesePinyin || mImeId == eImeChineseBihua
    || mImeId == eImeChineseZhuyin);
  bool isSingleKeyInT9 = (mKeyboardId == eKeyboardT9 && mTypedKeycodes.size() == 1 && !isChinese);

  mCandWord.clear();
  mCandGroup.clear();
  mTotalWord = 0;
  mTotalGroup = 0;
  mHasPreciseCandidate = false;

  // if only one key is inputed, it fetches candidates from layout instead of from dictionary
  if (isSingleKeyInT9) {
    // fetch candidate letters from layout
    wchar_t keycode = mTypedKeycodes.at(0);
    for (unsigned int i=0; i<mKeyLetters[keycode].size(); i++) {
      if (mKeyLetters[keycode][i] >= '0' && mKeyLetters[keycode][i] <= '9') {
        continue;
      }
      char16_t letter = mKeyLetters[keycode][i];
      u16string str = u16string(&letter, 0, 1);
      mCandWord.push_back(str);
    }

    PackCandidates();
    return;
  }

  struct CT_InputContext input = {0};
  struct CT_InputContextChineseInfo chineseInfo = {0};
  struct CT_SearchResult result = {0};
  struct CT_FilterList filterList = {0};
  struct CT_CandidateItem candItems[CT_DEFAULT_MAX_REQUEST] = {0};
  ctint32 ret;

  // setup input code expand callback function
  for (unsigned int i=0; i<mTypedKeycodes.size(); i++) {
    if (mImeId == eImeChineseBihua) {
      wchar_t keycode = mTypedKeycodes.at(i);
      input.input_codes[i] = mKeyLetters[keycode][0];
    } else {
      wchar_t keycode = mTypedKeycodes.at(i);
      input.input_codes[i] = keycode;
    }
    input.input_codes_size++;
  }
  input.callback_function = &GetInputCodeExpandResult;

  // setup history words
  for (unsigned int i=0; i<mHistoryWords.size() && i<CT_MAX_HISTORY_SIZE; i++) {
    for (unsigned int j=0; j<mHistoryWords[i].length() && j<CT_MAX_WORD_ARRAY_SIZE; j++) {
      input.history.history_items[i].word[j] = mHistoryWords[i][j];
    }
    input.history.history_size++;
  }

  // default settings
  input.request_size = CT_DEFAULT_MAX_REQUEST;
  input.filters_size = 0;
  input.options |= CT_OPTION_ENABLE_SPELL_CHECK;

  if (isChinese) {
    input.chinese_info = &chineseInfo;
    input.chinese_info->py_options |= CT_OPTION_PY_ADVANCE_LEVEL1;
    input.chinese_info->py_options |= CT_OPTION_PY_ENABLE_JIANPIN_SENTENCE;
    input.chinese_info->py_options |= CT_OPTION_PY_ERROR_PRONUNCIATION;
    input.chinese_info->py_options |= CT_OPTION_PY_ENABLE_WESTERN;
    input.chinese_info->py_options |= CT_OPTION_PY_ENABLE_ENV;
  }

  result.filter_list = &filterList;
  result.candidate_items = candItems;

  // search words
  ret = CT_SearchDictionary(mDictFile.dic, &input, &result);
  if (ret != 0) {
    CT_LOGW("FetchCandidates::CT_SearchDictionary failed ret=%d", ret);
  }

  if (isGroupSupported) {
    // take filter_list as group candidates
    for (unsigned int i=0; i<result.filter_list->list_size; i++) {
      u16string str = u16string((char16_t*)result.filter_list->list[i].filter_str);
      mCandGroup.push_back(str);
    }

    // setup chosen filter
    unsigned int idx = mActiveGroupIndex;
    u16string chosen = u16string((char16_t*)result.filter_list->list[idx].filter_str);
    CT_LOGD("FetchCandidates::chosen=%s, length=%d", U16STR_TO_U8CSTR(chosen), chosen.length());
    for (unsigned int i=0; i<chosen.length() && i<CT_PINYIN_INDEX_PINYIN_LETTER_SIZE; i++) {
      input.chinese_info->filter[0][i] = chosen[i];
    }
    input.chinese_info->filters_size++;

    memset(&result, 0, sizeof(result));
    result.filter_list = &filterList;
    result.candidate_items = candItems;

    // search words by chosen filter
    ret = CT_SearchDictionary(mDictFile.dic, &input, &result);
    if (ret != 0) {
      CT_LOGW("FetchCandidates::CT_SearchDictionary failed ret=%d", ret);
    }
  }

  // take candidate_items as prioritized candidates and precise_itmes as secondary candidates
  if (result.candidate_items_size > 0) {
    for (unsigned int i=0; i<result.candidate_items_size; i++) {
      u16string str = u16string((char16_t*)result.candidate_items[i].word_item.word);
      CT_LOGD("FetchCandidates::candidate_items[%d]=%s", i, U16STR_TO_U8CSTR(str));

      if (isGroupSupported) {
        if (result.candidate_items[i].word_item.tag == CT_TAG_DEFAULT) {
          continue;
        }
      }

      mCandWord.push_back(str);
    }

    // check if precise candidate exists
    if (result.highlight_precise == 0) {
      mHasPreciseCandidate = true;
    }
  } else if (result.precise_items_size > 0) {
    for (unsigned int i=0; i<result.precise_items_size; i++) {
      u16string str = u16string((char16_t*)result.precise_items[i].word_item.word);
      CT_LOGD("FetchCandidates::precise_items[%d]=%s", i, U16STR_TO_U8CSTR(str));

      mCandWord.push_back(str);
    }
  }

  // remove duplicated words
  mCandWord.unique();

  PackCandidates();
}

void
IMEConnect::PackCandidates()
{
  bool isGroupSupported = (mImeId == eImeChinesePinyin || mImeId == eImeChineseZhuyin)
    && mKeyboardId == eKeyboardT9;

  u16string str;

  // groups packing
  if (isGroupSupported) {
    mTotalGroup = mCandGroup.size();
    for (unsigned int i=0; i<mTotalGroup; i++) {
      u16string group = mCandGroup.front();
      mCandGroup.pop_front();
      CT_LOGD("PackCandidates::group[%d]=%s", i, U16STR_TO_U8CSTR(group));

      if (i == mActiveGroupIndex && mActiveSelectionBar == eSelectionBarGroup) {
        str.append((char16_t*)">", 1);
      }
      str.append(group);
      str.append((char16_t*)"|", 1);
    }
    str.append((char16_t*)"|", 1);
  }

  // words packing
  mTotalWord = mCandWord.size();
  for (unsigned int i=0; i<mTotalWord; i++) {
    u16string word = mCandWord.front();
    mCandWord.pop_front();
    CT_LOGD("PackCandidates::word[%d]=%s", i, U16STR_TO_U8CSTR(word));

    if (mKeyboardId == eKeyboardT9) {
      // for t9, it indicates the chosen candidate
      if (i == mActiveWordIndex && mActiveSelectionBar == eSelectionBarWord) {
        str.append((char16_t*)">", 1);
      }
    } else if (mKeyboardId == eKeyboardQwerty && i == 0) {
      // for qwerty, it indicates the precise candidate
      if (mHasPreciseCandidate) {
        str.append((char16_t*)">", 1);
      }
    }

    str.append(word);
    str.append((char16_t*)"|", 1);
  }

  // default empty candidate
  if (!mTotalWord) {
    if (mActiveSelectionBar == eSelectionBarWord) {
      str.append((char16_t*)">", 1);
    }
    str.append((char16_t*)" ", 1);
    str.append((char16_t*)"|", 1);
  }

  CT_LOGD("PackCandidates::str=%s", U16STR_TO_U8CSTR(str));

  mCandidateWord.Assign(nsString(str.c_str(), str.length()));
}

ctint32
IMEConnect::GetInputCodeExpandResult(ctint32 code, void* callbackToken,
  struct CT_InputCodeExpand* InputCodeExpand)
{
  memset(InputCodeExpand, 0, sizeof(struct CT_InputCodeExpand));

  unsigned int len = mKeyLetters[code].size();
  unsigned int count = 0, keyType = 0;
  for (unsigned int i=0; i<len && count<CT_MAX_INPUT_CODE_EXPAND_LENGTH; i++) {
    if (mKeyLetters[code][i] == '|') {
      count = 0;
      keyType++;
      continue;
    }

    // filter unused numbers
    if (mKeyLetters[code][i] >= '0' && mKeyLetters[code][i] <= '9') {
      continue;
    }

    switch (keyType) {
      case 0: // precise letters
        InputCodeExpand->precise_result.result[count++] = mKeyLetters[code][i];
        InputCodeExpand->precise_result.result_length++;
        break;
      case 1: // correction letters
        InputCodeExpand->correction_result[count].prob = CT_INPUT_CORRECTION_DEFAULT_PROB;
        InputCodeExpand->correction_result[count].result[0] = mKeyLetters[code][i];
        InputCodeExpand->correction_result[count++].result_length++;
        InputCodeExpand->correction_result_length = count <= CT_MAX_INPUT_CODE_CORRECTION_NUM ? count : CT_MAX_INPUT_CODE_CORRECTION_NUM;
        break;
      case 2: // vertical correction letters
        InputCodeExpand->vertical_correction[i] = mKeyLetters[code][i];
        InputCodeExpand->vertical_correction_length++;
        break;
      default:
        return ERROR;
    }
  }

  InputCodeExpand->precise_result.prob = CT_INPUT_PRECISE_DEFAULT_PROB;

  return 0;
}

JSObject*
IMEConnect::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return mozilla::dom::IMEConnectBinding::Wrap(aCx, this, aGivenProto);
}

IMEConnect::~IMEConnect()
{
  DeinitSysDictionary();
}

} // namespace dom
} // namespace mozilla
