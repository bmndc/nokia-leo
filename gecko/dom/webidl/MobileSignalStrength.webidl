/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners. */

[Pref="dom.mobileconnection.enabled"]
interface MobileSignalStrength
{
  /**
   * Level of signal bars.
   *
   * Possible range from -1 to 4.
   * Generally, we should return 0 - 4 as normal case.
   * However, some carrier might need to show out-of-service if signal is lower
   * than some criteria. We use -1 to represent this condition.
   */
  readonly attribute short level;

  /**
   * GSM/WCDMA signal strength.
   *
   * Valid values are (0-31, 99) as defined in TS 27.007 8.5.
   */
  readonly attribute short gsmSignalStrength;

  /**
   * GSM/WCDMA bitErrorRate.
   *
   * Bit error rate (0-7, 99) as defined in TS 27.007 8.5.
   */
  readonly attribute short gsmBitErrorRate;

  /**
   * CDMA dbm.
   *
   * Valid values are negative integers.
   *
   * -120 denotes invalid value.
   */
  readonly attribute short cdmaDbm;

  /**
   * CDMA ecio.
   *
   * Valid values are negative integers.  This value is the actual Ec/Io multiplied
   * by 10.  Example: If the actual Ec/Io is -12.5 dB, then this response value
   * will be -125.
   *
   * -160 denotes invalid value.
   */
  readonly attribute short cdmaEcio;

  /**
   * CDMA EVDO dbm.
   *
   * Valid values are negative integers.
   *
   * -120 denotes invalid value.
   */
  readonly attribute short cdmaEvdoDbm;

  /**
   * CDMA EVDO ecio.
   *
   * Valid values are negative integers.  This value is the actual Ec/Io multiplied
   * by -10.  Example: If the actual Ec/Io is -12.5 dB, then this response value
   * will be 125.
   *
   * -1 denotes invalid value.
   */
  readonly attribute short cdmaEvdoEcio;

  /**
   * CDMA Evdo SNR.
   *
   * Valid values are 0-8.  8 is the highest signal to noise ratio.
   *
   * -1 denotes invalid value.
   */
  readonly attribute short cdmaEvdoSNR;

  /**
   * LTE signal strength.
   *
   * Valid values are (0-31, 99) as defined in TS 27.007 8.5.
   */
  readonly attribute short lteSignalStrength;

  /**
   * LTE RSRP.
   *
   * The current Reference Signal Receive Power in dBm.
   * Range: -140 to -44 dBm.
   * INT_MAX: 0x7FFFFFFF denotes invalid value.
   * Reference: 3GPP TS 36.133 9.1.4
   */
  readonly attribute long lteRsrp;

  /**
   * LTE RSRQ.
   *
   * The current Reference Signal Receive Quality in dB..
   * Range: -20 to -3 dB.
   * INT_MAX: 0x7FFFFFFF denotes invalid value.
   * Reference: 3GPP TS 36.133 9.1.7
   */
  readonly attribute long lteRsrq;

  /**
   * LTE RSSNR.
   *
   * The current reference signal signal-to-noise ratio in 0.1 dB units.
   * Range: -200 to +300 (-200 = -20.0 dB, +300 = 30dB).
   * INT_MAX : 0x7FFFFFFF denotes invalid value.
   * Reference: 3GPP TS 36.101 8.1.1
   */
  readonly attribute long lteRssnr;

  /**
   * LTE cqi.
   *
   * The current Channel Quality Indicator.
   * Range: 0 to 15.
   * INT_MAX : 0x7FFFFFFF denotes invalid value.
   * Reference: 3GPP TS 36.101 9.2, 9.3, A.4
   */
  readonly attribute long lteCqi;

  /**
   * LTE timing advance.
   *
   * timing advance in micro seconds for a one way trip from cell to device.
   * Approximate distance can be calculated using 300m/us * timingAdvance.
   * Range: 0 to 0x7FFFFFFE
   * INT_MAX : 0x7FFFFFFF denotes invalid value.
   * Reference: 3GPP 36.321 section 6.1.3.5
   * also: http://www.cellular-planningoptimization.com/2010/02/timing-advance-with-calculation.html
   */
  readonly attribute long lteTimingAdvance;

  /**
   * TD-SCDMA rscp.
   *
   * Range : -120 to -25.
   * INT_MAX: 0x7FFFFFFF denotes invalid value.
   * Reference: 3GPP TS 25.123, section 9.1.1.1
   */
  readonly attribute long tdscdmaRscp;
};