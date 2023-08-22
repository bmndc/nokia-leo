/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const browserWindow = Services.wm.getMostRecentWindow("navigator:browser");
let contentWindow;

/**
 * Required to avoid black homescreen background
 */
Cu.import("resource://gre/modules/GlobalSimulatorScreen.jsm");
GlobalSimulatorScreen.width = 826;
GlobalSimulatorScreen.height = 540;

/**
 * Set debug mode or not.
 * Use function logger() to log.
 */
const DEBUG = false;
const BAR_TYPE = false;

Services.prefs.setBoolPref('browser.dom.window.dump.enabled', DEBUG);

if (BAR_TYPE) {
  Services.prefs.setBoolPref('device.capability.flip', false);
  Services.prefs.setBoolPref('device.capability.endcall-key', false);
  Services.prefs.setBoolPref('device.capability.volume-key', false);
} else {
  Services.prefs.setBoolPref('device.capability.flip', true);
  Services.prefs.setBoolPref('device.capability.endcall-key', true);
  Services.prefs.setBoolPref('device.capability.volume-key', true);
}

Services.prefs.setCharPref("b2g.reload_key", '{ "key": 116, "shift": false, "ctrl": false, "alt": false, "meta": false }');

/**
 * Dispatch events to content window
 */
function controlEventFactory(element, eventDetail){

  const keg = new contentWindow.KeyboardEventGenerator();
  let timer = 0;
  element.addEventListener('mousedown', function(evt) {
    logger('mousedown: evt, activeElement', eventDetail, contentWindow.document.activeElement);
    timer = Date.now();

    // prevent from grabbing input focus in contentWindow
    evt.preventDefault();

    // required: manually focus each time before sending event to contentWindow
    contentWindow.document.activeElement.focus();

    keg.generate(new contentWindow.KeyboardEvent('keydown', eventDetail));
  });

  element.addEventListener('mouseup', function(evt) {
    logger('mouseup: evt, activeElement', eventDetail, contentWindow.document.activeElement);
    logger('mouse press interval(ms):', Date.now() - timer);

    keg.generate(new contentWindow.KeyboardEvent('keyup', eventDetail));
  });

}

function setupControls() {
  contentWindow = shell.contentBrowser.contentWindow;
  if(!contentWindow.KeyboardEventGenerator){
    dump('KeyboardEventGenerator undefined, is pref("dom.keyboardEventGenerator.enabled", true) at b2g.js ?');
    return;
  }

  const controlEventMap = {
    'lsk-anchor': { key: 'SoftLeft', keyCode: 0 },
    'rsk-anchor': { key: 'SoftRight', keyCode: 0 },
    'call-anchor': { key: 'Call', keyCode: 0 },
    'endcall-anchor': { key: 'EndCall', keyCode: 95 },
    'csk-anchor': { key: 'Enter', keyCode: 13 },
    'csk-l-anchor': { key: 'ArrowLeft', keyCode: 37 },
    'csk-u-anchor': { key: 'ArrowUp', keyCode: 38 },
    'csk-r-anchor': { key: 'ArrowRight', keyCode: 39 },
    'csk-d-anchor': { key: 'ArrowDown', keyCode: 40 },
    'num1-anchor': { key: '1', keyCode: 49 },
    'num2-anchor': { key: '2', keyCode: 50 },
    'num3-anchor': { key: '3', keyCode: 51 },
    'num4-anchor': { key: '4', keyCode: 52 },
    'num5-anchor': { key: '5', keyCode: 53 },
    'num6-anchor': { key: '6', keyCode: 54 },
    'num7-anchor': { key: '7', keyCode: 55 },
    'num8-anchor': { key: '8', keyCode: 56 },
    'num9-anchor': { key: '9', keyCode: 57 },
    'num0-anchor': { key: '0', keyCode: 48 },
    'star-anchor': { key: '*', keyCode: 170 },
    'hash-anchor': { key: '#', keyCode: 163 }
  };

  // simulate endcall as backspace for bartype
  if (!Services.prefs.getBoolPref('device.capability.endcall-key')) {
    controlEventMap['endcall-anchor'] = { key: 'Backspace', keyCode: 8 };
  }

  Object.keys(controlEventMap).forEach( id => {
    const elem = document.getElementById(id);
    controlEventFactory(elem, controlEventMap[id]);
  });
}

function setupStorage() {
  let directory = null;

  // Get the --storage-path argument from the command line.
  try {
    let service = Cc['@mozilla.org/commandlinehandler/general-startup;1?type=b2gcmds'].getService(Ci.nsISupports);
    let args = service.wrappedJSObject.cmdLine;
    if (args) {
      let path = args.handleFlagWithParam('storage-path', false);
      directory = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsIFile);
      directory.initWithPath(path);
    }
  } catch(e) {
    directory = null;
  }

  // Otherwise, default to 'storage' folder within current profile.
  if (!directory) {
    directory = Services.dirsvc.get('ProfD', Ci.nsIFile);
    directory.append('storage');
    if (!directory.exists()) {
      directory.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt("755", 8));
    }
  }
  dump("Set storage path to: " + directory.path + "\n");

  // This is the magic, where we override the default location for the storages.
  Services.prefs.setCharPref('device.storage.overrideRootDir', directory.path);
}

function openDevtools() {
  // Open devtool panel while maximizing its size according to screen size
  Services.prefs.setIntPref('devtools.toolbox.sidebar.width',
                            browserWindow.outerWidth - 550);
  Services.prefs.setCharPref('devtools.toolbox.host', 'side');
  let {gDevTools} = Cu.import('resource://devtools/client/framework/gDevTools.jsm', {});
  let {devtools} = Cu.import("resource://devtools/shared/Loader.jsm", {});
  let target = devtools.TargetFactory.forTab(browserWindow.gBrowser.selectedTab);
  gDevTools.showToolbox(target);
}

window.addEventListener('ContentStart', function() {
  setupDebugger();
  setupControls();

//  setupStorage();
//  openDevtools();
});

/**
 * Handle for '-start-debugger-server somePort' option
 * Be able to use remote runtime in WebIDE
 */
function setupDebugger() {

  // Get the command line arguments that were passed to the b2g client
  const service = Cc["@mozilla.org/commandlinehandler/general-startup;1?type=b2gcmds"].getService(Ci.nsISupports);
  const args = service.wrappedJSObject.cmdLine;
  if(!args) return;

  const dbgport = args.handleFlagWithParam('start-debugger-server', false);
  if(dbgport){
    dump('Opening debugger server on ' + dbgport + '\n');
    Services.prefs.setCharPref('devtools.debugger.unix-domain-socket', dbgport);
    navigator.mozSettings.createLock().set({'debugger.remote-mode': 'adb-devtools'});
  }

}

/**
 * Logger for debug mode
 */
function logger(...args){
  if(!Services.prefs.getBoolPref('browser.dom.window.dump.enabled')) return;

  Cu.import('resource://gre/modules/devtools/Console.jsm');
  console.log.apply(console, args);
}

