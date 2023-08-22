'use strict';

(function() {
  const DEBUG = false;

  const {AppManager} = require("devtools/client/webide/modules/app-manager");
  const {RuntimeTypes} = require("devtools/client/webide/modules/runtimes");

  // need to wait UI.init() and AppManager.init() done
  window.addEventListener('load', function() {
    const runtimes = AppManager.runtimeList.other;
    for (let runtime of runtimes) {
      logger('Finding local runtime:', runtime);
      if (runtime.type == RuntimeTypes.LOCAL) {
        logger('Local runtime enabled and found');
        AppManager.connectToRuntime(runtime).then(null, logger);
        break;
      }
    }
  });

  /**
   * Logger for debug mode
   */
  function logger(...args){
    const info = ['[kairt.js]'].concat(args);
    if (Services.prefs.getBoolPref('browser.dom.window.dump.enabled')) {
      dump(info);
      dump('\n');
    }
    if (DEBUG) {
      console.log.apply(console, info);
    }
  }
})();
