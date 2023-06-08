/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=2 et tw=80 : */

#ifndef IPC_RECENT_HISTORY_H
#define IPC_RECENT_HISTORY_H

#include "mozilla/layers/PCompositorBridge.h"
#include "mozilla/Mutex.h"

namespace mozilla {
namespace layers {

class IpcRecentHistory {

  public:
    struct IpcCommand {

        int mId;

        const char *mName;
        // void *mPrivateData;
    };

    IpcRecentHistory(size_t capacity, struct IpcCommand* lut);

    ~IpcRecentHistory();

    void push(PCompositorBridge::MessageType cmd);

    void dump();

  private:
    struct CommandRecord {

      PCompositorBridge::MessageType  mCommand;

      struct timeval                  mTimestamp;
    };

    struct CommandRecord  *mBuffer;

    size_t            mSize;

    size_t            mCapacity;

    uint32_t          mCurrent;       // position of item to be written;

    mozilla::Mutex    mLock;

    struct IpcCommand *mCommandLut;

    const char *getCommandName(PCompositorBridge::MessageType cmd);

    void printRecord(unsigned index);
};

} // namespace layers                   
} // namespace mozilla

#endif
