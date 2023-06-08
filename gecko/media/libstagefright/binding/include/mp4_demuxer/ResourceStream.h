/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef RESOURCESTREAM_H_
#define RESOURCESTREAM_H_

#if defined(MOZ_WIDGET_GONK) && (ANDROID_VERSION == 19)
#include "MediaResource.h"
#else
#include "mozilla/MediaResource.h"
#endif
#include "mp4_demuxer/Stream.h"
#include "mozilla/RefPtr.h"

namespace mp4_demuxer
{

class ResourceStream : public Stream
{
public:
  explicit ResourceStream(mozilla::MediaResource* aResource);

  virtual bool ReadAt(int64_t offset, void* aBuffer, size_t aCount,
                      size_t* aBytesRead) override;
  virtual bool CachedReadAt(int64_t aOffset, void* aBuffer, size_t aCount,
                            size_t* aBytesRead) override;
  virtual bool Length(int64_t* size) override;

  void Pin()
  {
    mResource->Pin();
    ++mPinCount;
  }

  void Unpin()
  {
    mResource->Unpin();
    MOZ_ASSERT(mPinCount);
    --mPinCount;
  }

protected:
  virtual ~ResourceStream();

private:
  RefPtr<mozilla::MediaResource> mResource;
  uint32_t mPinCount;
};

}

#endif // RESOURCESTREAM_H_
