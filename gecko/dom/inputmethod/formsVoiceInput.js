/* -*- tab-width: 2; indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* vim: set ts=2 sw=2 sts=2 et: */

/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

"use strict";

var Ci = Components.interfaces;
var Cc = Components.classes;
var Cu = Components.utils;
var Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
  "@mozilla.org/childprocessmessagemanager;1", "nsISyncMessageSender");

const kPrefVoiceInputEnabled = "voice-input.enabled";
const kPrefVoiceInputSelected = "voice-input.selected";
const kPrefUiVersion = "gaia.ui.version";
const kVoiceInputIconURL = "chrome://global/content/images/voice-input.svg";

function vi_debug(msg) {
  dump("formsVoiceInput - " + msg + "\n");
}

var formsVoiceInput = {
  _enabled: false,
  _selected: "",
  _supportedTypes: [],
  _icon: "",
  _inputStyle: "",
  _iconObjURL: "",
  _style: {},
  _uiVersion: "",

  init: function() {
    Services.prefs.addObserver(kPrefVoiceInputEnabled, this, false);
    Services.prefs.addObserver(kPrefVoiceInputSelected, this, false);
    Services.prefs.addObserver(kPrefUiVersion, this, false);

    try {
      this._enabled = Services.prefs.getBoolPref(kPrefVoiceInputEnabled);
      this._selected = Services.prefs.getCharPref(kPrefVoiceInputSelected);
      if (this._selected) {
        this._requestVoiceInputData();
      }
    } catch(e) {}

    try {
      this._uiVersion = Services.prefs.getCharPref(kPrefUiVersion);
    } catch(e) {}
    addEventListener("load", this, true, false);
  },

  _requestVoiceInputData: function() {
    cpmm.addMessageListener("Activities:Get:OK", this);
    cpmm.addMessageListener("Activities:Get:KO", this);
    cpmm.sendAsyncMessage("Activities:Get", {
      activityName: "voice-input"});
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "load":
        this._requestVoiceInputData();
        break;
      case "input": {
        let element = aEvent.target;
        if (this._hasText(element) === undefined) {
          vi_debug("element does not have value nor textContent property.");
        } else if (this._hasText(element)) {
          this._removeVoiceInputStyle(element);
        } else {
          this._applyVoiceInputStyle(element);
        }
        break;
      }
    }

  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "nsPref:changed": {
        if (aData === kPrefVoiceInputEnabled) {
          try {
            this._enabled = Services.prefs.getBoolPref(kPrefVoiceInputEnabled);
          } catch(e) {}
        } else if (aData === kPrefVoiceInputSelected) {
          try {
            this._selected = Services.prefs.getCharPref(kPrefVoiceInputSelected);
            this._requestVoiceInputData();
          } catch(e) {}
        } else if (aData === kPrefUiVersion) {
          try {
            this._uiVersion = Services.prefs.getCharPref(kPrefUiVersion);
          } catch(e) {}
        }
        break;
      }
    }
  },

  receiveMessage: function(aMessage) {
    cpmm.removeMessageListener("Activities:Get:OK", this);
    cpmm.removeMessageListener("Activities:Get:KO", this);
    switch (aMessage.name) {
      case "Activities:Get:OK":
        let results = aMessage.json.results.options;
        for (let i in results) {
          if (this._selected === results[i].manifest) {
            let inputStyle = results[i].description.inputStyle;
            if (!inputStyle) {
              vi_debug("inputStyle is not defined in selected voice-input app.");
              break;
            }
            this._icon = inputStyle.icon;
            this._supportedTypes = inputStyle.types;
            this._iconObjURL = "";
            // use default icon
            if (!this._icon || this._icon.length == 0) {
              this._iconObjURL = kVoiceInputIconURL;
            }
            break;
          }
        }
        break;
      case "Activities:Get:KO":
        vi_debug("Error when requesting voice-input activities.");
        break;
    }
  },

  onElementFocus: function(aElement) {
    if (!this._enabled) {
      vi_debug("voice-input is not enabled.");
      return;
    }

    if (!aElement) {
      vi_debug("element is undefined: " + aElement);
      return false;
    }

    let inputMode = aElement.getAttribute('x-inputmode');
    inputMode = inputMode ? inputMode.toLowerCase() : "";
    if (inputMode === 'native' || inputMode === 'plain') {
      vi_debug("element has x-inputmode native/plain");
      return;
    }

    // check whether aElement is voice-input supported.
    if (!this._supportedTypes.includes(aElement.type) &&
        !aElement.isContentEditable) {
      vi_debug("element type: " + aElement.type +" is not in supported list, ");
      vi_debug("and element is not contenteditable.");
      return;
    }

    if(aElement.hasFocused) {
      vi_debug("Element is already focused.");
      return;
    }

    aElement.voiceInputSupported = true;
    aElement.hasFocused = true;
    aElement.addEventListener('input', this);

    this._style = {};
    this._style.backgroundImage = aElement.style.backgroundImage;
    this._style.backgroundRepeat = aElement.style.backgroundRepeat;
    this._style.backgroundSize = aElement.style.backgroundSize;
    this._style.backgroundPosition = aElement.style.backgroundPosition;

    // Only apply icon if input field is empty.
    if (this._hasText(aElement) === undefined) {
      vi_debug("element does not have value nor textContent property.");
    } else if (!this._hasText(aElement)) {
      this._applyVoiceInputStyle(aElement);
    }
  },

  onElementBlur: function(aElement) {
    aElement.hasFocused = false;
    aElement.removeEventListener('input', this);
    this._removeVoiceInputStyle(aElement);
    this._style = {};
  },

  /**
   * _hasText will return either true, false or undefined.
   * aElement is expected to be a contentEditable <div> or <input> <textarea>,
   * return undefined for unexpected case.
   */
  _hasText: function(aElement) {
    let text = (aElement.isContentEditable && aElement.textContent != undefined) ? aElement.textContent :
               (aElement.value != undefined) ? aElement.value :
               undefined;
    return (text === undefined) ? undefined : (text.length > 0);
  },

  _applyVoiceInputStyle: function(aElement) {
    if (this._uiVersion.startsWith('ST')) {
      return;
    }
    this._getIconURL((url) => {
      let win = aElement.ownerDocument.defaultView;
      let dir =
        aElement.ownerDocument.documentElement.dir === 'rtl' ? 'left' : 'right';
      let inputHeight = parseInt(
        win.getComputedStyle(aElement, null).getPropertyValue('height'));
      let inputPaddingRight = parseInt(
        win.getComputedStyle(aElement, null).getPropertyValue(`padding-${dir}`));
      let iconHeight = inputHeight * 0.6;
      let iconPadding = inputHeight * 0.4 * 0.5;

      aElement.style.backgroundImage = `url(${url})`;
      aElement.style.backgroundRepeat = 'no-repeat';
      aElement.style.backgroundSize = `${iconHeight}px ${iconHeight}px`;
      aElement.style.backgroundPosition = `${dir} ${iconPadding + inputPaddingRight}px center`;
    });
  },

  _removeVoiceInputStyle: function(aElement) {
    aElement.style.backgroundImage = this._style.backgroundImage;
    aElement.style.backgroundRepeat = this._style.backgroundRepeat;
    aElement.style.backgroundSize = this._style.backgroundSize;
    aElement.style.backgroundPosition = this._style.backgroundPosition;
  },

  _getIconURL: function(aCallback) {
    if (this._iconObjURL.length == 0) {
      let iconURL = this._selected.split("/manifest.webapp")[0] + this._icon;

      function loadIcon(aUrl, aCallback) {
        // Set up an xhr to download a blob.
        let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                    .createInstance(Ci.nsIXMLHttpRequest);
        xhr.mozBackgroundRequest = true;
        xhr.open("GET", aUrl, true);
        xhr.responseType = "blob";
        xhr.addEventListener("load", function() {
          if (xhr.status == 200) {
            let blob = xhr.response;
            aCallback(blob);
          } else if (xhr.status === 0) {
            vi_debug("NETWORK_ERROR");
          } else {
            vi_debug("FETCH_ICON_FAILED");
          }
        });
        xhr.addEventListener("error", function() {
          vi_debug("FETCH_ICON_FAILED");
        });
        xhr.send();
      }

      loadIcon(iconURL, (blob) => {
        this._iconObjURL = content.URL.createObjectURL(blob);
        aCallback(this._iconObjURL);
      });
    } else {
      aCallback(this._iconObjURL);
    }
  }
};

formsVoiceInput.init();
