---
layout: page
title: Launch hidden settings
custom_js: w2d
---
**Table of Contents**
- [W2D KaiOS Jailbreak](#w2d-kaios-jailbreak)
- [Hotspot for JioPhones](#hotspot-for-jiophones)
- [Connect to ADB wirelessly](#connect-to-adb-wirelessly)
- [Readout (not recommended)](#readout-not-recommended)

Note that for the buttons on this page to function, you need to open this page in the built-in Browser app on your KaiOS device.

## W2D KaiOS Jailbreak
This button below will attempt to open the hidden Developer menu in your phone's Settings app, where you can enable ADB and DevTools access to the low-level functions of the phone. This allows many devices previously categorized as debug-locked to be debugged from a computer. 

Works on both KaiOS 2.5 and KaiOS 3. 

*This was discovered in 2020 by [tbrrss](https://kaios.dev) and Luxferre, and later fine-tuned by [Cyan](https://github.com/cyan-2048) for GitHub Pages environment.*

<button onclick="openMenu('developer')">Launch Developer menu</button>

## Hotspot for JioPhones
On some devices, particularly JioPhones and newer locked Nokia phones, the Internet sharing menu under Network & Connectivity in Settings is hidden although the hardware is perfectly capable of doing such thing. The button below should attempt to open that page. 

Works on both KaiOS 2.5 and KaiOS 3.

<button onclick="openMenu('hotspot')">Launch Hotspot menu</button>

## Connect to ADB wirelessly
If you don't have an USB cable nearby and need to quickly debug your app, this button below will open an ADB port of 5555 on your phone so that you can connect to your computer's ADB wirelessly by typing `adb connect [your.phone.ip.address]:5555`. For more details, see Sideloading and debugging/ADB and WebIDE.

You can find your phone's local IP address (192.168.1.x) by going to *Settings, Network & Connectivity, Wi-Fi, Available networks* and click on the connected Wi-Fi access point; or download N4NO's [My IP Address](https://www.kaiostech.com/store/apps/?bundle_id=com.n4no.myipaddress) from KaiStore.

Note that both the phone and computer have to be on the same Wi-Fi network (you can tether to your computer), your phone has to allow at least one of these three permissions: `engmodeExtension`, `jrdExtension`, `kaiosExtension` and you have to turn on debugging mode on your phone prior to clicking this button.

<button onclick="wadb()">Set ADB port to 5555</button>

## Readout (not recommended)
This button below will attempt to open the hidden Readout screen reader menu in Settings, which is hidden on many devices because it is unusable outside of KaiOS's built-in apps. Note that on most devices, the menu will be blank. 

On devices with dedicated volume buttons, you can quickly toggle on/off this feature by repeatedly pressing Volume up and Volume down. You can also toggle the feature under Device Settings in WebIDE.

<button onclick="openMenu('readout')">Launch Readout menu</button>

*For list of all hidden settings accessible via mozActivity, check out [Cyan's dedicated website](https://cyan-2048.github.io/kaios_scripts).*