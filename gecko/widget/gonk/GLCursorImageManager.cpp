/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/*
 * Copyright (c) 2016 Acadine Technologies. All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GLCursorImageManager.h"
#include "imgIContainer.h"
#include "mozilla/dom/AnonymousContent.h"
#include "nsDOMTokenList.h"
#include "nsIFrame.h"
#include "nsIWidgetListener.h"
#include "nsWindow.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;

const LayoutDeviceIntPoint
  GLCursorImageManager::kOffscreenCursorPosition = LayoutDeviceIntPoint(-1, -1);

namespace {

nsString
GetCursorElementClassID(nsCursor aCursor)
{
  nsString strClassID;
  switch (aCursor) {
  case eCursor_standard:
    strClassID = NS_LITERAL_STRING("std");
    break;
  case eCursor_wait:
    strClassID = NS_LITERAL_STRING("wait");
    break;
  case eCursor_select:
    strClassID = NS_LITERAL_STRING("select");
    break;
  case eCursor_hyperlink:
    strClassID = NS_LITERAL_STRING("link");
    break;
  case eCursor_vertical_text:
    strClassID = NS_LITERAL_STRING("vertical_text");
    break;
  case eCursor_spinning:
    strClassID = NS_LITERAL_STRING("spinning");
    break;
  default:
    strClassID = NS_LITERAL_STRING("std");
  }
  return strClassID;
}

nsCursor
MapCursorState(nsCursor aCursor)
{
  nsCursor mappedCursor = aCursor;
  switch (mappedCursor) {
  case eCursor_standard:
  case eCursor_wait:
  case eCursor_select:
  case eCursor_hyperlink:
  case eCursor_spinning:
  case eCursor_vertical_text:
    break;
  default:
    mappedCursor = eCursor_standard;
  }
  return mappedCursor;
}

} // namespace

class RemoveLoadCursorTaskOnMainThread final : public nsRunnable {
public:
  RemoveLoadCursorTaskOnMainThread(nsCursor aCursor,
                                   GLCursorImageManager* aManager)
      : mCursor(aCursor)
      , mManager(aManager)
  { }

  NS_IMETHOD Run() override {
    if (mManager) {
      mManager->RemoveCursorLoadRequest(mCursor);
    }

    // Kickoff composition to draw new loaded cursor.
    nsWindow::KickOffComposition();
    return NS_OK;
  }

private:
  nsCursor mCursor;
  GLCursorImageManager* mManager;
};

NS_IMPL_ISUPPORTS(GLCursorImageManager::LoadCursorTask, imgINotificationObserver)

GLCursorImageManager::LoadCursorTask::LoadCursorTask(
    nsCursor aCursor,
    nsIntPoint aHotspot,
    GLCursorImageManager* aManager)
  : mCursor(aCursor)
  , mHotspot(aHotspot)
  , mManager(aManager)
{
}

GLCursorImageManager::LoadCursorTask::~LoadCursorTask()
{
}

NS_IMETHODIMP
GLCursorImageManager::LoadCursorTask::Notify(imgIRequest* aProxy,
                                             int32_t aType,
                                             const nsIntRect* aRect)
{
  if (aType != imgINotificationObserver::DECODE_COMPLETE) {
    return NS_OK;
  }

  int32_t width, height;
  nsCOMPtr<imgIContainer> imgContainer;
  aProxy->GetImage(getter_AddRefs(imgContainer));
  imgContainer->GetWidth(&width);
  imgContainer->GetHeight(&height);

  GLCursorImage glCursorImage;
  glCursorImage.mCursor = mCursor;
  glCursorImage.mImgSize = nsIntSize(width, height);
  glCursorImage.mHotspot = mHotspot;

  RefPtr<mozilla::gfx::SourceSurface> sourceSurface =
    imgContainer->GetFrame(
    imgIContainer::FRAME_CURRENT,
    imgIContainer::FLAG_SYNC_DECODE);

  glCursorImage.mSurface = sourceSurface->GetDataSurface();

  if (mManager) {
    mManager->NotifyCursorImageLoadDone(mCursor, glCursorImage);
  }

  // This function is called through imgRequest and LoadCursorTask,
  // so we cannot remove them here.
  NS_DispatchToMainThread(
    new RemoveLoadCursorTaskOnMainThread(mCursor,
                                         mManager));

  return NS_OK;
}

GLCursorImageManager::GLCursorImageManager()
  : mGLCursorImageManagerMonitor("GLCursorImageManagerMonitor")
  , mHasSetCursor(false)
  , mGLCursorPos(kOffscreenCursorPosition)
{
}

GLCursorImageManager::GLCursorImage
GLCursorImageManager::GetGLCursorImage(nsCursor aCursor)
{
  nsCursor supportedCursor = MapCursorState(aCursor);
  ReentrantMonitorAutoEnter lock(mGLCursorImageManagerMonitor);
  if (!IsCursorImageReady(supportedCursor)) {
    return GLCursorImage();
  }

  return mGLCursorImageMap[supportedCursor];
}

bool
GLCursorImageManager::IsCursorImageReady(nsCursor aCursor)
{
  ReentrantMonitorAutoEnter lock(mGLCursorImageManagerMonitor);
  return mGLCursorImageMap.count(MapCursorState(aCursor));
}

bool
GLCursorImageManager::IsCursorImageLoading(nsCursor aCursor)
{
  ReentrantMonitorAutoEnter lock(mGLCursorImageManagerMonitor);
  return mGLCursorLoadingRequestMap.count(MapCursorState(aCursor));
}

void
GLCursorImageManager::NotifyCursorImageLoadDone(nsCursor aCursor,
                                                GLCursorImage &GLCursorImage)
{
  ReentrantMonitorAutoEnter lock(mGLCursorImageManagerMonitor);
  mGLCursorImageMap.insert(std::make_pair(aCursor, GLCursorImage));
}

void
GLCursorImageManager::PrepareCursorImage(nsCursor aCursor, nsWindow* aWindow)
{
  nsCursor supportedCursor = MapCursorState(aCursor);
  ReentrantMonitorAutoEnter lock(mGLCursorImageManagerMonitor);

  if (!aWindow ||
    IsCursorImageReady(supportedCursor) ||
    IsCursorImageLoading(supportedCursor)) {
    // Cursor is ready or in loading process.
    return;
  }

  // Create a new loading task for cursor.
  RefPtr<mozilla::dom::AnonymousContent> cursorElementHolder;
  nsIPresShell* presShell = aWindow->GetWidgetListener()->GetPresShell();
  if (presShell && presShell->GetDocument()) {
    nsIDocument* doc = presShell->GetDocument();

    // Insert new element to ensure restyle
    nsCOMPtr<dom::Element> image = doc->CreateHTMLElement(nsGkAtoms::div);
    ErrorResult rv;
    image->ClassList()->Add(NS_LITERAL_STRING("kaios-cursor"), rv);
    image->ClassList()->Add(GetCursorElementClassID(supportedCursor), rv);
    cursorElementHolder = doc->InsertAnonymousContent(*image, rv);

    if (cursorElementHolder) {
      nsCOMPtr<dom::Element> element =
        cursorElementHolder->GetContentNode();
      nsIFrame* frame = element->GetPrimaryFrame();
      if (!frame) {
        // Force the document to construct a primary frame immediately if
        // it hasn't constructed yet.
        doc->FlushPendingNotifications(Flush_Frames);
        frame = element->GetPrimaryFrame();
      }
      MOZ_ASSERT(frame);

      // Create an empty GLCursorLoadRequest.
      GLCursorLoadRequest &loadRequest =
        mGLCursorLoadingRequestMap[supportedCursor];
      const nsStyleUserInterface* ui = frame->StyleUserInterface();

      // Retrieve first cursor property from css.
      MOZ_ASSERT(ui->mCursorArrayLength > 0);
      nsCursorImage* item = ui->mCursorArray;
      nsIntPoint hotspot(item->mHotspotX, item->mHotspotY);
      loadRequest.mTask =
        new LoadCursorTask(supportedCursor, hotspot, this);

      item->GetImage()->Clone(loadRequest.mTask.get(),
                              getter_AddRefs(loadRequest.mRequest));

      // Ask decode after load complete.
      loadRequest.mRequest->StartDecoding();

      // Since we have cloned the imgIRequest, we can remove the element.
      doc->RemoveAnonymousContent(*cursorElementHolder, rv);
    }
  }
}

void
GLCursorImageManager::RemoveCursorLoadRequest(nsCursor aCursor)
{
  ReentrantMonitorAutoEnter lock(mGLCursorImageManagerMonitor);
  mGLCursorLoadingRequestMap.erase(aCursor);
}

void
GLCursorImageManager::HasSetCursor()
{
  ReentrantMonitorAutoEnter lock(mGLCursorImageManagerMonitor);
  mHasSetCursor = true;
}

void
GLCursorImageManager::SetGLCursorPosition(LayoutDeviceIntPoint aPosition)
{
  ReentrantMonitorAutoEnter lock(mGLCursorImageManagerMonitor);
  mGLCursorPos = aPosition;
}

LayoutDeviceIntPoint
GLCursorImageManager::GetGLCursorPosition()
{
  ReentrantMonitorAutoEnter lock(mGLCursorImageManagerMonitor);
  return mGLCursorPos;
}

bool
GLCursorImageManager::ShouldDrawGLCursor()
{
  ReentrantMonitorAutoEnter lock(mGLCursorImageManagerMonitor);
  return mHasSetCursor && mGLCursorPos != kOffscreenCursorPosition;
}
