/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko.background.fxa;

import org.mozilla.gecko.background.fxa.FxAccountClient20.RequestDelegate;
import org.mozilla.gecko.background.fxa.FxAccountClient20.StatusResponse;
import org.mozilla.gecko.background.fxa.FxAccountClient20.TwoKeys;
import org.mozilla.gecko.sync.ExtendedJSONObject;

public interface FxAccountClient {
  public void status(byte[] sessionToken, RequestDelegate<StatusResponse> requestDelegate);
  public void keys(byte[] keyFetchToken, RequestDelegate<TwoKeys> requestDelegate);
  public void sign(byte[] sessionToken, ExtendedJSONObject publicKey, long certificateDurationInMilliseconds, RequestDelegate<String> requestDelegate);
}
