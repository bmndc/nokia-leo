/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-*/
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

dictionary HawkRestrictedToken
{
  // restricted access token stored as JWT (JSON Web Token)
  DOMString   accessToken = "";
  // type of this token, e.g. "hawk", bearer"
  DOMString   tokenType = "";
  // scope of this token, e.g. "core"
  DOMString   scope = "";
  // the key id of Hawk
  DOMString   kid = "";
  // the key to be used and matching the kid above, usually encoded in base64.
  DOMString   macKey = "";
  // the message authentication code (MAC) algorithm of Hawk,
  DOMString   macAlgorithm = "hmac-sha-256";
  // time to live of the current credential (in seconds)
  long long   expiresInSeconds = -1;
};

[JSImplementation="@mozilla.org/kaiauth/authorization-manager;1",
 NoInterfaceObject,
 NavigatorProperty="kaiAuth",
 CheckAnyPermissions="cloud-authorization",
 Pref="dom.kaiauth.enabled"]
interface AuthorizationManager {
  /**
   * Get a Hawk restricted token from cloud server via HTTPS.
   *
   * @param type
   *        Specify the cloud service you're asking for authorization
   * @return A promise object.
   *  Resolve params: a HawkRestrictedToken object represents an access token
   *  Reject params:  a integer represents HTTP status code
   */
  Promise<HawkRestrictedToken> getRestrictedToken(KaiServiceType type);
};

/**
 * Types of cloud service which require access token.
 * The uri (end point) and API key of individual service are configured by
 * Gecko Preference.
 */
enum KaiServiceType
{
  "apps",     // configured by apps.token.uri, apps.authorization.key
  "metrics"	  // configured by metrics.token.uri, metrics.authorization.key
};
