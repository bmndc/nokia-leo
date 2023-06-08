/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners. */

#ifndef mozilla_dom_MobileSignalStrength_h
#define mozilla_dom_MobileSignalStrength_h

#include "nsPIDOMWindow.h"
#include "nsIMobileSignalStrength.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {
class MobileSignalStrength final : public nsIMobileSignalStrength
                                 , public nsWrapperCache
{
public:
  NS_DECL_NSIMOBILESIGNALSTRENGTH
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(MobileSignalStrength)

  explicit MobileSignalStrength(nsPIDOMWindowInner* aWindow);

  MobileSignalStrength(nsPIDOMWindowInner* aWindow, const int16_t& aLevel,
    const int16_t& aGsmSignalStrength, const int16_t& aGsmSignalBitErrorRate,
    const int16_t& aCdmaDbm, const int16_t& aCdmaEcio,
    const int16_t& aCdmaEvdoDbm, const int16_t& aCdmaEvdoEcio,
    const int16_t& aCdmaEvdoSNR, const int16_t& aLteSignalStrength,
    const int32_t& aLteRsrp, const int32_t& aLteRsrq, const int32_t& aLteRssnr,
    const int32_t& aLteCqi, const int32_t& aLteTimingAdvance,
    const int32_t& aTdscdmaRscp);

  MobileSignalStrength(const int16_t& aLevel, const int16_t& aGsmSignalStrength,
    const int16_t& aGsmSignalBitErrorRate, const int16_t& aCdmaDbm,
    const int16_t& aCdmaEcio, const int16_t& aCdmaEvdoDbm,
    const int16_t& aCdmaEvdoEcio, const int16_t& aCdmaEvdoSNR,
    const int16_t& aLteSignalStrength, const int32_t& aLteRsrp,
    const int32_t& aLteRsrq, const int32_t& aLteRssnr, const int32_t& aLteCqi,
    const int32_t& aLteTimingAdvance, const int32_t& aTdscdmaRscp);

  void
  Update(nsIMobileSignalStrength* aInfo);

  nsPIDOMWindowInner*
  GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  // WebIDL interface
  int16_t
  Level() const
  {
    return mLevel;
  }

  int16_t
  GsmSignalStrength() const
  {
    return mGsmSignalStrength;
  }

  int16_t
  GsmBitErrorRate() const
  {
    return mGsmSignalBitErrorRate;
  }

  int16_t
  CdmaDbm() const
  {
    return mCdmaDbm;
  }

  int16_t
  CdmaEcio() const
  {
    return mCdmaEcio;
  }

  int16_t
  CdmaEvdoDbm() const
  {
    return mCdmaEvdoDbm;
  }

  int16_t
  CdmaEvdoEcio() const
  {
    return mCdmaEvdoEcio;
  }

  int16_t
  CdmaEvdoSNR() const
  {
    return mCdmaEvdoSNR;
  }

  int16_t
  LteSignalStrength() const
  {
    return mLteSignalStrength;
  }

  int32_t
  LteRsrp() const
  {
    return mLteRsrp;
  }

  int32_t
  LteRsrq() const
  {
    return mLteRsrq;
  }

  int32_t
  LteRssnr() const
  {
    return mLteRssnr;
  }

  int32_t
  LteCqi() const
  {
    return mLteCqi;
  }

  int32_t
  LteTimingAdvance() const
  {
    return mLteTimingAdvance;
  }

  int32_t
  TdscdmaRscp() const
  {
    return mTdscdmaRscp;
  }

private:
  ~MobileSignalStrength() {}

  nsCOMPtr<nsPIDOMWindowInner> mWindow;

  int16_t mLevel;

  // GSM/UMTS category.
  int16_t mGsmSignalStrength;
  int16_t mGsmSignalBitErrorRate;

  // CDMA category.
  int16_t mCdmaDbm;
  int16_t mCdmaEcio;
  int16_t mCdmaEvdoDbm;
  int16_t mCdmaEvdoEcio;
  int16_t mCdmaEvdoSNR;

  // LTE category.
  int16_t mLteSignalStrength;
  int32_t mLteRsrp;
  int32_t mLteRsrq;
  int32_t mLteRssnr;
  int32_t mLteCqi;
  int32_t mLteTimingAdvance;

  // TDS-CDMA category.
  int32_t mTdscdmaRscp;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_MobileSignalStrength_h