/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#include "mozilla/dom/IMEConnect.h"
#include "mozilla/dom/IMEConnectBinding.h"

namespace mozilla {
namespace dom {

static demoEditorInfo rEditor = { nsCString(),0,0,0,0,0 }; //used for storing T9 editor info
static demoEditorInfo mEditor = { nsCString(),0,0,0,0,0 }; //used for storing T9 editor info when T9 not able to predict anything(T9 works as multitap)

unsigned short words_get[30][30]; // Used for holding predicted output
nsString IMEConnect::selectedWord; // Used to hold current selected word
nsString IMEConnect::prevSelectedWord; // Used to hold previous selected word.
unsigned short charList[21][5]; // used to hold multitap output
unsigned int multitap = 0; // 1: indicate prediction output is null, T9 is behaving like multitap.
unsigned int revmultitap = 0; // indiacate prediction is off, normal typing is on.
unsigned int multitapCount = 0; // no of character count in output of normal typing
nsString IMEConnect::mCandidateWord; //Stores the output
uint16_t IMEConnect::mTotalWord; //Stores the typed word
uint16_t IMEConnect::lenCandidateWord; // length of candidate word
uint16_t IMEConnect::lenselectedWord; // len of selected word
uint16_t IMEConnect::lenprevSelectedWord; // len of previously selected word
uint32_t IMEConnect::mCurrentLID; // current language Id


void EditorInitEmptyWord()
{
  rEditor.snCursorPos = 0;
  rEditor.snBufferLen = 0;
  rEditor.pageCount = 0;
  rEditor.pageCursor = 1;
  rEditor.snWordCount = 0;
  Clear_nsCString(rEditor.psBuffer);

  mEditor.snCursorPos = 0;
  mEditor.snBufferLen = 0;
  mEditor.pageCount = 0;
  mEditor.pageCursor = 1;
  mEditor.snWordCount = 0;
  Clear_nsCString(mEditor.psBuffer);

  Clear_nsString(IMEConnect::selectedWord);
  Clear_nsString(IMEConnect::prevSelectedWord);
  Clear_nsString(IMEConnect::mCandidateWord);
  ClearAllWordString();
  ClearCharList();

  IMEConnect::mTotalWord = 0;
  IMEConnect::lenCandidateWord = 0;
  IMEConnect::lenselectedWord = 0;
  IMEConnect::lenprevSelectedWord = 0;
  multitap =0;
}

void ClearEditorInfo(demoEditorInfo* const pEditor)
{
  pEditor->snCursorPos = 0;
  pEditor->snBufferLen = 0;
  pEditor->pageCount = 0;
  pEditor->pageCursor = 1;
  pEditor->snWordCount = 0;
  Clear_nsCString(pEditor->psBuffer);
}

void UpdateTotalWord()
{
  if (revmultitap == 1){
    IMEConnect::mTotalWord = multitapCount;
  }
  else if (multitap > 0) {
    IMEConnect::mTotalWord = mEditor.snWordCount;
  } else {
    IMEConnect::mTotalWord = rEditor.snWordCount;
  }
}

void ClearAllSymbsShort(unsigned short *buffer, int size)
{
  memset(buffer,0,size);
}

void ClearAllWordString()
{
  int i;
  for (i=0; i<30; i++) {
    memset(words_get[i],0,30);
  }
}

void ClearCharList()
{
  int i;
  for (i=0; i<21; i++) {
    memset(charList[i],0,5);
  }
}


void Clear_nsCString(nsCString& str)
{
  if (!str.IsEmpty()) {
    str.Truncate(0);
  }
}

void Clear_nsString(nsString& str)
{
  if (!str.IsEmpty()) {
    str.Truncate(0);
  }
}

void EditorMoveBackward(demoEditorInfo* const pEditor)
{
  if (pEditor->snCursorPos > 0) {
    --pEditor->snCursorPos;

    if (pEditor->pageCursor > 0) {
      pEditor->pageCursor--;
    } else if (pEditor->pageCursor == 0 && pEditor->pageCount > 0) {
      pEditor->pageCursor = 4;
      pEditor->pageCount--;
    }
    if (multitap) {
      UpdateCandidateWordMultitap(pEditor);
    } else {
      UpdateCandidateWord(pEditor);
    }

  } else if (pEditor->snCursorPos == 0 && pEditor->pageCursor == 1 && pEditor->pageCount == 0) {
    pEditor->pageCursor--;

    if (multitap) {
      UpdateCandidateWordMultitap(pEditor);
    } else {
      UpdateCandidateWord(pEditor);
    }
  } else {
    pEditor->snCursorPos = pEditor->snWordCount -1;
    pEditor->pageCursor = pEditor->snWordCount%5;
    pEditor->pageCount = pEditor->snWordCount/5;
    if (multitap) {
      UpdateCandidateWordMultitap(pEditor);
    } else {
      UpdateCandidateWord(pEditor);
    }
  }
}

void EditorMoveForward(demoEditorInfo* const pEditor)
{
  if (pEditor->snCursorPos < pEditor->snWordCount-1) {
    if (!(pEditor->snCursorPos == 0 && pEditor->pageCursor == 0 && pEditor->pageCount == 0)) {
      ++pEditor->snCursorPos;
    }
    if (pEditor->pageCursor >= 4) {
      pEditor->pageCursor = 0;
      pEditor->pageCount++;
    } else {
      pEditor->pageCursor++;
    }
    if (multitap) {
      UpdateCandidateWordMultitap(pEditor);
    } else {
      UpdateCandidateWord(pEditor);
    }
  } else if (pEditor->snCursorPos == 0 && pEditor->pageCursor == 0 && pEditor->snWordCount == 1) {
    pEditor->snCursorPos = 0;
    pEditor->pageCursor = 1;
    pEditor->pageCount = 0;
    if (multitap) {
      UpdateCandidateWordMultitap(pEditor);
    } else {
      UpdateCandidateWord(pEditor);
    }
  } else {
    pEditor->snCursorPos = 0;
    pEditor->pageCursor = 0;
    pEditor->pageCount = 0;
    if (multitap) {
      UpdateCandidateWordMultitap(pEditor);
    } else {
      UpdateCandidateWord(pEditor);
    }
  }
}

void UpdateCandidateWordMultitap(demoEditorInfo* const pEditor)
{
  int i,k, cli = 0;
  unsigned int wordCount = 0;
  int startIndex=0;
  char *start;
  Clear_nsString(IMEConnect::mCandidateWord);
  Clear_nsString(IMEConnect::selectedWord);
  if (pEditor->snWordCount > 0) {
    if (pEditor->pageCount == 0) {
      if (pEditor->pageCursor== 0) {
        IMEConnect::mCandidateWord.Append((PRUnichar)'>');
        start = (char*)rEditor.psBuffer.get();
        for (k=0; k<(int)rEditor.snBufferLen; k++,++start) {
          IMEConnect::mCandidateWord.Append((PRUnichar)*start);
          IMEConnect::selectedWord.Append((PRUnichar)*start);
        }
        start = (char*)pEditor->psBuffer.get();
        for (k=0; k<(int)pEditor->snBufferLen; k++,++start) {
          IMEConnect::mCandidateWord.Append((PRUnichar)*start);
          IMEConnect::selectedWord.Append((PRUnichar)*start);
        }
        IMEConnect::mCandidateWord.Append((PRUnichar)' ');
        wordCount++;
      } else {
        start = (char*)rEditor.psBuffer.get();
        for (k=0; k<(int)rEditor.snBufferLen; k++,++start) {
          IMEConnect::mCandidateWord.Append((PRUnichar)*start);
        }
        start = (char*)pEditor->psBuffer.get();
        for (k=0; k<(int)pEditor->snBufferLen; k++,++start) {
          IMEConnect::mCandidateWord.Append((PRUnichar)*start);
        }
        IMEConnect::mCandidateWord.Append((PRUnichar)' ');
        wordCount++;
      }
    }

    if (pEditor->pageCount == 0) {
      startIndex = 0;
    } else {
      startIndex = (pEditor->pageCount*5) - 1;
    }
    cli = 0;
    for (i=startIndex; i<(int)pEditor->snWordCount; i++) {
      if (pEditor->pageCursor == wordCount) {
        IMEConnect::mCandidateWord.Append((PRUnichar)'>');
        CopynsString(IMEConnect::prevSelectedWord,IMEConnect::mCandidateWord,IMEConnect::lenprevSelectedWord);
        CopynsString(IMEConnect::prevSelectedWord,IMEConnect::selectedWord,IMEConnect::lenprevSelectedWord);
        for (cli = 1; cli < charList[i][0]+1; cli++) {
          IMEConnect::mCandidateWord.Append((PRUnichar)charList[i][cli]);
          IMEConnect::selectedWord.Append((PRUnichar)charList[i][cli]);
        }
      } else {
        CopynsString(IMEConnect::prevSelectedWord,IMEConnect::mCandidateWord,IMEConnect::lenprevSelectedWord);
        for (cli = 1; cli < charList[i][0]+1; cli++) {
          IMEConnect::mCandidateWord.Append((PRUnichar)charList[i][cli]);
        }
      }
      IMEConnect::mCandidateWord.Append((PRUnichar)' ');
      wordCount++;
      if (wordCount == 5) {
        break;
      }
    }
    IMEConnect::lenCandidateWord = IMEConnect::mCandidateWord.Length();
    IMEConnect::lenselectedWord = IMEConnect::selectedWord.Length();
  }
}


void UpdateCandidateWordMultitapDelete(demoEditorInfo* const pEditor)
{
  int i,k, cli = 0, j;
  unsigned int wordCount = 0;
  int startIndex=0;
  char *start;
  Clear_nsString(IMEConnect::mCandidateWord);
  Clear_nsString(IMEConnect::selectedWord);
  int lenps = IMEConnect::lenprevSelectedWord;
  int found = 0;
  if (!IMEConnect::prevSelectedWord.IsEmpty()) {
    if (pEditor->snWordCount > 0) {
      for (i=0; i<(int)pEditor->snWordCount; i++) {
        lenps = IMEConnect::lenprevSelectedWord;
        if (charList[i][0] > 1) {
          for (j=charList[i][0];j>0 && j<=lenps && lenps>0; j--, lenps--) {
            if (IMEConnect::prevSelectedWord.CharAt(lenps-1) == (PRUnichar)charList[i][j]) {
              found = 1;
            } else {
              found = 0;
              break;
            }
          }
          if (found == 1) {
            IMEConnect::prevSelectedWord.Truncate(IMEConnect::prevSelectedWord.Length()-charList[i][0]);
            IMEConnect::lenprevSelectedWord = IMEConnect::prevSelectedWord.Length();
            break;
          }
        }
      }
      if (found == 0) {
        IMEConnect::prevSelectedWord.Truncate(IMEConnect::prevSelectedWord.Length()-1);
        IMEConnect::lenprevSelectedWord = IMEConnect::prevSelectedWord.Length();
      }
      if (pEditor->pageCount == 0) {
        if (pEditor->pageCursor== 0) {
          IMEConnect::mCandidateWord.Append((PRUnichar)'>');
          start = (char*)rEditor.psBuffer.get();
          for (k=0; k<(int)rEditor.snBufferLen; k++,++start) {
            IMEConnect::mCandidateWord.Append((PRUnichar)*start);
            IMEConnect::selectedWord.Append((PRUnichar)*start);
          }
          start = (char*)pEditor->psBuffer.get();
          for (k=0; k<(int)pEditor->snBufferLen; k++,++start) {
            IMEConnect::mCandidateWord.Append((PRUnichar)*start);
            IMEConnect::selectedWord.Append((PRUnichar)*start);
          }
          IMEConnect::mCandidateWord.Append((PRUnichar)' ');
          wordCount++;
        } else {
          start = (char*)rEditor.psBuffer.get();
          for (k=0; k<(int)rEditor.snBufferLen; k++,++start) {
            IMEConnect::mCandidateWord.Append((PRUnichar)*start);
          }
          start = (char*)pEditor->psBuffer.get();
          for (k=0; k<(int)pEditor->snBufferLen; k++,++start) {
            IMEConnect::mCandidateWord.Append((PRUnichar)*start);
          }
          IMEConnect::mCandidateWord.Append((PRUnichar)' ');
          wordCount++;
        }
      }
      if (pEditor->pageCount == 0) {
        startIndex = 0;
      } else {
        startIndex = (pEditor->pageCount*5) - 1;
      }
      cli = 0;
      for (i=startIndex; i<(int)pEditor->snWordCount; i++) {
        if (pEditor->pageCursor == wordCount) {
          IMEConnect::mCandidateWord.Append((PRUnichar)'>');
          CopynsString(IMEConnect::prevSelectedWord,IMEConnect::mCandidateWord,IMEConnect::lenprevSelectedWord);
          CopynsString(IMEConnect::prevSelectedWord,IMEConnect::selectedWord,IMEConnect::lenprevSelectedWord);
          for (cli = 1; cli < charList[i][0]+1; cli++) {
            IMEConnect::mCandidateWord.Append((PRUnichar)charList[i][cli]);
            IMEConnect::selectedWord.Append((PRUnichar)charList[i][cli]);
          }
        } else {
          CopynsString(IMEConnect::prevSelectedWord,IMEConnect::mCandidateWord,IMEConnect::lenprevSelectedWord);
          for (cli = 1; cli < charList[i][0]+1; cli++) {
            IMEConnect::mCandidateWord.Append((PRUnichar)charList[i][cli]);
          }
        }
        IMEConnect::mCandidateWord.Append((PRUnichar)' ');
        wordCount++;
        if (wordCount == 5) {
          break;
        }
      }
      IMEConnect::lenCandidateWord = IMEConnect::mCandidateWord.Length();
      IMEConnect::lenselectedWord = IMEConnect::selectedWord.Length();
    }
  }
}

void UpdateCharList(demoEditorInfo* const pEditor, char ch)
{
  int count,status;
  unsigned short prevUnichar = 0x0000;
  ClearCharList();
  if (!IMEConnect::prevSelectedWord.IsEmpty()) {
    prevUnichar = (unsigned short)IMEConnect::prevSelectedWord.Last();
  }
  getReverieCharList((ch-48),charList,&count,prevUnichar,&status);
  pEditor->snWordCount = count;
  pEditor->pageCount = 0;
  pEditor->pageCursor = 1;
  pEditor->snCursorPos = 0;
}

void UpdateCandidateWord(demoEditorInfo* const pEditor)
{
  int i,j,k;
  unsigned int wordCount = 0;
  int startIndex=0;
  char *start;
  Clear_nsString(IMEConnect::mCandidateWord);
  Clear_nsString(IMEConnect::selectedWord);
  if (pEditor->snWordCount > 0) {
    if (pEditor->pageCount == 0) {
      if (pEditor->pageCursor == 0) {
        IMEConnect::mCandidateWord.Append((PRUnichar)'>');
        start = (char*)pEditor->psBuffer.get();
        for (k=0; k<(int)pEditor->snBufferLen; k++,++start) {
          IMEConnect::mCandidateWord.Append((PRUnichar)*start);
          IMEConnect::selectedWord.Append((PRUnichar)*start);
        }
        IMEConnect::mCandidateWord.Append((PRUnichar)' ');
        wordCount++;
      } else {
        start = (char*)pEditor->psBuffer.get();
        for (k=0; k<(int)pEditor->snBufferLen; k++,++start) {
          IMEConnect::mCandidateWord.Append((PRUnichar)*start);
        }
        IMEConnect::mCandidateWord.Append((PRUnichar)' ');
        wordCount++;
      }
    }

    if (pEditor->pageCount == 0) {
      startIndex = 0;
    } else {
      startIndex = (pEditor->pageCount*5) - 1;
    }

    for (i=startIndex; i < (int)pEditor->snWordCount; i++) {
      j = 0;
      if (pEditor->pageCursor == wordCount) {
        IMEConnect::mCandidateWord.Append((PRUnichar)'>');
        while (words_get[i][j]) {
          IMEConnect::mCandidateWord.Append((PRUnichar) words_get[i][j]);
          IMEConnect::selectedWord.Append((PRUnichar) words_get[i][j++]);
        }
      } else {
        while (words_get[i][j]) {
          IMEConnect::mCandidateWord.Append((PRUnichar) words_get[i][j++]);
        }
      }
      IMEConnect::mCandidateWord.Append((PRUnichar)' ');
      wordCount++;
      if (wordCount == 5) {
        break;
      }
    }
  }
  IMEConnect::lenCandidateWord = IMEConnect::mCandidateWord.Length();
  IMEConnect::lenselectedWord = IMEConnect::selectedWord.Length();
}

int Updatesymbol(demoEditorInfo* const pEditor,const unsigned char ch)
{
  pEditor->psBuffer.Append(ch);
  pEditor->snBufferLen++;
  return 0;
}

int DeleteSymbols(demoEditorInfo* const pEditor)
{
  if (pEditor->snBufferLen > 0) {
    pEditor->psBuffer.Truncate(pEditor->snBufferLen-1);
    pEditor->snBufferLen--;
  } else {
    return (-1);
  }
  return 0;
}

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
IMEConnect::Constructor(const GlobalObject& aGlobal,
                        ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsPIDOMWindowInner> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<IMEConnect> rt_object = new IMEConnect(window);
  aRv = rt_object->Init(English);

  return rt_object.forget();
}

already_AddRefed<IMEConnect>
IMEConnect::Constructor(const GlobalObject& aGlobal,
                        uint32_t aLID,
                        ErrorResult& aRv)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsPIDOMWindowInner> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  RefPtr<IMEConnect> rt_object = new IMEConnect(window);
  aRv = rt_object->Init(aLID);

