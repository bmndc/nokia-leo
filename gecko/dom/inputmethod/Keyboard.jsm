/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

'use strict';

this.EXPORTED_SYMBOLS = ['Keyboard'];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import('resource://gre/modules/Services.jsm');
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
  "@mozilla.org/parentprocessmessagemanager;1", "nsIMessageBroadcaster");

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy",
                                  "resource://gre/modules/SystemAppProxy.jsm");

XPCOMUtils.defineLazyGetter(this, "appsService", function() {
  return Cc["@mozilla.org/AppsService;1"].getService(Ci.nsIAppsService);
});

XPCOMUtils.defineLazyGetter(this, "KeyboardAppProxy", function() {
  return Cc["@acadine.com/keyboardAppProxy;1"].getService(
    Ci.nsIKeyboardAppProxy);
});


var Utils = {
  getMMFromMessage: function u_getMMFromMessage(msg) {
    let mm;
    try {
      mm = msg.target.QueryInterface(Ci.nsIFrameLoaderOwner)
                     .frameLoader.messageManager;
    } catch(e) {
      mm = msg.target;
    }

    return mm;
  },
  checkPermissionForMM: function u_checkPermissionForMM(mm, permName) {
    return mm.assertPermission(permName);
  }
};

this.Keyboard = {
  _formMM: null,      // The current web page message manager.
  _keyboardMM: null,  // The keyboard app message manager.
  _keyboardID: -1,    // The keyboard app's ID number. -1 = invalid
  _nextKeyboardID: 0, // The ID number counter.
  _systemMMs: [],     // The message managers registered to handle system async
                      // messages.
  _supportsSwitchingTypes: [],
  _systemMessageNames: [
    'SetValue', 'RemoveFocus', 'SetSelectedOption', 'SetSelectedOptions',
    'SetSupportsSwitchingTypes', 'RegisterSync', 'Unregister',
    'ClearAll'
  ],

  _messageNames: [
    'RemoveFocus',
    'SetSelectionRange', 'ReplaceSurroundingText', 'ShowInputMethodPicker',
    'SwitchToNextInputMethod', 'SwitchToPreviousInputMethod', 'HideInputMethod',
    'SendKey', 'GetContext',
    'SetComposition', 'EndComposition',
    'RegisterSync', 'Unregister',
    'ReplyHardwareKeyEvent', 'DeleteBackward'
  ],

  get formMM() {
    if (this._formMM && !Cu.isDeadWrapper(this._formMM))
      return this._formMM;

    return null;
  },

  set formMM(mm) {
    this._formMM = mm;
  },

  sendToForm: function(name, data) {
    if (!this.formMM) {
      dump("Keyboard.jsm: Attempt to send message " + name +
        " to form but no message manager exists.\n");

      return;
    }
    try {
      this.formMM.sendAsyncMessage(name, data);
    } catch(e) { }
  },

  sendToKeyboard: function(name, data) {
    try {
      this._keyboardMM.sendAsyncMessage(name, data);
    } catch(e) { }
  },

  sendToSystem: function(name, data) {
    if (!this._systemMMs.length) {
      dump("Keyboard.jsm: Attempt to send message " + name +
        " to system but no message manager registered.\n");

      return;
    }

    this._systemMMs.forEach((mm, i) => {
      try {
        data.inputManageId = i;
        mm.sendAsyncMessage(name, data);
      } catch(e) { }
    });
  },

  init: function keyboardInit() {
    Services.obs.addObserver(this, 'inprocess-browser-shown', false);
    Services.obs.addObserver(this, 'remote-browser-shown', false);
    Services.obs.addObserver(this, 'oop-frameloader-crashed', false);
    Services.obs.addObserver(this, 'message-manager-close', false);

    for (let name of this._messageNames) {
      ppmm.addMessageListener('Keyboard:' + name, this);
    }

    for (let name of this._systemMessageNames) {
      ppmm.addMessageListener('System:' + name, this);
    }

    this.inputRegistryGlue = new InputRegistryGlue();

    KeyboardAppProxy.keyboardEventForwarder(
      this._onKeyboardEventReceived.bind(this));
  },

  _onKeyboardEventReceived: function _onKeyboardEventReceived(
    eventType, keyCode, charCode, key, timeStamp, generation) {
    this.sendToKeyboard('Keyboard:SendHardwareKeyEvent', {
      eventType: eventType,
      keyCode: keyCode,
      charCode: charCode,
      key: key,
      timeStamp: timeStamp,
      generation: generation
    });
  },

  unregisterKeyboard: function unregisterKeyboard() {
    KeyboardAppProxy.isIMEActive = false;
    this._keyboardMM = null;
    this._keyboardID = -1;
  },

  observe: function keyboardObserve(subject, topic, data) {
    let frameLoader = null;
    let mm = null;

    if (topic == 'message-manager-close') {
      mm = subject;
    } else {
      frameLoader = subject.QueryInterface(Ci.nsIFrameLoader);
      mm = frameLoader.messageManager;
    }

    if (topic == 'oop-frameloader-crashed' ||
        topic == 'message-manager-close') {
      if (this.formMM == mm) {
        // The application has been closed unexpectingly. Let's tell the
        // keyboard app that the focus has been lost.
        this.sendToKeyboard('Keyboard:Blur', {});
        // Notify system app to hide keyboard.
        this.sendToSystem('System:Blur', {});
        // XXX: To be removed when content migrate away from mozChromeEvents.
        SystemAppProxy.dispatchEvent({
          type: 'inputmethod-contextchange',
          inputType: 'blur'
        });

        this.formMM = null;
      } else {
        for (var i = 0; i < this._systemMMs.length; ++i) {
          // Keyboard app has been closed unexpectedly. We should do the
          // same thing when receiving System:Unregister.
          if (this._systemMMs[i] == mm) {
            this._systemMMs.splice(i, 1);
            this.formMM = null;
            this.unregisterKeyboard();
            break;
          }
        }
      }
    } else {
      // Ignore notifications that aren't from a BrowserOrApp
      if (!frameLoader.ownerIsMozBrowserOrAppFrame) {
        return;
      }
      this.initFormsFrameScript(mm);
    }
  },

  initFormsFrameScript: function(mm) {
    mm.addMessageListener('Forms:Focus', this);
    mm.addMessageListener('Forms:Blur', this);
    mm.addMessageListener('Forms:SelectionChange', this);
    mm.addMessageListener('Forms:SetSelectionRange:Result:OK', this);
    mm.addMessageListener('Forms:SetSelectionRange:Result:Error', this);
    mm.addMessageListener('Forms:ReplaceSurroundingText:Result:OK', this);
    mm.addMessageListener('Forms:ReplaceSurroundingText:Result:Error', this);
    mm.addMessageListener('Forms:SendKey:Result:OK', this);
    mm.addMessageListener('Forms:SendKey:Result:Error', this);
    mm.addMessageListener('Forms:SequenceError', this);
    mm.addMessageListener('Forms:GetContext:Result:OK', this);
    mm.addMessageListener('Forms:SetComposition:Result:OK', this);
    mm.addMessageListener('Forms:EndComposition:Result:OK', this);
    mm.addMessageListener('Forms:DeleteBackward:Result:OK', this);
    mm.addMessageListener('Forms:DeleteBackward:Result:Error', this);
  },

  receiveMessage: function keyboardReceiveMessage(msg) {
    // If we get a 'Keyboard:XXX'/'System:XXX' message, check that the sender
    // has the required permission.
    let mm;

    // Assert the permission based on the prefix of the message.
    let permName;
    if (msg.name.startsWith("Keyboard:")) {
      permName = "input";
    } else if (msg.name.startsWith("System:")) {
      permName = "input-manage";
    }

    // There is no permission to check (nor we need to get the mm)
    // for Form: messages.
    if (permName) {
      mm = Utils.getMMFromMessage(msg);
      if (!mm) {
        dump("Keyboard.jsm: Message " + msg.name + " has no message manager.");
        return;
      }
      if (!Utils.checkPermissionForMM(mm, permName)) {
        dump("Keyboard.jsm: Message " + msg.name +
          " from a content process with no '" + permName + "' privileges.\n");
        return;
      }
    }

    // We'd like to reply back the hardware key even if keyboard has destroyed.
    // That says, if keyboard is unregisterd at keydown, we will still reply
    // gecko the keyup of that key, and gecko will dispatch that keyup as usual.
    if (msg.name === "Keyboard:ReplyHardwareKeyEvent") {
      KeyboardAppProxy.replyKey(msg.data.eventType, msg.data.preventDefaulted,
        msg.data.generation);
      return;
    }

    // we don't process kb messages (other than register)
    // if they come from a kb that we're currently not regsitered for.
    // this decision is made with the kbID kept by us and kb app
    let kbID = null;
    if ('kbID' in msg.data) {
      kbID = msg.data.kbID;
    }

    if (0 === msg.name.indexOf('Keyboard:') &&
        ('Keyboard:RegisterSync' !== msg.name && this._keyboardID !== kbID)
       ) {
      return;
    }

    switch (msg.name) {
      case 'Forms:Focus':
        this.handleFocus(msg);
        break;
      case 'Forms:Blur':
        this.handleBlur(msg);
        break;
      case 'Forms:SelectionChange':
      case 'Forms:SetSelectionRange:Result:OK':
      case 'Forms:ReplaceSurroundingText:Result:OK':
      case 'Forms:SendKey:Result:OK':
      case 'Forms:SendKey:Result:Error':
      case 'Forms:SequenceError':
      case 'Forms:GetContext:Result:OK':
      case 'Forms:SetComposition:Result:OK':
      case 'Forms:EndComposition:Result:OK':
      case 'Forms:SetSelectionRange:Result:Error':
      case 'Forms:ReplaceSurroundingText:Result:Error':
      case 'Forms:DeleteBackward:Result:OK':
      case 'Forms:DeleteBackward:Result:Error':
        let name = msg.name.replace(/^Forms/, 'Keyboard');
        this.forwardEvent(name, msg).then(() => {
        }, (reason) => {
          dump("Keyboard.jsm Error: " + reason);
        });
        break;
      case 'System:SetValue':
        this.setValue(msg);
        break;
      case 'System:ClearAll':
        this.clearAll(msg);
        break;
      case 'Keyboard:RemoveFocus':
      case 'System:RemoveFocus':
        this.removeFocus();
        break;
      case 'System:RegisterSync': {
        if (this._systemMMs.length !== 0) {
          dump('Keyboard.jsm Warning: There are more than one content page ' +
            'with input-manage permission. There will be undeterministic ' +
            'responses to addInput()/removeInput() if both content pages are ' +
            'trying to respond to the same request event.\n');
        }

        let id = this._systemMMs.length;
        this._systemMMs.push(mm);

        return id;
      }

      case 'System:Unregister':
        this._systemMMs.splice(msg.data.id, 1);

        break;
      case 'System:SetSelectedOption':
        this.setSelectedOption(msg);
        break;
      case 'System:SetSelectedOptions':
        this.setSelectedOption(msg);
        break;
      case 'System:SetSupportsSwitchingTypes':
        this.setSupportsSwitchingTypes(msg);
        break;
      case 'Keyboard:SetSelectionRange':
        this.setSelectionRange(msg);
        break;
      case 'Keyboard:ReplaceSurroundingText':
        this.replaceSurroundingText(msg);
        break;
      case 'Keyboard:SwitchToNextInputMethod':
        this.switchToNextInputMethod();
        break;
      case 'Keyboard:SwitchToPreviousInputMethod':
        this.switchToPreviousInputMethod();
        break;
      case 'Keyboard:ShowInputMethodPicker':
        this.showInputMethodPicker();
        break;
      case 'Keyboard:SendKey':
        this.sendKey(msg);
        break;
      case 'Keyboard:GetContext':
        this.getContext(msg);
        break;
      case 'Keyboard:SetComposition':
        this.setComposition(msg);
        break;
      case 'Keyboard:EndComposition':
        this.endComposition(msg);
        break;
      case 'Keyboard:DeleteBackward':
        this.deleteBackward(msg);
        break;
      case 'Keyboard:RegisterSync':
        KeyboardAppProxy.isIMEActive = true;
        this._keyboardMM = mm;
        if (kbID) {
          // keyboard identifies itself, use its kbID
          // this msg would be async, so no need to return
          this._keyboardID = kbID;
        }else{
          // generate the id for the keyboard
          this._keyboardID = this._nextKeyboardID;
          this._nextKeyboardID++;
          // this msg is sync,
          // and we want to return the id back to inputmethod
          return this._keyboardID;
        }
        break;
      case 'Keyboard:Unregister':
        this.unregisterKeyboard();
        break;
    }
  },

  handleFocus: function keyboardHandleFocus(msg) {
    // Set the formMM to the new message manager received.
    let mm = msg.target.QueryInterface(Ci.nsIFrameLoaderOwner)
                .frameLoader.messageManager;
    // Prevent sending focus event to keyboard if input context is currently
    // focused.
    if (mm === this.formMM) {
      return;
    }
    this.formMM = mm;

    // Since focus element is changed, to check whether this element
    // is from an mozApp window or not.
    let appManifest = msg.target.appManifestURL;
    msg.data.isFromApp = appManifest !== '';
    msg.data.manifestURL = appManifest;
    msg.data.appName = msg.target.src;

    // Notify the current active input app to gain focus.
    this.forwardEvent('Keyboard:Focus', msg).then(() => {
      // Notify System app, used also to render value selectors for now;
      // that's why we need the info about choices / min / max here as well...
      this.sendToSystem('System:Focus', msg.data);

      // XXX: To be removed when content migrate away from mozChromeEvents.
      SystemAppProxy.dispatchEvent({
        type: 'inputmethod-contextchange',
        inputType: msg.data.inputType,
        inputMode: msg.data.inputMode,
        voiceInputSupported: msg.data.voiceInputSupported,
        value: msg.data.value,
        choices: JSON.stringify(msg.data.choices),
        min: msg.data.min,
        max: msg.data.max,
        name: msg.data.name
      });
    }, (reason) => {
      dump("Keyboard.jsm Error: handleFocus is rejected by: " + reason);
    });
  },

  handleBlur: function keyboardHandleBlur(msg) {
    let mm = msg.target.QueryInterface(Ci.nsIFrameLoaderOwner)
                .frameLoader.messageManager;
    // A blur message can't be sent to the keyboard if the focus has
    // already been taken away at first place.
    // This check is here to prevent problem caused by out-of-order
    // ipc messages from two processes.
    if (mm !== this.formMM) {
      return;
    }

    msg.data.isFromApp = null;
    msg.data.defaultSoftkeyBar = null;
    msg.data.manifestURL = null;
    msg.data.appName = null;

    // unset formMM
    this.formMM = null;

    this.forwardEvent('Keyboard:Blur', msg).then(() => {
      this.sendToSystem('System:Blur', {});

      // XXX: To be removed when content migrate away from mozChromeEvents.
      SystemAppProxy.dispatchEvent({
        type: 'inputmethod-contextchange',
        inputType: 'blur'
      });
    }, (reason) => {
      dump("Keyboard.jsm Error: handleBlur is rejected by: " + reason);
    });
  },

  forwardEvent: function keyboardForwardEvent(newEventName, msg) {
    let shouldGetAppManifest = false;
    let appManifest;
    if (newEventName === 'Keyboard:GetContext:Result:OK') {
      appManifest = msg.target.appManifestURL;
      msg.data.isFromApp = appManifest !== '';
      msg.data.manifestURL = appManifest;
      msg.data.appName = msg.target.src;
      if (appManifest && appManifest.length) {
        shouldGetAppManifest = true;
      }
    }

    if (shouldGetAppManifest) {
      return appsService.getManifestFor(appManifest).then((manifest) => {
        msg.data.defaultSoftkeyBar = manifest.ime_default_softkey_bar === true;
        // If it is an App use name instead of src.
        msg.data.appName = manifest.name;
        this.sendToKeyboard(newEventName, msg.data);
      }, (reason) => {
        return Promise.reject('Failed in appsService.getManifestFor.')
      });
    } else {
      this.sendToKeyboard(newEventName, msg.data);
      return Promise.resolve(newEventName + ' has sent to Keyboard.');
    }
  },

  setSelectedOption: function keyboardSetSelectedOption(msg) {
    this.sendToForm('Forms:Select:Choice', msg.data);
  },

  setSelectedOptions: function keyboardSetSelectedOptions(msg) {
    this.sendToForm('Forms:Select:Choice', msg.data);
  },

  setSelectionRange: function keyboardSetSelectionRange(msg) {
    this.sendToForm('Forms:SetSelectionRange', msg.data);
  },

  setValue: function keyboardSetValue(msg) {
    this.sendToForm('Forms:Input:Value', msg.data);
  },

  removeFocus: function keyboardRemoveFocus() {
    if (!this.formMM) {
      return;
    }

    this.sendToForm('Forms:Select:Blur', {});
  },

  replaceSurroundingText: function keyboardReplaceSurroundingText(msg) {
    this.sendToForm('Forms:ReplaceSurroundingText', msg.data);
  },

  clearAll: function keyboardClearAll(msg) {
    this.sendToForm('Forms:ClearAll', msg.data);
  },

  deleteBackward: function keyboardDeleteBackward(msg) {
    this.sendToForm('Forms:DeleteBackward', msg.data);
  },

  showInputMethodPicker: function keyboardShowInputMethodPicker() {
    this.sendToSystem('System:ShowAll', {});

    // XXX: To be removed with mozContentEvent support from shell.js
    SystemAppProxy.dispatchEvent({
      type: "inputmethod-showall"
    });
  },

  switchToNextInputMethod: function keyboardSwitchToNextInputMethod() {
    this.sendToSystem('System:Next', {});

    // XXX: To be removed with mozContentEvent support from shell.js
    SystemAppProxy.dispatchEvent({
      type: "inputmethod-next"
    });
  },

  switchToPreviousInputMethod: function keyboardSwitchToPreviousInputMethod() {
    SystemAppProxy.dispatchEvent({
      type: "inputmethod-previous"
    });
  },

  sendKey: function keyboardSendKey(msg) {
    this.sendToForm('Forms:Input:SendKey', msg.data);
  },

  getContext: function keyboardGetContext(msg) {
    if (!this.formMM) {
      return;
    }

    this.sendToKeyboard('Keyboard:SupportsSwitchingTypesChange', {
      types: this._supportsSwitchingTypes
    });

    this.sendToForm('Forms:GetContext', msg.data);
  },

  setComposition: function keyboardSetComposition(msg) {
    this.sendToForm('Forms:SetComposition', msg.data);
  },

  endComposition: function keyboardEndComposition(msg) {
    this.sendToForm('Forms:EndComposition', msg.data);
  },

  setSupportsSwitchingTypes: function setSupportsSwitchingTypes(msg) {
    this._supportsSwitchingTypes = msg.data.types;
    this.sendToKeyboard('Keyboard:SupportsSwitchingTypesChange', msg.data);
  },
  // XXX: To be removed with mozContentEvent support from shell.js
  setLayouts: function keyboardSetLayouts(layouts) {
    // The input method plugins may not have loaded yet,
    // cache the layouts so on init we can respond immediately instead
    // of going back and forth between keyboard_manager
    var types = [];

    Object.keys(layouts).forEach((type) => {
      if (layouts[type] > 1) {
        types.push(type);
      }
    });

    this._supportsSwitchingTypes = types;

    this.sendToKeyboard('Keyboard:SupportsSwitchingTypesChange', {
      types: types
    });
  }
};

