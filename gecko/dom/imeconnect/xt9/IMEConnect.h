/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_imeconnect_xt9_IMEConnect_h
#define mozilla_dom_imeconnect_xt9_IMEConnect_h

#include "nsCOMPtr.h"
#include "nsPIDOMWindow.h"
#include "nsISupports.h"
#include "nsWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "MainThreadUtils.h"

extern "C" {
#include "et9api.h"
}

#include "l0609b00.h" // English US KDB HPD
#include "l1109b00.h" // English US LDB
#include "l0612b00.h" // French KDB HPD
#include "l1212b00.h" // French LDB HPD
#include "l0610b00.h" // Spanish KDB HPD
#include "l0810b00.h" // Spanish LDB HPD

#include "l09255b00.h"

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
#define LOG_XT9_TAG "GeckoXt9Connect"
#define XT9_DEBUG 0

#define XT9_LOGW(args...)  __android_log_print(ANDROID_LOG_WARN, LOG_XT9_TAG , ## args)
#define XT9_LOGE(args...)  __android_log_print(ANDROID_LOG_ERROR, LOG_XT9_TAG , ## args)

#if XT9_DEBUG
#define XT9_LOGD(args...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_XT9_TAG , ## args)
#else
#define XT9_LOGD(args...)
#endif

#else

#define XT9_LOGD(args...)
#define XT9_LOGW(args...)
#define XT9_LOGE(args...)

#endif

#if ET9COREVERSIONNUM == 0x090A0B00
#define ET9LIDEnglish_US                ((ET9U32)0x0B09)
#endif

#ifndef ET9_XML_KEYBOARD_LAYOUT
#define ET9_XML_KEYBOARD_LAYOUT NULL
#endif

namespace mozilla {
namespace dom {

static const ET9SYMB psEmptyString[] = { 0 };
static const unsigned char pcMapKeyHQR[] =
{ 12, 25, 23, 14, 4, 15, 16, 17, 9, 18, 19, 20, 27, 26, 10, 11, 2, 5, 13, 6, 8, 24, 3, 22, 7, 21 };
static const unsigned char pcMapKeyHPD[] =
{ 9, 0, 1, 2, 3, 4, 5, 6, 7, 8 };

static const ET9U32 GENERIC_HQR        = ET9PLIDNull | ET9SKIDATQwertyReg;
static const ET9U32 ENGLISH_HPD        = ET9PLIDEnglish | ET9SKIDPhonePad;
static const ET9U32 FRENCH_HPD         = ET9PLIDFrench  | ET9SKIDPhonePad;
static const ET9U32 SPANISH_HPD        = ET9PLIDSpanish  | ET9SKIDPhonePad;
static const ET9U16 SEL_LIST_SIZE      = 32;
static const ET9U32 ASDB_SIZE          = (10 * 1024);
static const ET9U32 BUFFER_LEN_MAX     = 2000;
static const ET9U16 ET9XmlLayoutWidth  = 0;
static const ET9U16 ET9XmlLayoutHeight = 0;
static const ET9U16 MT_TIMEOUT_TIME    = 400;

typedef uint32_t DWORD;

typedef enum { I_HQR, I_HQD, I_HPD, I_EXP } INPUTMODE;

typedef struct {
  ET9SYMB             psBuffer[BUFFER_LEN_MAX];
  ET9INT              snBufferLen;
  ET9INT              snCursorPos;
} demoEditorInfo;

typedef struct {
  ET9AWLingInfo       sLingInfo;
  ET9AWLingCmnInfo    sLingCmnInfo;

  ET9WordSymbInfo     sWordSymbInfo;

  ET9KDBInfo          sKdbInfo;

  ET9U32              dwDLMSize;
  _ET9DLM_info        *pDLMInfo;

  FILE                *pfEventFile;

  ET9U8               pbASdb[ASDB_SIZE];

  ET9U8               bTotWords;
  ET9U8               bActiveWordIndex;

  ET9U16              wGestureValue;

  INPUTMODE           eInputMode;
  ET9BOOL             bSupressSubstitutions;
  ET9U8               bLastKeyMT;
  DWORD               dwMultitapStartTime;

  demoEditorInfo      *pEditor;

  ET9SimpleWord       sExplicitLearningWord;
} demoIMEInfo;

uint32_t GetTickCount();

void EditorInitEmptyWord(demoIMEInfo *pIME, bool& initEmptyWord);

void EditorInitWord(demoIMEInfo *pIME, nsString& initWord);

void EditorSetCursor(demoIMEInfo * const pIME, const int32_t& initCursor, bool& enabledCursor);

void EditorInsertWord(demoIMEInfo * const pIME, ET9AWWordInfo *pWord, ET9BOOL bSupressSubstitutions);

void EditorGetWord(demoIMEInfo * const pIME, ET9SimpleWord * const pWord, const ET9BOOL bCut);

void EditorDeleteChar(demoIMEInfo * const pIME);

void EditorMoveBackward(demoIMEInfo * const pIME);

void EditorMoveForward(demoIMEInfo * const pIME);

void AcceptActiveWord(demoIMEInfo * const pIME, const ET9BOOL bAddSpace);

ET9STATUS ET9FARCALL ET9KDBLoad(ET9KDBInfo    * const pKdbInfo,
                                const ET9U32          dwKdbNum,
                                const ET9U16          wPageNum);

ET9STATUS ET9FARCALL ET9AWLdbReadData(ET9AWLingInfo *pLingInfo,
                                      ET9U8         * ET9FARDATA *ppbSrc,
                                      ET9U32        *pdwSizeInBytes);

ET9STATUS ET9Handle_KDB_Request(ET9KDBInfo      * const pKdbInfo,
                                ET9WordSymbInfo * const pWordSymbInfo,
                                ET9KDB_Request  * const pRequest);

ET9STATUS ET9Handle_DLM_Request(void                    * const pHandlerInfo,
                                ET9AWDLM_RequestInfo    * const pRequest);

ET9U8 CharLookup(demoIMEInfo     * const pIME,
                 const int               nChar);

void PrintLanguage(const ET9U32 dwLanguage);

void PrintState(demoIMEInfo *pIME);

void PrintExplicitLearning(demoIMEInfo *pIME);

void PrintCandidateList(demoIMEInfo *pIME);

void PrintEditorBuffer(demoEditorInfo *pEditor);

void PrintScreen(demoIMEInfo *pIME);

class IMEConnect final : public nsISupports, public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IMEConnect)

