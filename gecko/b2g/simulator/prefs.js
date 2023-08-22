// See http://mxr.mozilla.org/mozilla-central/source/dom/webidl/KeyEvent.webidl
// for keyCode values.
// Default value is F5
pref("b2g.reload_key", '{ "key": 116, "shift": false, "ctrl": false, "alt": false, "meta": false }');

#ifdef MOZ_MULET
// Set FxOS as the default homepage
// bug 1000122: this pref is fetched as a complex value,
// so that it can't be set a just a string.
// data: url is a workaround this.
pref("browser.startup.homepage", "data:text/plain,browser.startup.homepage=chrome://kaios-sdk/content/kairt.xul");
#endif

#ifndef MOZ_MULET
pref("toolkit.defaultChromeURI", "chrome://kaios-sdk/content/index.html");
pref("browser.chromeURL", "chrome://kaios-sdk/content/");
#endif

// To connect runtime at boot
pref("devtools.webide.enableLocalRuntime", true);

// Automatically open devtools on the firefox os panel
pref("devtools.toolbox.sidebar.width", 470);

// Enable keyboardEventGenerator
pref("dom.keyboardEventGenerator.enabled", true);

pref('browser.dom.window.dump.enabled', true);

// Required for Mulet in order to run the debugger server from the command line
pref('devtools.debugger.remote-enabled', true);
pref('devtools.chrome.enabled', true);

// Default filp type device. Set false for bar type
pref('device.capability.flip', true);
pref('device.capability.endcall-key', true);
pref('device.capability.volume-key', true);

// Relay keyBoard event to keyboard app
pref('b2g.relayToKeyboard.enabled', false);

pref("dom.w3c_touch_events.enabled", 0);
