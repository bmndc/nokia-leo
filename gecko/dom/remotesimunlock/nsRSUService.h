/* (c) 2020 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

#ifndef __NSRSUSEVICE__
#define __NSRSUSEVICE__

#include "nsIRSURequestService.h"
#include "RSUUtils.h"
#include "nsCOMPtr.h"
#include "nsThread.h"

namespace mozilla {
  namespace dom {

    class nsRSUService : public nsIRSURequestService
    {
    public:
      NS_DECL_ISUPPORTS
      NS_DECL_NSIRSUREQUESTSERVICE
      nsRSUService();
      static already_AddRefed<nsIRSURequestService> FactoryCreate();
      void DispatchResult(ResultOptions & aResult, int32_t aId, nsCOMPtr<nsIRSUCallback> aCallback);

    private:
      virtual ~nsRSUService();
      nsCOMPtr<nsIThread> mRsuThread;
      nsCOMPtr<nsIRSUEventListener> mListener;
      nsCOMArray<nsIRSUCallback> mRSUCallbacks;
      int32_t addCallback(nsIRSUCallback* aCallback);
      void removeCallback(nsIRSUCallback* aCallback);
    protected:
      /* additional members */
    };

    class RSUServiceImpl
    {
    public:
      // Suppress warning from |UniquePtr|
      virtual ~RSUServiceImpl() {}
      virtual char * do_tsd_invoke(char * request) = 0;
      virtual int32_t do_ModemWrapper_Send_request(uint32_t   request_type,
                                                   uint8_t  * buffer_ptr,
                                                   uint32_t   buffer_len,
                                                   uint32_t   payload_len
                                                   ) = 0;
 
    };

    // Concrete class to use to access the wpa supplicant.
    class RSUApiService final
    {
    public:
      RSUApiService();
      bool SendRSURequest(ReqOptions & aRequest, ResultOptions & aResult);

    private:
      mozilla::UniquePtr<RSUServiceImpl> mImpl;
    };

  } // dom
} // mozilla

#endif //__NSRSUSEVICE__
