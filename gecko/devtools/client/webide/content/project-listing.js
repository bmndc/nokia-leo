/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* eslint-env browser */

var Cu = Components.utils;
const {require} = Cu.import("resource://devtools/shared/Loader.jsm", {});
const ProjectList = require("devtools/client/webide/modules/project-list");
Cu.import("resource://gre/modules/Services.jsm");

var projectList = new ProjectList(window, window.parent);

window.addEventListener("load", function onLoad() {
  window.removeEventListener("load", onLoad, true);
  document.getElementById("new-app").onclick = CreateNewApp;
  document.getElementById("hosted-app").onclick = ImportHostedApp;
  document.getElementById("packaged-app").onclick = ImportPackagedApp;
  document.getElementById("refresh-tabs").onclick = RefreshTabs;
  projectList.update();
  projectList.updateCommands();
  Services.obs.addObserver(observe, "CMD_NEWAPP", false);
  Services.obs.addObserver(observe, "CMD_IMPORT_HOSTEDAPP", false);
  Services.obs.addObserver(observe, "CMD_IMPORT_PACKAGEDAPP", false);
}, true);

window.addEventListener("unload", function onUnload() {
  window.removeEventListener("unload", onUnload);
  Services.obs.removeObserver(observe, "CMD_NEWAPP");
  Services.obs.removeObserver(observe, "CMD_IMPORT_HOSTEDAPP");
  Services.obs.removeObserver(observe, "CMD_IMPORT_PACKAGEDAPP");
  projectList.destroy();
});


function observe(aSubject, aTopic, aData) {
  switch (aTopic) {
    case "CMD_NEWAPP":
      CreateNewApp();
      break;
    case "CMD_IMPORT_HOSTEDAPP":
      ImportHostedApp();
      break;
    case "CMD_IMPORT_PACKAGEDAPP":
      ImportPackagedApp();
      break;
    default:
      break;
  }
}

function RefreshTabs() {
  projectList.refreshTabs();
}

function CreateNewApp() {
  projectList.newApp();
}

function ImportHostedApp() {
  projectList.importHostedApp();
}

function ImportPackagedApp() {
  projectList.importPackagedApp();
}
