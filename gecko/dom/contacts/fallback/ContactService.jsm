/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const DEBUG = false;
function debug(s) { dump("-*- Fallback ContactService component: " + s + "\n"); }

// Added for Task5113500 20170930 lina.zhang@t2mobile.com -begin
const MIN_MATCH_7 = 7;
const MIN_MATCH_8 = 8;
const MIN_MATCH_9 = 9;
const MIN_MATCH_10 = 10;
const MIN_MATCH_11 = 11;
// Customization CLI default value controlled by the following 3 files:
// 1. gecko/dom/contacts/fallback/ContactService.jsm: CLI_DEFAULT_VAL
// 2. gecko/dom/phonenumberutils/PhoneNumberUtils.jsm: CLI_DEFAULT_VAL
// 3. apps/system/js/bootstrap.js: CLI_DEFAULT_VAL
const CLI_DEFAULT_VAL = MIN_MATCH_7;
// Added for Task5113500 20170930 lina.zhang@t2mobile.com -end

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = ["ContactService"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");


// Added for Task5113500 20170930 lina.zhang@t2mobile.com -begin
XPCOMUtils.defineLazyModuleGetter(this, "CustomizationConfigManager",
                                  "resource://gre/modules/CustomizationConfigManager.jsm");
// Added for Task5113500 20170930 lina.zhang@t2mobile.com -end

XPCOMUtils.defineLazyModuleGetter(this, "ContactDB",
                                  "resource://gre/modules/ContactDB.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PhoneNumberUtils",
                                  "resource://gre/modules/PhoneNumberUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyServiceGetter(this, "gSimContactService",
                                   "@kaiostech.com/simcontactservice;1",
                                   "nsISimContactService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIMobileConnectionService");

const CATEGORY_DEFAULT = ["DEVICE", "KAICONTACT"];
const CATEGORY_DEVICE = "DEVICE";
const CATEGORY_KAICONTACT = "KAICONTACT";
const CATEGORY_SIM = "SIM";

const SIM_CLIENT_NONE = -1;

/* all exported symbols need to be bound to this on B2G - Bug 961777 */
var ContactService = this.ContactService = {
  init: function() {
    if (DEBUG) debug("Init");
    this._messages = ["Contacts:Find", "Contacts:GetAll", "Contacts:GetAll:SendNow",
                      "Contacts:Clear", "Contact:Save",
                      "Contact:Remove", "Contacts:RegisterForMessages",
                      "child-process-shutdown", "Contacts:GetRevision",
                      "Contacts:SimContactsLoaded",
                      "Contacts:GetCount",
                      "Contacts:GetSpeedDials",
                      "Contacts:SetSpeedDial",
                      "Contacts:RemoveSpeedDial",
                      "Contacts:GetAllGroups",
                      "Contacts:FindGroups",
                      "Contacts:SaveGroup",
                      "Contacts:RemoveGroup",
                      "Contacts:FindWithCursor",
                      "Contacts:GetAllBlockedNumbers",
                      "Contacts:FindBlockedNumbers",
                      "Contacts:SaveBlockedNumber",
                      "Contacts:RemoveBlockedNumber"];
    this._children = [];
    this._cursors = new Map();
    this._messages.forEach(function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }.bind(this));

    Services.obs.addObserver(this, "profile-before-change", false);
    Services.prefs.addObserver("ril.lastKnownSimMcc", this, false);
  },

  _initDB: function() {
    if (!this._children) {
      if (DEBUG) debug("Not intialized yet.");
      return;
    }

    this._db = new ContactDB();
    this._db.init();

    this.configureSubstringMatching();
    // Added for Task5113500 20170930 lina.zhang@t2mobile.com -begin
    Services.prefs.addObserver("dom.contact.minmatch.num", this, false);
    // Added for Task5113500 20170930 lina.zhang@t2mobile.com -end
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic === 'profile-before-change') {
      this._messages.forEach(function(msgName) {
        ppmm.removeMessageListener(msgName, this);
      }.bind(this));
      Services.obs.removeObserver(this, "profile-before-change");
      Services.prefs.removeObserver("dom.phonenumber.substringmatching", this);
      // Added for Task5113500 20170930 lina.zhang@t2mobile.com -begin
      Services.prefs.removeObserver("dom.contact.minmatch.num", this);
      // Added for Task5113500 20170930 lina.zhang@t2mobile.com -end
      ppmm = null;
      this._messages = null;
      if (this._db)
        this._db.close();
      this._db = null;
      this._children = null;
      this._cursors = null;
    // Modified for Task5113500 20170930 lina.zhang@t2mobile.com -begin
/*
    } else if (aTopic === 'nsPref:changed' && aData === "ril.lastKnownSimMcc") {
*/
    } else if (aTopic === 'nsPref:changed' &&
        (aData === "ril.lastKnownSimMcc" || aData === "dom.contact.minmatch.num")) {
      debug('ContactService aData = ' + aData);
    // Modified for Task5113500 20170930 lina.zhang@t2mobile.com -end
      this.configureSubstringMatching();
    }
  },

  configureSubstringMatching: function() {
    // Modified for Task5113500 20170930 lina.zhang@t2mobile.com -begin
    try {
      let val = CLI_DEFAULT_VAL;
      CustomizationConfigManager.getValue(
          'stz.contact.min.match').then((result) => {
        debug('ContactService stz.contact.min.match getvalue result = ' + result);
        if (result == MIN_MATCH_7) {
          val = MIN_MATCH_7;
        } else if (result == MIN_MATCH_8) {
          val = MIN_MATCH_8;
        } else if (result == MIN_MATCH_9) {
          val = MIN_MATCH_9;
        } else if (result == MIN_MATCH_10) {
          val = MIN_MATCH_10;
        } else if (result == MIN_MATCH_11) {
          val = MIN_MATCH_11;
        } else {
          let countryName = PhoneNumberUtils.getCountryName();
          if (Services.prefs.getPrefType("dom.phonenumber.substringmatching." + countryName) == Ci.nsIPrefBranch.PREF_INT) {
            val = Services.prefs.getIntPref("dom.phonenumber.substringmatching." + countryName);
          }
        }
        debug('ContactService val = ' + val);
        if (val) {
          this._db.enableSubstringMatching(val);
          return;
        }
        // if we got here, we dont have a substring setting
        // for this country, so disable substring matching
        this._db.disableSubstringMatching();
      });
    } catch (e) {
      debug('ContactService stz.contact.min.match getvalue exception: ' + e);
	}
    // Modified for Task5113500 20170930 lina.zhang@t2mobile.com -end
    if (!this._db) {
      this._initDB();
    }

    let countryName = PhoneNumberUtils.getCountryName();
    if (Services.prefs.getPrefType("dom.phonenumber.substringmatching." + countryName) == Ci.nsIPrefBranch.PREF_INT) {
      let val = Services.prefs.getIntPref("dom.phonenumber.substringmatching." + countryName);
      if (val) {
        this._db.enableSubstringMatching(val);
        return;
      }
    }
    // if we got here, we dont have a substring setting
    // for this country, so disable substring matching
    this._db.disableSubstringMatching();
  },

  assertPermission: function(aMessage, aPerm) {
    if (!aMessage.target.assertPermission(aPerm)) {
      Cu.reportError("Contacts message " + aMessage.name +
                     " from a content process with no" + aPerm + " privileges.");
      return false;
    }
    return true;
  },

  broadcastMessage: function broadcastMessage(aMsgName, aContent) {
    this._children.forEach(function(msgMgr) {
      msgMgr.sendAsyncMessage(aMsgName, aContent);
    });
  },

  receiveMessage: function(aMessage) {
    if (DEBUG) debug("receiveMessage " + aMessage.name);
    let mm = aMessage.target;
    let msg = aMessage.data;
    let cursorList;

    if (this._messages.indexOf(aMessage.name) === -1) {
      if (DEBUG) debug("WRONG MESSAGE NAME: " + aMessage.name);
      return;
    }

    if (!this._db) {
      this._initDB();
    }

    switch (aMessage.name) {
      case "Contacts:Find":
        if (!this.assertPermission(aMessage, "contacts-read")) {
          return null;
        }
        if (gSimContactService.getSimContactsLoadingNum() > 0) {
          mm.sendAsyncMessage("Contacts:Find:Return:KO", { requestID: msg.requestID, errorMsg: 'Busy' });
        } else {
          let result = [];
          this._db.find(
            function(contacts) {
              for (let i in contacts) {
                result.push(contacts[i]);
              }
              if (DEBUG) debug("result:" + JSON.stringify(result));
              mm.sendAsyncMessage("Contacts:Find:Return:OK", {requestID: msg.requestID, results: result});
            }.bind(this),
            function(aErrorMsg) { mm.sendAsyncMessage("Contacts:Find:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg }); }.bind(this),
            msg.options.findOptions);
        }
        break;
      case "Contacts:GetAll":
        if (!this.assertPermission(aMessage, "contacts-read")) {
          return null;
        }
        cursorList = this._cursors.get(mm);
        if (!cursorList) {
          cursorList = [];
          this._cursors.set(mm, cursorList);
        }
        cursorList.push(msg.cursorId);

        if (gSimContactService.getSimContactsLoadingNum() > 0) {
          mm.sendAsyncMessage("Contacts:GetAll:Return:KO", { requestID: msg.cursorId, errorMsg: 'Busy' });
        } else {
          this._db.getAll(
            function(aContacts) {
              try {
                mm.sendAsyncMessage("Contacts:GetAll:Next", {cursorId: msg.cursorId, results: aContacts});
                if (aContacts === null) {
                  let cursorList = this._cursors.get(mm);
                  let index = cursorList.indexOf(msg.cursorId);
                  cursorList.splice(index, 1);
                }
              } catch (e) {
                if (DEBUG) debug("Child is dead, DB should stop sending contacts");
                throw e;
              }
            }.bind(this),
            function(aErrorMsg) { mm.sendAsyncMessage("Contacts:GetAll:Return:KO", { requestID: msg.cursorId, errorMsg: aErrorMsg }); },
            msg.options.findOptions, msg.cursorId);
        }
        break;
      case "Contacts:GetAll:SendNow":
        // sendNow is a no op if there isn't an existing cursor in the DB, so we
        // don't need to assert the permission again.
        this._db.sendNow(msg.cursorId);
        break;
      case "Contacts:SimContactsLoaded":
        if (0 === gSimContactService.getSimContactsLoadingNum()) {
          this.broadcastMessage("Contact:Changed", { reason: "refreshed" });
        }
        break;
      case "Contact:Save":
      {
        if (msg.options.reason === "create") {
          if (!this.assertPermission(aMessage, "contacts-create")) {
            return null;
          }
        } else {
          if (!this.assertPermission(aMessage, "contacts-write")) {
            return null;
          }
        }

        let contact = msg.options.contact;
        this._makeCategory(contact);
        let simClient = this._getSimClient(contact);
        let isSimContact = simClient != SIM_CLIENT_NONE;
        let newSimContact = isSimContact &&
                            msg.options.reason && msg.options.reason === "create";

        let successCb = (result) => {
          mm.sendAsyncMessage("Contact:Save:Return:OK", { requestID: msg.requestID, contactID: msg.options.contact.id });
          this.broadcastMessage("Contact:Changed", { contactID: msg.options.contact.id,
                                                     reason: msg.options.reason,
                                                     contact: result });
        };

        let failureCb = (aErrorMsg) => {
          mm.sendAsyncMessage("Contact:Save:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
        };
        let saveContact = function(msg) {
          this._db.saveContact(
            contact,
            successCb.bind(this),
            failureCb.bind(this)
          );
        }.bind(this);

        if (!isSimContact) {
          saveContact(msg);
          return;
        }

        if (newSimContact) {
          contact.id = undefined;
        }

        let callback = {
          QueryInterface: XPCOMUtils.generateQI([Ci.nsISimContactCallback]),

          notifyIccContactUpdated: function(aContact) {
            msg.options.contact.id = aContact.id;
            if (DEBUG) debug('notifyIccContactUpdated id: ' + msg.options.contact.id);
            msg.options.contact.properties.name = aContact.getNames();
            this._injectPropsIntoSIM(msg.options.contact);
            saveContact(msg);
          }.bind(this),

          notifyError: function(aErrorMsg) {
            if (DEBUG) debug('notifyError: ' + aErrorMsg);
            mm.sendAsyncMessage("Contact:Save:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
          },
        };

        let updateContact = this._convertToIccContact(
              contact.id,
              contact.properties.name && contact.properties.name[0],
              contact.properties.tel && contact.properties.tel[0] && contact.properties.tel[0].value,
              contact.properties.email && contact.properties.email[0] && contact.properties.email[0].value);
        gSimContactService.getClient(simClient).updateSimContact(updateContact, callback);
      }
      break;
      case "Contact:Remove":
      {
        if (!this.assertPermission(aMessage, "contacts-write")) {
          return null;
        }

        let successCb = () => {
          mm.sendAsyncMessage("Contact:Remove:Return:OK", { requestID: msg.requestID, contactID: msg.options.id });
          this.broadcastMessage("Contact:Changed", { contactID: msg.options.id, reason: "remove" });
        };

        let failureCb = (aErrorMsg) => {
            mm.sendAsyncMessage("Contact:Remove:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
        };

        let removeContact = function(msg) {
          this._db.removeContact(
            msg.options.id,
            successCb.bind(this),
            failureCb.bind(this)
          );
        }.bind(this);

        let simClient = gSimContactService.getSimClient(msg.options.id);
        if (simClient === SIM_CLIENT_NONE) {
          removeContact(msg);
          return;
        }

        let contactToRemove = this._convertToIccContact(msg.options.id);
        let deleteCallback = {
          QueryInterface: XPCOMUtils.generateQI([Ci.nsISimContactCallback]),

          notifyIccContactUpdated: function(aContact) {
            removeContact(msg);
          },

          notifyError: function(aErrorMsg) {
            if (DEBUG) debug('notifyError: ' + aErrorMsg);
            failureCb(aErrorMsg);
          },
        };
        gSimContactService.getClient(simClient).removeSimContact(contactToRemove, deleteCallback);
      }
      break;
      case "Contacts:Clear":
        this._clear(aMessage);
        break;
      case "Contacts:GetRevision":
        if (!this.assertPermission(aMessage, "contacts-read")) {
          return null;
        }
        this._db.getRevision(
          function(revision) {
            mm.sendAsyncMessage("Contacts:Revision", {
              requestID: msg.requestID,
              revision: revision
            });
          },
          function(aErrorMsg) {
            mm.sendAsyncMessage("Contacts:GetRevision:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
          }.bind(this)
        );
        break;
      case "Contacts:GetCount":
        if (!this.assertPermission(aMessage, "contacts-read")) {
          return null;
        }
        this._db.getCount(
          function(count) {
            mm.sendAsyncMessage("Contacts:Count", {
              requestID: msg.requestID,
              count: count
            });
          },
          function(aErrorMsg) {
            mm.sendAsyncMessage("Contacts:Count:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
          }.bind(this)
        );
        break;
      case "Contacts:GetSpeedDials":
        if (!this.assertPermission(aMessage, "contacts-read")) {
          return null;
        }
        let speedDials = [];
        this._db.getSpeedDials(
          function(_speedDials) {
            for (let i in _speedDials) {
              speedDials.push(_speedDials[i]);
            }
            mm.sendAsyncMessage("Contacts:GetSpeedDials:Return:OK", {
              requestID: msg.requestID,
              speedDials: speedDials
            });
          }.bind(this),
          function(aErrorMsg) {
            mm.sendAsyncMessage("Contacts:GetSpeedDials:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
          }.bind(this));
        break;
      case "Contacts:SetSpeedDial":
        if (!this.assertPermission(aMessage, "contacts-write")) {
          return null;
        }
        this._db.setSpeedDial(
          msg.options.speedDial,
          msg.options.tel,
          msg.options.contactId,
          function() {
            mm.sendAsyncMessage("Contacts:SetSpeedDial:Return:OK", { requestID: msg.requestID });
            this.broadcastMessage("Contacts:SpeedDial:Changed", { speedDial: msg.options.speedDial, reason: 'set' });
          }.bind(this),
          function(aErrorMsg) {
            mm.sendAsyncMessage("Contacts:SetSpeedDial:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
          }.bind(this));
        break;
      case "Contacts:RemoveSpeedDial":
        if (!this.assertPermission(aMessage, "contacts-write")) {
          return null;
        }
        this._db.removeSpeedDial(
          msg.options.speedDial,
          function() {
            mm.sendAsyncMessage("Contacts:RemoveSpeedDial:Return:OK", { requestID: msg.requestID });
            this.broadcastMessage("Contacts:SpeedDial:Changed", { speedDial: msg.options.speedDial, reason: 'remove' });
          }.bind(this),
          function(aErrorMsg) {
            mm.sendAsyncMessage("Contacts:RemoveSpeedDial:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
          }.bind(this));
        break;
      case "Contacts:RegisterForMessages":
        if (!aMessage.target.assertPermission("contacts-read")) {
          return null;
        }
        if (DEBUG) debug("Register!");
        if (this._children.indexOf(mm) == -1) {
          this._children.push(mm);
        }
        break;
      case "child-process-shutdown":
        if (DEBUG) debug("Unregister");
        let index = this._children.indexOf(mm);
        if (index != -1) {
          if (DEBUG) debug("Unregister index: " + index);
          this._children.splice(index, 1);
        }
        cursorList = this._cursors.get(mm);
        if (cursorList) {
          for (let id of cursorList) {
            this._db.clearDispatcher(id);
          }
          this._cursors.delete(mm);
        }
        break;
      case "Contacts:GetAllGroups":
        this._getAllGroups(aMessage);
        break;
      case "Contacts:FindGroups":
        if (DEBUG) debug("Contacts:FindGroups");
        this._findGroups(aMessage, "Contacts:FindGroups");
        break;
      case "Contacts:SaveGroup":
        this._saveGroup(aMessage);
        break;
      case "Contacts:RemoveGroup":
        this._removeGroup(aMessage);
        break;
      case "Contacts:FindWithCursor":
        if (!this.assertPermission(aMessage, "contacts-read")) {
          return null;
        }
        let findWithCursorResult = [];
        this._db.find(
          function(contacts) {
            for (let i in contacts) {
              findWithCursorResult.push(contacts[i]);
            }
            if (DEBUG) debug("result:" + JSON.stringify(findWithCursorResult));
            mm.sendAsyncMessage("Contacts:FindWithCursor:Return:OK", {cursorId: msg.cursorId, results: findWithCursorResult});
          }.bind(this),
          function(aErrorMsg) { mm.sendAsyncMessage("Contacts:FindWithCursor:Return:KO", { cursorId: msg.cursorId, errorMsg: aErrorMsg }); }.bind(this),
          msg.options.findOptions);
        break;
      case "Contacts:GetAllBlockedNumbers": {
        this._findBlockedNumbers(aMessage, "Contacts:GetAllBlockedNumbers");
        break;
      }
      case "Contacts:FindBlockedNumbers":
        this._findBlockedNumbers(aMessage, "Contacts:FindBlockedNumbers");
        break;
      case "Contacts:SaveBlockedNumber":
        this._saveBlockedNumber(aMessage);
        break;
      case "Contacts:RemoveBlockedNumber":
        this._removeBlockedNumber(aMessage);
        break;
      default:
        if (DEBUG) debug("WRONG MESSAGE NAME: " + aMessage.name);
    }
  },

  _convertToIccContact: function (aId, aName, aNumber, aEmail) {
    let iccContact = {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccContact]),
      id: aId,
      _names: aName ? [aName] : [],
      _numbers: aNumber ? [aNumber] : [],
      _emails: aEmail ? [aEmail] : [],

      getNames: function(aCount) {
        if (!this._names) {
          if (aCount) {
            aCount.value = 0;
          }
          return [];
        }

        if (aCount) {
          aCount.value = this._names.length;
        }

        return this._names.slice();
      },

      getNumbers: function(aCount) {
        if (!this._numbers) {
          if (aCount) {
            aCount.value = 0;
          }
          return [];
        }

        if (aCount) {
          aCount.value = this._numbers.length;
        }

        return this._numbers.slice();
      },

      getEmails: function(aCount) {
        if (!this._emails) {
          if (aCount) {
            aCount.value = 0;
          }
          return [];
        }

        if (aCount) {
          aCount.value = this._emails.length;
        }

        return this._emails.slice();
      }
    };

    return iccContact;
  },

  _getSimClient: function (aContactOrId) {
    if (typeof aContactOrId === "string") {
      return gSimContactService.getSimClient(aContactOrId);
    } else {
      return this._getSimContactClientByStruct(aContactOrId);
    }
  },

  _getSimContactClientByStruct: function (aContact) {
    if (!aContact || !aContact.properties|| !aContact.properties.category) {
      return SIM_CLIENT_NONE;
    }

    let numOfSIM = gMobileConnectionService.numItems;
    for (let i = 0; i < numOfSIM; i++) {
      if (aContact.properties.category.indexOf("SIM"+i) !== -1) {
        return i;
      }
    }

    return SIM_CLIENT_NONE;
  },

  _makeCategory: function(aContact) {
    if (!aContact || !aContact.properties) {
      return;
    }

    if (!aContact.properties.category) {
      aContact.properties.category = CATEGORY_DEFAULT;
      return;
    }

    let category = aContact.properties.category;

    if (category.indexOf(CATEGORY_KAICONTACT) < 0) {
      category.push(CATEGORY_KAICONTACT);
    }

    if (category.indexOf(CATEGORY_DEVICE) < 0 &&
        category.indexOf(CATEGORY_SIM) < 0) {
      category.push(CATEGORY_DEVICE);
    }
  },

  _injectPropsIntoSIM: function(aContact) {
    if (!aContact || !aContact.properties) {
      return;
    }

    if (aContact.properties.name) {
      //No matter sort by family name, given name ,sim contact will always 
      //sort by name According to SFP_IxD_Spec_Contacts_v2.5R3.15 page42
      aContact.properties.givenName = aContact.properties.familyName
                                    = aContact.properties.name;
    }
  },

  _getAllGroups: function(aMessage) {
    this._findGroups(aMessage, "Contacts:GetAllGroups");
  },

  _findGroups: function(aMessage, aMessageName) {
    if (DEBUG) debug(aMessageName);
    if (!this.assertPermission(aMessage, "contacts-read")) {
      return null;
    }

    let groupResult = [];
    let mm = aMessage.target;
    let msg = aMessage.data;
    this._db.findGroups(
      function(groups) {
        for (let i in groups) {
          groupResult.push(groups[i]);
        }
        if (DEBUG) debug("result: " + JSON.stringify(groupResult));
        mm.sendAsyncMessage(aMessageName + ":Return:OK", {
          requestID: msg.requestID,
          cursorId: msg.cursorId,
          groups: groupResult
        });
      }.bind(this),
      function(aErrorMsg) {
        mm.sendAsyncMessage(aMessageName + ":Return:KO", {
          requestID: msg.requestID,
          cursorId: msg.cursorId,
          errorMsg: aErrorMsg });},
      msg.options.findOptions);
  },

  _saveGroup: function (aMessage) {
    if (DEBUG) debug("Contacts:SaveGroup");
    if (!this.assertPermission(aMessage, "contacts-write")) {
      return null;
    }

    let msg = aMessage.data;
    let mm = aMessage.target;
    this._db.saveGroup(
      msg.options.id,
      msg.options.name,
      function() {
        mm.sendAsyncMessage("Contacts:SaveGroup:Return:OK", { requestID: msg.requestID, groupId: msg.options.id });
        this.broadcastMessage("Contacts:Group:Changed", { groupId: msg.options.id,
                                                          groupName: msg.options.name,
                                                          reason: msg.options.reason });
      }.bind(this),
      function(aErrorMsg) {
        mm.sendAsyncMessage("Contacts:SaveGroup:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
      }.bind(this));
  },

  _removeGroup: function (aMessage) {
    if (DEBUG) debug("Contacts:RemoveGroup");
    if (!this.assertPermission(aMessage, "contacts-write")) {
      return null;
    }

    let msg = aMessage.data;
    let mm = aMessage.target;
    this._db.removeGroup(
      msg.options.id,
      function() {
        mm.sendAsyncMessage("Contacts:RemoveGroup:Return:OK", { requestID: msg.requestID, groupId: msg.options.id });
        this.broadcastMessage("Contacts:Group:Changed", { groupId: msg.options.id, reason: 'remove' });
      }.bind(this),
      function(aErrorMsg) {
        mm.sendAsyncMessage("Contacts:RemoveGroup:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
      }
    );
  },

  _findBlockedNumbers: function (aMessage, aMessageName) {
    if (DEBUG) debug("findBlockedNumbers");
    if (!this.assertPermission(aMessage, "contacts-read")) {
      return null;
    }

    let numberResults = [];
    let mm = aMessage.target;
    let msg = aMessage.data;
    this._db.findBlockedNumbers(
      function(numbers) {
        for (let i in numbers) {
          numberResults.push(numbers[i]);
        }
        if (DEBUG) debug("result: " + JSON.stringify(numberResults));
        mm.sendAsyncMessage(aMessageName + ":Return:OK", {
          requestID: msg.requestID,
          cursorId: msg.cursorId,
          numbers: numberResults
        });
      }.bind(this),
      function(aErrorMsg) {
        mm.sendAsyncMessage(aMessageName + ":Return:KO", {
          requestID: msg.requestID,
          cursorId: msg.cursorId,
          errorMsg: aErrorMsg });},
      msg.options.findOptions);
  },

  _saveBlockedNumber: function (aMessage) {
    if (DEBUG) debug("Contacts:SaveBlockedNumber");
    if (!this.assertPermission(aMessage, "contacts-write")) {
      return null;
    }

    let msg = aMessage.data;
    let mm = aMessage.target;
    this._db.saveBlockedNumber(
      msg.options.number,
      function() {
        mm.sendAsyncMessage("Contacts:SaveBlockedNumber:Return:OK", { requestID: msg.requestID,
                                                                      number: msg.options.number});
        this.broadcastMessage("Contacts:BlockedNumber:Changed", { number: msg.options.number,
                                                                  reason: msg.options.reason });
      }.bind(this),
      function(aErrorMsg) {
        mm.sendAsyncMessage("Contacts:SaveBlockedNumber:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
      }.bind(this));
  },

  _removeBlockedNumber: function(aMessage) {
    if (DEBUG) debug("Contacts:RemoveBlockedNumber");
    if (!this.assertPermission(aMessage, "contacts-write")) {
      return null;
    }

    let msg = aMessage.data;
    let mm = aMessage.target;
    this._db.removeBlockedNumber(
      msg.options.number,
      function() {
        mm.sendAsyncMessage("Contacts:RemoveBlockedNumber:Return:OK", { requestID: msg.requestID,
                                                                        number: msg.options.number});
        this.broadcastMessage("Contacts:BlockedNumber:Changed", { number: msg.options.number, reason: 'remove' });
      }.bind(this),
      function(aErrorMsg) {
        mm.sendAsyncMessage("Contacts:RemoveBlockedNumber:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
      }
    );
  },

  _clear: function(aMessage) {
    if (!this.assertPermission(aMessage, "contacts-write")) {
      return null;
    }

    let mm = aMessage.target;
    let msg = aMessage.data;

    let successCb = () => {
      mm.sendAsyncMessage("Contacts:Clear:Return:OK", { requestID: msg.requestID });
      this.broadcastMessage("Contact:Changed", { reason: "remove" });
    };

    let failureCb = (aErrorMsg) => {
      mm.sendAsyncMessage("Contacts:Clear:Return:KO", { requestID: msg.requestID, errorMsg: aErrorMsg });
    };

    let clearContact = function(msg) {
      this._db.clear(
        { type: msg.options.type,
          contactOnly: msg.options.contactOnly },
        successCb.bind(this),
        failureCb.bind(this)
      );
    }.bind(this);

    if (msg.options.type === 'device') {
      clearContact(msg);
    } else {
      let deleteCallback = {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsISimContactCallback]),

        notifySuccess: function() {
          clearContact(msg);
        },

        notifyError: function(aErrorMsg) {
          if (DEBUG) debug('notifyError: ' + aErrorMsg);
          failureCb(aErrorMsg);
        },
      };

      gSimContactService.clearSimContacts(deleteCallback);
    }

  }
};

ContactService.init();
