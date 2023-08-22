/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

[JSImplementation="@mozilla.org/account/account-manager;1",
 NoInterfaceObject,
 NavigatorProperty="accountManager",
 AvailableIn="CertifiedApps",
 Pref="dom.accountManager.enabled"]
interface AccountManager : EventTarget {
  Promise<sequence<Authenticator>> getAuthenticators();
  Promise<sequence<Account>> getAccounts();
  Promise<Account> showLoginPage(Authenticator authenticator, optional object extraInfo);
  Promise<DOMString> reauthenticate(Account account, optional object extraInfo);
  Promise<any> getCredential(Account account, optional GetCredentialOptions options);
  Promise<DOMString> logout(Account account);

  attribute EventHandler onchanged;
};

dictionary Authenticator {
  required DOMString authenticatorId;
};

dictionary Account {
  required DOMString accountId;
  required DOMString authenticatorId;
};

dictionary GetCredentialOptions {
  boolean refreshCredential;
};
