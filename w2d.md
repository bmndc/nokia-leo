---
layout: page
title: Launch hidden settings
custom_js: w2d
---
Please note that for the buttons on this page to function correctly, you need to open this page in the built-in Browser app on your KaiOS phone.

It's unclear how a page in the Settings app can be hidden on condition. My theory is, when you pop a SIM card into the phone, B2G looks up some information of the carrier and matches a certain ID with one of the default configurations under `/system/b2g/defaults/customization/` (see [GerdaOS's system](https://gitlab.com/project-pris/system/-/tree/master/src/system/b2g/defaults/customization) and [leo's system](https://github.com/bmndc/nokia-leo/tree/system/b2g/defaults/customization)). If it finds a match, B2G will toggle Device Settings flags based on the JSON values, and decide whether to enable certain features of the phone or not.

Now, what this does is merely hiding the menus from the Settings app so that normal users cannot access it. But they can still be opened with other means.

mozActivity on Firefox OS and KaiOS 2.5, and webActivity on KaiOS 3+, is a system-level function which allows applications to share data, view and perform actions across each other. An app can register to handle a mozActivity/webActivity in its `manifest.webapp` file, and other apps can reference to a registered mozActivity/webActivity to perform actions e.g. sharing photos and videos from Gallery to Messages, or opening a page in the Settings app if the phone isn't connected to the Internet (see [Web Activities on KaiOS](https://kaios.dev/2023/02/web-activities-on-kaios/)).

On 22 September 2020, Tom Barrasso (tbrrss) and Luxferre made a striking discovery: not only installed apps can activate a mozActivity and webActivity, websites when browsed in the built-in Browser app can also trigger them. This page works by using mozActivity and webActivity to open hidden menus in the Settings app.

Source code of the JavaScript code used on this page can be found [here](https://github.com/bmndc/nokia-leo/blob/docs/assets/js/w2d.js).

## W2D KaiOS Jailbreak
Under Settings, Device, the Developer menu houses options to turn on ADB and DevTools, plus a number of GUI debugging tools. On production devices this menu is hidden by default. This button below will attempt to open the menu. Works on both KaiOS 2.5 and KaiOS 3. (W2D: web-to-dev)

Once you connect to WebIDE, you can toggle the Device Settings flag `developer.menu.enabled` to have this menu shown permanently.

<button onclick="openMenu('developer')">Launch Developer menu</button>

## Hotspot for JioPhones
Attempt to open Internet sharing menu under Settings, Network & Connectivity, which is hidden on JioPhones and newer, locked HMD/Nokia phones, although the hardware is perfectly capable of doing such thing. Works on both KaiOS 2.5 and KaiOS 3.

*Note: On the latest JioPhone Prima 4G, the phone will appear to open the menu, then redirect to a blank page.*

<button onclick="openMenu('hotspot')">Launch Hotspot menu</button>

## Turn on Greyscale Mode or change contrast level
Attempt to open the incomplete Color Filter menu, where you can set your screen to display black-and-white only or invert all colours. If you want to adjust the contrast level of the screen, click on the second button; Contrast option in the Color Filter menu does not do anything.

Use D-Pad Up and Down to increase or decrease the contrast level respectively.

<button onclick="openMenu('accessibility-colors')">Launch Color Filter menu</button>
<button onclick="openMenu('colorfilter-contrast')">Launch Contrast menu</button>

## Turn off Flip open to answer
On older KaiOS flip or sliding phones, such as the Nokia 8110 4G and Nokia 2720 Flip, opening the phone will automatically answer any incoming calls. This button below will attempt to open the Answer Mode menu where you can disable that behaviour. On KaiOS 3 flip phones, this menu is shown by default.

Alternatively, you can connect the phone to WebIDE and toggle the Device Settings flag `phone.answer.flipopen.enabled`.

<button onclick="openMenu('answer-mode')">Launch Answer Mode menu</button>

## Readout (not recommended)
Attempt to open the Readout menu in Settings, where you can turn on KaiOS's built-in screen reader and change its speech rate. Only consider using this feature if you plan to *only* use built-in apps; most apps from KaiStore do not properly label their buttons, which makes this feature unusable.

On phones with dedicated volume buttons, you can toggle this feature by repeatedly pressing the side volume buttons Vol+ and Vol-. You can also toggle the Device Settings flag `accessibility.screenreader` in WebIDE.

<button onclick="openMenu('accessibility-readout-mode')">Launch Readout menu</button>

## Connect to ADB wirelessly (experimental)
If you don't have an USB cable nearby, this button below will attempt to open a 5555 port for `adbd` on your phone so that you can connect to ADB on your computer wirelessly by typing `adb connect [your.phone.ip.address]:5555`. For more details, see [Sideloading and debugging third-party applications](https://github.com/bmndc/nokia-leo/wiki/Sideloading-and-debugging-third%E2%80%90party-applications).

You can find your phone's local IP address (192.168.1.x) by going to Settings, Network & Connectivity, Wi-Fi, Available networks and click on the connected Wi-Fi access point; or download N4NO's [My IP Address](https://www.kaiostech.com/store/apps/?bundle_id=com.n4no.myipaddress) from KaiStore.

Note that both the phone and computer have to be on the same Wi-Fi network (you can tether to your computer), your phone has to allow at least one of these three permissions: `engmodeExtension`, `jrdExtension`, `kaiosExtension` and you have to turn on debugging mode on your phone prior to clicking this button.

<button onclick="wadb()">Set ADB port to 5555</button>

## VoLTE/VoWiFi (doesn't work)
Attempt to open the VoLTE/VoWiFi menu under Network &amp; Connectivity, as some carriers may not support VoLTE and/or VoWiFi for certain devices. mozActivity fails to highlight list items in this menu and you won't be able to select anything.

<button onclick="openMenu('volte-vowifi')">Launch VoLTE/VoWiFi menu</button>

*For list of all hidden settings accessible via mozActivity, check out [Cyan's dedicated website](https://cyan-2048.github.io/kaios_scripts).*