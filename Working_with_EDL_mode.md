# Working with EDL mode

## Get to know EDL mode
**Qualcomm Emergency Download mode**, commonly known as EDL mode, is a special engineering interface implemented on devices with Qualcomm chipsets. Its purpose is to perform special operations on the phone that are intended for device manufacturer only, such as unlocking the bootloader, read and flash firmwares on the phone's filesystem or recover it from being a dead paperweight. Unlike bootloader or Fastboot mode, system files needed by the EDL mode resides on a separate 'primary bootloader' that cannot be affected by software modifications. 

Aleph Security has a deep-dive blog post into exploiting the nature of EDL mode on Qualcomm-chipset devices that you can read [here](https://alephsecurity.com/2018/01/22/qualcomm-edl-1).

Booting into this mode, the phone's screen will turn almost black as if it has been turned off, but in fact it still receives commands over Qualcomm's proprietary protocol called Sahara (Firehose on newer devices). With a [suitable digitally-signed programmer in MBN/ELF file format](https://edl.bananahackers.net) and some instruction-bundled tools, the most popular one being QFIL (Qualcomm Flash Image Loader), one can send commands from a computer to the phone over USB.

For the sake of cross-platform usage (and my obsession of open-source tools), instead of QFIL which is proprietary and only supports Windows, we'll be using open-sourced Python scripts from GitHub, such as [bkerler's](https://github.com/bkerler/edl) and [andybalholm's](https://github.com/andybalholm/edl) that are great as alternatives.

## Booting into EDL mode
### Using button combos
Depending on the form factor and the motherboard's design, each device model has different button combos that can be hold down while inserting the USB cable to trigger the circuits into switching to EDL mode.

**While plugging in the USB cable:**
- Nokia 8110 4G, Nokia 800 Tough: hold both D-Pad Up and Down;
- CAT B35, Nokia 8000 4G, Nokia 6300 4G: hold both * and #;
- Nokia 2720 Flip: hold both the side volume keys. Be careful not to accidentally press the SOS button.
- Alcatel Cingular Flip: hold both volume keys until you see the booting logo blinks, then hold only one of them.

If you manage to get it, the booting logo will flash momentarily, then the screen turns black as if you've turned off the phone. If you fail, the normal boot sequence will be triggered instead, and you'll have to start over.

### Using Android Debug Bridge (`adb`)
While having the phone on and connected to the computer via an USB cable, turn on debugging mode on the phone and set up ADB on your computer (there's a guide over [WebIDE](/development/webide)), then in the command-line window, type in `adb reboot edl`. This will send a signal to your phone telling to reboot to EDL mode.

Note that this method cannot be used if you're unbricking your phone.
 
### Using an EDL cable
An EDL cable is an USB cable that specializes in transmitting signals for EDL mode between your phone and computer.

You can make an EDL cable for yourself by stripping the insulation of a spare USB cable and wiring part of D+ and Ground wires in a way that sends the signal to tell the phone to switch to EDL mode. [Here's a specific guide on how to do just that.](/root/edl/diy-edl-cable)

Alternatively, you can find lots of pre-made EDL cables online for as cheap as $2 in Philippines (citing Cyan). Pre-made EDL cables has a button attached: while holding down the button, insert the cable to the device, wait for 5 seconds and then you can let it go.

### Disassembling the phone
This method is RISKY and mainly used by professionals, as it requires you to disassemble your phone, find and short the pins that trigger the primary bootloader, which in turn boot your phone in EDL mode. If you don't know what you're doing, you can damage your phone in the process altogether. **Proceed with caution.**

A separate guide for this method can be found [here](/root/edl/disassemble).

## Exiting EDL mode
This is just as critical to know, because if the EDL utility happened to not work properly (wrong programmer?), you wouldn't be able to enter any reset command from the computer to exit this mode.

### Removing the battery
On devices with removable battery, taking the battery off will shut the phone down and (abruptly) also exit EDL mode in the process.

### Using another button combo
On most devices, there's also a button combo hardcoded for force rebooting the device at any time. This is normally useful when the system hangs up, but also comes in handy if you want to exit EDL mode. 

i.e. To exit EDL mode on the Nokia 800 Tough hold both the Power/End call and D-Pad Down keys (just pressing Power/End call won't work).

### Wait
If you can't get the combo button to work or open the phone... Well, the last option is to wait for eternity for the battery to die.