function InputRegistryGlue() {
  this._messageId = 0;
  this._msgMap = new Map();

  ppmm.addMessageListener('InputRegistry:Add', this);
  ppmm.addMessageListener('InputRegistry:Remove', this);
  ppmm.addMessageListener('System:InputRegistry:Add:Done', this);
  ppmm.addMessageListener('System:InputRegistry:Remove:Done', this);
};

InputRegistryGlue.prototype.receiveMessage = function(msg) {
  let mm = Utils.getMMFromMessage(msg);

  let permName = msg.name.startsWith("System:") ? "input-mgmt" : "input";
  if (!Utils.checkPermissionForMM(mm, permName)) {
    dump("InputRegistryGlue message " + msg.name +
      " from a content process with no " + permName + " privileges.");
    return;
  }

  switch (msg.name) {
    case 'InputRegistry:Add':
      this.addInput(msg, mm);

      break;

    case 'InputRegistry:Remove':
      this.removeInput(msg, mm);

      break;

    case 'System:InputRegistry:Add:Done':
    case 'System:InputRegistry:Remove:Done':
      this.returnMessage(msg.data);

      break;
  }
};

InputRegistryGlue.prototype.addInput = function(msg, mm) {
  let msgId = this._messageId++;
  this._msgMap.set(msgId, {
    mm: mm,
    requestId: msg.data.requestId
  });

  let manifestURL = appsService.getManifestURLByLocalId(msg.data.appId);

  Keyboard.sendToSystem('System:InputRegistry:Add', {
    id: msgId,
    manifestURL: manifestURL,
    inputId: msg.data.inputId,
    inputManifest: msg.data.inputManifest
  });

  // XXX: To be removed when content migrate away from mozChromeEvents.
  SystemAppProxy.dispatchEvent({
    type: 'inputregistry-add',
    id: msgId,
    manifestURL: manifestURL,
    inputId: msg.data.inputId,
    inputManifest: msg.data.inputManifest
  });
};

