|     | **Nokia 6300 4G (nokia-leo)** |
| --- | --- |
| Released | 13 November 2020 |
| Model | TA-1286, TA-1287, TA-1294, TA-1324 |
| ***Specifications*** |  |
| SoC | Qualcomm MSM8909 Snapdragon 210 (4 x 1.1Ghz Cortex-A7) |
| RAM | 512MB LPDDR2/3 |
| GPU | Adreno 304 |
| Storage | 4GB (+ up to 32GB microSDHC card) |
| Network | - 2G GSM<br>- 3G UMTS<br>- 4G LTE (Cat 4)<br>    + EU (except East Ukraine, Azerbaijan, Georgia), APAC: band 1, 3, 5, 7, 8, 20<br>   +  MENA, CN, SSA: band 1, 3, 5, 7, 8, 20, 28, 38, 39, 40, 41<br>    + US: band 2, 4, 5, 12, 17, 66, 71<br>    + LATAM: band 2, 3, 4, 5, 7, 28<br>    + ROW: band 1, 3, 5, 7, 8, 20, 38, 40<br>-   VoLTE & VoWiFi support<br>-   Single or Dual SIM (Nano-SIM, dual-standby) |
| Screen | 320 x 240 (167 PPI)<br>2.8 inches QVGA TFT LCD, 16M colors |
| Bluetooth | 4.0, A2DP, LE |
| Wi-Fi | 802.11b/g/n, Hotspot |
| Peripherals | GPS |
| Cameras | Rear: VGA, LED flash |
| Dimensions<br>(H x W x D) | 131.4 x 53 x 13.7 (mm)<br>5.17 x 2.09 x 0.54 (in) |
| Weight | 104.7g (3.70oz) |
| Ports | - microUSB charging & USB 2.0 data transferring port<br>- 3.5mm headphone jack |
| Battery | Removable Li-Ion 1500mAh (BL-4XL) |
| Specials | TBD |
| ***KaiOS info*** |  |
| Version | KaiOS 2.5.4 |
| Build number | (TA-1286) [20.00.17.01](tel:20.00.17.01), [30.00.17.01](tel:30.00.17.01) |

## Secret codes

- `*#*#33284*#*#`: Toggle debugging mode, allow the phone to be accessed with ADB and DevTools.
- `*#06#`: Display the IMEI(s).
- `*#0000#`: Display device information, such as firmware version, build date, model number, variant and CUID.

## Special boot modes

- Recovery mode: With the device powered off, hold the top `Power` + `*`, or type `adb reboot recovery` when connected to a computer. Allows you to factory reset the device by wiping /data and /cache, view boot and kernel logs, and install patches from `adb sideload` interface or SD card.
- EDL mode: With the device powered off, hold the top `Power` + `*` + `#`, or type `adb reboot edl` when connected to a computer. Boots into a black screen, allows you to read and write partitions in low-level with proprietary Qualcomm tools. Remove the battery to exit.

EDL loader for the international version of this phone (not TA-1324) can be found on BananaHackers' [EDL archive site](https://edl.bananahackers.net/loaders/8k.mbn) with hardware ID 0x009600e100420029. The US version of this phone has been signed with a different PK_HASH and needs a different firehose loader which we currently don't have in archive.

## ROOT: Boot partition patching (non-US only)

On the 6300 4G, 8000 4G and other KaiOS 2.5.4 devices, ADB and WebIDE can be used to sideload third-party applications. However, you won't be able to sideload apps that has ‘forbidden’ permissions (namely `engmode-extension` which can be used to gain exclusive access of the phone, and can be found in most BananaHackers-made apps like Wallace Toolbox) or make changes to the system. Because in order to achieve WhatsApp VoIP feature on this KaiOS version, the security module SELinux is now set to be `Enforced` which checks and reverts system modifications on boot. To gain total read-write access to the device, you'll now have to permanently root the device by setting SELinux to `Permissive` mode.

The guide below has its backbones taken from the main guide on BananaHackers website, but has been rewritten for the most parts to be easy-to-follow. The process will also take somewhat considerable 30 minutes to an hour, so do this when you have the time.

