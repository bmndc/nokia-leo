/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=2 et tw=80 : */

#include <sys/time.h>
#include "mozilla/layers/IpcRecentHistory.h"
#include "mozilla/layers/PCompositorBridge.h"

//#define IRH_DEBUG
#define ARRAY_SIZE(arr)                 (sizeof(arr) / sizeof((arr)[0]))

namespace mozilla {
namespace layers {

IpcRecentHistory::IpcRecentHistory(size_t capacity, struct IpcCommand* lut) :
  mLock("IpcRecentHistory")
{
  mCapacity = capacity;
  mSize = 0;
  mCurrent = 0;
  mBuffer = new CommandRecord[capacity];
  mCommandLut = lut;

  MOZ_ASSERT(!mBuffer);
}

IpcRecentHistory::~IpcRecentHistory()
{
#ifdef IRH_DEBUG
  printf_stderr("IRH: destructing");
#endif
  delete mBuffer;
}

void IpcRecentHistory::push(PCompositorBridge::MessageType cmd)
{
  MutexAutoLock lock(mLock);

  CommandRecord *cr = &mBuffer[mCurrent];
  gettimeofday(&cr->mTimestamp, nullptr);
  cr->mCommand = cmd;

  if (mSize < mCapacity) {
    mSize++;
    mCurrent++;
  } else {
    mCurrent = (mCurrent + 1) % mCapacity;
  }
#ifdef IRH_DEBUG
  printf_stderr("IRH: current=%d, cmd=%s", mCurrent, getCommandName(cmd));
#endif
}

const char* IpcRecentHistory::getCommandName(PCompositorBridge::MessageType cmd)
{
  for (unsigned i = 0; mCommandLut[i].mId != 0; i++) {
    if (cmd == mCommandLut[i].mId) {
      return mCommandLut[i].mName;
    }
  }
  return nullptr;
}

void IpcRecentHistory::printRecord(unsigned index)
{
  char tmbuf[64], buf[64];

  strftime(tmbuf, sizeof tmbuf, "%m-%d %H:%M:%S", localtime(&mBuffer[index].mTimestamp.tv_sec));
  snprintf(buf, sizeof buf, "%s.%03ld", tmbuf, mBuffer[index].mTimestamp.tv_usec / 1000);

  printf_stderr("[%d] %s; %s", index, getCommandName(mBuffer[index].mCommand), buf);
}

void IpcRecentHistory::dump()
{
#ifdef IRH_DEBUG
  printf_stderr("IRH: dumping %d records", mSize);
#endif
  MutexAutoLock lock(mLock);

  if (mSize < mCapacity) {
    for (size_t i = 0; i < mSize; i++) {
      printRecord(i);
    }
  } else {
    for (size_t i = (mCurrent + 1) % mCapacity; i < mCapacity; i++) {
      printRecord(i);
    }
    for (size_t i = 0; i < mCurrent; i++) {
      printRecord(i);
    }
  }
}

} // namespace layers                   
} // namespace mozilla
