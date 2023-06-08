// screen.js:
// Set the screen size, pixel density and scaling of the b2g client screen
// based on the --screen command-line option, if there is one.
// 

Cu.import("resource://gre/modules/GlobalSimulatorScreen.jsm");

window.addEventListener('ContentStart', onStart);
window.addEventListener('SafeModeStart', onStart);

// We do this on ContentStart and SafeModeStart because querying the
// displayDPI fails otherwise.
function onStart() {
  // This is the toplevel <window> element
  let shell = document.getElementById('shell');

  // The <browser> element inside it
  let browser = document.getElementById('systemapp');

  // Figure out the native resolution of the screen
  let windowUtils = window.QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Components.interfaces.nsIDOMWindowUtils);
  let hostDPI = windowUtils.displayDPI;

  const DEFAULT_SCREEN = '240x320';

  // This is a somewhat random selection of named screens.
  // Add more to this list when we support more hardware.
  // Data from: http://en.wikipedia.org/wiki/List_of_displays_by_pixel_density
  let screens = {
    iphone: {
      name: 'Apple iPhone', width:320, height:480,  dpi:163
    },
    ipad: {
      name: 'Apple iPad', width:1024, height:768,  dpi:132
    },
    nexus_s: {
      name: 'Samsung Nexus S', width:480, height:800, dpi:235
    },
    galaxy_s2: {
      name: 'Samsung Galaxy SII (I9100)', width:480, height:800, dpi:219
    },
    galaxy_nexus: {
      name: 'Samsung Galaxy Nexus', width:720, height:1280, dpi:316
    },
    galaxy_tab: {
      name: 'Samsung Galaxy Tab 10.1', width:800, height:1280, dpi:149
    },
    wildfire: {
      name: 'HTC Wildfire', width:240, height:320, dpi:125
    },
    tattoo: {
      name: 'HTC Tattoo', width:240, height:320, dpi:143
    },
    salsa: {
      name: 'HTC Salsa', width:320, height:480, dpi:170
    },
    chacha: {
      name: 'HTC ChaCha', width:320, height:480, dpi:222
    },
  };

  // Get the command line arguments that were passed to the b2g client
  let args;
  try {
    let service = Cc["@mozilla.org/commandlinehandler/general-startup;1?type=b2gcmds"].getService(Ci.nsISupports);
    args = service.wrappedJSObject.cmdLine;
  } catch(e) {}

  let screenarg = null;

  // Get the --screen argument from the command line
  try {
    if (args) {
      screenarg = args.handleFlagWithParam('screen', false);
    }

    // Override default screen size with a pref
    if (screenarg === null && Services.prefs.prefHasUserValue('b2g.screen.size')) {
      screenarg = Services.prefs.getCharPref('b2g.screen.size');
    }

    // If there isn't one, use the default screen
    if (screenarg === null)
      screenarg = DEFAULT_SCREEN;

    // With no value, tell the user how to use it
    if (screenarg == '')
      usage();
  }
  catch(e) {
    // If getting the argument value fails, its an error
    usage();
  }
  
  // Special case --screen=full goes into fullscreen mode
  if (screenarg === 'full') {
    shell.setAttribute('sizemode', 'fullscreen');
    return;
  } 

  let width, height, ratio = 1.0;
  let lastResizedWidth;

  if (screenarg in screens) {
    // If this is a named screen, get its data
    let screen = screens[screenarg];
    width = screen.width;
    height = screen.height;
    ratio = screen.ratio;
  } else {
    // Otherwise, parse the resolution and density from the --screen value.
    // The supported syntax is WIDTHxHEIGHT[@DPI]
    let match = screenarg.match(/^(\d+)x(\d+)(@(\d+(\.\d+)?))?$/);
    
    // Display usage information on syntax errors
    if (match == null)
      usage();
    
    // Convert strings to integers
    width = parseInt(match[1], 10);
    height = parseInt(match[2], 10);
    if (match[4])
      ratio = parseFloat(match[4], 10);

    // If any of the values came out 0 or NaN or undefined, display usage
    if (!width || !height || !ratio) {
      usage();
    }
  }

  Services.prefs.setCharPref('layout.css.devPixelsPerPx',
                             ratio == 1 ? -1 : ratio);
  GlobalSimulatorScreen.width = width;
  GlobalSimulatorScreen.height = height;
  const defaultOrientation = width < height ? 'portrait' : 'landscape';
  GlobalSimulatorScreen.mozOrientation = GlobalSimulatorScreen.screenOrientation = defaultOrientation;

  const container = document.getElementById('container');
  container.style.width = width + 'px';
  container.style.height = height + 'px';

  print('[screen.js]: set size: width, height:', width, height);

  // A utility function like console.log() for printing to the terminal window
  // Uses dump(), but enables it first, if necessary
  function print() {
    let dump_enabled =
      Services.prefs.getBoolPref('browser.dom.window.dump.enabled');

    if (!dump_enabled)
      Services.prefs.setBoolPref('browser.dom.window.dump.enabled', true);

    dump(Array.prototype.join.call(arguments, ' ') + '\n');

    if (!dump_enabled) 
      Services.prefs.setBoolPref('browser.dom.window.dump.enabled', false);
  }

  // Print usage info for --screen and exit
  function usage() {
    // Documentation for the --screen argument
    let msg = 
      'The --screen argument specifies the desired resolution and\n' +
      'pixel density of the simulated device screen. Use it like this:\n' +
      '\t--screen=WIDTHxHEIGHT\t\t\t// E.g.: --screen=320x480\n' +
      '\t--screen=WIDTHxHEIGHT@DOTS_PER_INCH\t// E.g.: --screen=480x800@250\n' +
      '\t--screen=full\t\t\t\t// run in fullscreen mode\n' +
      '\nYou can also specify certain device names:\n';
    for(let p in screens)
      msg += '\t--screen=' + p + '\t// ' + screens[p].name + '\n';

    // Display the usage message
    print(msg);

    // Exit the b2g client
    Services.startup.quit(Ci.nsIAppStartup.eAttemptQuit);
  }
}
