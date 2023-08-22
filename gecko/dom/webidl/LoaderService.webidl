/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

[Pref="dom.secureelement.enabled",
 CheckAnyPermissions="secureelement-manage",
 AvailableIn="CertifiedApps",
 NavigatorProperty="loaderService",
 JSImplementation="@mozilla.org/loaderservice/lsscript;1",
 NoInterfaceObject]
interface LoaderService {
  /**
   * Loader service is used to update applets within secure element. The loader service application
   * receives applet binary from specific server, the binary is saved to the LsScriptFile file using
   * device storage APIs. LsExecuteScript API notified NFC hal layer where to get the applet,
   * and starts applet upgrade. The upgrade report is saved in the LsResponseFile file.
   *
   * @param LsScriptFile
   *     Full path name for where the applet is stored.
   *
   * @param LsResponseFile
   *     Full path name for where the upgrade report is stored.
   *
   * @return the promise is resolved or rejected with the new created
   * LoaderServiceResponse object.
   */
  [Throws]
  Promise<LoaderServiceResponse> lsExecuteScript(DOMString LsScriptFile, DOMString LsResponseFile);

  /**
   * Get applets version
   *
   *
   * @return the promise is resolved or rejected with the new created
   * SEResponse object.
   */
  [Throws]
  Promise<LoaderServiceResponse> lsGetVersion();
};

[Pref="dom.secureelement.enabled",
 CheckAnyPermissions="secureelement-manage",
 AvailableIn="CertifiedApps",
 JSImplementation="@mozilla.org/loaderservice/response;1"]
interface LoaderServiceResponse {
  [Cached, Pure] readonly attribute sequence<octet>?  data;
};

