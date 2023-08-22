/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

const PREF_PERMISSION_FAKE = "media.navigator.permission.fake";

function _mm() {
  return gBrowser.selectedBrowser.messageManager;
}

function promiseObserverCalled(aTopic) {
  return new Promise(resolve => {
    let mm = _mm();
    mm.addMessageListener("Test:ObserverCalled", function listener({data}) {
      if (data == aTopic) {
        ok(true, "got " + aTopic + " notification");
        mm.removeMessageListener("Test:ObserverCalled", listener);
        resolve();
      }
    });
    mm.sendAsyncMessage("Test:WaitForObserverCall", aTopic);
  });
}

function expectObserverCalled(aTopic) {
  return new Promise(resolve => {
    let mm = _mm();
    mm.addMessageListener("Test:ExpectObserverCalled:Reply",
                          function listener({data}) {
      is(data.count, 1, "expected notification " + aTopic);
      mm.removeMessageListener("Test:ExpectObserverCalled:Reply", listener);
      resolve();
    });
    mm.sendAsyncMessage("Test:ExpectObserverCalled", aTopic);
  });
}

function expectNoObserverCalled() {
  return new Promise(resolve => {
    let mm = _mm();
    mm.addMessageListener("Test:ExpectNoObserverCalled:Reply",
                          function listener({data}) {
      mm.removeMessageListener("Test:ExpectNoObserverCalled:Reply", listener);
      for (let topic in data) {
        if (data[topic])
          is(data[topic], 0, topic + " notification unexpected");
      }
      resolve();
    });
    mm.sendAsyncMessage("Test:ExpectNoObserverCalled");
  });
}

function promiseTodoObserverNotCalled(aTopic) {
  return new Promise(resolve => {
    let mm = _mm();
    mm.addMessageListener("Test:TodoObserverNotCalled:Reply",
                          function listener({data}) {
      mm.removeMessageListener("Test:TodoObserverNotCalled:Reply", listener);
      resolve(data.count);
    });
    mm.sendAsyncMessage("Test:TodoObserverNotCalled", aTopic);
  });
}

function promiseMessage(aMessage, aAction) {
  let deferred = Promise.defer();

  content.addEventListener("message", function messageListener(event) {
    content.removeEventListener("message", messageListener);
    is(event.data, aMessage, "received " + aMessage);
    if (event.data == aMessage)
      deferred.resolve();
    else
      deferred.reject();
  });

  if (aAction)
    aAction();

  return deferred.promise;
}

function promisePopupNotificationShown(aName, aAction) {
  let deferred = Promise.defer();

  PopupNotifications.panel.addEventListener("popupshown", function popupNotifShown() {
    PopupNotifications.panel.removeEventListener("popupshown", popupNotifShown);

    ok(!!PopupNotifications.getNotification(aName), aName + " notification shown");
    ok(PopupNotifications.isPanelOpen, "notification panel open");
    ok(!!PopupNotifications.panel.firstChild, "notification panel populated");

    deferred.resolve();
  });

  if (aAction)
    aAction();

  return deferred.promise;
}

function promisePopupNotification(aName) {
  let deferred = Promise.defer();

  waitForCondition(() => PopupNotifications.getNotification(aName),
                   () => {
    ok(!!PopupNotifications.getNotification(aName),
       aName + " notification appeared");

    deferred.resolve();
  }, "timeout waiting for popup notification " + aName);

  return deferred.promise;
}

function promiseNoPopupNotification(aName) {
  let deferred = Promise.defer();

  waitForCondition(() => !PopupNotifications.getNotification(aName),
                   () => {
    ok(!PopupNotifications.getNotification(aName),
       aName + " notification removed");
    deferred.resolve();
  }, "timeout waiting for popup notification " + aName + " to disappear");

  return deferred.promise;
}

const kActionAlways = 1;
const kActionDeny = 2;
const kActionNever = 3;

function activateSecondaryAction(aAction) {
  let notification = PopupNotifications.panel.firstChild;
  notification.button.focus();
  let popup = notification.menupopup;
  popup.addEventListener("popupshown", function () {
    popup.removeEventListener("popupshown", arguments.callee, false);

    // Press 'down' as many time as needed to select the requested action.
    while (aAction--)
      EventUtils.synthesizeKey("VK_DOWN", {});

    // Activate
    EventUtils.synthesizeKey("VK_RETURN", {});
  }, false);

  // One down event to open the popup
  EventUtils.synthesizeKey("VK_DOWN",
                           { altKey: !navigator.platform.includes("Mac") });
}

function getMediaCaptureState() {
  return new Promise(resolve => {
    let mm = _mm();
    mm.addMessageListener("Test:MediaCaptureState", ({data}) => {
      resolve(data);
    });
    mm.sendAsyncMessage("Test:GetMediaCaptureState");
  });
}

function promiseRequestDevice(aRequestAudio, aRequestVideo, aFrameId, aType) {
  info("requesting devices");
  return ContentTask.spawn(gBrowser.selectedBrowser,
                           {aRequestAudio, aRequestVideo, aFrameId, aType},
                           function*(args) {
    let global = content.wrappedJSObject;
    if (args.aFrameId)
      global = global.document.getElementById(args.aFrameId).contentWindow;
    global.requestDevice(args.aRequestAudio, args.aRequestVideo, args.aType);
  });
}

function* closeStream(aAlreadyClosed, aFrameId) {
  yield expectNoObserverCalled();

  let promise;
  if (!aAlreadyClosed)
    promise = promiseObserverCalled("recording-device-events");

  info("closing the stream");
  yield ContentTask.spawn(gBrowser.selectedBrowser, aFrameId, function*(aFrameId) {
    let global = content.wrappedJSObject;
    if (aFrameId)
      global = global.document.getElementById(aFrameId).contentWindow;
    global.closeStream();
  });

  if (!aAlreadyClosed)
    yield promise;

  yield promiseNoPopupNotification("webRTC-sharingDevices");
  if (!aAlreadyClosed)
    yield expectObserverCalled("recording-window-ended");

  yield* assertWebRTCIndicatorStatus(null);
}

function checkDeviceSelectors(aAudio, aVideo) {
  let micSelector = document.getElementById("webRTC-selectMicrophone");
  if (aAudio)
    ok(!micSelector.hidden, "microphone selector visible");
  else
    ok(micSelector.hidden, "microphone selector hidden");

  let cameraSelector = document.getElementById("webRTC-selectCamera");
  if (aVideo)
    ok(!cameraSelector.hidden, "camera selector visible");
  else
    ok(cameraSelector.hidden, "camera selector hidden");
}

function* checkSharingUI(aExpected) {
  yield promisePopupNotification("webRTC-sharingDevices");

  yield* assertWebRTCIndicatorStatus(aExpected);
}

function* checkNotSharing() {
  is((yield getMediaCaptureState()), "none", "expected nothing to be shared");

  ok(!PopupNotifications.getNotification("webRTC-sharingDevices"),
     "no webRTC-sharingDevices popup notification");

  yield* assertWebRTCIndicatorStatus(null);
}
