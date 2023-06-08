/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

// Everything but "ContactDB" is only exported here for testing.
this.EXPORTED_SYMBOLS = ["ContactDB", "DB_NAME", "STORE_NAME", "SAVED_GETALL_STORE_NAME",
                         "SPEED_DIALS_STORE_NAME", "REVISION_STORE", "DB_VERSION",
                         "GROUP_STORE_NAME",
                         "BLOCKED_NUMBER_STORE_NAME"];

const DEBUG = false;
function debug(s) { dump("-*- ContactDB component: " + s + "\n"); }
function getReversed(number) {
  number = PhoneNumberNormalizer.Normalize(number);
  return number.split("").reverse().join("");
}

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/IndexedDBHelper.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PhoneNumberUtils",
                                  "resource://gre/modules/PhoneNumberUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PhoneNumberNormalizer",
                                  "resource://gre/modules/PhoneNumberNormalizer.jsm");
Cu.importGlobalProperties(["indexedDB"]);

/* all exported symbols need to be bound to this on B2G - Bug 961777 */
this.DB_NAME = "contacts";
this.DB_VERSION = 25;
this.STORE_NAME = "contacts";
this.SAVED_GETALL_STORE_NAME = "getallcache";
this.SPEED_DIALS_STORE_NAME = "speeddials";
const CHUNK_SIZE = 20;
this.REVISION_STORE = "revision";
const REVISION_KEY = "revision";
this.GROUP_STORE_NAME = "group";
this.BLOCKED_NUMBER_STORE_NAME = "blockednumbers";

const CATEGORY_DEFAULT = ["DEVICE", "KAICONTACT"];
const CATEGORY_DEVICE = "DEVICE";
const CATEGORY_KAICONTACT = "KAICONTACT";
const CATEGORY_SIM = "SIM";

const MIN_MATCH_DIGITS = 7;
const MIN_MATCH_DIGITS_11 = 11;  /*[BTS-2308]change the CN number min match to 11 */

function exportContact(aRecord) {
  if (aRecord) {
    delete aRecord.search;
  }
  return aRecord;
}

function ContactDispatcher(aContacts, aFullContacts, aCallback, aNewTxn, aClearDispatcher, aFailureCb) {
  let nextIndex = 0;

  let sendChunk;
  let count = 0;
  if (aFullContacts) {
    sendChunk = function() {
      try {
        let chunk = aContacts.splice(0, CHUNK_SIZE);
        if (chunk.length > 0) {
          aCallback(chunk);
        }
        if (aContacts.length === 0) {
          aCallback(null);
          aClearDispatcher();
        }
      } catch (e) {
        aClearDispatcher();
      }
    };
  } else {
    sendChunk = function() {
      try {
        let start = nextIndex;
        nextIndex += CHUNK_SIZE;
        let chunk = [];
        aNewTxn("readonly", STORE_NAME, function(txn, store) {
          for (let i = start; i < Math.min(start+CHUNK_SIZE, aContacts.length); ++i) {
            store.get(aContacts[i]).onsuccess = function(e) {
              chunk.push(exportContact(e.target.result));
              count++;
              if (count === aContacts.length) {
                aCallback(chunk);
                aCallback(null);
                aClearDispatcher();
              } else if (chunk.length === CHUNK_SIZE) {
                aCallback(chunk);
                chunk.length = 0;
              }
            };
          }
        }, null, function(errorMsg) {
          aFailureCb(errorMsg);
        });
      } catch (e) {
        aClearDispatcher();
      }
    };
  }

  return {
    sendNow: function() {
      sendChunk();
    }
  };
}

this.ContactDB = function ContactDB() {
  if (DEBUG) debug("Constructor");
};

