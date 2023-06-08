/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef mozilla_dom_imeconnect_kika_IMEConnect_h
#define mozilla_dom_imeconnect_kika_IMEConnect_h

#include "nsCOMPtr.h"
#include "nsPIDOMWindow.h"
#include "nsISupports.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/File.h"

#include <list>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

extern "C" {
#include "iqqilib.h"
#include "DataCandidate.h"
#include "WType.h"
}

#include <sys/time.h>
#include <stdio.h>

#include <string.h>
#include <errno.h>

#if ANDROID_VERSION >= 23
#include <algorithm>
#else
#include <stl/_algo.h>
#endif

#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define LOG_KIKA_TAG "GeckoKikaConnect"
#define KIKA_DEBUG 0

#define KIKA_LOGI(args...)  __android_log_print(ANDROID_LOG_INFO, LOG_KIKA_TAG , ## args)
#define KIKA_LOGW(args...)  __android_log_print(ANDROID_LOG_WARN, LOG_KIKA_TAG , ## args)
#define KIKA_LOGE(args...)  __android_log_print(ANDROID_LOG_ERROR, LOG_KIKA_TAG , ## args)

#if KIKA_DEBUG
#define KIKA_LOGD(args...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_KIKA_TAG , ## args)
#else
#define KIKA_LOGD(args...)
#endif

#else

#define KIKA_LOGD(args...)
#define KIKA_LOGW(args...)
#define KIKA_LOGE(args...)

#endif


#define KIKA_OK                   0
#define KIKA_NOT_OK              -1

#define MAX_GROUP_COUNT          10
#define MAX_GROUP_LENGTH          6

#define MAX_WORD_COUNT           21
#define MAX_WORD_LENGTH          45
#define MAX_WORD_ZH_COUNT       120
#define MAX_WORD_ZH_LENGTH        1

#define MAX_NEXTWORD_COUNT       20
#define MAX_NEXTWORD_LENGTH      45
#define MAX_NEXTWORD_ZH_COUNT    20
#define MAX_NEXTWORD_ZH_LENGTH    1

#define IQQI_IME_ID_MASK       0xFF
#define KEYBOARD_ID_MASK     0xFF00
#define KEYBOARD_ID_SHIFT         8

#define DICT_ROOT_PATH       "/system/vendor/dict/kika/"

