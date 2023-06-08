/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright © 2015, Deutsche Telekom, Inc. */

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/systemlibs.js");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "UiccConnector",
                                   "@mozilla.org/secureelement/connector/uicc;1",
                                   "nsISecureElementConnector");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyModuleGetter(this, "SEUtils",
                                  "resource://gre/modules/SEUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "SE", function() {
  let obj = {};
  Cu.import("resource://gre/modules/se_consts.js", obj);
  return obj;
});

XPCOMUtils.defineLazyGetter(this, "GP", function() {
  let obj = {};
  Cu.import("resource://gre/modules/gp_consts.js", obj);
  return obj;
});

XPCOMUtils.defineLazyGetter(this, "UiccConnector", () => {
  let uiccClass = Cc["@mozilla.org/secureelement/connector/uicc;1"];
  return uiccClass ? uiccClass.getService(Ci.nsISecureElementConnector) : null;
});

XPCOMUtils.defineLazyGetter(this, "EseConnector", () => {
  let eseClass = Cc["@mozilla.org/secureelement/connector/ese;1"];
  return eseClass ? eseClass.getService(Ci.nsISecureElementConnector) : null;
});

XPCOMUtils.defineLazyGetter(this, "NFC", function () {
  let obj = {};
  Cu.import("resource://gre/modules/nfc_consts.js", obj);
  return obj;
});

// Ref: Secure Element Access Control – Public Release v1.1
//
// Command and response for obtaining ARAM access rules,
//
// Request to obtain all access rules,
//
// CLA INS P1 P2 Lc Data Le                Response-ALL-REF-AR-DO
//  80  CA FF 40         00   => case 1,    FF 40 7F REF-AR-DO
//                            => case 2,    FF 40 83 LEN LEN REF-AR-DO
//
// Response-ALL-REF-AR-DO: FF 40 length REF-AR-DO REF-AR-DO REF-AR-DO ....
//
// REF-AR-DO             : E2 length REF-DO AR-DO
// REF-DO                : E1 length AID-REF-DO DeviceAppID-REF-DO
// AR-DO                 : E3 length APDU-AR-DO or NFC-AR-DO or APDU-AR-DO NFC-AR-DO
// AID-REF-DO            : 4F length(5~16) AID
// DeviceAppID-REF-DO    : C1 length(20 or 0) "HASH_ORIGIN"
// APDU-AR-DO            : D0 length 00 or 01 or "APDU_FILTER" "APDU_FILTER" ....
// NFC-AR-DO             : D1 length 00 or 01

const APDU_STATUS_LEN = 2;
const GET_ALL_DATA_COMMAND_LENGTH = 2;
const RESPONSE_STATUS_LENGTH = 2;
const GET_REFRESH_DATA_CMD_LEN = 2;
const GET_REFRESH_DATA_TAG_LEN = 1;
const GET_REFRESH_TAG_LEN = 8;

// Data Object Tags
const REF_AR_DO      = 0xE2;
const REF_DO         = 0xE1;
const AR_DO          = 0xE3;
const HASH_REF_DO    = 0xC1;
const AID_REF_DO     = 0x4F;
const APDU_AR_DO     = 0xD0;
const NFC_AR_DO      = 0xD1;

var DEBUG = SE.DEBUG_ACE;
var debug;
function updateDebug() {
  if (DEBUG) {
    debug = function (s) {
      dump("ACE:GPAccessRulesManager: " + s + "\n");
    };
  } else {
    debug = function (s) {};
  }
};
updateDebug();

function getConnector(type) {
  switch (type) {
    case SE.TYPE_UICC:
      return UiccConnector;
    case SE.TYPE_ESE:
      return EseConnector;
    default:
      debug("Unsupported SEConnector : " + type);
      return null;
  }
}

/**
 * Based on [1] - "GlobalPlatform Device Technology
 * Secure Element Access Control Version 1.0".
 * GPAccessRulesManager reads and parses access rules from SE file system
 * as defined in section #7 of [1]: "Structure of Access Rule Files (ARF)".
 * Rules retrieval from ARA-M applet is not implmented due to lack of
 * commercial implemenations of ARA-M.
 * @todo Bug 1137537: Implement ARA-M support according to section #4 of [1]
 */
