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
-  EDL mode: With the device powered off, hold the top `Power` + `*` + `#`, or type `adb reboot edl` when connected to a computer. Boots into a black screen, allows you to read and write partitions in low-level with proprietary Qualcomm tools. Remove the battery to exit.

EDL loader for the international version of this phone can be found on BananaHackers' [EDL archive site](https://edl.bananahackers.net/loaders/8k.mbn) with hardware ID 0x009600e100420029.