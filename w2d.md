---
layout: page
title: Launch hidden settings
custom_js: w2d
---
Please note that for the buttons on this page to function correctly, you need to open this page in the built-in Browser app on your KaiOS phone.

It's unclear how a page in the Settings app can be hidden on conditions. My theory is, when you pop a SIM card into the phone, B2G looks up some information of the carrier and matches a certain ID with one of the default configurations under `/system/b2g/defaults/customization/` (see [GerdaOS's main system](https://gitlab.com/project-pris/system/-/tree/master/src/system/b2g/defaults/customization) and [leo's system](https://github.com/bmndc/nokia-leo/tree/system/b2g/defaults/customization)). If it finds a match, B2G will toggle Device Settings flags based on the JSON values, and decide whether to enable certain features of the phone or not.

Now, what this does is merely hiding the menus from the Settings app so that normal users cannot access it. But they can still be opened with other means.

mozActivity on Firefox OS and KaiOS 2.5, and webActivity on KaiOS 3+, is a system-level function which allows applications to share data, view and perform actions across each other. An app can register to handle a mozActivity/webActivity in its `manifest.webapp` file, and other apps can reference to a registered mozActivity/webActivity to perform actions e.g. sharing photos and videos from Gallery to Messages, or opening a page in the Settings app if the phone isn't connected to the Internet.

On 22 September 2020, Tom Barrasso (tbrrss) and Luxferre made a striking discovery: not only installed apps can activate a mozActivity and webActivity, websites when browsed in the built-in Browser app can also trigger them. This page works by using mozActivity and webActivity to open hidden menus in the Settings app.

## W2D KaiOS Jailbreak
Attempt to open the Developer menu in the Settings app, where you can enable ADB and DevTools access. Works on both KaiOS 2.5 and KaiOS 3. (W2D: web-to-dev)

Once you're connected to WebIDE, you can toggle the Device Settings flag `developer.menu.enabled` to have this menu shown permanently.

<button onclick="openMenu('developer')">Launch Developer menu</button>

## Hotspot for JioPhones
Attempt to open Internet sharing menu under Settings, Network & Connectivity, which is hidden on JioPhones and newer locked HMD/Nokia phones... although the hardware is perfectly capable of doing such thing. Works on both KaiOS 2.5 and KaiOS 3.

<button onclick="openMenu('hotspot')">Launch Hotspot menu</button>

## Connect to ADB wirelessly
If you don't have an USB cable nearby, this button below will open an ADB port of 5555 on your phone so that you can connect to your computer's ADB wirelessly by typing `adb connect [your.phone.ip.address]:5555`. For more details, see [ADB and WebIDE/Sideloading and debugging third-party applications](https://github.com/bmndc/nokia-leo/wiki/Sideloading-and-debugging-third%E2%80%90party-applications).

You can find your phone's local IP address (192.168.1.x) by going to Settings, Network & Connectivity, Wi-Fi, Available networks and click on the connected Wi-Fi access point; or download N4NO's [My IP Address](https://www.kaiostech.com/store/apps/?bundle_id=com.n4no.myipaddress) from KaiStore.

Note that both the phone and computer have to be on the same Wi-Fi network (you can tether to your computer), your phone has to allow at least one of these three permissions: `engmodeExtension`, `jrdExtension`, `kaiosExtension` and you have to turn on debugging mode on your phone prior to clicking this button.

<button onclick="wadb()">Set ADB port to 5555</button>

## Turn on Greyscale Mode
In this menu, you can set your screen to greyscale or invert all colours.

<button onclick="openMenu('accessibility-colors')">Launch Color Filter menu</button>

## Turn off Flip to answer

<button onclick="openMenu('answer-mode')">Launch menu</button>

## VoLTE/VoWiFi (not recommended)
Some carriers don't support VoLTE/VoWiFi on KaiOS phones. This attempts to open the VoLTE/VoWiFi menu. Note that this will open a blank page on most devices.

<button onclick="openMenu('volte-vowifi')">Launch VoLTE/VoWiFi menu</button>

## Readout (not recommended)
This button below will attempt to open the hidden Readout screen reader menu in Settings, which is hidden on many devices because it is unusable outside of KaiOS's built-in apps. Note that on most devices, the menu will be blank. 

On devices with dedicated volume buttons, you can quickly toggle on/off this feature by repeatedly pressing Volume up and Volume down. You can also toggle the feature under Device Settings in WebIDE.

<button onclick="openMenu('readout')">Launch Readout menu</button>

*For list of all hidden settings accessible via mozActivity, check out [Cyan's dedicated website](https://cyan-2048.github.io/kaios_scripts).*