namespace mozilla {
namespace dom {


enum EImeId {
  eImeStart = 0,
  eImeEnglishUs,
  eImeFrenchCa,
  eImePortugeseBr,
  eImeSpanishUs,
  eImeAssamese,
  eImeBengaliIn,
  eImeBodo,
  eImeDogri,
  eImeGujarati,
  eImeHindi,
  eImeKannada,
  eImeKashmiri,
  eImeKonkani,
  eImeMalayalam,
  eImeManipuri,
  eImeMarathi,
  eImeNepali,
  eImeOriya,
  eImePunjabi,
  eImeSanskrit,
  eImeSanthali,
  eImeSindhi,
  eImeTamil,
  eImeTelugu,
  eImeUrdu,
  eImeAfrikaans,
  eImeArabic,
  eImeChineseCn, // ChinesePinyin
  eImeDutch,
  eImeEnglishGb,
  eImeFrenchFr,
  eImeGerman,
  eImeHungarian,
  eImeIndonesian,
  eImeItalian,
  eImeMalay,
  eImePersian,
  eImePolish,
  eImePortugesePt,
  eImeRomanian,
  eImeRussian,
  eImeSpanishEs,
  eImeSwahili,
  eImeThai,
  eImeTurkish,
  eImeVietnamese,
  eImeZulu,
  eImeBengaliBd,
  eImeBulgarian,
  eImeCroatian,
  eImeCzech,
  eImeFinnish,
  eImeGreek,
  eImeKazakh,
  eImeKhmer,
  eImeMacedonian,
  eImeSerbian,
  eImeSinhala,
  eImeSlovak,
  eImeSlovenian,
  eImeSwedish,
  eImeTagalog,
  eImeUkrainian,
  eImeXhosa,
  eImeAlbanian,
  eImeArmenian,
  eImeAzerbaijani,
  eImeBelarusian,
  eImeBosnian,
  eImeChineseHk, // ChineseBihua
  eImeChineseTw, // ChineseZhuyin
  eImeDanish,
  eImeEstonian,
  eImeGeorgian,
  eImeHebrew,
  eImeIcelandic,
  eImeLao,
  eImeLatvian,
  eImeLithuanian,
  eImeNorwegian,
  eImeUzbek,
  eImeBasque,
  eImeGalician,
  eImeMalagasy,
  eImeYiddish,
  eImeKorean,
  eImeCatalan,
  eImeEnd
};

const string DICT_TABLE[eImeEnd] = {
  "",
  "English_US.dict",
  "French_CA.dict",
  "Portuguese_BR.dict",
  "Spanish_US.dict",
  "",
  "Bengali_IN.dict",
  "",
  "",
  "",
  "Hindi.dict",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "Tamil.dict",
  "Telugu.dict",
  "Urdu.dict",
  "Afrikaans.dict",
  "Arabic.dict",
  "Chinese_CN.dict", // Chinese_Pinyin
  "Dutch.dict",
  "English_GB.dict",
  "French_FR.dict",
  "German.dict",
  "Hungarian.dict",
  "Indonesian.dict",
  "Italian.dict",
  "Malay.dict",
  "Persian.dict",
  "Polish.dict",
  "Portuguese_PT.dict",
  "Romanian.dict",
  "Russian.dict",
  "Spanish_ES.dict",
  "Swahili.dict",
  "Thai.dict",
  "Turkish.dict",
  "Vietnamese.dict",
  "Zulu.dict",
  "Bengali_BD.dict",
  "Bulgarian.dict",
  "Croatian.dict",
  "Czech.dict",
  "Finnish.dict",
  "Greek.dict",
  "Kazakh.dict",
  "Khmer.dict",
  "Macedonian.dict",
  "Serbian.dict",
  "Sinhala.dict",
  "Slovak.dict",
  "Slovenian.dict",
  "Swedish.dict",
  "Tagalog.dict",
  "Ukrainian.dict",
  "Xhosa.dict",
  "Albanian.dict",
  "Armenian.dict",
  "Azerbaijani.dict",
  "Belarusian.dict",
  "Bosnian.dict",
  "Chinese_HK.dict", // Chinese_Bihua
  "Chinese_TW.dict", // Chinese_Zhuyin
  "Danish.dict",
  "Estonian.dict",
  "Georgian.dict",
  "Hebrew.dict",
  "Icelandic.dict",
  "Lao.dict",
  "Latvian.dict",
  "Lithuanian.dict",
  "Norwegian.dict",
  "Uzbek_Cyrillic.dict",
  "Basque.dict",
  "Galician.dict",
  "",
  "",
  "Korean.dict",
  "Catalan.dict"
};

enum ESelectionBarId {
  eSelectionBarStart = 0,
  eSelectionBarWord,
  eSelectionBarGroup,
  eSelectionBarEnd
};

enum EKeyboardId {
  eKeyboardStart  = 0,
  eKeyboardT9     = 1,
  eKeyboardQwerty = 2,
  eKeyboardEnd
};

enum EOptionId {
  eOptionKeyboardMode   = 0x1000,
  eOptionReportActivate = 0x1001
};

enum EInitStatus {
  eInitStatusSuccess      = 0,
  eInitStatusError        = 1,
  eInitStatusDictNotFound = 2
};

class IMEConnect final : public nsISupports, public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IMEConnect)

  static already_AddRefed<IMEConnect>
  Constructor(const GlobalObject& aGlobal, ErrorResult& aRv);

  static already_AddRefed<IMEConnect>
  Constructor(const GlobalObject& aGlobal, uint32_t aLid, ErrorResult& aRv);

  void GetCandidateWord(nsAString& aResult)
  {
    aResult = mCandidateWord;
  }

  uint16_t TotalWord() const
  {
    return mTotalWord;
  }

  uint32_t CurrentLID() const
  {
    return mCurrentLID;
  }

  void GetName(nsAString& aName) const
  {
    CopyASCIItoUTF16("Kika", aName);
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  nsCOMPtr<nsPIDOMWindowInner> mWindow;

  nsPIDOMWindowInner* GetOwner() const {
    return mWindow;
  }

  nsPIDOMWindowInner* GetParentObject()
  {
    return GetOwner();
  }

  explicit IMEConnect(nsPIDOMWindowInner* aWindow);

  nsresult Init(uint32_t aLid);

  static void SetLetter(const unsigned long aHexPrefix, const unsigned long aHexLetter, ErrorResult& aRv);
  static void SetLetterMultiTap(const unsigned long aKeyCode,const unsigned long aTapCount, unsigned short aPrevUnichar);
  static void GetNextWordCandidates(const nsAString& aWord, nsAString& aRetval);
  static void GetComposingWords(const nsAString& aLetters, const long aIndicator, nsAString& aRetval);
  static void ImportDictionary(Blob& aBlob, ErrorResult& aRv);
  static uint32_t SetLanguage(const uint32_t aLid);
  static nsString mCandidateWord;
  static uint16_t mTotalWord;
  static uint16_t mTotalGroup;
  static uint16_t mActiveWordIndex;
  static uint16_t mActiveGroupIndex;
  static uint8_t  mActiveSelectionBar;
  static uint32_t mCurrentLID;
  static uint8_t  mInitStatus;
  static vector<wchar_t> mKeyBuff;
  static CandidateCH mCandVoca;
  static CandidateCH mCandGroup;

private:
  ~IMEConnect();
};


} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_imeconnect_kika_IMEConnect_h