*Note that for the most part, you don't actually need to gain root access in order to do some functions associated with root e.g. instead of completely removing apps with Wallace Toolbox, you can essentially disable them to be accessed from launcher with [this fork of Luxferre's AppBuster](https://github.com/minhduc-bui1/AppBuster). Luxferre also released a Wallace Toolbox alternative called [CrossTweak](https://gitlab.com/suborg/crosstweak) that does not require `engmode-extension` and therefore can be easily sideloaded on KaiOS 2.5.4 devices.*

**This process will void your phone's warranty, disable its ability to do WhatsApp calls and retrieve over-the-air updates, but you can revert this if you keep a backup of the original boot partition. However, there might also be a chance of bricking your phone if you don't do the steps correctly, so do think twice before actually consider doing this and follow the steps carefully! I won't be responsible for any damages done to your phone by following these.**

### What we'll need

- an international non-US version of Nokia 6300 4G (not TA-1324)
- an USB cable capable of data transferring (EDL cables will also do)
- an Internet connection to download the tools needed
- a somewhat-working firehose loader MBN file for the phone (linked above)
- a EDL tools package to read and write system partitions in low-level access (in this guide we'll be using [bkerler's edl.py v3.1](https://github.com/bkerler/edl/releases/tag/3.1))
- a computer with Python and `pip` installed for the EDL tools to work (both are packaged on Python's [official website](https://www.python.org/))
- an image file of Gerda Recovery for the Nokia 8110 4G, since the firehose loader above has a reading bug, we'll use this to access ADB from the recovery mode (download [here](https://cloud.disroot.org/s/3ojAfcF6J2jQrRg/download) or [here](https://drive.google.com/open?id=1ot9rQDTYON8mZu57YWDy52brEhK3-PGh))
- Android Debug Bridge (ADB) installed to read the boot image in Gerda Recovery (see [Development/WebIDE on BananaHackers Wiki](https://wiki.bananahackers.net/en/development/webide))

*Python 2.7 bundled with macOS 10.8 to 12.3 is NOT recommended for following this guide. If you're on Linux, Python and ADB can also be quickly set up by installing with your package manager. We won't be covering this here, as each Linux distro has its own way of installing from package manager.*

- **If you're on Windows, you'll also need these to set up EDL tools:**
	- Qualcomm driver for your PC to detect the phone in EDL mode (included in the EDL tools)
	- the latest Zadig tool to configure `libusb-win32` driver (download [here](https://zadig.akeo.ie/))

- **If you're going the automatic boot partition patching and compilation via Docker route (only recommended for 5-6 year old computers):**
	- Git to clone/download the repository of the patcher tool to your computer ([install guide](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git))
	- Docker Compose to provide the environment for the patcher tool to work (included in Docker Desktop, whose download links can be found [here](https://docs.docker.com/compose/install/))
	- (Windows) WSL 2 with [Linux kernel update package](https://learn.microsoft.com/en-us/windows/wsl/install-manual#step-4---download-the-linux-kernel-update-package) installed (to install WSL 2 turn on Virtualization in BIOS, then open Command Prompt with administrative rights and type `wsl --install`)

- **If you're going the extracting and manual editing by hand route:**
	- Android Image Kitchen v3.8 for Windows (download from XDA [here](https://forum.xda-developers.com/attachments/android-image-kitchen-v3-8-win32-zip.5300919/))
	- Notepad++ to edit the needed files while [preserving line endings](https://www.cs.toronto.edu/~krueger/csc209h/tut/line-endings.html) (needed for Windows, download [here](https://notepad-plus-plus.org/downloads/))
	- Java Runtime Environment for properly signing the boot image (optional, download [here](https://www.java.com/en/download/))

For the sake of simplicity, the guide assumes you've moved the Gerda Recovery image and the MBN loader file into the root of EDL tools folder, which you should do for convenience. If you'd like to have those in other folders, change the directory path accordingly.

### Part 1: Set up EDL

> This portion of the guide was taken from [Development/EDL tools on BananaHackers Wiki](https://wiki.bananahackers.net/development/edl) so that you don't have to switch tabs. Kudos to Cyan for the guides!

#### MacOS & Linux

1. Install Python as normal. (macOS) After the installation, open the `Install Certificates.command` to install SSL certificates needed for downloading packages. 
2. Then, open Terminal and type `sudo -H pip3 install pyusb pyserial capstone keystone-engine docopt` to install the dependencies for EDL tools.
3. Switch your phone to EDL mode and connect it to your computer.
- From the turned on state, turn on debugging mode on your phone by dialing `*#*#33284#*#*`, connect it to your computer and type `adb reboot edl` in a command-line window.
- From the turned off state, hold down `*` and `#` at the same time while inserting the USB cable to the phone.

In both cases, the phone's screen should blink with a 'enabled by KaiOS' logo then become blank. This is normal behaviour letting you know you're in EDL mode and you can proceed.

Additionally, if you're running Linux and have issue with device access:

- Open `/etc/modprobe.d/blacklist.conf` in a text editor and append `blacklist qcserial`.
- Copy both `51-edl.rules` and `50-android.rules` in the root of extracted EDL tools folder to `/etc/udev/rules.d`.

#### Windows

1. Open the Python installer and proceed with installation. Remember to tick the box next to "Add python.exe to PATH". This would make Python able to be called everywhere in the command-line instead of specifically pointing to its folder, which the next part of the guide won't cover on.

![python.png](/assets/python.png)

2. Open Command Prompt with administrator privileges and run this command:<br>`pip3 install pyusb pyserial capstone keystone-engine docopt`

![pythoooon.png](/assets/pythoooon.png)

3. Open the extracted EDL tools folder, go to the Drivers > Windows folder and run `Qualcomm_Diag_QD_Loader_2016_driver.exe` with administrator rights. Proceed with installation and leave everything as default, restart the computer if it prompts you to do so.

![whatever.png](/assets/whatever.png)

4. Switch your phone to EDL mode and connect it to your computer.
- From the turned on state, turn on debugging mode on your phone by dialing `*#*#33284#*#*`, connect it to your computer and type `adb reboot edl` in a command-line window.
- From the turned off state, hold down `*` and `#` at the same time while inserting the USB cable to the phone.

In both cases, the phone's screen should blink with a 'enabled by KaiOS' logo then become blank. This is normal behaviour letting you know you're in EDL mode and you can proceed.

5. Run the Zadig tool and select Options > List All Devices. In the front dropdown menu, select `QHSUSB__BULK` (your device in EDL mode). In the target driver box (which the green arrow is pointing to), click on the up/down arrows until you see `libusb-win32` and click on Replace Driver.

![listall.png](/assets/listall.png)
![qhsusb.png](/assets/qhsusb.png)
![arg.png](/assets/arg.png)

6. If you're installing the driver for the first time, an "USB Device Not Recognised" pop-up may appear. Exit EDL mode by removing and re-inserting the battery, then turn on the phone in EDL mode again.

*If the driver installation takes too much time and the tool aborted it, exit Zadig, exit and re-enter EDL mode, then try to install again.*

### Part 2: Obtaining the boot partition

1. Turn on the phone in EDL mode.

2. Open the EDL tools folder in a command-line window. Flash the Gerda Recovery image to the recovery partition by typing this command:
```
python edl.py w recovery recovery-8110.img --loader=8k.mbn
```
If the progress bar stops at 99% and you get this error `'usb.core.USBError: [Errno None] b'libusb0-dll:err [_usb_reap_async] timeout error\n'`, don't panic! This is because the phone doesn't send any indicator information back to the EDL tool when in fact the image has been successfully written. Don't mind the error and proceed with the next step.

3. When finished, disconnect the phone from your computer and exit EDL mode by removing and re-inserting the battery. 

4. Then, hold down the top Power button and `*` to turn on the phone in recovery mode. Connect the phone to your computer again.  

**Be careful not to boot into normal operation mode at this point! As stated above, while SELinux is still in `Enforced` mode, it'll try to revert all system modifications on startup, in this case, the custom recovery image we've just flashed will be overwritten by the stock one. If you accidentally start into normal mode (with the Nokia logo), you'll have to start over from step 1.**

Don't worry if this boots into a white screen, you can still use ADB right after boot. This is because the display driver for the Nokia 8110 4G included in the recovery image are not compatible with the display of 8000 4G/6300 4G.

Check if ADB can recognise the phone by typing `adb devices` into the command-line.

5. Navigate the command-line to the `platform-tools` folder (if needed) and pull the boot image from the phone by typing this command:
```
adb pull /dev/block/bootdevice/by-name/boot boot.img
```
You should now see `/dev/block/bootdevice/by-name/boot: 1 file pulled, 0 skipped.` and have a copy of the boot partition with the size of 32.0MB (32,768KB). Fetched boot image will be saved to the current directory.

6. Reboot the phone into normal operation by typing `adb reboot` into the command-line, or remove and re-insert the battery. Our custom Gerda Recovery partition will now be overwritten by the default one.

You can disconnect the phone from your computer for now.

**Copy and keep the original boot partition somewhere safe in case you need to restore to the original state for over-the-air updates or re-enabling WhatsApp calls.**

### Part 3: Patching the boot partition

#### Automatic patching with `8k-boot-patcher`

1. Follow Docker's tutorial on installing Docker Desktop. Once set up, open the program, click Agree on this box and let the Docker Engine start before exiting.

![docker_abomination.png](/assets/docker_abomination.png)

2. Clone/download the boot patcher toolkit by typing this into a command-line window. This will download the toolkit and have Docker set it up. Do not omit the dot/period at the end of this command, this tells Docker where our downloaded toolkit are located on the system.
```
git clone https://gitlab.com/suborg/8k-boot-patcher.git && cd 8k-boot-patcher && docker build -t 8kbootpatcher .
```

![docker_build.png](/assets/docker_build.png)

3. Copy the `boot.img` file we've just pulled from our phone to the desktop and do not change its name. Type this into the command-line to run the patching process:
```
docker run --rm -it -v ~/Desktop:/image 8kbootpatcher
```

![docker_patch.png](/assets/docker_patch.png)

That's it! On your desktop there will be two new image files, the patched `boot.img` and the original `boot-orig.img`. You can now head to part 4.

#### Manual patching with Android Image Kitchen

1. Open the extracted Android Image Kitchen tools folder and copy the boot image we've just obtained over to the root of it.

![aik.png](/assets/aik.png)

2. Open the folder in a command-line window and type `unpackimg boot.img`. This will split the image file and unpack the ramdisk to their subdirectories.

![unpack.png](/assets/unpack.png)

3. Let the editing begin! First, open `ramdisk/default.prop` using Notepad++ and change:

- line 7: `ro.secure=1` -> `ro.secure=0`
- line 8: `security.perf_harden=1` -> `security.perf_harden=0`
- line 10: `ro.debuggable=0` -> `ro.debuggable=1`

![default_prop.png](/assets/default_prop.png)
![default_prop_edited.png](/assets/default_prop_edited.png)

4. Open `ramdisk/init.qcom.early_boot.sh` in Notepad++ and add `setenforce 0` as a new line at the end of the file.

![setenforce.png](/assets/setenforce.png)

5. Go back to the root Android Image Kitchen folder and open `boot.img-cmdline` in Notepad++. Scroll to the end of the line and append `androidboot.selinux=permissive enforcing=0` at the end of it without adding a new line.

![append.png](/assets/append.png)

6. Open `ramdisk/init.rc` (NOT `ramdisk/init`) and delete the reference of `setprop selinux.reload_policy 1` at line 393. This will ultimately prevent SELinux from overwriting the policy changes we made above. 

![reload_policy.png](/assets/reload_policy.png)

7. And that's a wrap! Open the root Android Image Kitchen folder in a command-line window and type `repackimg` to package our patched boot partition.

![repack_unsigned.png](/assets/repack_unsigned.png)

> If you happen to encounter an error during the signing process, that's likely because the process uses `java` to power the `boot-signer.jar` sequence and you don't have it installed. The image will still be packaged and ready for flashing, but if you're a perfectionist, you can install JRE and try again.

![repackimg_signed.png](/assets/repackimg_signed.png)

> If the newly packaged image is barely over 1/3 the size of the original image, it's a normal behaviour and you can proceed.

### Part 4: Replacing the boot partition with patched one

1. Turn on your phone in EDL mode and connect it to your computer.

2. Move the newly created `unsigned-new.img` or `image-new.img` to the EDL tools folder and open a command-line window within it. From here type either of two commands depending on which image file you have:

```
python edl.py w boot unsigned-new.img --loader=8k.mbn
```
OR
```
python edl.py w boot image-new.img --loader=8k.mbn
```

> Again, if the progress bar stops at 99% and you get this error `'usb.core.USBError: [Errno None] b'libusb0-dll:err [_usb_reap_async] timeout error\n'`, this is because the phone doesn't send any indicator information back to the EDL tool when in fact the image has been successfully written. Don't mind the error and go on with the next step.

3. Restart the phone to normal operation mode: `python edl.py reset`. And we're done!

> If you still have the original boot partition and wish to revert all the messes and damages, connect the phone to your computer in EDL mode, move the image file to the EDL tools folder, open a command-line window within it and type:<br>`python edl.py w boot boot.img --loader=8k.mbn`<br>`python edl.py reset`

![edl_bootog.png](/assets/edl_bootog.png)

## Backups

TBD

## Source code

HMD Global/Nokia Mobile has published the device's source code for its Linux 4.9 kernel, B2G and certain third-party libraries used in this phone, which can be downloaded directly from [here](https://nokiaphones-opensource.azureedge.net/download/phones/Nokia_6300_4G_20.00.17.01_OSS.tar.gz).

Note that the source code released does not contain proprietary parts from other parties like Qualcomm.
