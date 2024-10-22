/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

const { interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Log.jsm");

// loglevel should be one of "Fatal", "Error", "Warn", "Info", "Config",
// "Debug", "Trace" or "All". If none is specified, "Debug" will be used by
// default.  Note "Debug" is usually appropriate so that when this log is
// included in the Sync file logs we get verbose output.
const PREF_LOG_LEVEL = "identity.kaiaccounts.loglevel";
// The level of messages that will be dumped to the console.  If not specified,
// "Error" will be used.
const PREF_LOG_LEVEL_DUMP = "identity.kaiaccounts.log.appender.dump";

// A pref that can be set so "sensitive" information (eg, personally
// identifiable info, credentials, etc) will be logged.
const PREF_LOG_SENSITIVE_DETAILS = "identity.kaiaccounts.log.sensitive";

let exports = Object.create(null);

XPCOMUtils.defineLazyGetter(exports, 'log', function() {
  let log = Log.repository.getLogger("KaiOSAccounts");
  // We set the log level to debug, but the default dump appender is set to
  // the level reflected in the pref.  Other code that consumes FxA may then
  // choose to add another appender at a different level.
  log.level = Log.Level.Debug;
  let appender = new Log.DumpAppender();
  appender.level = Log.Level.Error;

  log.addAppender(appender);
  try {
    // The log itself.
    let level =
      Services.prefs.getPrefType(PREF_LOG_LEVEL) == Ci.nsIPrefBranch.PREF_STRING
      && Services.prefs.getCharPref(PREF_LOG_LEVEL);
    log.level = Log.Level[level] || Log.Level.Debug;

    // The appender.
    level =
      Services.prefs.getPrefType(PREF_LOG_LEVEL_DUMP) == Ci.nsIPrefBranch.PREF_STRING
      && Services.prefs.getCharPref(PREF_LOG_LEVEL_DUMP);
    appender.level = Log.Level[level] || Log.Level.Error;
  } catch (e) {
    log.error(e);
  }

  return log;
});

// A boolean to indicate if personally identifiable information (or anything
// else sensitive, such as credentials) should be logged.
XPCOMUtils.defineLazyGetter(exports, 'logPII', function() {
  try {
    return Services.prefs.getBoolPref(PREF_LOG_SENSITIVE_DETAILS);
  } catch (_) {
    return false;
  }
});

exports.KAIACCOUNTS_SERVICE_ID = "lr9Cts0RhbaNRJ5i_gKf";
exports.KAIACCOUNTS_PARTNER_ID = "ddsMreKpOJixSvYF5cvz";

exports.KAIACCOUNTS_PERMISSION = "kaios-accounts";

exports.DATA_FORMAT_VERSION = 3;
exports.DEFAULT_STORAGE_FILENAME = "signedInUser.json";

// Observer notifications.
exports.ONLOGIN_NOTIFICATION = "fxaccounts:onlogin";
exports.ONVERIFIED_NOTIFICATION = "fxaccounts:onverified";
exports.ONLOGOUT_NOTIFICATION = "fxaccounts:onlogout";
// Internal to services/fxaccounts only
exports.ON_FXA_UPDATE_NOTIFICATION = "fxaccounts:update";

// UI Requests.
exports.UI_REQUEST_SIGN_IN_FLOW = "signInFlow";
exports.UI_REQUEST_REFRESH_AUTH = "refreshAuthentication";

// Server errno.
// From https://github.com/mozilla/fxa-auth-server/blob/master/docs/api.md#response-format

// Errors that recognized by HTTP status code.
// The 1xx errno values are used internally.
exports.ERRNO_ACCOUNT_DOES_NOT_EXIST         = 102;
exports.ERRNO_INCORRECT_PASSWORD             = 103;
exports.ERRNO_INTERNAL_INVALID_USER          = 104;
exports.ERRNO_INVALID_ACCOUNTID              = 105;
exports.ERRNO_UNVERIFIED_ACCOUNT             = 106;
exports.ERRNO_INVALID_VERIFICATION_CODE      = 107;
exports.ERRNO_ALREADY_VERIFIED               = 108;
exports.ERRNO_INVALID_AUTH_TOKEN             = 109;
// Errors that recognized by application-level body errno.
// The errno values with four digits are defined by backend api doc.
exports.ERRNO_MISSING_ID_IN_URL                        = 1001;
exports.ERRNO_EMPTY_PAYLOAD                            = 1002;
exports.ERRNO_PAYLOAD_CANNOT_BE_UNMARSHALLED           = 1003;
exports.ERRNO_JWT_CANNOT_BE_RECOVERED                  = 1004;
exports.ERRNO_FAILED_TO_ATTACH_DATA_TO_REQUEST         = 1005;
exports.ERRNO_FAILED_TO_ATTACH_DATA_TO_RESPONSE        = 1006;
exports.ERRNO_FAILED_TO_DETACH_DATA_FROM_REQUEST       = 1007;
exports.ERRNO_FAILED_TO_DETACH_DATA_FROM_RESPONSE      = 1008;
exports.ERRNO_NATS_COMMUNICATION_FAILED                = 1009;
exports.ERRNO_JWT_BLACKLISTED                          = 1010;
exports.ERRNO_DL_SUBSYSTEM_FAILED                      = 1011;
exports.ERRNO_NOT_PROPERLY_INITIALIZED                 = 1012;
exports.ERRNO_WRONG_ACCOUNT_ID_FORMAT                  = 2001;
exports.ERRNO_WRONG_FIRST_NAME_FORMAT                  = 2002;
exports.ERRNO_WRONG_LAST_NAME_FORMAT                   = 2003;
exports.ERRNO_WRONG_BIRTHDAY_FORMAT                    = 2004;
exports.ERRNO_WRONG_GENDER_FORMAT                      = 2005;
exports.ERRNO_WRONG_EMAIL_FORMAT                       = 2006;
exports.ERRNO_MISSING_PASSWORD_FIELD                   = 2007;
exports.ERRNO_DUPLICATED_EMAIL_VALUE                   = 2008;
exports.ERRNO_WRONG_PHONE_FORMAT                       = 2009;
exports.ERRNO_MISSING_PHONE_PASSWORD_FIELD             = 2010;
exports.ERRNO_WRONG_SECOND_PHONE_FORMAT                = 2011;
exports.ERRNO_MISSING_AT_LEAST_ONE_CONTACT_FIELD       = 2012;
exports.ERRNO_DUPLICATED_EMAIL_VALUE                   = 2013;
exports.ERRNO_DUPLICATED_PHONE_VALUE                   = 2014;
exports.ERRNO_NOT_OWNER                                = 2015;
exports.ERRNO_NOT_PARTNER                              = 2016;
exports.ERRNO_NOT_ROOT                                 = 2017;
exports.ERRNO_FAILED_TO_UNMARSHAL_DL_RECORD            = 2018;
exports.ERRNO_WRONG_USER_IDENTITY                      = 2019;
exports.ERRNO_TOO_MANY_CONTACT_INFO_UPDATED            = 2020;
exports.ERRNO_PHONE_UPDATE_LEADS_TO_INVALID_ACCOUNT    = 2021;
exports.ERRNO_EMAIL_UPDATE_LEADS_TO_INVALID_ACCOUNT    = 2022;
exports.ERRNO_DUPLICATE_ACCOUNT_WITH_PHONE             = 2023;
exports.ERRNO_DUPLICATE_ACCOUNT_WITH_EMAIL             = 2024;
exports.ERRNO_FAILED_TO_RETRIEVE_ACCOUNT_FROM_DB       = 2025;
exports.ERRNO_FAILED_TO_SAVE_ACCOUNT_TO_DB             = 2026;
exports.ERRNO_FAILED_TO_RETRIEVE_ACCOUNT_FROM_EPHEM_DB = 2027;
exports.ERRNO_FAILED_TO_SAVE_ACCOUNT_TO_EPHEM_DB       = 2028;
exports.ERRNO_FAILED_TO_MARSHAL_RECORD                 = 2029;
exports.ERRNO_DUPLICATE_ACCOUNT_WITH_LOGIN             = 2030;
exports.ERRNO_DUPLICATE_ACCOUNT_WITH_ID                = 2031;
exports.ERRNO_DUPLICATE_ACCOUNT                        = 2032;
exports.ERRNO_FAILED_TO_MARSHAL_ACCOUNT                = 2033;
exports.ERRNO_PENDING_ACTIVATION_MODULE_FAILED         = 2034;
exports.ERRNO_UNVERIFIED_CONTACT_MODULE_FAILED         = 2035;
exports.ERRNO_ACCOUNT_MISSING_PARAMETER                = 2036;
exports.ERRNO_ACCOUNT_BAD_FORMATTED_PARAMETER          = 2037;
exports.ERRNO_ACCOUNT_BAD_EPHEMERAL_PARAMETER_VALUE    = 2038;
exports.ERRNO_EPHEMERAL_ACCOUNT_NOT_FOUND              = 2039;
exports.ERRNO_ACCOUNT_NOT_FOUND                        = 2040;
exports.ERRNO_INVALID_ACCOUNT_CONTACT_PARAMETER        = 2041;
exports.ERRNO_EPHEMERAL_ACCOUNT_LIKELY_EXPIRED         = 2042;
exports.ERRNO_MISSING_MANDATORY_EMAIL_FIELD            = 2043;
exports.ERRNO_NO_ACCOUNT_BOUND_WITH_CONTACT_FOUND      = 2044;
exports.ERRNO_FAILED_TO_READ_PUBLIC_KEY                = 3001;
exports.ERRNO_FAILED_TO_READ_PRIVATE_KEY               = 3002;
exports.ERRNO_FAILED_TO_MARSHAL_CLAIMS                 = 3003;
exports.ERRNO_FAILED_TO_UNMARSHAL_CLAIMS               = 3004;
exports.ERRNO_FAILED_TO_MARSHAL_LOGIN_REQ              = 3005;
exports.ERRNO_FAILED_TO_UNMARSHAL_LOGIN_RSP            = 3006;
exports.ERRNO_FAILED_TO_MARSHAL_ACCESS_TOKEN           = 3007;
exports.ERRNO_FAILED_TO_UNMARSHAL_ACCESS_TOKEN         = 3008;
exports.ERRNO_FAILED_TO_EXTRACT_CLAIMS                 = 3009;
exports.ERRNO_FAILED_TO_PARSE_CLAIMS                   = 3010;
exports.ERRNO_MISSING_FIELDS_IN_CLAIMS                 = 3011;
exports.ERRNO_AUDIENCE_MISMATCH                        = 3012;
exports.ERRNO_JWT_IS_NOT_A_REFRESH_TOKEN               = 3013;
exports.ERRNO_REFRESH_TOKEN_EXPIRED_OR_NOT_REGISTERED  = 3014;
exports.ERRNO_FAILED_TO_INVALIDATE_HAWK_KEY            = 3015;
exports.ERRNO_FAILED_TO_UNREGISTER_REFRESH_TOKEN       = 3016;
exports.ERRNO_FAILED_TO_INVALIDATE_SESSION             = 3017;

// Errors.
exports.ERROR_ACCOUNT_DOES_NOT_EXIST         = "ACCOUNT_DOES_NOT_EXIST";
exports.ERROR_ALREADY_SIGNED_IN_USER         = "ALREADY_SIGNED_IN_USER";
exports.ERROR_ENDPOINT_NO_LONGER_SUPPORTED   = "ENDPOINT_NO_LONGER_SUPPORTED";
exports.ERROR_INCORRECT_API_VERSION          = "INCORRECT_API_VERSION";
exports.ERROR_INCORRECT_EMAIL_CASE           = "INCORRECT_EMAIL_CASE";
exports.ERROR_INCORRECT_KEY_RETRIEVAL_METHOD = "INCORRECT_KEY_RETRIEVAL_METHOD";
exports.ERROR_INCORRECT_LOGIN_METHOD         = "INCORRECT_LOGIN_METHOD";
exports.ERROR_INVALID_ACCOUNTID              = "INVALID_ACCOUNTID";
exports.ERROR_INVALID_AUDIENCE               = "INVALID_AUDIENCE";
exports.ERROR_INVALID_AUTH_TOKEN             = "INVALID_AUTH_TOKEN";
exports.ERROR_INVALID_AUTH_TIMESTAMP         = "INVALID_AUTH_TIMESTAMP";
exports.ERROR_INVALID_AUTH_NONCE             = "INVALID_AUTH_NONCE";
exports.ERROR_INVALID_BODY_PARAMETERS        = "INVALID_BODY_PARAMETERS";
exports.ERROR_INVALID_PASSWORD               = "INVALID_PASSWORD";
exports.ERROR_INVALID_VERIFICATION_CODE      = "INVALID_VERIFICATION_CODE";
exports.ERROR_INVALID_REFRESH_AUTH_VALUE     = "INVALID_REFRESH_AUTH_VALUE";
exports.ERROR_INVALID_REQUEST_SIGNATURE      = "INVALID_REQUEST_SIGNATURE";
exports.ERROR_INTERNAL_INVALID_USER          = "INTERNAL_ERROR_INVALID_USER";
exports.ERROR_MISSING_BODY_PARAMETERS        = "MISSING_BODY_PARAMETERS";
exports.ERROR_MISSING_CONTENT_LENGTH         = "MISSING_CONTENT_LENGTH";
exports.ERROR_NO_TOKEN_SESSION               = "NO_TOKEN_SESSION";
exports.ERROR_NO_SILENT_REFRESH_AUTH         = "NO_SILENT_REFRESH_AUTH";
exports.ERROR_NOT_VALID_JSON_BODY            = "NOT_VALID_JSON_BODY";
exports.ERROR_OFFLINE                        = "OFFLINE";
exports.ERROR_PERMISSION_DENIED              = "PERMISSION_DENIED";
exports.ERROR_REQUEST_BODY_TOO_LARGE         = "REQUEST_BODY_TOO_LARGE";
exports.ERROR_SERVER_ERROR                   = "SERVER_ERROR";
exports.ERROR_TOO_MANY_CLIENT_REQUESTS       = "TOO_MANY_CLIENT_REQUESTS";
exports.ERROR_SERVICE_TEMP_UNAVAILABLE       = "SERVICE_TEMPORARY_UNAVAILABLE";
exports.ERROR_UI_ERROR                       = "UI_ERROR";
exports.ERROR_UI_REQUEST                     = "UI_REQUEST";
exports.ERROR_PARSE                          = "PARSE_ERROR";
exports.ERROR_NETWORK                        = "NETWORK_ERROR";
exports.ERROR_UNKNOWN                        = "UNKNOWN_ERROR";
exports.ERROR_UNVERIFIED_ACCOUNT             = "UNVERIFIED_ACCOUNT";
exports.ERROR_INVALID_OPERATION              = "INVALID_OPERATION";
exports.ERROR_ALREADY_VERIFIED               = "ALREADY_VERIFIED";

// Status code errors
exports.ERROR_CODE_METHOD_NOT_ALLOWED        = 405;
exports.ERROR_MSG_METHOD_NOT_ALLOWED         = "METHOD_NOT_ALLOWED";

// Error strings that defined by backend api doc.
exports.ERROR_MISSING_ID_IN_URL                        = "MISSING_ID_IN_URL";
exports.ERROR_EMPTY_PAYLOAD                            = "EMPTY_PAYLOAD";
exports.ERROR_PAYLOAD_CANNOT_BE_UNMARSHALLED           = "PAYLOAD_CANNOT_BE_UNMARSHALLED";
exports.ERROR_JWT_CANNOT_BE_RECOVERED                  = "JWT_CANNOT_BE_RECOVERED";
exports.ERROR_FAILED_TO_ATTACH_DATA_TO_REQUEST         = "FAILED_TO_ATTACH_DATA_TO_REQUEST";
exports.ERROR_FAILED_TO_ATTACH_DATA_TO_RESPONSE        = "FAILED_TO_ATTACH_DATA_TO_RESPONSE";
exports.ERROR_FAILED_TO_DETACH_DATA_FROM_REQUEST       = "FAILED_TO_DETACH_DATA_FROM_REQUEST";
exports.ERROR_FAILED_TO_DETACH_DATA_FROM_RESPONSE      = "FAILED_TO_DETACH_DATA_FROM_RESPONSE";
exports.ERROR_NATS_COMMUNICATION_FAILED                = "NATS_COMMUNICATION_FAILED";
exports.ERROR_JWT_BLACKLISTED                          = "JWT_BLACKLISTED";
exports.ERROR_DL_SUBSYSTEM_FAILED                      = "DL_SUBSYSTEM_FAILED";
exports.ERROR_NOT_PROPERLY_INITIALIZED                 = "NOT_PROPERLY_INITIALIZED";
exports.ERROR_WRONG_ACCOUNT_ID_FORMAT                  = "WRONG_ACCOUNT_ID_FORMAT";
exports.ERROR_WRONG_FIRST_NAME_FORMAT                  = "WRONG_FIRST_NAME_FORMAT";
exports.ERROR_WRONG_LAST_NAME_FORMAT                   = "WRONG_LAST_NAME_FORMAT";
exports.ERROR_WRONG_BIRTHDAY_FORMAT                    = "WRONG_BIRTHDAY_FORMAT";
exports.ERROR_WRONG_GENDER_FORMAT                      = "WRONG_GENDER_FORMAT";
exports.ERROR_WRONG_EMAIL_FORMAT                       = "WRONG_EMAIL_FORMAT";
exports.ERROR_MISSING_PASSWORD_FIELD                   = "MISSING_PASSWORD_FIELD";
exports.ERROR_DUPLICATED_EMAIL_VALUE                   = "DUPLICATED_EMAIL_VALUE";
exports.ERROR_WRONG_PHONE_FORMAT                       = "WRONG_PHONE_FORMAT";
exports.ERROR_MISSING_PHONE_PASSWORD_FIELD             = "MISSING_PHONE_PASSWORD_FIELD";
exports.ERROR_WRONG_SECOND_PHONE_FORMAT                = "WRONG_SECOND_PHONE_FORMAT";
exports.ERROR_MISSING_AT_LEAST_ONE_CONTACT_FIELD       = "MISSING_AT_LEAST_ONE_CONTACT_FIELD";
exports.ERROR_DUPLICATED_EMAIL_VALUE                   = "DUPLICATED_EMAIL_VALUE";
exports.ERROR_DUPLICATED_PHONE_VALUE                   = "DUPLICATED_PHONE_VALUE";
exports.ERROR_NOT_OWNER                                = "NOT_OWNER";
exports.ERROR_NOT_PARTNER                              = "NOT_PARTNER";
exports.ERROR_NOT_ROOT                                 = "NOT_ROOT";
exports.ERROR_FAILED_TO_UNMARSHAL_DL_RECORD            = "FAILED_TO_UNMARSHAL_DL_RECORD";
exports.ERROR_WRONG_USER_IDENTITY                      = "WRONG_USER_IDENTITY";
exports.ERROR_TOO_MANY_CONTACT_INFO_UPDATED            = "TOO_MANY_CONTACT_INFO_UPDATED";
exports.ERROR_PHONE_UPDATE_LEADS_TO_INVALID_ACCOUNT    = "PHONE_UPDATE_LEADS_TO_INVALID_ACCOUNT";
exports.ERROR_EMAIL_UPDATE_LEADS_TO_INVALID_ACCOUNT    = "EMAIL_UPDATE_LEADS_TO_INVALID_ACCOUNT";
exports.ERROR_DUPLICATE_ACCOUNT_WITH_PHONE             = "DUPLICATE_ACCOUNT_WITH_PHONE";
exports.ERROR_DUPLICATE_ACCOUNT_WITH_EMAIL             = "DUPLICATE_ACCOUNT_WITH_EMAIL";
exports.ERROR_FAILED_TO_RETRIEVE_ACCOUNT_FROM_DB       = "FAILED_TO_RETRIEVE_ACCOUNT_FROM_DB";
exports.ERROR_FAILED_TO_SAVE_ACCOUNT_TO_DB             = "FAILED_TO_SAVE_ACCOUNT_TO_DB";
exports.ERROR_FAILED_TO_RETRIEVE_ACCOUNT_FROM_EPHEM_DB = "FAILED_TO_RETRIEVE_ACCOUNT_FROM_EPHEM_DB";
exports.ERROR_FAILED_TO_SAVE_ACCOUNT_TO_EPHEM_DB       = "FAILED_TO_SAVE_ACCOUNT_TO_EPHEM_DB";
exports.ERROR_FAILED_TO_MARSHAL_RECORD                 = "FAILED_TO_MARSHAL_RECORD";
exports.ERROR_DUPLICATE_ACCOUNT_WITH_LOGIN             = "DUPLICATE_ACCOUNT_WITH_LOGIN";
exports.ERROR_DUPLICATE_ACCOUNT_WITH_ID                = "DUPLICATE_ACCOUNT_WITH_ID";
exports.ERROR_DUPLICATE_ACCOUNT                        = "DUPLICATE_ACCOUNT";
exports.ERROR_FAILED_TO_MARSHAL_ACCOUNT                = "FAILED_TO_MARSHAL_ACCOUNT";
exports.ERROR_PENDING_ACTIVATION_MODULE_FAILED         = "PENDING_ACTIVATION_MODULE_FAILED";
exports.ERROR_UNVERIFIED_CONTACT_MODULE_FAILED         = "UNVERIFIED_CONTACT_MODULE_FAILED";
exports.ERROR_ACCOUNT_MISSING_PARAMETER                = "ACCOUNT_MISSING_PARAMETER";
exports.ERROR_ACCOUNT_BAD_FORMATTED_PARAMETER          = "ACCOUNT_BAD_FORMATTED_PARAMETER";
exports.ERROR_ACCOUNT_BAD_EPHEMERAL_PARAMETER_VALUE    = "ACCOUNT_BAD_EPHEMERAL_PARAMETER_VALUE";
exports.ERROR_EPHEMERAL_ACCOUNT_NOT_FOUND              = "EPHEMERAL_ACCOUNT_NOT_FOUND";
exports.ERROR_ACCOUNT_NOT_FOUND                        = "ACCOUNT_NOT_FOUND";
exports.ERROR_INVALID_ACCOUNT_CONTACT_PARAMETER        = "INVALID_ACCOUNT_CONTACT_PARAMETER";
exports.ERROR_EPHEMERAL_ACCOUNT_LIKELY_EXPIRED         = "EPHEMERAL_ACCOUNT_LIKELY_EXPIRED";
exports.ERROR_MISSING_MANDATORY_EMAIL_FIELD            = "MISSING_MANDATORY_EMAIL_FIELD";
exports.ERROR_NO_ACCOUNT_BOUND_WITH_CONTACT_FOUND      = "NO_ACCOUNT_BOUND_WITH_CONTACT_FOUND";
exports.ERROR_FAILED_TO_READ_PUBLIC_KEY                = "FAILED_TO_READ_PUBLIC_KEY";
exports.ERROR_FAILED_TO_READ_PRIVATE_KEY               = "FAILED_TO_READ_PRIVATE_KEY";
exports.ERROR_FAILED_TO_MARSHAL_CLAIMS                 = "FAILED_TO_MARSHAL_CLAIMS";
exports.ERROR_FAILED_TO_UNMARSHAL_CLAIMS               = "FAILED_TO_UNMARSHAL_CLAIMS";
exports.ERROR_FAILED_TO_MARSHAL_LOGIN_REQ              = "FAILED_TO_MARSHAL_LOGIN_REQ";
exports.ERROR_FAILED_TO_UNMARSHAL_LOGIN_RSP            = "FAILED_TO_UNMARSHAL_LOGIN_RSP";
exports.ERROR_FAILED_TO_MARSHAL_ACCESS_TOKEN           = "FAILED_TO_MARSHAL_ACCESS_TOKEN";
exports.ERROR_FAILED_TO_UNMARSHAL_ACCESS_TOKEN         = "FAILED_TO_UNMARSHAL_ACCESS_TOKEN";
exports.ERROR_FAILED_TO_EXTRACT_CLAIMS                 = "FAILED_TO_EXTRACT_CLAIMS";
exports.ERROR_FAILED_TO_PARSE_CLAIMS                   = "FAILED_TO_PARSE_CLAIMS";
exports.ERROR_MISSING_FIELDS_IN_CLAIMS                 = "MISSING_FIELDS_IN_CLAIMS";
exports.ERROR_AUDIENCE_MISMATCH                        = "AUDIENCE_MISMATCH";
exports.ERROR_JWT_IS_NOT_A_REFRESH_TOKEN               = "JWT_IS_NOT_A_REFRESH_TOKEN";
exports.ERROR_REFRESH_TOKEN_EXPIRED_OR_NOT_REGISTERED  = "REFRESH_TOKEN_EXPIRED_OR_NOT_REGISTERED";
exports.ERROR_FAILED_TO_INVALIDATE_HAWK_KEY            = "FAILED_TO_INVALIDATE_HAWK_KEY";
exports.ERROR_FAILED_TO_UNREGISTER_REFRESH_TOKEN       = "FAILED_TO_UNREGISTER_REFRESH_TOKEN";
exports.ERROR_FAILED_TO_INVALIDATE_SESSION             = "FAILED_TO_INVALIDATE_SESSION";

// FxAccounts has the ability to "split" the credentials between a plain-text
// JSON file in the profile dir and in the login manager.
// These constants relate to that.

// Error matching.
exports.SERVER_ERRNO_TO_ERROR = {};

for (let id in exports) {
  this[id] = exports[id];
}

// Allow this file to be imported via Components.utils.import().
this.EXPORTED_SYMBOLS = Object.keys(exports);

// Set these up now that everything has been loaded into |this|.
SERVER_ERRNO_TO_ERROR[ERRNO_ACCOUNT_DOES_NOT_EXIST]         = ERROR_ACCOUNT_DOES_NOT_EXIST;
SERVER_ERRNO_TO_ERROR[ERRNO_INCORRECT_PASSWORD]             = ERROR_INVALID_PASSWORD;
SERVER_ERRNO_TO_ERROR[ERRNO_INTERNAL_INVALID_USER]          = ERROR_INTERNAL_INVALID_USER;
SERVER_ERRNO_TO_ERROR[ERRNO_INVALID_ACCOUNTID]              = ERROR_INVALID_ACCOUNTID;
SERVER_ERRNO_TO_ERROR[ERRNO_UNVERIFIED_ACCOUNT]             = ERROR_UNVERIFIED_ACCOUNT;
SERVER_ERRNO_TO_ERROR[ERRNO_INVALID_VERIFICATION_CODE]      = ERROR_INVALID_VERIFICATION_CODE;
SERVER_ERRNO_TO_ERROR[ERRNO_ALREADY_VERIFIED]               = ERROR_ALREADY_VERIFIED;
SERVER_ERRNO_TO_ERROR[ERRNO_INVALID_AUTH_TOKEN]             = ERROR_INVALID_AUTH_TOKEN;
SERVER_ERRNO_TO_ERROR[ERRNO_MISSING_ID_IN_URL]                        = ERROR_MISSING_ID_IN_URL;
SERVER_ERRNO_TO_ERROR[ERRNO_EMPTY_PAYLOAD]                            = ERROR_EMPTY_PAYLOAD;
SERVER_ERRNO_TO_ERROR[ERRNO_PAYLOAD_CANNOT_BE_UNMARSHALLED]           = ERROR_PAYLOAD_CANNOT_BE_UNMARSHALLED;
SERVER_ERRNO_TO_ERROR[ERRNO_JWT_CANNOT_BE_RECOVERED]                  = ERROR_JWT_CANNOT_BE_RECOVERED;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_ATTACH_DATA_TO_REQUEST]         = ERROR_FAILED_TO_ATTACH_DATA_TO_REQUEST;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_ATTACH_DATA_TO_RESPONSE]        = ERROR_FAILED_TO_ATTACH_DATA_TO_RESPONSE;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_DETACH_DATA_FROM_REQUEST]       = ERROR_FAILED_TO_DETACH_DATA_FROM_REQUEST;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_DETACH_DATA_FROM_RESPONSE]      = ERROR_FAILED_TO_DETACH_DATA_FROM_RESPONSE;
SERVER_ERRNO_TO_ERROR[ERRNO_NATS_COMMUNICATION_FAILED]                = ERROR_NATS_COMMUNICATION_FAILED;
SERVER_ERRNO_TO_ERROR[ERRNO_JWT_BLACKLISTED]                          = ERROR_JWT_BLACKLISTED;
SERVER_ERRNO_TO_ERROR[ERRNO_DL_SUBSYSTEM_FAILED]                      = ERROR_DL_SUBSYSTEM_FAILED;
SERVER_ERRNO_TO_ERROR[ERRNO_NOT_PROPERLY_INITIALIZED]                 = ERROR_NOT_PROPERLY_INITIALIZED;
SERVER_ERRNO_TO_ERROR[ERRNO_WRONG_ACCOUNT_ID_FORMAT]                  = ERROR_WRONG_ACCOUNT_ID_FORMAT;
SERVER_ERRNO_TO_ERROR[ERRNO_WRONG_FIRST_NAME_FORMAT]                  = ERROR_WRONG_FIRST_NAME_FORMAT;
SERVER_ERRNO_TO_ERROR[ERRNO_WRONG_LAST_NAME_FORMAT]                   = ERROR_WRONG_LAST_NAME_FORMAT;
SERVER_ERRNO_TO_ERROR[ERRNO_WRONG_BIRTHDAY_FORMAT]                    = ERROR_WRONG_BIRTHDAY_FORMAT;
SERVER_ERRNO_TO_ERROR[ERRNO_WRONG_GENDER_FORMAT]                      = ERROR_WRONG_GENDER_FORMAT;
SERVER_ERRNO_TO_ERROR[ERRNO_WRONG_EMAIL_FORMAT]                       = ERROR_WRONG_EMAIL_FORMAT;
SERVER_ERRNO_TO_ERROR[ERRNO_MISSING_PASSWORD_FIELD]                   = ERROR_MISSING_PASSWORD_FIELD;
SERVER_ERRNO_TO_ERROR[ERRNO_DUPLICATED_EMAIL_VALUE]                   = ERROR_DUPLICATED_EMAIL_VALUE;
SERVER_ERRNO_TO_ERROR[ERRNO_WRONG_PHONE_FORMAT]                       = ERROR_WRONG_PHONE_FORMAT;
SERVER_ERRNO_TO_ERROR[ERRNO_MISSING_PHONE_PASSWORD_FIELD]             = ERROR_MISSING_PHONE_PASSWORD_FIELD;
SERVER_ERRNO_TO_ERROR[ERRNO_WRONG_SECOND_PHONE_FORMAT]                = ERROR_WRONG_SECOND_PHONE_FORMAT;
SERVER_ERRNO_TO_ERROR[ERRNO_MISSING_AT_LEAST_ONE_CONTACT_FIELD]       = ERROR_MISSING_AT_LEAST_ONE_CONTACT_FIELD;
SERVER_ERRNO_TO_ERROR[ERRNO_DUPLICATED_EMAIL_VALUE]                   = ERROR_DUPLICATED_EMAIL_VALUE;
SERVER_ERRNO_TO_ERROR[ERRNO_DUPLICATED_PHONE_VALUE]                   = ERROR_DUPLICATED_PHONE_VALUE;
SERVER_ERRNO_TO_ERROR[ERRNO_NOT_OWNER]                                = ERROR_NOT_OWNER;
SERVER_ERRNO_TO_ERROR[ERRNO_NOT_PARTNER]                              = ERROR_NOT_PARTNER;
SERVER_ERRNO_TO_ERROR[ERRNO_NOT_ROOT]                                 = ERROR_NOT_ROOT;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_UNMARSHAL_DL_RECORD]            = ERROR_FAILED_TO_UNMARSHAL_DL_RECORD;
SERVER_ERRNO_TO_ERROR[ERRNO_WRONG_USER_IDENTITY]                      = ERROR_WRONG_USER_IDENTITY;
SERVER_ERRNO_TO_ERROR[ERRNO_TOO_MANY_CONTACT_INFO_UPDATED]            = ERROR_TOO_MANY_CONTACT_INFO_UPDATED;
SERVER_ERRNO_TO_ERROR[ERRNO_PHONE_UPDATE_LEADS_TO_INVALID_ACCOUNT]    = ERROR_PHONE_UPDATE_LEADS_TO_INVALID_ACCOUNT;
SERVER_ERRNO_TO_ERROR[ERRNO_EMAIL_UPDATE_LEADS_TO_INVALID_ACCOUNT]    = ERROR_EMAIL_UPDATE_LEADS_TO_INVALID_ACCOUNT;
SERVER_ERRNO_TO_ERROR[ERRNO_DUPLICATE_ACCOUNT_WITH_PHONE]             = ERROR_DUPLICATE_ACCOUNT_WITH_PHONE;
SERVER_ERRNO_TO_ERROR[ERRNO_DUPLICATE_ACCOUNT_WITH_EMAIL]             = ERROR_DUPLICATE_ACCOUNT_WITH_EMAIL;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_RETRIEVE_ACCOUNT_FROM_DB]       = ERROR_FAILED_TO_RETRIEVE_ACCOUNT_FROM_DB;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_SAVE_ACCOUNT_TO_DB]             = ERROR_FAILED_TO_SAVE_ACCOUNT_TO_DB;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_RETRIEVE_ACCOUNT_FROM_EPHEM_DB] = ERROR_FAILED_TO_RETRIEVE_ACCOUNT_FROM_EPHEM_DB;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_SAVE_ACCOUNT_TO_EPHEM_DB]       = ERROR_FAILED_TO_SAVE_ACCOUNT_TO_EPHEM_DB;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_MARSHAL_RECORD]                 = ERROR_FAILED_TO_MARSHAL_RECORD;
SERVER_ERRNO_TO_ERROR[ERRNO_DUPLICATE_ACCOUNT_WITH_LOGIN]             = ERROR_DUPLICATE_ACCOUNT_WITH_LOGIN;
SERVER_ERRNO_TO_ERROR[ERRNO_DUPLICATE_ACCOUNT_WITH_ID]                = ERROR_DUPLICATE_ACCOUNT_WITH_ID;
SERVER_ERRNO_TO_ERROR[ERRNO_DUPLICATE_ACCOUNT]                        = ERROR_DUPLICATE_ACCOUNT;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_MARSHAL_ACCOUNT]                = ERROR_FAILED_TO_MARSHAL_ACCOUNT;
SERVER_ERRNO_TO_ERROR[ERRNO_PENDING_ACTIVATION_MODULE_FAILED]         = ERROR_PENDING_ACTIVATION_MODULE_FAILED;
SERVER_ERRNO_TO_ERROR[ERRNO_UNVERIFIED_CONTACT_MODULE_FAILED]         = ERROR_UNVERIFIED_CONTACT_MODULE_FAILED;
SERVER_ERRNO_TO_ERROR[ERRNO_ACCOUNT_MISSING_PARAMETER]                = ERROR_ACCOUNT_MISSING_PARAMETER;
SERVER_ERRNO_TO_ERROR[ERRNO_ACCOUNT_BAD_FORMATTED_PARAMETER]          = ERROR_ACCOUNT_BAD_FORMATTED_PARAMETER;
SERVER_ERRNO_TO_ERROR[ERRNO_ACCOUNT_BAD_EPHEMERAL_PARAMETER_VALUE]    = ERROR_ACCOUNT_BAD_EPHEMERAL_PARAMETER_VALUE;
SERVER_ERRNO_TO_ERROR[ERRNO_EPHEMERAL_ACCOUNT_NOT_FOUND]              = ERROR_EPHEMERAL_ACCOUNT_NOT_FOUND;
SERVER_ERRNO_TO_ERROR[ERRNO_ACCOUNT_NOT_FOUND]                        = ERROR_ACCOUNT_NOT_FOUND;
SERVER_ERRNO_TO_ERROR[ERRNO_INVALID_ACCOUNT_CONTACT_PARAMETER]        = ERROR_INVALID_ACCOUNT_CONTACT_PARAMETER;
SERVER_ERRNO_TO_ERROR[ERRNO_EPHEMERAL_ACCOUNT_LIKELY_EXPIRED]         = ERROR_EPHEMERAL_ACCOUNT_LIKELY_EXPIRED;
SERVER_ERRNO_TO_ERROR[ERRNO_MISSING_MANDATORY_EMAIL_FIELD]            = ERROR_MISSING_MANDATORY_EMAIL_FIELD;
SERVER_ERRNO_TO_ERROR[ERRNO_NO_ACCOUNT_BOUND_WITH_CONTACT_FOUND]      = ERROR_NO_ACCOUNT_BOUND_WITH_CONTACT_FOUND;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_READ_PUBLIC_KEY]                = ERROR_FAILED_TO_READ_PUBLIC_KEY;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_READ_PRIVATE_KEY]               = ERROR_FAILED_TO_READ_PRIVATE_KEY;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_MARSHAL_CLAIMS]                 = ERROR_FAILED_TO_MARSHAL_CLAIMS;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_UNMARSHAL_CLAIMS]               = ERROR_FAILED_TO_UNMARSHAL_CLAIMS;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_MARSHAL_LOGIN_REQ]              = ERROR_FAILED_TO_MARSHAL_LOGIN_REQ;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_UNMARSHAL_LOGIN_RSP]            = ERROR_FAILED_TO_UNMARSHAL_LOGIN_RSP;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_MARSHAL_ACCESS_TOKEN]           = ERROR_FAILED_TO_MARSHAL_ACCESS_TOKEN;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_UNMARSHAL_ACCESS_TOKEN]         = ERROR_FAILED_TO_UNMARSHAL_ACCESS_TOKEN;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_EXTRACT_CLAIMS]                 = ERROR_FAILED_TO_EXTRACT_CLAIMS;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_PARSE_CLAIMS]                   = ERROR_FAILED_TO_PARSE_CLAIMS;
SERVER_ERRNO_TO_ERROR[ERRNO_MISSING_FIELDS_IN_CLAIMS]                 = ERROR_MISSING_FIELDS_IN_CLAIMS;
SERVER_ERRNO_TO_ERROR[ERRNO_AUDIENCE_MISMATCH]                        = ERROR_AUDIENCE_MISMATCH;
SERVER_ERRNO_TO_ERROR[ERRNO_JWT_IS_NOT_A_REFRESH_TOKEN]               = ERROR_JWT_IS_NOT_A_REFRESH_TOKEN;
SERVER_ERRNO_TO_ERROR[ERRNO_REFRESH_TOKEN_EXPIRED_OR_NOT_REGISTERED]  = ERROR_REFRESH_TOKEN_EXPIRED_OR_NOT_REGISTERED;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_INVALIDATE_HAWK_KEY]            = ERROR_FAILED_TO_INVALIDATE_HAWK_KEY;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_UNREGISTER_REFRESH_TOKEN]       = ERROR_FAILED_TO_UNREGISTER_REFRESH_TOKEN;
SERVER_ERRNO_TO_ERROR[ERRNO_FAILED_TO_INVALIDATE_SESSION]             = ERROR_FAILED_TO_INVALIDATE_SESSION;