  static already_AddRefed<IMEConnect>
  Constructor(const GlobalObject& aGlobal,
              ErrorResult& aRv);

  static already_AddRefed<IMEConnect>
  Constructor(const GlobalObject& aGlobal,
              uint32_t aLID,
              ErrorResult& aRv);

  bool InitEmptyWord() const
  {
    return mEmptyWord;
  }

  void SetInitEmptyWord(const bool aResult)
  {
    mEmptyWord = aResult;
  }

  void GetWholeWord(nsAString& aResult)
  {
    aResult = mWholeWord;
    mWholeWord.AssignLiteral("");
  }

  void SetWholeWord(const nsAString& aResult)
  {
    sWholeWord = aResult;
  }

  void GetCandidateWord(nsAString& aResult)
  {
    aResult = mCandidateWord;
  }

  uint16_t TotalWord() const
  {
    return mTotalWord;
  }

  uint32_t CursorPosition() const
  {
    return mCursorPosition;
  }

  void SetCursorPosition(const uint32_t aResult)
  {
    sCursorPosition = aResult;
    cursorPositionEnabled = true;
  }

  uint32_t CurrentLID() const
  {
    return mCurrentLID;
  }

  void GetName(nsAString& aName) const
  {
    CopyASCIItoUTF16("XT9", aName);
  }

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  nsCOMPtr<nsPIDOMWindowInner> mWindow;
  nsPIDOMWindowInner* GetOwner() const { return mWindow; }

  nsPIDOMWindowInner* GetParentObject()
  {
    return GetOwner();
  }

  explicit IMEConnect(nsPIDOMWindowInner* aWindow);

  nsresult Init(uint32_t aLID);

  static void SetLetter(const unsigned long aHexPrefix, const unsigned long aHexLetter, ErrorResult& aRv);
  static uint32_t SetLanguage(const uint32_t lid);

  static bool mEmptyWord;
  static nsString mWholeWord;
  static nsString sWholeWord;
  static nsString mCandidateWord;
  static uint16_t  mTotalWord;
  static uint32_t  mCursorPosition;
  static uint32_t  sCursorPosition;
  static bool cursorPositionEnabled;
  static uint32_t mCurrentLID;

private:
  ~IMEConnect();
};
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_imeconnect_xt9_IMEConnect_h
