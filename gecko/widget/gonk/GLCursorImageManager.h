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

#ifndef GLCursorImageManager_h
#define GLCursorImageManager_h

#include <map>

#include "imgIRequest.h"
#include "imgINotificationObserver.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsWindow.h"

// Managers asynchronous loading of gl cursor image.
class GLCursorImageManager {
public:
  struct GLCursorImage {
    nsCursor mCursor;
    nsIntSize mImgSize;
    nsIntPoint mHotspot;
    RefPtr<mozilla::gfx::DataSourceSurface> mSurface;
  };

  GLCursorImageManager();

  // Called by nsWindow.
  // Prepare asynchronous load task for cursor if it doesn't exist.
  void PrepareCursorImage(nsCursor aCursor, nsWindow* aWindow);
  bool IsCursorImageReady(nsCursor aCursor);
  // Get the GLCursorImage corresponding to a cursor, should be called after
  // checking by IsCursorImageReady().
  GLCursorImage GetGLCursorImage(nsCursor aCursor);

  // Called by nsWindow::SetCursor, on the main thread.
  void HasSetCursor();

  // Called by nsWindow::DispatchEvent, on the main thread.
  void SetGLCursorPosition(mozilla::LayoutDeviceIntPoint aPosition);

  // Called by nsWindow::DrawWindowOverlay, on the compositor thread.
  mozilla::LayoutDeviceIntPoint GetGLCursorPosition();

  // Called by nsWindow::DrawWindowOverlay, on the compositor thread.
  bool ShouldDrawGLCursor();

  // Called back by LoadCursorTask.
  void NotifyCursorImageLoadDone(nsCursor aCursor,
                                 GLCursorImage &GLCursorImage);
  // Called back by RemoveLoadCursorTaskOnMainThread.
  void RemoveCursorLoadRequest(nsCursor aCursor);

  static const mozilla::LayoutDeviceIntPoint kOffscreenCursorPosition;

private:
  bool IsCursorImageLoading(nsCursor aCursor);

  class LoadCursorTask final : public imgINotificationObserver {
  public:
    NS_DECL_ISUPPORTS

    LoadCursorTask(nsCursor aCursor,
                   nsIntPoint aHotspot,
                   GLCursorImageManager* aManager);

    // This callback function will be called on main thread and notify the
    // status of image decoding process.
    NS_IMETHODIMP Notify(imgIRequest* aProxy,
                         int32_t aType,
                         const nsIntRect* aRect) override;
  private:
    ~LoadCursorTask();
    nsCursor mCursor;
    nsIntPoint mHotspot;
    GLCursorImageManager* mManager;
  };

  struct GLCursorLoadRequest {
    RefPtr<imgIRequest> mRequest;
    RefPtr<LoadCursorTask> mTask;
  };

  // Locks against both mGLCursorImageMap and mGLCursorLoadingRequestMap.
  // Also locks against mRecvSetCursor and mGLCursorPos.
  mozilla::ReentrantMonitor mGLCursorImageManagerMonitor;

  // Store cursors which are in loading process.
  std::map<nsCursor, GLCursorLoadRequest> mGLCursorLoadingRequestMap;

  // Store cursor images which are ready to render.
  std::map<nsCursor, GLCursorImage> mGLCursorImageMap;

  bool mHasSetCursor; // True if we received a SetCursor event.

  mozilla::LayoutDeviceIntPoint mGLCursorPos; // Save the current cursor position.
};

#endif /* GLCursorImageManager_h */