  return rt_object.forget();
}

nsresult
IMEConnect::Init(uint32_t aLID)
{
  return NS_OK;
}
void IMEConnect::SetLetterMultiTap(const unsigned long keyCode,const unsigned long tapCount, unsigned short prevUnichar)
{
  int count, status, i, cli = 0;
  int tCount = 0;
  reverieGetLanguage(&status);
  if(status == -1){
    RT9_LOGE("RT9::IMEConnect::SetLetterMultiTap::Invalid Language Identifier");
    return;
  }

  Clear_nsString(IMEConnect::mCandidateWord);
  ClearCharList();
  revmultitap = 1;
  multitap = 0;
  if (keyCode >= 49 && keyCode <= 57) {
    getReverieCharListMulti_tap((keyCode-48),charList,&count,prevUnichar,&status);
  } else if(keyCode == 58) {
    getReverieCharListMulti_tap(0,charList,&count,prevUnichar,&status);
  } else {
    EditorInitEmptyWord();
    return;
  }
  multitapCount = count;
  tCount = tapCount%count;
  if (tCount == 0) {
    tCount = count - 1;
  } else {
    tCount = tCount - 1;
  }
  for (i = 0; i < count; i++) {
    if (i == tCount) {
      IMEConnect::mCandidateWord.Append((PRUnichar)'>');
    }
    for (cli = 1; cli < charList[i][0]+1; cli++) {
      IMEConnect::mCandidateWord.Append((PRUnichar)charList[i][cli]);
    }
    IMEConnect::mCandidateWord.Append((PRUnichar)' ');
  }
  IMEConnect::lenCandidateWord = IMEConnect::mCandidateWord.Length();
}


void IMEConnect::SetLetter(const unsigned long aHexPrefix, const unsigned long aHexLetter, ErrorResult& aRv)
{
  int Status,count,i;
  char mlast;
  PRUnichar* start;
  unsigned short period;

  reverieGetLanguage(&Status);
  if(Status == -1){
    RT9_LOGE("RT9::IMEConnect::SetLetter::Invalid Language Identifier");
    return;
  }

  revmultitap = 0;
  if (aHexLetter == 0x1B) {
    return;
  }

  if (aHexPrefix == 224) {
    switch (aHexLetter) {
      case 72: // left arrow -
        if (mEditor.snBufferLen <= 0) {
          EditorMoveBackward(&rEditor);
        } else {
          EditorMoveBackward(&mEditor);
        }
        break;
      case 80: // right arrow -
        if (mEditor.snBufferLen <= 0) {
          EditorMoveForward(&rEditor);
        } else {
          EditorMoveForward(&mEditor);
        }
        break;
      default:
        break;
    }
  } else {
    switch (aHexLetter) {
      case 8: // delete(back space)
        if (mEditor.snBufferLen <= 0) {
          DeleteSymbols(&rEditor);
          reverieGetPredictedWords((unsigned char*)rEditor.psBuffer.get(),
                                   rEditor.snBufferLen,
                                   words_get,
                                   &count,
                                   &Status,0);

          rEditor.snWordCount = count;
          rEditor.pageCount = 0;
          rEditor.pageCursor = 1;
          rEditor.snCursorPos = 0;
          UpdateCandidateWord(&rEditor);
        } else if(mEditor.snBufferLen == 1) {
          ClearEditorInfo(&mEditor);
          ClearCharList();
          Clear_nsString(IMEConnect::prevSelectedWord);
          IMEConnect::lenprevSelectedWord = 0;
          multitap = 0;
          rEditor.pageCount = 0;
          rEditor.pageCursor = 1;
          rEditor.snCursorPos = 0;
          UpdateCandidateWord(&rEditor);
        } else {
          DeleteSymbols(&mEditor);
          mlast = (char)mEditor.psBuffer.Last();
          UpdateCharList(&mEditor,mlast);
          UpdateCandidateWordMultitapDelete(&mEditor);
        }
        break;
      case 13: //enter
        EditorInitEmptyWord();
        break;
      case 49:
        if (mEditor.snBufferLen <= 0) {
          Updatesymbol(&rEditor,'1');
          reverieGetPredictedWords((unsigned char*)rEditor.psBuffer.get(),
                                   rEditor.snBufferLen,
                                   words_get,
                                   &count,
                                   &Status,1);
          if (count > 1) {
            //changes to add period sign when 1 is pressed
            start = (PRUnichar*)IMEConnect::selectedWord.get();
            if (lenselectedWord<28) {
              for(i=0;i<IMEConnect::lenselectedWord;i++) {
                words_get[0][i] = (unsigned short)start[i];
              }
              period = getReveriePeriodChar((LanguageIndex)IMEConnect::mCurrentLID);
              words_get[0][i++] = period;
              words_get[0][i++] = 0;
            }
            rEditor.snWordCount = count;
            rEditor.pageCount = 0;
            rEditor.pageCursor = 1;
            rEditor.snCursorPos = 0;
            UpdateCandidateWord(&rEditor);
          } else {
            DeleteSymbols(&rEditor);
            Clear_nsString(IMEConnect::prevSelectedWord);
            CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
            IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
            multitap = 1;
            Updatesymbol(&mEditor,'1');
            UpdateCharList(&mEditor,'1');
            UpdateCandidateWordMultitap(&mEditor);
          }
        } else {
          Clear_nsString(IMEConnect::prevSelectedWord);
          CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
          IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
          multitap = 1;
          Updatesymbol(&mEditor,'1');
          UpdateCharList(&mEditor,'1');
          UpdateCandidateWordMultitap(&mEditor);
        }
        break;
      case 50:
        if (mEditor.snBufferLen <= 0) {
          Updatesymbol(&rEditor,'2');
          reverieGetPredictedWords((unsigned char*)rEditor.psBuffer.get(),
                                   rEditor.snBufferLen,
                                   words_get,
                                   &count,
                                   &Status,0);
          if (count > 0) {
            rEditor.snWordCount = count;
            rEditor.pageCount = 0;
            rEditor.pageCursor = 1;
            rEditor.snCursorPos = 0;
            UpdateCandidateWord(&rEditor);
          } else {
            DeleteSymbols(&rEditor);
            Clear_nsString(IMEConnect::prevSelectedWord);
            CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
            IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
            multitap = 1;
            Updatesymbol(&mEditor,'2');
            UpdateCharList(&mEditor,'2');
            UpdateCandidateWordMultitap(&mEditor);
          }
        } else {
          Clear_nsString(IMEConnect::prevSelectedWord);
          CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
          IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
          multitap = 1;
          Updatesymbol(&mEditor,'2');
          UpdateCharList(&mEditor,'2');
          UpdateCandidateWordMultitap(&mEditor);
        }
        break;
      case 51:
        if (mEditor.snBufferLen <= 0) {
          Updatesymbol(&rEditor,'3');
          reverieGetPredictedWords((unsigned char*)rEditor.psBuffer.get(),
                                   rEditor.snBufferLen,
                                   words_get,
                                   &count,
                                   &Status,0);
          if (count > 0) {
            rEditor.snWordCount = count;
            rEditor.pageCount = 0;
            rEditor.pageCursor = 1;
            rEditor.snCursorPos = 0;
            UpdateCandidateWord(&rEditor);
          } else {
            DeleteSymbols(&rEditor);
            Clear_nsString(IMEConnect::prevSelectedWord);
            CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
            IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
            multitap = 1;
            Updatesymbol(&mEditor,'3');
            UpdateCharList(&mEditor,'3');
            UpdateCandidateWordMultitap(&mEditor);
          }
        } else {
          Clear_nsString(IMEConnect::prevSelectedWord);
          CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
          IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
          multitap = 1;
          Updatesymbol(&mEditor,'3');
          UpdateCharList(&mEditor,'3');
          UpdateCandidateWordMultitap(&mEditor);
        }
        break;
      case 52:
        if (mEditor.snBufferLen <= 0) {
          Updatesymbol(&rEditor,'4');
          reverieGetPredictedWords((unsigned char*)rEditor.psBuffer.get(),
                                   rEditor.snBufferLen,
                                   words_get,
                                   &count,
                                   &Status,0);
          if (count > 0) {
            rEditor.snWordCount = count;
            rEditor.pageCount = 0;
            rEditor.pageCursor = 1;
            rEditor.snCursorPos = 0;
            UpdateCandidateWord(&rEditor);
          } else {
            DeleteSymbols(&rEditor);
            Clear_nsString(IMEConnect::prevSelectedWord);
            CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
            IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
            multitap = 1;
            Updatesymbol(&mEditor,'4');
            UpdateCharList(&mEditor,'4');
            UpdateCandidateWordMultitap(&mEditor);
          }
        } else {
          Clear_nsString(IMEConnect::prevSelectedWord);
          CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
          IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
          multitap = 1;
          Updatesymbol(&mEditor,'4');
          UpdateCharList(&mEditor,'4');
          UpdateCandidateWordMultitap(&mEditor);
        }
        break;
      case 53:
        if (mEditor.snBufferLen <= 0) {
          Updatesymbol(&rEditor,'5');
          reverieGetPredictedWords((unsigned char*)rEditor.psBuffer.get(),
                                   rEditor.snBufferLen,
                                   words_get,
                                   &count,
                                   &Status,0);
          if (count > 0) {
            rEditor.snWordCount = count;
            rEditor.pageCount = 0;
            rEditor.pageCursor = 1;
            rEditor.snCursorPos = 0;
            UpdateCandidateWord(&rEditor);
          } else {
            DeleteSymbols(&rEditor);
            Clear_nsString(IMEConnect::prevSelectedWord);
            CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
            IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
            multitap = 1;
            Updatesymbol(&mEditor,'5');
            UpdateCharList(&mEditor,'5');
            UpdateCandidateWordMultitap(&mEditor);
          }
        } else {
          Clear_nsString(IMEConnect::prevSelectedWord);
          CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
          IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
          multitap = 1;
          Updatesymbol(&mEditor,'5');
          UpdateCharList(&mEditor,'5');
          UpdateCandidateWordMultitap(&mEditor);
        }
        break;
      case 54:
        if (mEditor.snBufferLen <= 0) {
          Updatesymbol(&rEditor,'6');
          reverieGetPredictedWords((unsigned char*)rEditor.psBuffer.get(),
                                   rEditor.snBufferLen,
                                   words_get,
                                   &count,
                                   &Status,0);
          if (count > 0) {
            rEditor.snWordCount = count;
            rEditor.pageCount = 0;
            rEditor.pageCursor = 1;
            rEditor.snCursorPos = 0;
            UpdateCandidateWord(&rEditor);
          } else {
            DeleteSymbols(&rEditor);
            Clear_nsString(IMEConnect::prevSelectedWord);
            CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
            IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
            multitap = 1;
            Updatesymbol(&mEditor,'6');
            UpdateCharList(&mEditor,'6');
            UpdateCandidateWordMultitap(&mEditor);
          }
        } else {
          Clear_nsString(IMEConnect::prevSelectedWord);
          CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
          IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
          multitap = 1;
          Updatesymbol(&mEditor,'6');
          UpdateCharList(&mEditor,'6');
          UpdateCandidateWordMultitap(&mEditor);
        }
        break;
      case 55:
        if (mEditor.snBufferLen <= 0) {
          Updatesymbol(&rEditor,'7');
          reverieGetPredictedWords((unsigned char*)rEditor.psBuffer.get(),
                                   rEditor.snBufferLen,
                                   words_get,
                                   &count,
                                   &Status,0);
          if (count > 0) {
            rEditor.snWordCount = count;
            rEditor.pageCount = 0;
            rEditor.pageCursor = 1;
            rEditor.snCursorPos = 0;
            UpdateCandidateWord(&rEditor);
          } else {
            DeleteSymbols(&rEditor);
            Clear_nsString(IMEConnect::prevSelectedWord);
            CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
            IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
            multitap = 1;
            Updatesymbol(&mEditor,'7');
            UpdateCharList(&mEditor,'7');
            UpdateCandidateWordMultitap(&mEditor);
          }
        } else {
          Clear_nsString(IMEConnect::prevSelectedWord);
          CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
          IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
          multitap = 1;
          Updatesymbol(&mEditor,'7');
          UpdateCharList(&mEditor,'7');
          UpdateCandidateWordMultitap(&mEditor);
        }
        break;
      case 56:
        if (mEditor.snBufferLen <= 0) {
          Updatesymbol(&rEditor,'8');
          reverieGetPredictedWords((unsigned char*)rEditor.psBuffer.get(),
                                   rEditor.snBufferLen,
                                   words_get,
                                   &count,
                                   &Status,0);
          if (count > 0) {
            rEditor.snWordCount = count;
            rEditor.pageCount = 0;
            rEditor.pageCursor = 1;
            rEditor.snCursorPos = 0;
            UpdateCandidateWord(&rEditor);
          } else {
            DeleteSymbols(&rEditor);
            Clear_nsString(IMEConnect::prevSelectedWord);
            CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
            IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
            multitap = 1;
            Updatesymbol(&mEditor,'8');
            UpdateCharList(&mEditor,'8');
            UpdateCandidateWordMultitap(&mEditor);
          }
        } else {
          Clear_nsString(IMEConnect::prevSelectedWord);
          CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
          IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
          multitap = 1;
          Updatesymbol(&mEditor,'8');
          UpdateCharList(&mEditor,'8');
          UpdateCandidateWordMultitap(&mEditor);
        }
        break;
      case 57:
        if (mEditor.snBufferLen <= 0) {
          Updatesymbol(&rEditor,'9');
          reverieGetPredictedWords((unsigned char*)rEditor.psBuffer.get(),
                                   rEditor.snBufferLen,
                                   words_get,
                                   &count,
                                   &Status,0);
          if (count > 0) {
            rEditor.snWordCount = count;
            rEditor.pageCount = 0;
            rEditor.pageCursor = 1;
            rEditor.snCursorPos = 0;
            UpdateCandidateWord(&rEditor);
          } else {
            DeleteSymbols(&rEditor);
            Clear_nsString(IMEConnect::prevSelectedWord);
            CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
            IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
            multitap = 1;
            Updatesymbol(&mEditor,'9');
            UpdateCharList(&mEditor,'9');
            UpdateCandidateWordMultitap(&mEditor);
          }
        } else {
          Clear_nsString(IMEConnect::prevSelectedWord);
          CopynsString(IMEConnect::selectedWord,IMEConnect::prevSelectedWord,IMEConnect::lenselectedWord);
          IMEConnect::lenprevSelectedWord = IMEConnect::lenselectedWord;
          multitap = 1;
          Updatesymbol(&mEditor,'9');
          UpdateCharList(&mEditor,'9');
          UpdateCandidateWordMultitap(&mEditor);
        }
        break;
      case 58:
        EditorInitEmptyWord();
        break;
    }
  }
}

uint32_t IMEConnect::SetLanguage(const uint32_t lid)
{
  int status;
  reverieSetLanguage(lid,&status);
  reverieSetHalfWord(1);
  if(status == 0) {
    IMEConnect::mCurrentLID = lid;
    EditorInitEmptyWord();
    return 0;
  } else {
    RT9_LOGE("RT9::Language_load error: [%d], aRt9LID = 0x%x", status, lid);
    return 2;
  }
}

void CopyRUTF16toUTF16(nsString& source, nsAString& aResult)
{
  int i;
  PRUnichar* start =(PRUnichar*) source.get();
  for (i=0; i<IMEConnect::lenCandidateWord; i++) {
    aResult.Append((char16_t)start[i]);
  }
}

void CopynsString(nsString& source, nsString& dest, uint16_t length)
{
  int i;
  PRUnichar* start = (PRUnichar*)source.get();
  for(i=0;i<length;i++)
  {
    dest.Append((PRUnichar)start[i]);
  }
}

JSObject* IMEConnect::WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto)
{
  return mozilla::dom::IMEConnectBinding::Wrap(aCx, this, aGivenProto);
}

IMEConnect::~IMEConnect()
{
  RT9_LOGD("RT9::Inside IMEConnect destructor");
}
} // namespace dom
} // namespace mozilla
