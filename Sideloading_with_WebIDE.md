# Sideloading with WebIDE

## Terminology

According to [How-To Geek](https://www.howtogeek.com/773639/what-is-sideloading-and-should-you-do-it), sideloading is 'the practice of installing software on a device without using the approved app store or software distribution channel'. You may want to do so because a cool piece of software is not available in the device's app store, either because it doesn't meet the app store's requirements, it's not available in your region, or you're a developer and looking to test your newly-made app before making it public.

**This is where Android Debug Bridge (ADB) and WebIDE comes in.** Every piece of electronic devices has some sort of special protocols for developers and engineers to communicate and perform special functions, including sideloading apps, and KaiOS devices are not an exception. ADB and WebIDE are handy tools for engineers to decode those protocols.

For those born after the 90s, ADB works in the command-line interface, which is a computer interface that only accept keyboard input, in which you type in instructions for the computer to execute ([freeCodeCamp](https://www.freecodecamp.org/news/command-line-for-beginners)). ADB is instruction-bundled so you just have to tell ADB what to do, and it'll do the rest. On the other hand, WebIDE works in graphical, which is what you usually see on phones and computers these days, and even what you're looking at right now.

...I think that should be it. If you understand all of these you're good to go.

**Why do we use ADB and a browser for WebIDE?**

KaiOS inherited most of its development from Mozilla's now-defunct Firefox OS. Firefox OS and KaiOS devices, in general, use an Android compatibility layer for hardware reasons.

Mozilla also needed to have a place in their browser for Firefox OS development.

## What we'll need

* a computer with built-in command-line tool ready: Command Prompt on Windows, Terminal on macOS/Linux
* a phone running KaiOS 2.5 (**KaiOS 3 users should not follow this guide**)
* an USB cable capable of data transferring
* an Internet connection for your phone to navigate to the W2D site (not needed if yours has a code to toggle debugging mode) and for your computer to read this guide (apparently) and download tools needed
* an archive extractor installed to extract our tools: Linux/macOS's built-in file manager, [7-Zip](https://www.7-zip.org) on Windows or, if you prefer, WinRAR

*KaiOS 3 has a different mean of sideloading apps developed by KaiOS Technologies themselves called `appscmd` which can be used if your device allows to do so. Instructions for using the tool can be found on [their Developer Portal](https://developer.kaiostech.com/docs/sfp-3.0/getting-started/env-setup/os-env-setup).*

## Sideloading 101

*Need a video tutorial? If you're on Linux, KaiOS Technologies officially made one for their own WebIDE client KaiOSRT which can be found [here](https://www.youtube.com/watch?v=wI-HW2cLrew). Alternatively, there's also one on [BananaHackers' YouTube channel](https://www.youtube.com/watch?v=SoKD7IBTvM4).*

### Turning on debugging mode

1. Check whether your phone can be debugged and if there are any special notes to follow in the [Devices page on BananaHackers Wiki](https://wiki.bananahackers.net/devices).

*Some devices may have specific codes that can be dialed right from the home screen to quickly activate debugging mode, i.e. `*#*#33284#*#*` for Nokia devices and both `*#*#33284#*#*` and `*#*#0574#*#*` for Energizers and some other devices. More details of this can be found on the [Devices page](https://wiki.bananahackers.net/devices).*

*For our Nokia 6300 4G, debugging mode can be turned on within *Settings > Device > Developer* or dialing `*#*#33284#*#*`. No further actions needed if you aren't sideloading apps with 'forbidden' permissions (see README).*

2. Open the phone's Browser and go to https://w2d.bananahackers.net. Use D-Pad keys to move the cursor and click on the big front *Launch Developer menu* button.

![Demostration of the W2D website opened in KaiOS browser, with a huge Launch developer menu button shown in front](/development/webide/w2d.jpeg)

3. In the newly opened Developer menu, select the first *Debugger* option, then select *ADB and DevTools* in the dropdown menu. You should see a bug icon in the status bar letting you know ~~your phone has bugs inside~~ you're in debugging mode.

![Demostration of the Debugger menu with three options Disabled, ADB only and ADB and DevTools shown. The last is highlighted and selected. An icon in shape of a bug can be seen in the status bar](/development/webide/developer_menu.jpeg)

4. Connect the phone to your computer with the USB cable.

**If you're connecting to a Linux-based PC, you may need to go to Settings > Storage and turn on USB Storage for `udev` to properly register your phone as an USB peripheral. An icon in the status bar will appear indicating storage access via USB.**

![Demostration of the Storage page within Settings where the USB Storage option is highlighted and shown as Enabled. An icon in shape of the USB logo can be seen in the status bar](/development/webide/usb_storage.jpeg)

### Setting up ADB

> Now, if your operating system has a package manager, you can utilize that to quickly install and set up ADB:
> 
> * Windows: `choco install adb` (`winget` unfortunately [prohibits installing executables with symlinks](https://github.com/microsoft/winget-pkgs/issues/4082))
> * macOS: `brew install android-platform-tools`
> * Linux (Debian/Ubuntu): `sudo apt-get install adb`
> * Linux (Fedora): `sudo dnf install android-tools`
> * Linux (Arch): `sudo pacman -S android-tools`
> 
> Skip to step 7 when you're done.

5. On your computer, visit [Android Developers' SDK Platform Tools website](https://developer.android.com/tools/releases/platform-tools) and click on the link correspond to your operating system under the Downloads section to download ADB to your computer. Read the terms if you want to, tick the box and click the green button to have the SDK downloaded to your computer.

![Screenshot of Android Developers' SDK download page with download links for three operating systems under the Downloads section](/development/webide/sdk_website.png)

![Screenshot of SDK term agreement pop-up](/development/webide/sdk_download.png)

6. Extract the downloaded archive to a folder (double-click the file on macOS/Linux, *7-Zip > Extract into {folder-name}* on Windows), navigate to its `platform-tools` root and open a command-line window within that.

7. Type `adb devices` to start the ADB server. If a `device` shows, that means your phone is being detected by ADB and you're good to go, otherwise double-check.

![Demostration of a Windows Terminal window showing the result after typing the command above, listing '1a2b3c4d device' as one of the devices attached](/development/webide/adb_devices.png)

> For the sake of simplicity, I've moved the archive to the desktop and extract its content there. When I open the Command Prompt on my Windows machine, I just had to type `cd Desktop\platform-tools`, followed by `adb devices`.

*Tip: If you've downloaded the SDK package from Android Developers' website, for quicker access next time, include the extracted ADB folder in PATH. We won't cover this here as this would be a lengthy process. This will be automatically handled if you've installed ADB via package manager.*

### Setting up WebIDE

8. We'll use [Waterfox Classic](https://classic.waterfox.net) for WebIDE (Firefox v59, Pale Moon <28.6.1 and command-line tools will also do the job just fine, see below). To download, head to the browser's homepage, download and install the version correspond to your OS.

![Screenshot of Waterfox Classic browser's homepage with blue download buttons for three operating systems at the middle of the page](/development/webide/waterfox_classic.png)

9. Open the browser and press the hamburger menu button at the top right of the toolbar, and click the Developer entry. Then click WebIDE to open it.

![Demostration of the main dropdown menu when pressing the hamburger button, with Developer highlighted as the tenth option from top to bottom, left to right](/development/webide/waterfox_classic_menu.jpeg)

![Demostration of the main menu sliding out to show the Web Developer submenu, with WebIDE highlighted as the tenth option from top to bottom](/development/webide/waterfox_classic_dev.jpeg)

*Tip: For quicker access to WebIDE, press its shortcut `Shift` + `F8` while you're in the browser.*

### Connecting phone to WebIDE

10. Your phone's name should already appear in the right pane. Click it to connect and skip to step 12. If you don't see any, type this into the command-line window:
```
adb forward tcp:6000 localfilesystem:/data/local/debugger-socket
```

![Demostration of WebIDE interface, with the device's name as the first option under USB Devices section in the right pane](/development/webide/device_name.png)

![Demostration of a Windows Terminal window showing the '6000' result after typing the command above](/development/webide/adb_forward.png)

11. In WebIDE, click Remote Runtime, leave it as default at `localhost:6000` and press OK. If you still cannot connect your phone to WebIDE, double-check if you missed any step.

![Demostration of WebIDE interface with Remote Runtime pop-up shown after pressing the option in the right pane. The pop-up has 'localhost:6300' as the content of the input box, with two buttons to confirm or close.](/development/webide/localhost_6000.png)

*If you're using other means to access WebIDE such as Firefox v59 or Pale Moon <28.6.1, you may now see a warning header about mismatched build date. You can safely ignore it as WebIDE was mainly designed to support Firefox OS device builds released alongside that Firefox/Pale Moon versions.*

### Sideloading apps

12. To sideload an app, download it and extract its ZIP content (if you see an OmniSD-packaged `application.zip` you may need to extract once more). Select Open Packaged Apps in WebIDE's left sidebar and navigate to the root of the app folder you just extracted.

![Demostration of WebIDE interface with a device connected. The Open Packaged Apps is being highlighted as the second option in the left pane from top to bottom](/development/webide/open_packaged_apps.png)

13. Once you've got the app loaded, press the triangle Install and Run in the top bar to sideload!

![Demostration of WebIDE interface with an app selected. The triangle Install and Run button (first one left to right) is being highlighted on the top pane](/development/webide/install_and_run.png)

* If you happen to encounter an issue in a sideload app and want to debug, click the wrench to open the Developer Tools.*

## gdeploy

`gdeploy` is a small cross-platform command-line utility developed by Luxferre as an alternative to the graphical WebIDE, and can even be used as NodeJS module/library. According to Luxferre, 'it uses the same `firefox-client` backend but has much simpler architecture for application management'.

For Windows 10 version 1709 and later, type these commands one by one into Command Prompt, with `[DIR_PATH]` replaced by the extracted folder directory of the app you want to install (see step 12 above):
```
winget install Git.Git
winget install OpenJS.NodeJS.LTS
cd /d "%USERPROFILE%\Desktop"
git clone https://gitlab.com/suborg/gdeploy.git
curl -Lo platform-tools.zip https://dl.google.com/android/repository/platform-tools-latest-windows.zip
tar -xf platform-tools.zip
cd platform-tools\
adb devices
adb forward tcp:6000 localfilesystem:/data/local/debugger-socket
cd ..\gdeploy\
npm i && npm link
gdeploy install [DIR_PATH]
```
For macOS and Linux, if you have Homebrew installed as a package manager:
```
brew install git node android-platform-tools
cd ~/Desktop
git clone https://gitlab.com/suborg/gdeploy.git
adb devices
adb forward tcp:6000 localfilesystem:/data/local/debugger-socket
cd gdeploy
npm i && npm link
gdeploy install [DIR_PATH]
```

## Other alternatives

* [KaiOS RunTime](https://developer.kaiostech.com/docs/02.getting-started/01.env-setup/simulator) (Linux): official developing environment for KaiOS 2.5 made by KaiOS Technologies the company. 

To download and set up KaiOSRT on Ubuntu, type these commands one-by-one in Terminal:
```
wget https://s3.amazonaws.com/kaicloudsimulatordl/developer-portal/simulator/Kaiosrt_ubuntu.tar.bz2
tar -axvf Kaiosrt_ubuntu.tar.bz2
cd kaiosrt-v2.5-ubuntu-20190925163557-n378
tar -axvf kaiosrt-v2.5.en-US.linux-x86_64.tar.bz2
cd kaiosrt
./kaiosrt
```
*It's also possible to get KaiOSRT to work on Windows 10 and later using Windows Subsystem for Linux (WSLg). [See this video on YouTube for action](https://youtu.be/eg2SOCTMxYU).*

* Firefox 59 (ESR 52.9): the last official Firefox version to bundle with working WebIDE and other tools for development on Firefox OS devices, before Mozilla decided to kill the project in 2016. Archives of all Firefox releases can be found on https://archive.mozilla.org.
* Pale Moon 28.6.1 (Windows/Linux): a popular fork of Firefox with older user interface, legacy Firefox add-on support and always running in single-process mode. Archives of all releases can be found on https://www.palemoon.org/archived.shtml.
* [Make KaiOS Install](https://github.com/jkelol111/make-kaios-install): another command-line tool to install apps using KaiOS's remote debugging protocol.