function GPAccessRulesManager() {
  let lock = gSettingsService.createLock();
  lock.get(NFC.SETTING_NFC_DEBUG, this);
  Services.obs.addObserver(this, NFC.TOPIC_MOZSETTINGS_CHANGED, false);
}

GPAccessRulesManager.prototype = {
  // source [1] section 7.1.3 PKCS#15 Selection
  PKCS_AID: "a000000063504b43532d3135",

  ARAM_AID: "A00000015141434C00",

  // APDUs (ISO 7816-4) for accessing rules on SE file system
  // see for more details: http://www.cardwerk.com/smartcards/
  // smartcard_standard_ISO7816-4_6_basic_interindustry_commands.aspx
  READ_BINARY:   [GP.CLA_SM, GP.INS_RB, GP.P1_RB, GP.P2_RB],
  GET_RESPONSE:  [GP.CLA_SM, GP.INS_GR, GP.P1_GR, GP.P2_GR],
  SELECT_BY_DF:  [GP.CLA_SM, GP.INS_SF, GP.P1_SF_DF, GP.P2_SF_FCP],

  // Non-null if there is a channel open
  channel: null,

  // Refresh tag path in the acMain file as described in GPD spec,
  // sections 7.1.5 and C.1.
  REFRESH_TAG_PATH: [GP.TAG_SEQUENCE, GP.TAG_OCTETSTRING],
  refreshTag: null,

  // Contains rules as read from the SE
  rules: [],

  seType: SE.TYPE_ESE,

  // Returns the latest rules. Results are cached.
  getAccessRules: function getAccessRules(seType) {
    debug("getAccessRules: seType:" + seType);

    this.seType = seType;
    return new Promise((resolve, reject) => {
      if (seType == SE.TYPE_ESE) {
        this._readARAMAccessRules(() => resolve(this.rules));
      } else {
        this._readAccessRules(() => resolve(this.rules));
      }
    });
  },

  _updateAccessRules: Task.async(function*(done) {
    try {
      yield this._openChannel(this.ARAM_AID);
      yield this._updateAllGPData();
      yield this._closeChannel();
      done();
    } catch (error) {
      debug("_updateAccessRules: " + error);
      yield this._closeChannel();
      done();
    }
  }),

  _updateAllGPData: function _updateAllGPData() {
    let apdu = this._bytesToAPDU(GP.UPDATE_ALL_GP_DATA);
    return new Promise((resolve, reject) => {
      getConnector(this.seType).exchangeAPDU(this.channel, apdu.cla,
        apdu.ins, apdu.p1, apdu.p2, apdu.data, apdu.le,
        {
          notifyExchangeAPDUResponse: (sw1, sw2, data) => {
            debug("Update all data command response is " + sw1.toString(16) + sw2.toString(16) +
                  " data: " + data + " length:" + (data.length/2));

            // 90 00 is "success"
            if (sw1 !== 0x90 && sw2 !== 0x00) {
              debug("rejecting APDU response");
              reject(new Error("Response " + sw1 + "," + sw2));
              return;
            }

            resolve();
          },

          notifyError: (error) => {
            debug("_exchangeAPDU/notifyError " + error);
            reject(error);
          }
        }
      );
    });
  },

  _readARAMAccessRules: Task.async(function*(done) {
    try {
      yield this._openChannel(this.ARAM_AID);
      // We have obtained access rules, check if we need to update the access rules.
      if (this.rules) {
        let refreshTag = yield this._readRefreshGPData();
        debug("New refresh tag is " + refreshTag + " old refresh tag is " + this.refreshTag);
        // Update cached rules based on refreshTag.
        if (SEUtils.arraysEqual(this.refreshTag, refreshTag)) {
          debug("Refresh tag equals to the one saved.");
          yield this._closeChannel();
          return done();
        }

        // Update the tag.
        this.refreshTag = refreshTag;
      }

      this.rules = yield this._readAllGPData();

      DEBUG && debug("_readAccessRules: " + JSON.stringify(this.rules).toString(16));

      yield this._closeChannel();
      done();
    } catch (error) {
      debug("_readARAMAccessRules: " + error);
      this.rules = [];
      yield this._closeChannel();
      done();
    }
  }),

  _getRulesLength: function _getRulesLength(apdu, rulesLength) {
    if (apdu.length < 3)
      return -1;

    let length;
    length = apdu[2];
    rulesLength.numBytes = 1;
    //
    // BER: If length bit 8 is one, bit [0..7] is the number
    // of bytes used for encoding the length.
    //
    if (length & 0x80) {
      let _length = length & 0x7f;
      debug("Length:" + _length);

      if (apdu.length < 3 + _length)
        return -1;

      length = 0;
      let base
      base = 1 << 8 * _length;
      for (let i = 0; i < _length; i++) {
        base >>= 8;
        length += apdu[3 + i] * base;
      }

      rulesLength.numBytes = _length + 1;
    }

    rulesLength.length = length;
  },

  _parseArDo: function _parseArDo(bytes) {
    let result = {};
    result[APDU_AR_DO] = [];
    result[NFC_AR_DO] = [];
    debug("Parse AR_DO:")
    for (let pos = 0; pos < bytes.length; ) {
      let tag = bytes[pos],
          length = bytes[pos + 1],
          parsed = null;

      switch(tag) {
      case APDU_AR_DO:
        let apduRuleValue = bytes.slice(pos + 2,  pos + 2 + length);
        debug("apdu rule:" + apduRuleValue);
        result[APDU_AR_DO] = apduRuleValue;
        break;
      case NFC_AR_DO:
        let nfcRuleValue = bytes.slice(pos + 2, pos + 2 + length);
        debug("nfc event rule:" + nfcRuleValue);
        result[NFC_AR_DO] = nfcRuleValue;
        break;
      default:
        break;
      }

      pos = pos + 2 + length;
    }
    return result;
  },

  _parseRefDo: function _parseRefDo(bytes) {
    let result = {};
    result[HASH_REF_DO] = [];
    result[AID_REF_DO] = [];
    debug("Parse REF_DO:")
    for (let pos = 0; pos < bytes.length; ) {
      let tag = bytes[pos],
          length = bytes[pos + 1],
          parsed = null;

      switch(tag) {
      case HASH_REF_DO:
        let hashRefValue = bytes.slice(pos + 2, pos + 2 + length);
        debug("application hash:" + hashRefValue);
        result[HASH_REF_DO] = hashRefValue;
        break;
      case AID_REF_DO:
        let aidRefValue = bytes.slice(pos + 2, pos + 2 + length);
        debug("aid hash:" + aidRefValue);
        result[AID_REF_DO] = aidRefValue;
        break;
      default:
        break;
      }

      pos = pos + 2 + length;
    }
    return result;
  },

  _parseARRule: function _parseARRule(rule) {
    let result = {};
    result[REF_DO] = [];
    result[AR_DO] = [];
    debug("Parse Rule:")
    for (let pos = 0; pos < rule.length; ) {
      let tag = rule[pos],
          length = rule[pos + 1],
          value = rule.slice(pos + 2, pos + 2 + length),
          parsed = null;

      switch(tag) {
      case REF_DO:
        result[REF_DO] = this._parseRefDo(value);
        break;
      case AR_DO:
        result[AR_DO] = this._parseArDo(value);
        break;
      default:
        break;
      }

      pos = pos + 2 + length;
    }

    return {
      hash: result[REF_DO][HASH_REF_DO],
      aid:  result[REF_DO][AID_REF_DO],
      apduRules: result[AR_DO][APDU_AR_DO],
      nfcRule: result[AR_DO][NFC_AR_DO]
    }
  },

  _parseARRules: function _parseARRules(payload) {
    let rules = [];
    let containerTags = [
      REF_DO,
      AR_DO,
    ];
    debug("Parse Rules:")
    let rule;
    for (let pos = 0; pos < payload.length;) {
      if (payload[pos] != REF_AR_DO) {
        pos++;
        continue;
      }

      let refArDoLength = payload[pos + 1];

      let refArDo = payload.slice(pos + 2, pos + 2 + refArDoLength);
      rule = this._parseARRule(refArDo);
      debug("rule:" + JSON.stringify(rule));
      pos += 2 + refArDoLength;
      rules.push(rule);
    }

    return rules;
  },

  _parseResponse: function _parseResponse(apdu) {
    let rulesLength = {
      numBytes: 0,
      length: 0
    };

    this._getRulesLength(apdu, rulesLength);
    debug("Total rules length: " + rulesLength.length + " number of bytes to represent length:" +
             rulesLength.numBytes);

    if (rulesLength.length < 0) {
      debug("Invalid total rules length");
      return [];
    }

    if (apdu.length > GET_ALL_DATA_COMMAND_LENGTH + rulesLength.numBytes +
          rulesLength.length + RESPONSE_STATUS_LENGTH) {
      debug("Invalid apdu length");
      return [];
    }

    let payloadLength = apdu.length - GET_ALL_DATA_COMMAND_LENGTH -
          rulesLength.numBytes - RESPONSE_STATUS_LENGTH;

    if (payloadLength < rulesLength.length) {
      // send get next command
      debug("Should send get next data command, received data length " +
            payloadLength + " expected length " + rulesLength.length);
      let headerLength = GET_ALL_DATA_COMMAND_LENGTH + rulesLength.numBytes;
      let payload = apdu.slice(headerLength, headerLength + payloadLength);
      return this._readNextGPData(payload, rulesLength.length);
    }

    let headerLength = GET_ALL_DATA_COMMAND_LENGTH + rulesLength.numBytes;
    let payload = apdu.slice(headerLength, headerLength + payloadLength);
    return this._parseARRules(payload);
  },

  _readNextGPData: function _readNextGPData(prevPayload, totalRulesLength) {
    let apdu = this._bytesToAPDU(GP.GET_NEXT_GP_DATA);
    return new Promise((resolve, reject) => {
      getConnector(this.seType).exchangeAPDU(this.channel, apdu.cla,
        apdu.ins, apdu.p1, apdu.p2, apdu.data, apdu.le,
        {
          notifyExchangeAPDUResponse: (sw1, sw2, data) => {
            debug("Read next data command response is " + sw1.toString(16) + sw2.toString(16) +
                  " data: " + data + " length:" + (data.length/2));

            // 90 00 is "success"
            if (sw1 !== 0x90 && sw2 !== 0x00) {
              debug("rejecting APDU response");
              reject(new Error("Response " + sw1 + "," + sw2));
              return;
            }

            let raw = SEUtils.hexStringToByteArray(data);
            // Slice sw1 and sw2 bytes.
            let resp = raw.slice(0, (raw.length - 2));
            // Concat previous payload.
            let accumulated = prevPayload.concat(resp);
            if (accumulated.length < totalRulesLength) {
              debug("Get next data command current length:" + accumulated.length + " expected length:" + totalRulesLength);
              this._readNextGPData(accumulated, totalRulesLength);
            } else {
              debug("Get all access rules");
              let rules = this._parseARRules(accumulated);
              resolve(rules);
            }
          },

          notifyError: (error) => {
            debug("_exchangeAPDU/notifyError " + error);
            reject(error);
          }
        }
      );
    });
  },

  _readAllGPData: function _readAllGPData() {
    let apdu = this._bytesToAPDU(GP.GET_ALL_GP_DATA);
    return new Promise((resolve, reject) => {
      getConnector(this.seType).exchangeAPDU(this.channel, apdu.cla,
        apdu.ins, apdu.p1, apdu.p2, apdu.data, apdu.le,
        {
          notifyExchangeAPDUResponse: (sw1, sw2, data) => {
            debug("Read all data command response is " + sw1.toString(16) + sw2.toString(16) +
                  " data: " + data + " length:" + (data.length/2));

            // 90 00 is "success"
            if (sw1 !== 0x90 && sw2 !== 0x00) {
              debug("rejecting APDU response");
              reject(new Error("Response " + sw1 + "," + sw2));
              return;
            }

            let resp = SEUtils.hexStringToByteArray(data);
            let rules = this._parseResponse(resp);
            resolve(rules);
          },

          notifyError: (error) => {
            debug("_exchangeAPDU/notifyError " + error);
            reject(error);
          }
        }
      );
    });
  },

  _readRefreshGPData: function _readRefreshGPData() {
    let apdu = this._bytesToAPDU(GP.GET_REFRESH_GP_DATA);
    return new Promise((resolve, reject) => {
      getConnector(this.seType).exchangeAPDU(this.channel, apdu.cla,
        apdu.ins, apdu.p1, apdu.p2, apdu.data, apdu.le,
        {
          notifyExchangeAPDUResponse: (sw1, sw2, data) => {
            debug("Read refresh tag command response is " + sw1.toString(16) + sw2.toString(16) +
                  " data: " + data);

            // 90 00 is "success"
            if (sw1 !== 0x90 && sw2 !== 0x00) {
              debug("rejecting APDU response");
              reject(new Error("Response " + sw1 + "," + sw2));
              return;
            }

            let resp = SEUtils.hexStringToByteArray(data);

            if (resp.length != (GET_REFRESH_DATA_CMD_LEN + GET_REFRESH_DATA_TAG_LEN +
                  GET_REFRESH_TAG_LEN + APDU_STATUS_LEN)) {
              reject(new Error("Incorrect data length"));
              return;
            }

            // Should be refresh tag and length of the tag is 8.
            if (resp[0] != 0xDF || resp[1] != 0x20 || resp[2] != 0x08) {
              reject(new Error("Invalid refersh tag"));
              return;
            }

            let refreshTag = resp.slice(GET_REFRESH_DATA_CMD_LEN +
                                          GET_REFRESH_DATA_TAG_LEN,
                                          GET_REFRESH_DATA_CMD_LEN +
                                          GET_REFRESH_DATA_TAG_LEN +
                                          GET_REFRESH_TAG_LEN);
            resolve(refreshTag);
          },

          notifyError: (error) => {
            debug("_exchangeAPDU/notifyError " + error);
            reject(error);
          }
        }
      );
    });
  },

  _readAccessRules: Task.async(function*(done) {
    try {
      yield this._openChannel(this.PKCS_AID);

      let odf = yield this._readODF();
      let dodf = yield this._readDODF(odf);

      let acmf = yield this._readACMF(dodf);
      let refreshTag = acmf[this.REFRESH_TAG_PATH[0]]
                           [this.REFRESH_TAG_PATH[1]];

      // Update cached rules based on refreshTag.
      if (SEUtils.arraysEqual(this.refreshTag, refreshTag)) {
        debug("_readAccessRules: refresh tag equals to the one saved.");
        yield this._closeChannel();
        return done();
      }

      this.refreshTag = refreshTag;
      debug("_readAccessRules: refresh tag saved: " + this.refreshTag);

      let acrf = yield this._readACRules(acmf);
      let accf = yield this._readACConditions(acrf);
      this.rules = yield this._parseRules(acrf, accf);

      DEBUG && debug("_readAccessRules: " + JSON.stringify(this.rules, 0, 2));

      yield this._closeChannel();
      done();
    } catch (error) {
      debug("_readAccessRules: " + error);
      this.rules = [];
      yield this._closeChannel();
      done();
    }
  }),

  _openChannel: function _openChannel(aid) {
    if (this.channel !== null) {
      debug("_openChannel: Channel already opened, rejecting.");
      return Promise.reject();
    }

    return new Promise((resolve, reject) => {
      getConnector(this.seType).openChannel(aid, {
        notifyOpenChannelSuccess: (channel, openResponse) => {
          debug("_openChannel/notifyOpenChannelSuccess: Channel " + channel +
                " opened, open response: " + openResponse);
          this.channel = channel;
          resolve();
        },
        notifyError: (error) => {
          debug("_openChannel/notifyError: failed to open channel, error: " +
                error);
          reject(error);
        }
      });
    });
  },

  _closeChannel: function _closeChannel() {
    if (this.channel === null) {
      debug("_closeChannel: Channel not opened, rejecting.");
      return Promise.reject();
    }

    return new Promise((resolve, reject) => {
      getConnector(this.seType).closeChannel(this.channel, {
        notifyCloseChannelSuccess: () => {
          debug("_closeChannel/notifyCloseChannelSuccess: chanel " +
                this.channel + " closed");
          this.channel = null;
          resolve();
        },
        notifyError: (error) => {
          debug("_closeChannel/notifyError: error closing channel, error" +
                error);
          reject(error);
        }
      });
    });
  },

  _exchangeAPDU: function _exchangeAPDU(bytes) {
    DEBUG && debug("apdu " + JSON.stringify(bytes));

    let apdu = this._bytesToAPDU(bytes);
    return new Promise((resolve, reject) => {
      getConnector(this.seType).exchangeAPDU(this.channel, apdu.cla,
        apdu.ins, apdu.p1, apdu.p2, apdu.data, apdu.le,
        {
          notifyExchangeAPDUResponse: (sw1, sw2, data) => {
            debug("APDU response is " + sw1.toString(16) + sw2.toString(16) +
                  " data: " + data);

            // 90 00 is "success"
            if (sw1 !== 0x90 && sw2 !== 0x00) {
              debug("rejecting APDU response");
              reject(new Error("Response " + sw1 + "," + sw2));
              return;
            }

            resolve(this._parseTLV(data));
          },

          notifyError: (error) => {
            debug("_exchangeAPDU/notifyError " + error);
            reject(error);
          }
        }
      );
    });
  },

  _readBinaryFile: function _readBinaryFile(selectResponse) {
    DEBUG && debug("Select response: " + JSON.stringify(selectResponse));
    // 0x80 tag parameter - get the elementary file (EF) length
    // without structural information.
    let fileLength = selectResponse[GP.TAG_FCP][0x80];

    // If file is empty, no need to attempt to read it.
    if (fileLength[0] === 0 && fileLength[1] === 0) {
      return Promise.resolve(null);
    }

    // TODO READ BINARY with filelength not supported
    // let readApdu = this.READ_BINARY.concat(fileLength);
    return this._exchangeAPDU(this.READ_BINARY);
  },

  _selectAndRead: function _selectAndRead(df) {
    return this._exchangeAPDU(this.SELECT_BY_DF.concat(df.length & 0xFF, df))
           .then((resp) => this._readBinaryFile(resp));
  },

  _readODF: function _readODF() {
    debug("_readODF");
    return this._selectAndRead(GP.ODF_DF);
  },

  _readDODF: function _readDODF(odfFile) {
    debug("_readDODF, ODF file: " + odfFile);

    // Data Object Directory File (DODF) is used as an entry point to the
    // Access Control data. It is specified in PKCS#15 section 6.7.6.
    // DODF is referenced by the ODF file, which looks as follows:
    //   A7 06
    //      30 04
    //         04 02 XY WZ
    // where [0xXY, 0xWZ] is a DF of DODF file.
    let DODF_DF = odfFile[GP.TAG_EF_ODF][GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING];
    return this._selectAndRead(DODF_DF);
  },

  _readACMF: function _readACMF(dodfFile) {
    debug("_readACMF, DODF file: " + dodfFile);

    // ACMF file DF is referenced in DODF file, which looks like this:
    //
    //  A1 29
    //     30 00
    //     30 0F
    //        0C 0D 47 50 20 53 45 20 41 63 63 20 43 74 6C
    //     A1 14
    //        30 12
    //           06 0A 2A 86 48 86 FC 6B 81 48 01 01  <-- GPD registered OID
    //           30 04
    //              04 02 AB CD  <-- ACMF DF
    //  A1 2B
    //     30 00
    //     30 0F
    //        0C 0D 53 41 54 53 41 20 47 54 4F 20 31 2E 31
    //     A1 16
    //        30 14
    //           06 0C 2B 06 01 04 01 2A 02 6E 03 01 01 01  <-- some other OID
    //           30 04
    //              04 02 XY WZ  <-- some other file's DF
    //
    // DODF file consists of DataTypes with oidDO entries. Entry with OID
    // equal to "1.2.840.114283.200.1.1" ("2A 86 48 86 FC 6B 81 48 01 01")
    // contains DF of the ACMF. In the file above, it means that ACMF DF
    // equals to [0xAB, 0xCD], and not [0xXY, 0xWZ].
    //
    // Algorithm used to encode OID to an byte array:
    //   http://www.snmpsharpnet.com/?p=153

    let gpdOid = [0x2A,                 // 1.2
                  0x86, 0x48,           // 840
                  0x86, 0xFC, 0x6B,     // 114283
                  0x81, 0x48,           // 129
                  0x01,                 // 1
                  0x01];                // 1

    let records = SEUtils.ensureIsArray(dodfFile[GP.TAG_EXTERNALDO]);

    // Look for the OID registered for GPD SE.
    let gpdRecords = records.filter((record) => {
      let oid = record[GP.TAG_EXTERNALDO][GP.TAG_SEQUENCE][GP.TAG_OID];
      return SEUtils.arraysEqual(oid, gpdOid);
    });

    // [1] 7.1.5: "There shall be only one ACMF file per Secure Element.
    // If a Secure Element contains several ACMF files, then the security shall
    // be considered compromised and the Access Control enforcer shall forbid
    // access to all (...) apps."
    if (gpdRecords.length !== 1) {
      return Promise.reject(new Error(gpdRecords.length + " ACMF files found"));
    }

    let ACMain_DF = gpdRecords[0][GP.TAG_EXTERNALDO][GP.TAG_SEQUENCE]
                                 [GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING];
    return this._selectAndRead(ACMain_DF);
  },

  _readACRules: function _readACRules(acMainFile) {
    debug("_readACRules, ACMain file: " + acMainFile);

    // ACMF looks like this:
    //
    //  30 10
    //    04 08 XX XX XX XX XX XX XX XX
    //    30 04
    //       04 02 XY WZ
    //
    // where [XY, WZ] is a DF of ACRF, and XX XX XX XX XX XX XX XX is a refresh
    // tag.

    let ACRules_DF = acMainFile[GP.TAG_SEQUENCE][GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING];
    return this._selectAndRead(ACRules_DF);
  },

  _readACConditions: function _readACConditions(acRulesFile) {
    debug("_readACCondition, ACRules file: " + acRulesFile);

    let acRules = SEUtils.ensureIsArray(acRulesFile[GP.TAG_SEQUENCE]);
    if (acRules.length === 0) {
      debug("No rules found in ACRules file.");
      return Promise.reject(new Error("No rules found in ACRules file"));
    }

    // We first read all the condition files referenced in the ACRules file,
    // because ACRules file might reference one ACCondition file more than
    // once. Since reading it isn't exactly fast, we optimize here.
    let acReadQueue = Promise.resolve({});

    acRules.forEach((ruleEntry) => {
      let df = ruleEntry[GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING];

      // Promise chain read condition entries:
      let readAcCondition = (acConditionFiles) => {
        if (acConditionFiles[df] !== undefined) {
          debug("Skipping previously read acCondition df: " + df);
          return acConditionFiles;
        }

        return this._selectAndRead(df)
               .then((acConditionFileContents) => {
                 acConditionFiles[df] = acConditionFileContents;
                 return acConditionFiles;
               });
      }

      acReadQueue = acReadQueue.then(readAcCondition);
    });

    return acReadQueue;
  },

  _parseRules: function _parseRules(acRulesFile, acConditionFiles) {
    DEBUG && debug("_parseRules: acConditionFiles " + JSON.stringify(acConditionFiles));
    let rules = [];

    let acRules = SEUtils.ensureIsArray(acRulesFile[GP.TAG_SEQUENCE]);
    acRules.forEach((ruleEntry) => {
      DEBUG && debug("Parsing one rule: " + JSON.stringify(ruleEntry));
      let rule = {};

      // 0xA0 and 0x82 tags as per GPD spec sections C.1 - C.3. 0xA0 means
      // that rule describes access to one SE applet only (and its AID is
      // given). 0x82 means that rule describes acccess to all SE applets.
      let oneApplet = ruleEntry[GP.TAG_GPD_AID];
      let allApplets = ruleEntry[GP.TAG_GPD_ALL];

      if (oneApplet) {
        rule.applet = oneApplet[GP.TAG_OCTETSTRING];
      } else if (allApplets) {
        rule.applet = Ci.nsIAccessRulesManager.ALL_APPLET;
      } else {
        throw Error("Unknown applet definition");
      }

      let df = ruleEntry[GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING];
      let condition = acConditionFiles[df];
      if (condition === null) {
        rule.application = Ci.nsIAccessRulesManager.DENY_ALL;
      } else if (condition[GP.TAG_SEQUENCE]) {
        if (!Array.isArray(condition[GP.TAG_SEQUENCE]) &&
            !condition[GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING]) {
          rule.application = Ci.nsIAccessRulesManager.ALLOW_ALL;
        } else {
          rule.application = SEUtils.ensureIsArray(condition[GP.TAG_SEQUENCE])
                             .map((conditionEntry) => {
            return conditionEntry[GP.TAG_OCTETSTRING];
          });
        }
      } else {
        throw Error("Unknown application definition");
      }

      DEBUG && debug("Rule parsed, adding to the list: " + JSON.stringify(rule));
      rules.push(rule);
    });

    DEBUG && debug("All rules parsed, we have those in total: " + JSON.stringify(rules));
    return rules;
  },

  _parseTLV: function _parseTLV(bytes) {
    let containerTags = [
      GP.TAG_SEQUENCE,
      GP.TAG_FCP,
      GP.TAG_GPD_AID,
      GP.TAG_EXTERNALDO,
      GP.TAG_INDIRECT,
      GP.TAG_EF_ODF
    ];
    return SEUtils.parseTLV(bytes, containerTags);
  },

  // TODO consider removing if better format for storing
  // APDU consts will be introduced
  _bytesToAPDU: function _bytesToAPDU(arr) {
    let apdu;
    if (arr.length > 4) {
      apdu = {
        cla: arr[0] & 0xFF,
        ins: arr[1] & 0xFF,
        p1: arr[2] & 0xFF,
        p2: arr[3] & 0xFF,
        le: -1
      };

      let data = arr.slice(4);
      apdu.data = (data.length) ? SEUtils.byteArrayToHexString(data) : "";
    } else {
      apdu = {
        cla: arr[0] & 0xFF,
        ins: arr[1] & 0xFF,
        p1: arr[2] & 0xFF,
        p2: arr[3] & 0xFF,
        data: "",
        le: -1
      };
    }
    return apdu;
  },

  /**
   * nsISettingsServiceCallback
   */
  handle: function handle(name, result) {
    switch (name) {
      case NFC.SETTING_NFC_DEBUG:
        DEBUG = result;
        updateDebug();
        break;
    }
  },

  /**
   * nsIObserver interface methods.
   */

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case NFC.TOPIC_MOZSETTINGS_CHANGED:
        if ("wrappedJSObject" in subject) {
          subject = subject.wrappedJSObject;
        }
        if (subject) {
          this.handle(subject.key, subject.value);
        }
        break;
    }
  },

  classID: Components.ID("{3e046b4b-9e66-439a-97e0-98a69f39f55f}"),
  contractID: "@mozilla.org/secureelement/access-control/rules-manager;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAccessRulesManager])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([GPAccessRulesManager]);
