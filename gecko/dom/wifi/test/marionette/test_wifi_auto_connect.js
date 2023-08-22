/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const TESTING_HOSTAPD = [{ ssid: 'ap0' }];

gTestSuite.doTestWithoutStockAp(function() {
  let firstNetwork;
  return gTestSuite.ensureWifiEnabled(true)
    // Start custom hostapd for testing.
    .then(() => gTestSuite.startHostapds(TESTING_HOSTAPD))
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', TESTING_HOSTAPD.length))
    // Request the first scan.
    .then(() => gTestSuite.testWifiScanWithRetry(5, TESTING_HOSTAPD))
    .then(function(networks) {
      for(var net in networks) {
        let network = networks[net];
        if (!gTestSuite.isStockSSID(network.ssid))
          continue;

        firstNetwork = network;
        return gTestSuite.testAssociate(firstNetwork);
      }

      throw "Can't find correct network";
    })
    // Note that due to CORE-3134, we need to re-start testing hostapd
    // after wifi has been re-enabled.

    // Disable wifi and kill running hostapd.
    .then(() => gTestSuite.requestWifiEnabled(false))
    .then(gTestSuite.killAllHostapd)
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', 0))
    // Re-enable wifi.
    .then(() => gTestSuite.requestWifiEnabled(true))
    // Restart hostapd.
    .then(() => gTestSuite.startHostapds(TESTING_HOSTAPD))
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', TESTING_HOSTAPD.length))
    // Wait for connection automatically.
    .then(() => gTestSuite.waitForConnected(firstNetwork))
    // Kill running hostapd.
    .then(gTestSuite.killAllHostapd)
    .then(() => gTestSuite.verifyNumOfProcesses('hostapd', 0));
});