ContactDB.prototype = {
  __proto__: IndexedDBHelper.prototype,

  _dispatcher: {},

  useFastUpgrade: true,

  upgradeSchema: function upgradeSchema(aTransaction, aDb, aOldVersion, aNewVersion) {
    let loadInitialContacts = function() {
      // Add default contacts
      let jsm = {};
      Cu.import("resource://gre/modules/FileUtils.jsm", jsm);
      Cu.import("resource://gre/modules/NetUtil.jsm", jsm);
      // Loading resource://app/defaults/contacts.json doesn't work because
      // contacts.json is not in the omnijar.
      // So we look for the app dir instead and go from here...
      let contactsFile = jsm.FileUtils.getFile("DefRt", ["contacts.json"], false);
      if (!contactsFile || (contactsFile && !contactsFile.exists())) {
        // For b2g desktop
        contactsFile = jsm.FileUtils.getFile("ProfD", ["contacts.json"], false);
        if (!contactsFile || (contactsFile && !contactsFile.exists())) {
          return;
        }
      }

      let chan = jsm.NetUtil.newChannel({
        uri: NetUtil.newURI(contactsFile),
        loadUsingSystemPrincipal: true});

      let stream = chan.open2();
      // Obtain a converter to read from a UTF-8 encoded input stream.
      let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                      .createInstance(Ci.nsIScriptableUnicodeConverter);
      converter.charset = "UTF-8";
      let rawstr = converter.ConvertToUnicode(jsm.NetUtil.readInputStreamToString(
                                              stream,
                                              stream.available()) || "");
      stream.close();
      let contacts;
      try {
        contacts = JSON.parse(rawstr);
      } catch(e) {
        if (DEBUG) debug("Error parsing " + contactsFile.path + " : " + e);
        return;
      }

      let idService = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);
      objectStore = aTransaction.objectStore(STORE_NAME);

      for (let i = 0; i < contacts.length; i++) {
        let contact = {};
        contact.properties = contacts[i];
        contact.id = idService.generateUUID().toString().replace(/[{}-]/g, "");
        contact = this.makeImport(contact);
        this.updateRecordMetadata(contact);
        if (DEBUG) debug("import: " + JSON.stringify(contact));
        objectStore.put(contact);
      }
    }.bind(this);

    function createFinalSchema() {
      if (DEBUG) debug("creating final schema");
      let objectStore = aDb.createObjectStore(STORE_NAME, {keyPath: "id"});
      objectStore.createIndex("familyName", "properties.familyName", { multiEntry: true });
      objectStore.createIndex("givenName",  "properties.givenName",  { multiEntry: true });
      objectStore.createIndex("name",      "properties.name",        { multiEntry: true });
      objectStore.createIndex("familyNameLowerCase", "search.familyName", { multiEntry: true });
      objectStore.createIndex("givenNameLowerCase",  "search.givenName",  { multiEntry: true });
      objectStore.createIndex("nameLowerCase",       "search.name",       { multiEntry: true });
      objectStore.createIndex("telLowerCase",        "search.tel",        { multiEntry: true });
      objectStore.createIndex("emailLowerCase",      "search.email",      { multiEntry: true });
      objectStore.createIndex("tel", "search.exactTel", { multiEntry: true });
      objectStore.createIndex("category", "properties.category", { multiEntry: true });
      objectStore.createIndex("email", "search.email", { multiEntry: true });
      objectStore.createIndex("telMatch", "search.parsedTel", {multiEntry: true});
      objectStore.createIndex("phoneticFamilyName", "properties.phoneticFamilyName", { multiEntry: true });
      objectStore.createIndex("phoneticGivenName", "properties.phoneticGivenName", { multiEntry: true });
      objectStore.createIndex("phoneticFamilyNameLowerCase", "search.phoneticFamilyName", { multiEntry: true });
      objectStore.createIndex("phoneticGivenNameLowerCase",  "search.phoneticGivenName",  { multiEntry: true });
      objectStore.createIndex("speedDial", "properties.speedDial", { unique: true });
      objectStore.createIndex("telFuzzy",  "search.telFuzzy",  { multiEntry: true });
      objectStore.createIndex("group",  "properties.group",  { multiEntry: true });
      aDb.createObjectStore(SAVED_GETALL_STORE_NAME);
      aDb.createObjectStore(SPEED_DIALS_STORE_NAME, {keyPath: "speedDial"});
      aDb.createObjectStore(REVISION_STORE).put(0, REVISION_KEY);
      let groupObjectStore = aDb.createObjectStore(GROUP_STORE_NAME, { keyPath: "id" });
      groupObjectStore.createIndex("name", "properties.name", { unique: true });
      groupObjectStore.createIndex("nameLowerCase", "search.name", { unique: true });
      let blockedNumberObjectStore = aDb.createObjectStore(BLOCKED_NUMBER_STORE_NAME, {keyPath: "number"});
      blockedNumberObjectStore.createIndex("number", "number", { unique: true });
      blockedNumberObjectStore.createIndex('numberFuzzy', "search.fuzzyNumber");
    }

    let valueUpgradeSteps = [];

    function scheduleValueUpgrade(upgradeFunc) {
      var length = valueUpgradeSteps.push(upgradeFunc);
      if (DEBUG) debug("Scheduled a value upgrade function, index " + (length - 1));
    }

    // We always output this debug line because it's useful and the noise ratio
    // very low.
    debug("upgrade schema from: " + aOldVersion + " to " + aNewVersion + " called!");
    let db = aDb;
    let objectStore;

    if (aOldVersion === 0 && this.useFastUpgrade) {
      createFinalSchema();
      loadInitialContacts();
      return;
    }

    let steps = [
      function upgrade0to1() {
        /**
         * Create the initial database schema.
         *
         * The schema of records stored is as follows:
         *
         * {id:            "...",       // UUID
         *  published:     Date(...),   // First published date.
         *  updated:       Date(...),   // Last updated date.
         *  properties:    {...}        // Object holding the ContactProperties
         * }
         */
        if (DEBUG) debug("create schema");
        objectStore = db.createObjectStore(STORE_NAME, {keyPath: "id"});

        // Properties indexes
        objectStore.createIndex("familyName", "properties.familyName", { multiEntry: true });
        objectStore.createIndex("givenName",  "properties.givenName",  { multiEntry: true });

        objectStore.createIndex("familyNameLowerCase", "search.familyName", { multiEntry: true });
        objectStore.createIndex("givenNameLowerCase",  "search.givenName",  { multiEntry: true });
        objectStore.createIndex("telLowerCase",        "search.tel",        { multiEntry: true });
        objectStore.createIndex("emailLowerCase",      "search.email",      { multiEntry: true });
        next();
      },
      function upgrade1to2() {
        if (DEBUG) debug("upgrade 1");

        // Create a new scheme for the tel field. We move from an array of tel-numbers to an array of
        // ContactTelephone.
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }
        // Delete old tel index.
        if (objectStore.indexNames.contains("tel")) {
          objectStore.deleteIndex("tel");
        }

        // Upgrade existing tel field in the DB.
        objectStore.openCursor().onsuccess = function(event) {
          let cursor = event.target.result;
          if (cursor) {
            if (DEBUG) debug("upgrade tel1: " + JSON.stringify(cursor.value));
            for (let number in cursor.value.properties.tel) {
              cursor.value.properties.tel[number] = {number: number};
            }
            cursor.update(cursor.value);
            if (DEBUG) debug("upgrade tel2: " + JSON.stringify(cursor.value));
            cursor.continue();
          } else {
            next();
          }
        };

        // Create new searchable indexes.
        objectStore.createIndex("tel", "search.tel", { multiEntry: true });
        objectStore.createIndex("category", "properties.category", { multiEntry: true });
      },
      function upgrade2to3() {
        if (DEBUG) debug("upgrade 2");
        // Create a new scheme for the email field. We move from an array of emailaddresses to an array of
        // ContactEmail.
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }

        // Delete old email index.
        if (objectStore.indexNames.contains("email")) {
          objectStore.deleteIndex("email");
        }

        // Upgrade existing email field in the DB.
        objectStore.openCursor().onsuccess = function(event) {
          let cursor = event.target.result;
          if (cursor) {
            if (cursor.value.properties.email) {
              if (DEBUG) debug("upgrade email1: " + JSON.stringify(cursor.value));
              cursor.value.properties.email =
                cursor.value.properties.email.map(function(address) { return { address: address }; });
              cursor.update(cursor.value);
              if (DEBUG) debug("upgrade email2: " + JSON.stringify(cursor.value));
            }
            cursor.continue();
          } else {
            next();
          }
        };

        // Create new searchable indexes.
        objectStore.createIndex("email", "search.email", { multiEntry: true });
      },
      function upgrade3to4() {
        if (DEBUG) debug("upgrade 3");

        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }

        // Upgrade existing impp field in the DB.
        objectStore.openCursor().onsuccess = function(event) {
          let cursor = event.target.result;
          if (cursor) {
            if (cursor.value.properties.impp) {
              if (DEBUG) debug("upgrade impp1: " + JSON.stringify(cursor.value));
              cursor.value.properties.impp =
                cursor.value.properties.impp.map(function(value) { return { value: value }; });
              cursor.update(cursor.value);
              if (DEBUG) debug("upgrade impp2: " + JSON.stringify(cursor.value));
            }
            cursor.continue();
          }
        };
        // Upgrade existing url field in the DB.
        objectStore.openCursor().onsuccess = function(event) {
          let cursor = event.target.result;
          if (cursor) {
            if (cursor.value.properties.url) {
              if (DEBUG) debug("upgrade url1: " + JSON.stringify(cursor.value));
              cursor.value.properties.url =
                cursor.value.properties.url.map(function(value) { return { value: value }; });
              cursor.update(cursor.value);
              if (DEBUG) debug("upgrade impp2: " + JSON.stringify(cursor.value));
            }
            cursor.continue();
          } else {
            next();
          }
        };
      },
      function upgrade4to5() {
        if (DEBUG) debug("Add international phone numbers upgrade");
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }

        objectStore.openCursor().onsuccess = function(event) {
          let cursor = event.target.result;
          if (cursor) {
            if (cursor.value.properties.tel) {
              if (DEBUG) debug("upgrade : " + JSON.stringify(cursor.value));
              cursor.value.properties.tel.forEach(
                function(duple) {
                  let parsedNumber = PhoneNumberUtils.parse(duple.value.toString());
                  if (parsedNumber) {
                    if (DEBUG) {
                      debug("InternationalFormat: " + parsedNumber.internationalFormat);
                      debug("InternationalNumber: " + parsedNumber.internationalNumber);
                      debug("NationalNumber: " + parsedNumber.nationalNumber);
                      debug("NationalFormat: " + parsedNumber.nationalFormat);
                    }
                    if (duple.value.toString() !== parsedNumber.internationalNumber) {
                      cursor.value.search.tel.push(parsedNumber.internationalNumber);
                    }
                  } else {
                    dump("Warning: No international number found for " + duple.value + "\n");
                  }
                }
              );
              cursor.update(cursor.value);
            }
            if (DEBUG) debug("upgrade2 : " + JSON.stringify(cursor.value));
            cursor.continue();
          } else {
            next();
          }
        };
      },
      function upgrade5to6() {
        if (DEBUG) debug("Add index for equals tel searches");
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }

        // Delete old tel index (not on the right field).
        if (objectStore.indexNames.contains("tel")) {
          objectStore.deleteIndex("tel");
        }

        // Create new index for "equals" searches
        objectStore.createIndex("tel", "search.exactTel", { multiEntry: true });

        objectStore.openCursor().onsuccess = function(event) {
          let cursor = event.target.result;
          if (cursor) {
            if (cursor.value.properties.tel) {
              if (DEBUG) debug("upgrade : " + JSON.stringify(cursor.value));
              cursor.value.properties.tel.forEach(
                function(duple) {
                  let number = duple.value.toString();
                  let parsedNumber = PhoneNumberUtils.parse(number);

                  cursor.value.search.exactTel = [number];
                  if (parsedNumber &&
                      parsedNumber.internationalNumber &&
                      number !== parsedNumber.internationalNumber) {
                    cursor.value.search.exactTel.push(parsedNumber.internationalNumber);
                  }
                }
              );
              cursor.update(cursor.value);
            }
            if (DEBUG) debug("upgrade : " + JSON.stringify(cursor.value));
            cursor.continue();
          } else {
            next();
          }
        };
      },
      function upgrade6to7() {
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }
        let names = objectStore.indexNames;
        let whiteList = ["tel", "familyName", "givenName",  "familyNameLowerCase",
                         "givenNameLowerCase", "telLowerCase", "category", "email",
                         "emailLowerCase"];
        for (var i = 0; i < names.length; i++) {
          if (whiteList.indexOf(names[i]) < 0) {
            objectStore.deleteIndex(names[i]);
          }
        }
        next();
      },
      function upgrade7to8() {
        if (DEBUG) debug("Adding object store for cached searches");
        db.createObjectStore(SAVED_GETALL_STORE_NAME);
        next();
      },
      function upgrade8to9() {
        if (DEBUG) debug("Make exactTel only contain the value entered by the user");
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }

        objectStore.openCursor().onsuccess = function(event) {
          let cursor = event.target.result;
          if (cursor) {
            if (cursor.value.properties.tel) {
              cursor.value.search.exactTel = [];
              cursor.value.properties.tel.forEach(
                function(tel) {
                  let normalized = PhoneNumberUtils.normalize(tel.value.toString());
                  cursor.value.search.exactTel.push(normalized);
                }
              );
              cursor.update(cursor.value);
            }
            cursor.continue();
          } else {
            next();
          }
        };
      },
      function upgrade9to10() {
        // no-op, see https://bugzilla.mozilla.org/show_bug.cgi?id=883770#c16
        next();
      },
      function upgrade10to11() {
        if (DEBUG) debug("Adding object store for database revision");
        db.createObjectStore(REVISION_STORE).put(0, REVISION_KEY);
        next();
      },
      function upgrade11to12() {
        if (DEBUG) debug("Add a telMatch index with national and international numbers");
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }
        if (!objectStore.indexNames.contains("telMatch")) {
          objectStore.createIndex("telMatch", "search.parsedTel", {multiEntry: true});
        }
        objectStore.openCursor().onsuccess = function(event) {
          let cursor = event.target.result;
          if (cursor) {
            if (cursor.value.properties.tel) {
              cursor.value.search.parsedTel = [];
              cursor.value.properties.tel.forEach(
                function(tel) {
                  let parsed = PhoneNumberUtils.parse(tel.value.toString());
                  if (parsed) {
                    cursor.value.search.parsedTel.push(parsed.nationalNumber);
                    cursor.value.search.parsedTel.push(PhoneNumberUtils.normalize(parsed.nationalFormat));
                    cursor.value.search.parsedTel.push(parsed.internationalNumber);
                    cursor.value.search.parsedTel.push(PhoneNumberUtils.normalize(parsed.internationalFormat));
                  }
                  cursor.value.search.parsedTel.push(PhoneNumberUtils.normalize(tel.value.toString()));
                }
              );
              cursor.update(cursor.value);
            }
            cursor.continue();
          } else {
            next();
          }
        };
      },
      function upgrade12to13() {
        if (DEBUG) debug("Add phone substring to the search index if appropriate for country");
        if (this.substringMatching) {
          scheduleValueUpgrade(function upgradeValue12to13(value) {
            if (value.properties.tel) {
              value.search.parsedTel = value.search.parsedTel || [];
              value.properties.tel.forEach(
                function(tel) {
                  let normalized = PhoneNumberUtils.normalize(tel.value.toString());
                  if (normalized) {
                    if (this.substringMatching && normalized.length > this.substringMatching) {
                      let sub = normalized.slice(-this.substringMatching);
                      if (value.search.parsedTel.indexOf(sub) === -1) {
                        if (DEBUG) debug("Adding substring index: " + tel + ", " + sub);
                        value.search.parsedTel.push(sub);
                      }
                    }
                  }
                }.bind(this)
              );
              return true;
            } else {
              return false;
            }
          }.bind(this));
        }
        next();
      },
      function upgrade13to14() {
        if (DEBUG) debug("Cleaning up empty substring entries in telMatch index");
        scheduleValueUpgrade(function upgradeValue13to14(value) {
          function removeEmptyStrings(value) {
            if (value) {
              const oldLength = value.length;
              for (let i = 0; i < value.length; ++i) {
                if (!value[i] || value[i] == "null") {
                  value.splice(i, 1);
                }
              }
              return oldLength !== value.length;
            }
          }

          let modified = removeEmptyStrings(value.search.parsedTel);
          let modified2 = removeEmptyStrings(value.search.tel);
          return (modified || modified2);
        });

        next();
      },
      function upgrade14to15() {
        if (DEBUG) debug("Fix array properties saved as scalars");
        const ARRAY_PROPERTIES = ["photo", "adr", "email", "url", "impp", "tel",
                                 "name", "honorificPrefix", "givenName",
                                 "additionalName", "familyName", "honorificSuffix",
                                 "nickname", "category", "org", "jobTitle",
                                 "note", "key"];
        const PROPERTIES_WITH_TYPE = ["adr", "email", "url", "impp", "tel"];

        scheduleValueUpgrade(function upgradeValue14to15(value) {
          let changed = false;

          let props = value.properties;
          for (let prop of ARRAY_PROPERTIES) {
            if (props[prop]) {
              if (!Array.isArray(props[prop])) {
                value.properties[prop] = [props[prop]];
                changed = true;
              }
              if (PROPERTIES_WITH_TYPE.indexOf(prop) !== -1) {
                let subprop = value.properties[prop];
                for (let i = 0; i < subprop.length; ++i) {
                  if (!Array.isArray(subprop[i].type)) {
                    value.properties[prop][i].type = [subprop[i].type];
                    changed = true;
                  }
                }
              }
            }
          }

          return changed;
        });

        next();
      },
      function upgrade15to16() {
        if (DEBUG) debug("Fix Date properties");
        const DATE_PROPERTIES = ["bday", "anniversary"];

        scheduleValueUpgrade(function upgradeValue15to16(value) {
          let changed = false;
          let props = value.properties;
          for (let prop of DATE_PROPERTIES) {
            if (props[prop] && !(props[prop] instanceof Date)) {
              value.properties[prop] = new Date(props[prop]);
              changed = true;
            }
          }

          return changed;
        });

        next();
      },
      function upgrade16to17() {
        if (DEBUG) debug("Fix array with null values");
        const ARRAY_PROPERTIES = ["photo", "adr", "email", "url", "impp", "tel",
                                 "name", "honorificPrefix", "givenName",
                                 "additionalName", "familyName", "honorificSuffix",
                                 "nickname", "category", "org", "jobTitle",
                                 "note", "key"];

        const PROPERTIES_WITH_TYPE = ["adr", "email", "url", "impp", "tel"];

        const DATE_PROPERTIES = ["bday", "anniversary"];

        scheduleValueUpgrade(function upgradeValue16to17(value) {
          let changed;

          function filterInvalidValues(val) {
            let shouldKeep = val != null; // null or undefined
            if (!shouldKeep) {
              changed = true;
            }
            return shouldKeep;
          }

          function filteredArray(array) {
            return array.filter(filterInvalidValues);
          }

          let props = value.properties;

          for (let prop of ARRAY_PROPERTIES) {

            // properties that were empty strings weren't converted to arrays
            // in upgrade14to15
            if (props[prop] != null && !Array.isArray(props[prop])) {
              props[prop] = [props[prop]];
              changed = true;
            }

            if (props[prop] && props[prop].length) {
              props[prop] = filteredArray(props[prop]);

              if (PROPERTIES_WITH_TYPE.indexOf(prop) !== -1) {
                let subprop = props[prop];

                for (let i = 0; i < subprop.length; ++i) {
                  let curSubprop = subprop[i];
                  // upgrade14to15 transformed type props into an array
                  // without checking invalid values
                  if (curSubprop.type) {
                    curSubprop.type = filteredArray(curSubprop.type);
                  }
                }
              }
            }
          }

          for (let prop of DATE_PROPERTIES) {
            if (props[prop] != null && !(props[prop] instanceof Date)) {
              // props[prop] is probably '' and wasn't converted
              // in upgrade15to16
              props[prop] = null;
              changed = true;
            }
          }

          if (changed) {
            value.properties = props;
            return true;
          } else {
            return false;
          }
        });

        next();
      },
      function upgrade17to18() {
        // this upgrade function has been moved to the next upgrade path because
        // a previous version of it had a bug
        next();
      },
      function upgrade18to19() {
        if (DEBUG) {
          debug("Adding the name index");
        }

        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }

        // an earlier version of this code could have run, so checking whether
        // the index exists
        if (!objectStore.indexNames.contains("name")) {
          objectStore.createIndex("name", "properties.name", { multiEntry: true });
          objectStore.createIndex("nameLowerCase", "search.name", { multiEntry: true });
        }

        scheduleValueUpgrade(function upgradeValue18to19(value) {
          value.search.name = [];
          if (value.properties.name) {
            value.properties.name.forEach(function addNameIndex(name) {
              var lowerName = name.toLowerCase();
              // an earlier version of this code could have added it already
              if (value.search.name.indexOf(lowerName) === -1) {
                value.search.name.push(lowerName);
              }
            });
          }
          return true;
        });

        next();
      },
      function upgrade19to20() {
        if (DEBUG) debug("upgrade19to20 create schema(phonetic)");
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }
        objectStore.createIndex("phoneticFamilyName", "properties.phoneticFamilyName", { multiEntry: true });
        objectStore.createIndex("phoneticGivenName", "properties.phoneticGivenName", { multiEntry: true });
        objectStore.createIndex("phoneticFamilyNameLowerCase", "search.phoneticFamilyName", { multiEntry: true });
        objectStore.createIndex("phoneticGivenNameLowerCase",  "search.phoneticGivenName",  { multiEntry: true });
        next();
      },
      function upgrade20to21() {
        if (DEBUG) debug("Adding object store for speed dials");
        db.createObjectStore(SPEED_DIALS_STORE_NAME, { keyPath: "speedDial" });
        next();
      },
      function upgrade21to22() {
        if (DEBUG) debug("Adding default category");
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }
        objectStore.openCursor().onsuccess = function(event) {
          let cursor = event.target.result;
          if (cursor) {
            if (cursor.value.properties.category) {
              let changed = false;
              let category = cursor.value.properties.category;
              if (category.indexOf(CATEGORY_KAICONTACT) < 0) {
                category.push(CATEGORY_KAICONTACT);
                changed = true;
              }
              if (category.indexOf(CATEGORY_DEVICE) < 0 &&
                  category.indexOf(CATEGORY_SIM) < 0) {
                category.push(CATEGORY_DEVICE);
                changed = true;
              }
              if (changed) {
                cursor.update(cursor.value);
              }
            } else {
              cursor.value.properties.category = CATEGORY_DEFAULT;
              cursor.update(cursor.value);
            }
            cursor.continue();
          } else {
            next();
          }
        };
      },
      function upgrade22to23() {
        if (DEBUG) debug("Adding the telFuzzy index");
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }
        objectStore.createIndex("telFuzzy",  "search.telFuzzy",  { multiEntry: true });

        scheduleValueUpgrade(function upgradeValue22to23(value) {
          value.search.telFuzzy = value.search.telFuzzy || [];
          if (value.properties.tel) {
            value.properties.tel.forEach(function addTelFuzzyIndex(tel) {
              let number = tel.value && tel.value.toString();
              if (number) {
                let reversedNum = getReversed(number);
                if (value.search.telFuzzy.indexOf(reversedNum) === -1) {
                  value.search.telFuzzy.push(reversedNum);
                }
              }
            });
          }
          return true;
        });

        next();
      },
      function upgrade23to24() {
        if (DEBUG) debug("Adding the group index and create a new ObjectStore for group.");
        if (!objectStore) {
          objectStore = aTransaction.objectStore(STORE_NAME);
        }
        if (DEBUG) debug("add properties.group to group");
        objectStore.createIndex("group", "properties.group", { multiEntry: true });

        if (DEBUG) debug('Adding group store');
        let groupObjectStore = db.createObjectStore(GROUP_STORE_NAME, { keyPath: "id" });
        if (DEBUG) debug('Adding name');
        groupObjectStore.createIndex("name", "properties.name", { unique: true });
        if (DEBUG) debug('Adding nameLowerCase');
        groupObjectStore.createIndex("nameLowerCase", "search.name", { unique: true });

        next();
      },
      function upgrade24to25() {
        if (DEBUG) debug("Adding object store for blocked numbers");
        let blockedNumberObjectStore = db.createObjectStore(BLOCKED_NUMBER_STORE_NAME, {keyPath: "number"});
        blockedNumberObjectStore.createIndex("number", "number", { unique: true });
        blockedNumberObjectStore.createIndex('numberFuzzy', "search.fuzzyNumber");

        next();
      },
    ];

    let index = aOldVersion;
    let outer = this;

    /* This function runs all upgrade functions that are in the
     * valueUpgradeSteps array. These functions have the following properties:
     * - they must be synchronous
     * - they must take the value as parameter and modify it directly. They
     *   must not create a new object.
     * - they must return a boolean true/false; true if the value was actually
     *   changed
     */
    function runValueUpgradeSteps(done) {
      if (DEBUG) debug("Running the value upgrade functions.");
      if (!objectStore) {
        objectStore = aTransaction.objectStore(STORE_NAME);
      }
      objectStore.openCursor().onsuccess = function(event) {
        let cursor = event.target.result;
        if (cursor) {
          let changed = false;
          let oldValue;
          let value = cursor.value;
          if (DEBUG) {
            oldValue = JSON.stringify(value);
          }
          valueUpgradeSteps.forEach(function(upgradeFunc, i) {
            if (DEBUG) debug("Running upgrade function " + i);
            changed = upgradeFunc(value) || changed;
          });

          if (changed) {
            cursor.update(value);
          } else if (DEBUG) {
            let newValue = JSON.stringify(value);
            if (newValue !== oldValue) {
              // oops something went wrong
              debug("upgrade: `changed` was false and still the value changed! Aborting.");
              aTransaction.abort();
              return;
            }
          }
          cursor.continue();
        } else {
          done();
        }
      };
    }

    function finish() {
      // We always output this debug line because it's useful and the noise ratio
      // very low.
      debug("Upgrade finished");

      outer.incrementRevision(aTransaction);
    }

    function next() {
      if (index == aNewVersion) {
        runValueUpgradeSteps(finish);
        return;
      }

      try {
        var i = index++;
        if (DEBUG) debug("Upgrade step: " + i + "\n");
        steps[i].call(outer);
      } catch(ex) {
        dump("Caught exception" + ex);
        aTransaction.abort();
        return;
      }
    }

    function fail(why) {
      why = why || "";
      if (this.error) {
        why += " (root cause: " + this.error.name + ")";
      }

      debug("Contacts DB upgrade error: " + why);
      aTransaction.abort();
    }

    if (aNewVersion > steps.length) {
      fail("No migration steps for the new version!");
    }

    this.cpuLock = Cc["@mozilla.org/power/powermanagerservice;1"]
                     .getService(Ci.nsIPowerManagerService)
                     .newWakeLock("cpu");

    function unlockCPU() {
      if (outer.cpuLock) {
        if (DEBUG) debug("unlocking cpu wakelock");
        outer.cpuLock.unlock();
        outer.cpuLock = null;
      }
    }

    aTransaction.addEventListener("complete", unlockCPU);
    aTransaction.addEventListener("abort", unlockCPU);

    next();
  },

  makeImport: function makeImport(aContact) {
    let contact = {properties: {}};

    contact.search = {
      name:            [],
      givenName:       [],
      familyName:      [],
      email:           [],
      category:        [],
      tel:             [],
      exactTel:        [],
      parsedTel:       [],
      telFuzzy:        [],
      phoneticFamilyName:   [],
      phoneticGivenName:    [],
    };

    for (let field in aContact.properties) {
      contact.properties[field] = aContact.properties[field];
      // Add search fields
      if (aContact.properties[field] && contact.search[field]) {
        for (let i = 0; i <= aContact.properties[field].length; i++) {
          if (aContact.properties[field][i]) {
            if (field == "tel" && aContact.properties[field][i].value) {
              let number = aContact.properties.tel[i].value.toString();
              let normalized = PhoneNumberUtils.normalize(number);
              // We use an object here to avoid duplicates
              let containsSearch = {};
              let matchSearch = {};

              if (normalized) {
                // exactTel holds normalized version of entered phone number.
                // normalized: +1 (949) 123 - 4567 -> +19491234567
                contact.search.exactTel.push(normalized);
                // matchSearch holds normalized version of entered phone number,
                // nationalNumber, nationalFormat, internationalNumber, internationalFormat
                matchSearch[normalized] = 1;
                let parsedNumber = PhoneNumberUtils.parse(number);
                if (parsedNumber) {
                  if (DEBUG) {
                    debug("InternationalFormat: " + parsedNumber.internationalFormat);
                    debug("InternationalNumber: " + parsedNumber.internationalNumber);
                    debug("NationalNumber: " + parsedNumber.nationalNumber);
                    debug("NationalFormat: " + parsedNumber.nationalFormat);
                    debug("NationalMatchingFormat: " + parsedNumber.nationalMatchingFormat);
                  }
                  matchSearch[parsedNumber.nationalNumber] = 1;
                  matchSearch[parsedNumber.internationalNumber] = 1;
                  matchSearch[PhoneNumberUtils.normalize(parsedNumber.nationalFormat)] = 1;
                  matchSearch[PhoneNumberUtils.normalize(parsedNumber.internationalFormat)] = 1;
                  matchSearch[PhoneNumberUtils.normalize(parsedNumber.nationalMatchingFormat)] = 1;
                } else if (this.substringMatching && normalized.length > this.substringMatching) {
                  matchSearch[normalized.slice(-this.substringMatching)] = 1;
                }

                // containsSearch holds incremental search values for:
                // normalized number and national format
                for (let i = 0; i < normalized.length; i++) {
                  containsSearch[normalized.substring(i, normalized.length)] = 1;
                }
                if (parsedNumber && parsedNumber.nationalFormat) {
                  let number = PhoneNumberUtils.normalize(parsedNumber.nationalFormat);
                  for (let i = 0; i < number.length; i++) {
                    containsSearch[number.substring(i, number.length)] = 1;
                  }
                }
              }
              for (let num in containsSearch) {
                if (num && num != "null") {
                  contact.search.tel.push(num);
                }
              }
              for (let num in matchSearch) {
                if (num && num != "null") {
                  contact.search.parsedTel.push(num);
                }
              }
              contact.search.telFuzzy.push(getReversed(number));
            } else if ((field == "impp" || field == "email") && aContact.properties[field][i].value) {
              let value = aContact.properties[field][i].value;
              if (value && typeof value == "string") {
                contact.search[field].push(value.toLowerCase());
              }
            } else {
              let val = aContact.properties[field][i];
              if (typeof val == "string") {
                contact.search[field].push(val.toLowerCase());
              }
            }
          }
        }
      }
    }

    contact.updated = aContact.updated;
    contact.published = aContact.published;
    contact.id = aContact.id;

    return contact;
  },

  updateRecordMetadata: function updateRecordMetadata(record) {
    if (!record.id) {
      Cu.reportError("Contact without ID");
    }
    if (!record.published) {
      record.published = new Date();
    }
    record.updated = new Date();
  },

  removeObjectFromCache: function CDB_removeObjectFromCache(aObjectId, aCallback, aFailureCb) {
    if (DEBUG) debug("removeObjectFromCache: " + aObjectId);
    if (!aObjectId) {
      if (DEBUG) debug("No object ID passed");
      return;
    }
    this.newTxn("readwrite", this.dbStoreNames, function(txn, stores) {
      let store = txn.objectStore(SAVED_GETALL_STORE_NAME);
      store.openCursor().onsuccess = function(e) {
        let cursor = e.target.result;
        if (cursor) {
          for (let i = 0; i < cursor.value.length; ++i) {
            if (cursor.value[i] == aObjectId) {
              if (DEBUG) debug("id matches cache");
              cursor.value.splice(i, 1);
              cursor.update(cursor.value);
              break;
            }
          }
          cursor.continue();
        } else {
          aCallback(txn);
        }
      }.bind(this);
    }.bind(this), null, aFailureCb);
  },


  incrementRevision: function CDB_incrementRevision(txn) {
    let revStore = txn.objectStore(REVISION_STORE);
    revStore.get(REVISION_KEY).onsuccess = function(e) {
      revStore.put(parseInt(e.target.result, 10) + 1, REVISION_KEY);
    };
  },

  saveContact: function CDB_saveContact(aContact, successCb, errorCb) {
    let contact = this.makeImport(aContact);
    this.newTxn("readwrite", this.dbStoreNames, function (txn, stores) {
      if (DEBUG) debug("Going to update" + JSON.stringify(contact));
      let store = txn.objectStore(STORE_NAME);

      // Look up the existing record and compare the update timestamp.
      // If no record exists, just add the new entry.
      let newRequest = store.get(contact.id);
      newRequest.onsuccess = function (event) {
        if (!event.target.result) {
          if (DEBUG) debug("new record!");
          this.updateRecordMetadata(contact);
          store.put(contact);
        } else {
          if (DEBUG) debug("old record!");
          if (new Date(typeof contact.updated === "undefined" ? 0 : contact.updated) < new Date(event.target.result.updated)) {
            if (DEBUG) debug("rev check fail!");
            txn.abort();
            return;
          } else {
            if (DEBUG) debug("rev check OK");
            contact.published = event.target.result.published;
            contact.updated = new Date();
            store.put(contact);
          }
        }

        // Return the upated record.
        txn.result = contact;

        // Invalidate the entire cache. It will be incrementally regenerated on demand
        // See getCacheForQuery
        let getAllStore = txn.objectStore(SAVED_GETALL_STORE_NAME);
        getAllStore.clear().onerror = errorCb;
      }.bind(this);

      this.incrementRevision(txn);
    }.bind(this), successCb, errorCb);
  },

  removeContact: function removeContact(aId, aSuccessCb, aErrorCb) {
    if (DEBUG) debug("removeContact: " + aId);
    this.removeObjectFromCache(aId, function(txn) {
      let store = txn.objectStore(STORE_NAME);
      store.delete(aId).onsuccess = function() {
        aSuccessCb();
      };
      this.incrementRevision(txn);
    }.bind(this), aErrorCb);
  },

  clear: function clear(aOptions, aSuccessCb, aErrorCb) {
    this.newTxn("readwrite", this.dbStoreNames, function (txn, stores) {
      if (DEBUG) debug("Going to clear all with type " + aOptions.type + ", contactOnly: " + aOptions.contactOnly);
      let getAllStore = txn.objectStore(SAVED_GETALL_STORE_NAME);
      getAllStore.clear();

      let store = txn.objectStore(STORE_NAME);
      if (aOptions.type === 'device') {
        let index = store.index('category');
        let request = index.openKeyCursor(IDBKeyRange.only(CATEGORY_DEVICE));
        request.onsuccess = function(event) {
          let cursor = event.target.result;
          if(cursor) {
            store.delete(cursor.primaryKey );
            cursor.continue();
          }
        };
      } else {
        store.clear();
      }

      if (!aOptions.contactOnly) {
        let groupStore = txn.objectStore(GROUP_STORE_NAME);
        groupStore.clear();
        let blockedNumbersStore = txn.objectStore(BLOCKED_NUMBER_STORE_NAME);
        blockedNumbersStore.clear();
      }

      this.incrementRevision(txn);
    }.bind(this), aSuccessCb, aErrorCb);
  },

  createCacheForQuery: function CDB_createCacheForQuery(aQuery, aSuccessCb, aFailureCb) {
    this.find(function (aContacts) {
      if (aContacts) {
        let contactsArray = [];
        for (let i in aContacts) {
          contactsArray.push(aContacts[i]);
        }

        let contactIdsArray = contactsArray.map(el => el.id);

        // save contact ids in cache
        this.newTxn("readwrite", SAVED_GETALL_STORE_NAME, function(txn, store) {
          store.put(contactIdsArray, aQuery);
        }, null, aFailureCb);

        // send full contacts
        aSuccessCb(contactsArray, true);
      } else {
        aSuccessCb([], true);
      }
    }.bind(this),
    function (aErrorMsg) { aFailureCb(aErrorMsg); },
    JSON.parse(aQuery));
  },

  getCacheForQuery: function CDB_getCacheForQuery(aQuery, aSuccessCb, aFailureCb) {
    if (DEBUG) debug("getCacheForQuery");
    // Here we try to get the cached results for query `aQuery'. If they don't
    // exist, it means the cache was invalidated and needs to be recreated, so
    // we do that. Otherwise, we just return the existing cache.
    this.newTxn("readonly", SAVED_GETALL_STORE_NAME, function(txn, store) {
      let req = store.get(aQuery);
      req.onsuccess = function(e) {
        if (e.target.result) {
          if (DEBUG) debug("cache exists");
          aSuccessCb(e.target.result, false);
        } else {
          if (DEBUG) debug("creating cache for query " + aQuery);
          this.createCacheForQuery(aQuery, aSuccessCb);
        }
      }.bind(this);
      req.onerror = function(e) {
        aFailureCb(e.target.errorMessage);
      };
    }.bind(this), null, aFailureCb);
  },

  sendNow: function CDB_sendNow(aCursorId) {
    if (aCursorId in this._dispatcher) {
      this._dispatcher[aCursorId].sendNow();
    }
  },

  clearDispatcher: function CDB_clearDispatcher(aCursorId) {
    if (DEBUG) debug("clearDispatcher: " + aCursorId);
    if (aCursorId in this._dispatcher) {
      delete this._dispatcher[aCursorId];
    }
  },

  getAll: function CDB_getAll(aSuccessCb, aFailureCb, aOptions, aCursorId) {
    if (DEBUG) debug("getAll");
    let optionStr = JSON.stringify(aOptions);
    this.getCacheForQuery(optionStr, function(aCachedResults, aFullContacts) {
      // aFullContacts is true if the cache didn't exist and had to be created.
      // In that case, we receive the full contacts since we already have them
      // in memory to create the cache. This allows us to avoid accessing the
      // object store again.
      if (aCachedResults && aCachedResults.length > 0) {
        let newTxnFn = this.newTxn.bind(this);
        let clearDispatcherFn = this.clearDispatcher.bind(this, aCursorId);
        this._dispatcher[aCursorId] = new ContactDispatcher(aCachedResults, aFullContacts,
                                                            aSuccessCb, newTxnFn,
                                                            clearDispatcherFn, aFailureCb);
        this._dispatcher[aCursorId].sendNow();
      } else { // no contacts
        if (DEBUG) debug("query returned no contacts");
        aSuccessCb(null);
      }
    }.bind(this), aFailureCb);
  },

  getRevision: function CDB_getRevision(aSuccessCb, aErrorCb) {
    if (DEBUG) debug("getRevision");
    this.newTxn("readonly", REVISION_STORE, function (txn, store) {
      store.get(REVISION_KEY).onsuccess = function (e) {
        aSuccessCb(e.target.result);
      };
    },null, aErrorCb);
  },

  getCount: function CDB_getCount(aSuccessCb, aErrorCb) {
    if (DEBUG) debug("getCount");
    this.newTxn("readonly", STORE_NAME, function (txn, store) {
      store.count().onsuccess = function (e) {
        aSuccessCb(e.target.result);
      };
    }, null, aErrorCb);
  },

  getSortByParam: function CDB_getSortByParam(aFindOptions) {
    switch (aFindOptions.sortBy) {
      case "familyName":
        return [ "familyName", "givenName" ];
      case "givenName":
        return [ "givenName" , "familyName" ];
      case "phoneticFamilyName":
        return [ "phoneticFamilyName" , "phoneticGivenName" ];
      case "phoneticGivenName":
        return [ "phoneticGivenName" , "phoneticFamilyName" ];
      default:
        return [ "givenName" , "familyName" ];
    }
  },

  compare: function(aReferenceStr, aCompareStr, aLanguage, collator) {
    try {
      return collator.compare(aReferenceStr, aCompareStr);
    } catch(e) {
      if (!aLanguage || aLanguage.includes("en")) {
        return aReferenceStr.localeCompare(aCompareStr);
      } else {
        return aReferenceStr.localeCompare(aCompareStr, aLanguage);
      }
    }
  },

  /*
   * Sorting the contacts by sortBy field. aSortBy can either be familyName or givenName.
   * If 2 entries have the same sortyBy field or no sortBy field is present, we continue
   * sorting with the other sortyBy field.
   */
  sortResults: function CDB_sortResults(aResults, aFindOptions) {
    if (!aFindOptions)
      return;
    if (typeof(aFindOptions.sortBy) != "undefined") {
      const sortOrder = aFindOptions.sortOrder;
      const sortBy = this.getSortByParam(aFindOptions);
      const sortLanguage = aFindOptions.sortLanguage;

      let self = this;
      let collator = new Intl.Collator(sortLanguage);
      aResults.sort(function (a, b) {
        let x, y;
        let result = 0;
        let xIndex = 0;
        let yIndex = 0;

        do {
          while (xIndex < sortBy.length && !x) {
            x = a.properties[sortBy[xIndex]];
            if (x) {
              x = x.join("").toLowerCase();
            }
            xIndex++;
          }
          while (yIndex < sortBy.length && !y) {
            y = b.properties[sortBy[yIndex]];
            if (y) {
              y = y.join("").toLowerCase();
            }
            yIndex++;
          }
          if (!x) {
            if (!y) {
              let px, py;
              px = JSON.stringify(a.published);
              py = JSON.stringify(b.published);
              if (px && py) {
                return self.compare(px, py, sortLanguage, collator);
              }
            } else {
              return sortOrder == 'descending' ? 1 : -1;
            }
          }
          if (!y) {
            return sortOrder == "ascending" ? 1 : -1;
          }

          result = self.compare(x, y, sortLanguage, collator);
          x = null;
          y = null;
        } while (result == 0);

        return sortOrder == "ascending" ? result : -result;
      });
    }
    if (aFindOptions.filterLimit && aFindOptions.filterLimit != 0) {
      if (DEBUG) debug("filterLimit is set: " + aFindOptions.filterLimit);
      aResults.splice(aFindOptions.filterLimit, aResults.length);
    }
  },

  /**
   * @param successCb
   *        Callback function to invoke with result array.
   * @param failureCb [optional]
   *        Callback function to invoke when there was an error.
   * @param options [optional]
   *        Object specifying search options. Possible attributes:
   *        - filterBy
   *        - filterOp
   *        - filterValue
   *        - count
   */
  find: function find(aSuccessCb, aFailureCb, aOptions) {
    if (DEBUG) debug("ContactDB:find val:" + aOptions.filterValue + " by: " + aOptions.filterBy + " op: " + aOptions.filterOp);
    let self = this;
    this.newTxn("readonly", STORE_NAME, function (txn, store) {
      let filterOps = ["equals", "contains", "match", "startsWith", "fuzzyMatch"];
      if (aOptions && (filterOps.indexOf(aOptions.filterOp) >= 0)) {
        self._findWithIndex(txn, store, aOptions);
      } else {
        self._findAll(txn, store, aOptions);
      }
    }, aSuccessCb, aFailureCb);
  },

  _findWithIndex: function _findWithIndex(txn, store, options) {
    if (DEBUG) debug("_findWithIndex: " + options.filterValue +" " + options.filterOp + " " + options.filterBy + " ");
    let fields = options.filterBy;
    if (!this._validFields(txn, store, fields)) {
      return;
    }

    // lookup for all keys
    if (options.filterBy.length == 0) {
      if (DEBUG) debug("search in all fields!" + JSON.stringify(store.indexNames));
      for(let myIndex = 0; myIndex < store.indexNames.length; myIndex++) {
        fields = Array.concat(fields, store.indexNames[myIndex]);
      }
    }

    // Sorting functions takes care of limit if set.
    let limit = options.sortBy === 'undefined' ? options.filterLimit : null;

    let filter_keys = fields.slice();
    for (let key = filter_keys.shift(); key; key = filter_keys.shift()) {
      let request;
      let substringResult = {};
      if (key == "id") {
        // store.get would return an object and not an array
        request = store.mozGetAll(options.filterValue);
      } else if (key == "category") {
        let index = store.index(key);
        request = index.mozGetAll(options.filterValue, limit);
      } else if (options.filterOp == "equals") {
        if (DEBUG) debug("Getting index: " + key);
        // case sensitive
        let index = store.index(key);
        let filterValue = options.filterValue;
        if (key == "tel") {
          filterValue = PhoneNumberUtils.normalize(filterValue,
                                                   /*numbersOnly*/ true);
        }
        request = index.mozGetAll(filterValue, limit);
      } else if (options.filterOp == "match") {
        if (DEBUG) debug("match");
        if (key != "tel") {
          dump("ContactDB: 'match' filterOp only works on tel\n");
          return txn.abort();
        }

        let index = store.index("telMatch");
        let normalized = PhoneNumberUtils.normalize(options.filterValue,
                                                    /*numbersOnly*/ true);

        if (!normalized.length) {
          dump("ContactDB: normalized filterValue is empty, can't perform match search.\n");
          return txn.abort();
        }

        // Some countries need special handling for number matching. Bug 877302
        if (this.substringMatching && normalized.length > this.substringMatching) {
          let substring = normalized.slice(-this.substringMatching);
          if (DEBUG) debug("Substring: " + substring);

          let substringRequest = index.mozGetAll(substring, limit);

          substringRequest.onsuccess = function (event) {
            if (DEBUG) debug("Request successful. Record count: " + event.target.result.length);
            for (let i in event.target.result) {
              substringResult[event.target.result[i].id] = event.target.result[i];
            }
          }.bind(this);
        } else if (normalized[0] !== "+") {
          // We might have an international prefix like '00'
          let parsed = PhoneNumberUtils.parse(normalized);
          if (parsed && parsed.internationalNumber &&
              parsed.nationalNumber  &&
              parsed.nationalNumber !== normalized &&
              parsed.internationalNumber !== normalized) {
            if (DEBUG) debug("Search with " + parsed.internationalNumber);
            let prefixRequest = index.mozGetAll(parsed.internationalNumber, limit);

            prefixRequest.onsuccess = function (event) {
              if (DEBUG) debug("Request successful. Record count: " + event.target.result.length);
              for (let i in event.target.result) {
                substringResult[event.target.result[i].id] = event.target.result[i];
              }
            }.bind(this);
          }
        }

        request = index.mozGetAll(normalized, limit);
      } else if (options.filterOp == 'fuzzyMatch') {
        if (DEBUG) debug("fuzzyMatch");
        if (key != "tel") {
          dump("ContactDB: 'fuzzyMatch' filterOp only works on tel\n");
          return txn.abort();
        }

        let index = store.index("telFuzzy");
        let filterValue = options.filterValue.toString();
        //Use property this.substringMatching which is customised per MCC/MNC
        let substringDigits = this.substringMatching; //this._getMinMatchDigits();
        if (filterValue.length >= substringDigits) {
          filterValue = filterValue.slice(-substringDigits);
          filterValue = getReversed(filterValue);
          request = index.mozGetAll(IDBKeyRange.bound(filterValue, filterValue + "\uFFFF"), limit);
        } else {
          filterValue = getReversed(filterValue);
          request = index.mozGetAll(filterValue, limit);
        }
      } else {
        // XXX: "contains" should be handled separately, this is "startsWith"
        if (options.filterOp === 'contains' && key !== 'tel') {
          dump("ContactDB: 'contains' only works for 'tel'. Falling back " +
               "to 'startsWith'.\n");
        }
        // not case sensitive
        let lowerCase = options.filterValue.toString().toLowerCase();
        if (key === "tel") {
          let origLength = lowerCase.length;
          let tmp = PhoneNumberUtils.normalize(lowerCase, /*numbersOnly*/ true);
          if (tmp.length != origLength) {
            let NON_SEARCHABLE_CHARS = /[^#+\*\d\s()-]/;
            // e.g. number "123". find with "(123)" but not with "123a"
            if (tmp === "" || NON_SEARCHABLE_CHARS.test(lowerCase)) {
              if (DEBUG) debug("Call continue!");
              continue;
            }
            lowerCase = tmp;
          }
        }
        if (DEBUG) debug("lowerCase: " + lowerCase);
        let range = IDBKeyRange.bound(lowerCase, lowerCase + "\uFFFF");
        let index = store.index(key + "LowerCase");
        request = index.mozGetAll(range, limit);
      }
      if (!txn.result)
        txn.result = {};

      request.onsuccess = function (event) {
        if (DEBUG) debug("Request successful. Record count: " + event.target.result.length);
        if (Object.keys(substringResult).length > 0) {
          for (let attrname in substringResult) {
            event.target.result[attrname] = substringResult[attrname];
          }
        }
        this.sortResults(event.target.result, options);
        for (let i in event.target.result)
          txn.result[event.target.result[i].id] = exportContact(event.target.result[i]);
      }.bind(this);
    }
  },

  _findAll: function _findAll(txn, store, options) {
    if (DEBUG) debug("ContactDB:_findAll:  " + JSON.stringify(options));
    if (!txn.result) txn.result = {};

    // Sorting functions takes care of limit if set.
    let limit = options.sortBy === 'undefined' ? options.filterLimit : null;
    store.mozGetAll(null, limit).onsuccess = function (event) {
      if (DEBUG) debug("Request successful. Record count:" + event.target.result.length);
      this.sortResults(event.target.result, options);
      for (let i in event.target.result) {
        txn.result[event.target.result[i].id] = exportContact(event.target.result[i]);
      }
    }.bind(this);
  },

  getSpeedDials: function getSpeedDials(aSuccessCb, aFailureCb) {
    this.newTxn("readonly", SPEED_DIALS_STORE_NAME, function (txn, store) {
      if (!txn.result)
        txn.result = {};

      var req = store.mozGetAll();
      req.onsuccess = function (event) {
        let speedDials = event.target.result;
        if (DEBUG) debug("Request successful. Speed Dials :" + speedDials.length);
        speedDials.sort();
        for (let i in speedDials) {
          txn.result[speedDials[i].speedDial] = speedDials[i];
        }
      }.bind(this);
      req.onerror = function(e) {
        dump("getSpeedDials: " + e);
      }.bind(this);
    }, aSuccessCb, aFailureCb);
  },

  setSpeedDial: function setSpeedDial(aSpeedDial, aTel, aContactId, aSuccessCb, aFailureCb) {
    if (DEBUG) debug("setSpeedDial: aSpeedDial " + aSpeedDial + " aTel " + aTel + " aContactId " + aContactId);

    this.newTxn("readwrite", this.dbStoreNames, function (txn, stores) {
      if (!aSpeedDial || !aTel) {
        dump("ContactDB: speed dial or phone number are empty");
        txn.abort();
        return;
      }
      let speedDialObj = {
        speedDial: aSpeedDial,
        tel: aTel
      };

      if (aContactId) {
        let req = txn.objectStore(STORE_NAME).get(aContactId);
        req.onsuccess = function (event) {
          if (!event.target.result) {
            dump("ContactDB: contact with id " + aContactId + " does not exist!");
            txn.abort();
          }
          speedDialObj.contactId = aContactId;
          txn.objectStore(SPEED_DIALS_STORE_NAME).put(speedDialObj);
          this.incrementRevision(txn);
        }.bind(this);
      } else {
        txn.objectStore(SPEED_DIALS_STORE_NAME).put(speedDialObj);
        this.incrementRevision(txn);
      }
    }.bind(this), aSuccessCb, aFailureCb);
  },

  saveGroup: function saveGroup(aId, aName, aSuccessCb, aFailureCb) {
    if (DEBUG) debug("saveGroup: aId " + aId + " aName " + aName);
    this.newTxn("readwrite", GROUP_STORE_NAME, function (txn, store) {
      if (!aId || !aName) {
        dump("ContactDB: aId or aName is empty");
        txn.abort();
        return;
      }

      let group = {
        id: aId,
        properties: {
          name: aName
        },
        search: {
          name: aName.toLowerCase()
        }
      };

      store.put(group);
      this.incrementRevision(txn);

    }.bind(this), aSuccessCb, aFailureCb);
  },

  removeSpeedDial: function removeSpeedDial(aSpeedDial, aSuccessCb, aFailureCb) {
    if (DEBUG) debug("removeSpeedDial: aSpeedDial " + aSpeedDial);
    this.newTxn("readwrite", this.dbStoreNames, function (txn, store) {
      txn.objectStore(SPEED_DIALS_STORE_NAME).delete(aSpeedDial).onsuccess = function() {
        aSuccessCb();
      }.bind(this);

      this.incrementRevision(txn);
    }.bind(this), null, aFailureCb);
  },

  // Enable special phone number substring matching. Does not update existing DB entries.
  enableSubstringMatching: function enableSubstringMatching(aDigits) {
    if (DEBUG) debug("MCC enabling substring matching " + aDigits);
    this.substringMatching = aDigits;
  },

  disableSubstringMatching: function disableSubstringMatching() {
    if (DEBUG) debug("MCC disabling substring matching");
    delete this.substringMatching;
  },

  _getMinMatchDigits: function _getMinMatchDigits() {
    if (Services.prefs.getPrefType("dom.phonenumber.substringmatching") == Ci.nsIPrefBranch.PREF_INT) {
      return Services.prefs.getIntPref("dom.phonenumber.substringmatching");
    }
    /*[BTS-2308]change the CN number min match to 11 start*/
    let countryName = PhoneNumberUtils.getCountryName();
    if (DEBUG) debug(" _getMinMatchDigits countryName =" + countryName );
    if (countryName =="CN"){
    if (DEBUG) debug(" _getMinMatchDigits CN return min 11 " );
    return MIN_MATCH_DIGITS_11;
    }
    /*[BTS-2308]change the CN number min match to 11 end*/
    // TODO To customize MIN_MATCH_DIGITS by MCC/MNC
    return MIN_MATCH_DIGITS;
  },

  /**
   * @param aSuccessCb
   *        Callback function to invoke with result array.
   * @param aFailureCb [optional]
   *        Callback function to invoke when there was an error.
   * @param aOptions [optional]
   *        Object specifying search options. Possible attributes:
   *        - filterBy
   *        - filterOp
   *        - filterValue
   *        - filterLimit
   */
  findGroups: function findGroups(aSuccessCb, aFailureCb, aOptions) {
    if (DEBUG) debug("ContactDB:findGroups val:" + aOptions.filterValue + " by: " + aOptions.filterBy + " op: " + aOptions.filterOp);
    this.newTxn("readonly", GROUP_STORE_NAME, function (txn, store) {
      let filterOps = ["equals", "startsWith"];
      if (aOptions && filterOps.indexOf(aOptions.filterOp) >= 0) {
        this._findGroupsWithIndex(txn, store, aOptions);
      } else {
        this._findAllGroups(txn, store, aOptions);
      }
    }.bind(this), aSuccessCb, aFailureCb);
  },

  removeGroup: function removeGroup(aId, aSuccessCb, aFailureCb) {
    if (DEBUG) debug("removeContactGroup: aId " + aId);
    this._removeGroupFromContacts(aId, function(txn) {
      txn.objectStore(GROUP_STORE_NAME).delete(aId).onsuccess = function() {
        aSuccessCb();
      }.bind(this);
      this.incrementRevision(txn);
    }.bind(this), aFailureCb);
  },

  _removeGroupFromContacts: function CDB_removeGroupFromContacts(aGroupId, aCallback, aFailureCb) {
    if (DEBUG) debug("_removeGroupFromContacts: " + aGroupId);
    if (!aGroupId) {
      if (DEBUG) debug("No group ID passed");
      aFailure("No group ID passed");
      return;
    }

    this.newTxn("readwrite", this.dbStoreNames, function (txn, store) {
      let contactStore = txn.objectStore(STORE_NAME);
      let groupIndex = contactStore.index("group");
      let request = groupIndex.mozGetAll(aGroupId);

      request.onsuccess = function(event) {
        let contacts = event.target.result;
        contacts.forEach((contact)=> {
          if (DEBUG) debug("group contact: " + JSON.stringify(contact));
          if (DEBUG) debug("contact group: " + contact.properties.group);
          let index = contact.properties.group.indexOf(aGroupId);
          if (DEBUG) debug("contact group index: " + index);
          if (index>=0) {
            contact.properties.group.splice(index, 1);
            if (DEBUG) debug("contact group after removed: " + contact.properties.group);
            contactStore.put(contact);
          }
        });

        aCallback(txn);
      }.bind(this);

    }, null, aFailureCb);

  },

  _findGroupsWithIndex: function _findGroupsWithIndex(txn, store, options) {
    if (DEBUG) debug("_findGroupsWithIndex: " + options.filterValue + " " + options.filterOp + " " + options.filterBy + " ");
    let fields = options.filterBy;
    if (!this._validFields(txn, store, fields)) {
      return;
    }

    let limit = options.sortBy === "undefined" ? options.filterLimit : null;
    let filter_keys = fields.slice();
    for (let key = filter_keys.shift(); key; key = filter_keys.shift()) {
      let request;
      if (key === "id") {
        request = store.mozGetAll(options.filterValue, limit);
      } else if (options.filterOp === "equals") {
        if (DEBUG) debug("Getting index: " + key);
        let index = store.index(key);
        request = index.mozGetAll(options.filterValue, limit);
      } else {
        let lowerCase = options.filterValue.toString().toLowerCase();
        if (DEBUG) debug("lowerCase: " + lowerCase);
        let range = IDBKeyRange.bound(lowerCase, lowerCase + "\uFFFF");
        let index = store.index(key + "LowerCase");
        request = index.mozGetAll(range, limit);
      }
      if (!txn.result) {
        txn.result = {};
      }

      request.onsuccess = function(event) {
        if (DEBUG) debug("Request successful. Record count: " + event.target.result.length);
        let groups = event.target.result;
        this._sortGroups(groups, options);
        for (let i in groups) {
          if (DEBUG) debug("Groups i: " + i);
          txn.result[groups[i].id] = {
            id: groups[i].id,
            name: groups[i].properties.name
          };
        }
      }.bind(this);
    }
  },

  _validFields: function(txn, store, fields) {
    for (let key in fields) {
      if (DEBUG) debug("key: " + fields[key]);
      if (!store.indexNames.contains(fields[key]) && fields[key] != "id") {
        if (DEBUG) debug("Key not valid!" + fields[key] + ", " + JSON.stringify(store.indexNames));
        txn.abort();
        return false;
      }
    }

    return true;
  },

  _findAllGroups: function _findAllGroups(txn, store, options) {
    if (!txn.result) {
      txn.result = {};
    }

    let req = store.mozGetAll();
    req.onsuccess = function(event) {
      let groups = event.target.result;
      if (DEBUG) debug("Request successful. Groups size: " + groups.length + ", " + JSON.stringify(groups));
      this._sortGroups(groups, options);
      for (let i in groups) {
        if (DEBUG) debug("Groups i: " + i);
        txn.result[groups[i].id] = {
          id: groups[i].id,
          name: groups[i].properties.name
        };
      }
    }.bind(this);

    req.onerror = function (e) {
      dump("findGroups: " + e);
    }.bind(this);
  },

  _sortGroups: function _sortGroups(aGroups, aOptions) {
    if (aOptions.sortOrder) {
      let ascending = aOptions.sortOrder == 'ascending';
      let self = this;
      let collator = new Intl.Collator(aOptions.sortLanguage);
      aGroups.sort(function(a, b) {
        let result = self.compare(a.search.name, b.search.name, aOptions.sortLanguage, collator);
        return ascending ? result : -result;
      });
    }

    if (aOptions.filterLimit) {
      aGroups.splice(aOptions.filterLimit, aGroups.length);
    }
  },

  findBlockedNumbers: function CDB_getAllBlockedNumbers(aSuccessCb, aFailureCb, aOptions, aCursorId) {
    if (DEBUG) debug("ContactDB:findBlockedNumbers val:" + aOptions.filterValue +
                     " by: " + aOptions.filterBy + " op: " + aOptions.filterOp);
    this.newTxn("readonly", BLOCKED_NUMBER_STORE_NAME, function (txn, store) {
      let filterOps = ["equals", "fuzzyMatch"];
      if (aOptions && filterOps.indexOf(aOptions.filterOp) >= 0) {
        this._findBlockedNumbersWithIndex(txn, store, aOptions);
      } else {
        this._findAllBlockedNumbers(txn, store, aOptions);
      }
    }.bind(this), aSuccessCb, aFailureCb);
  },

  _findBlockedNumbersWithIndex: function CDB_findBlockedNumbersWithIndex(txn, store, options) {
    if (DEBUG) debug("_findBlockedNumbersWithIndex: " + options.filterValue + " " + options.filterOp + " " + options.filterBy + " ");
    let fields = options.filterBy;
    if (!this._validFields(txn, store, fields));

    let limit = options.filterLimit | null;
    let filter_keys = fields.slice();
    for (let key = filter_keys.shift(); key; key = filter_keys.shift()) {
      let request;
      if (options.filterOp === 'fuzzyMatch') {
        if (key !== "number") {
          dump("ContactDB: 'fuzzyMatch' filterOp only works on number\n");
          return txn.abort();
        }

        let index = store.index("numberFuzzy");
        let filterValue = options.filterValue.toString();
        let substringDigits = this.substringMatching; //this._getMinMatchDigits();
        if (filterValue.length >= substringDigits) {
          filterValue = filterValue.slice(-substringDigits);
          filterValue = filterValue.split("").reverse().join("");
          request = index.mozGetAll(IDBKeyRange.bound(filterValue, filterValue + "\uFFFF"), limit);
        } else {
          request = index.mozGetAll(filterValue.split("").reverse().join(""), limit);
        }
      } else {
        request = store.mozGetAll(options.filterValue, limit);
      }
      if (!txn.result) {
        txn.result = {};
      }

      request.onsuccess = function(event) {
        if (DEBUG) debug("Request successful. Record count: " + event.target.result.length);
        let numbers = event.target.result;
        for (let i in numbers) {
          if (DEBUG) debug("Blocked numbers i: " + i);
          txn.result[numbers[i].number] = numbers[i].number;
        }
      }.bind(this);
    }
  },

  _findAllBlockedNumbers: function CDB_findAllBlockedNumbers(txn, store, options) {
    if (!txn.result) {
      txn.result = {};
    }

    let req = store.mozGetAll();
    req.onsuccess = function(event) {
      let numbers = event.target.result;
      if (DEBUG) debug("Request successful. Blocked numbers size: " + numbers.length + ", " + JSON.stringify(numbers));
      for (let i in numbers) {
        if (DEBUG) debug("blocked number i: " + i);
        txn.result[numbers[i].number] = numbers[i].number;
      }
    }.bind(this);

    req.onerror = function (e) {
      dump("_findAllBlockedNumbers: " + e);
    }.bind(this);
  },

  saveBlockedNumber: function CDB_saveBlockNumber(aNumber, aSuccessCb, aFailureCb) {
    if (DEBUG) debug("saveBlockNumber: aNumber " + aNumber);
    this.newTxn("readwrite", BLOCKED_NUMBER_STORE_NAME, function (txn, store) {
      if (!aNumber) {
        dump("ContactDB: aName is empty");
        txn.abort();
        return;
      }

      let blockedNumber = {
        number: aNumber,
        search: {
          fuzzyNumber: getReversed(aNumber)
        }
      };

      store.put(blockedNumber);
      this.incrementRevision(txn);

    }.bind(this), aSuccessCb, aFailureCb);
  },

  removeBlockedNumber: function CDB_removeBlockedNumber(aNumber, aSuccessCb, aFailureCb) {
    if (DEBUG) debug("removeBlockedNumber: aNumber " + aNumber);
    this.newTxn("readwrite", this.dbStoreNames, function (txn, store) {
      txn.objectStore(BLOCKED_NUMBER_STORE_NAME).delete(aNumber).onsuccess = function() {
        aSuccessCb();
      }.bind(this);

      this.incrementRevision(txn);
    }.bind(this), null, aFailureCb);
  },

  init: function init() {
    this.initDBHelper(DB_NAME, DB_VERSION, [STORE_NAME, SPEED_DIALS_STORE_NAME,
                                            SAVED_GETALL_STORE_NAME, REVISION_STORE,
                                            GROUP_STORE_NAME, BLOCKED_NUMBER_STORE_NAME]);
  }
};