InputRegistryGlue.prototype.removeInput = function(msg, mm) {
  let msgId = this._messageId++;
  this._msgMap.set(msgId, {
    mm: mm,
    requestId: msg.data.requestId
  });

  let manifestURL = appsService.getManifestURLByLocalId(msg.data.appId);

  Keyboard.sendToSystem('System:InputRegistry:Remove', {
    id: msgId,
    manifestURL: manifestURL,
    inputId: msg.data.inputId
  });

  // XXX: To be removed when content migrate away from mozChromeEvents.
  SystemAppProxy.dispatchEvent({
    type: 'inputregistry-remove',
    id: msgId,
    manifestURL: manifestURL,
    inputId: msg.data.inputId
  });
};

InputRegistryGlue.prototype.returnMessage = function(detail) {
  if (!this._msgMap.has(detail.id)) {
    dump('InputRegistryGlue: Ignoring already handled message response. ' +
         'id=' + detail.id + '\n');
    return;
  }

  let { mm, requestId } = this._msgMap.get(detail.id);
  this._msgMap.delete(detail.id);

  if (Cu.isDeadWrapper(mm)) {
    dump('InputRegistryGlue: Message manager has already died.\n');
    return;
  }

  if (!('error' in detail)) {
    mm.sendAsyncMessage('InputRegistry:Result:OK', {
      requestId: requestId
    });
  } else {
    mm.sendAsyncMessage('InputRegistry:Result:Error', {
      error: detail.error,
      requestId: requestId
    });
  }
};

this.Keyboard.init();
