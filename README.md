<details markdown="block"><summary dir="rtl">View device specification table</summary>
<table style="font-size:small">
	<thead><tr><th></th><th>Nokia 6300 4G (nokia-leo)</th><th>Nokia 8000 4G (nokia-sparkler)</th></tr></thead>
	<tbody>
		<tr><td>Released</td><td colspan=2>13 November 2020</td></tr>
		<tr><td>Model</td><td>TA-1286, TA-1287, TA-1291, TA-1294, TA-1307, TA-1324</td><td>TA-1300, TA-1303, TA-1305, TA-1311</td></tr>
		<tr><td>Colors</td><td>Light Charcoal, White, Cyan Green</td><td>Onyx/Black, Opal/White, Topaz/Blue, Cintrine/Gold</td></tr>
		<tr><td>MSRP</td><td>€49/£59.99/$69.99/1,290,000₫</td><td>€79/£79.99/1,790,000₫</td></tr>
		<tr><td colspan=3 align="center"><strong>Specifications</strong></td></tr>
		<tr><td>SoC</td><td colspan=2>Qualcomm MSM8909 Snapdragon 210 (4 × 1.1GHz Cortex-A7)</td></tr>
		<tr><td>RAM</td><td colspan=2>512MB LPDDR2/3</td></tr>
		<tr><td>GPU</td><td colspan=2>Adreno 304</td></tr>
		<tr><td>Storage</td><td colspan=2>4GB eMMC 4.5 (+ up to 32GB microSDHC card)</td></tr>
		<tr><td>Network</td><td>2G GSM, 3G UMTS, 4G LTE Cat4 150/50Mbps<br><em>
			+ EU (except East Ukraine, Azerbaijan, Georgia), APAC: band 1, 3, 5, 7, 8, 20<br>
			+ MENA, CN, Nigeria, Tanzania: band 1, 3, 5, 7, 8, 20, 28, 38, 39, 40, 41<br>
			+ US: band 2, 4, 5, 12, 17, 66, 71<br>
			+ LATAM: band 2, 3, 4, 5, 7, 28<br>
			+ ROW: band 1, 3, 5, 7, 8, 20, 38, 40</em><br>
			VoLTE &amp; VoWiFi support<br>Single or Dual SIM (Nano-SIM, dual-standby)</td>
			<td>2G GSM, 3G UMTS, 4G LTE Cat4 150/50Mbps<br><em>
			+ EU (except East Ukraine, Azerbaijan, Georgia), APAC: band 1, 3, 5, 7, 8, 20<br>
			+ HK, Macau, MENA, CN, Nigeria, Tanzania: band 1, 3, 5, 7, 8, 20, 28, 38, 39, 40, 41<br>
			+ ROW: band 1, 3, 5, 7, 8, 20, 28, 38, 40</em><br>
			VoLTE &amp; VoWiFi support<br>Single or Dual SIM (Nano-SIM, dual-standby)</td></tr>
		<tr><td>Screen</td><td>320 × 240 @ 167 PPI<br>2.4 inches QVGA TFT LCD, 16M colors</td>
			<td>320 × 240 @ 143 PPI<br>2.8 inches QVGA TFT LCD, 16M colors</td></tr>
		<tr><td>Bluetooth</td><td colspan=2>4.0, A2DP, LE</td></tr>
		<tr><td>Wi-Fi</td><td colspan=2>802.11b/g/n, 2.4GHz, Hotspot (up to 8 devices)</td></tr>
		<tr><td>Peripherals</td><td colspan=2>GPS &amp; GLONASS</td></tr>
		<tr><td>Cameras</td><td>Rear: VGA with fixed focus, LED flash</td><td>Rear: 2MP with fixed focus, LED flash</td></tr>
		<tr><td>Dimensions<br>(HWD)</td><td>131.4 × 53 × 13.7 (mm)<br>5.17 × 2.09 × 0.54 (in)</td>
			<td>132.2 × 56.5 × 12.3 (mm)<br>5.20 × 2.22 × 0.48 (in)</td></tr>
		<tr><td>Weight</td><td>With battery: 104.1g (3.67oz)</td><td>With battery: 107.9g (3.81oz)</td></tr>
		<tr><td>Ports</td><td colspan=2>- microUSB charging &amp; USB 2.0 data transferring port<br>- 3.5mm headphone jack</td></tr>
		<tr><td>Battery</td><td colspan=2>Removable Li-Ion 1500mAh (BL-4XL), 5W wired charging<br>(up to 25 days of 4G standby advertised)</td></tr>
		<tr><td colspan=3 align="center"><strong>KaiOS info</strong></td></tr>
		<tr><td>Version</td><td colspan=2>KaiOS 2.5.4</td></tr>
		<tr><td>WA VoIP</td><td colspan=2>Supported (12.00.17.01 onwards)</td></tr>
		<tr><td>Build no.</td><td colspan=2>10.00.17.01, 12.00.17.01, 20.00.17.01, 30.00.17.01</td></tr>
	</tbody>
</table></details>

*Source code [provided by HMD] for B2G, Linux 4.9 kernel and certain LGPL-2.1 licensed libraries used on the 6300 4G can be found in [`leo-v20` branch of this repository]. Note that it doesn't contain proprietary code from parties i.e. Qualcomm and thus cannot be used to compile an entire KaiOS build.*

<img width="410" align="right" src="img/nokia_6300_4G-emotional-Range.png">

**Table of Contents**
- [Don’t buy a counterfeit]
	- [About Kosher phones]
- [Differences between US and international variants]
- [Tips and tricks]
- [Known issues]
	- [KaiOS-specific]
	- [WhatsApp-specific]
- [Secret codes]
- [Special boot modes]
	- [UART debugging testpoint]
- [Sideloading and debugging third-party applications]
- [ROOT: Boot partition patching (non-US only)]
	- [Before proceeding: back up your data]
	- [What we’ll need]
	- [Part 1: Set up environment for EDL tools]
	- [Part 2: Obtaining the boot partition]
		- [Nokia 8000 4G and Nokia 6300 4G with bkerler’s EDL]
		- [Nokia 2720 Flip and Nokia 800 Tough with andybalholm’s EDL]
	- [Part 3: Patching the boot partition]
		- [Automatic patching with `8k-boot-patcher`]
		- [Manual patching with Android Image Kitchen]
	- [Part 4: Flashing the modified boot partition]
- [External links]

<!-- In late 2020, following the successful relaunch of the retro 2720 Flip and 800 Tough, HMD Global quietly introduced the new Nokia 6300 4G based on KaiOS 2.5.4. The new phone takes a minimalistic twist whilst inheriting the classic candy-bar design of the original Nokia 6300.

Being the fifth Nokia-branded KaiOS devices to be released under HMD-era, it is one of the most affordable the company has ever offered in its KaiOS lineup[^1] at €49/$69.99.

The new phone takes a modern twist whilst resembling the classic candy-bar design of its predecessor Nokia 6300. It features a tall, narrow look with rounded corners, and is made out of polycarbonate shell that *feels* sturdy and comfortable to hold in the palm of hand.

It included 4G LTE support with capability of broadcasting Wi-Fi hotspot.

It runs on KaiOS 2.5.4, which features popular apps like WhatsApp, Facebook, YouTube, Google Maps, and until recently, Google Assistant.

With the firmware update to 12.00.17.01 (see [ROOT: Boot partition patching (non-US only)]), the new 6300 4G became one of the few KaiOS phones to be able to make and receive WhatsApp voice calls, allowing you to easily stay connected with your loved ones amid the [COVID-19 pandemic] lockdown.

Reception of the phone has been mostly unfavorable, nevertheless, with users on sites like Amazon, Best Buy and GSMArena complaining about software issues: poor battery life when using on Wi-Fi or mobile data, unintended keypresses, RAM management issues and outdated operating system kernel (see [Known issues]). Starting with the update to 30.00.17.01 in selected regions, as Google phased out support, Google Assistant was removed from the OS. -->

<!-- One of the features of the Nokia 6300 4G phone is that it has a dual-SIM feature. This means that you can use two different phone numbers on the same device. This can be useful if you travel to different countries or want to separate your personal and work calls. You can also switch between the SIM cards easily on the phone settings.

Another feature of the phone is that it supports 4G LTE and Wi-Fi calling. This means that you can make and receive calls over the internet. This can save you money on roaming charges and improve your call quality in areas with poor cellular coverage. You can also use the phone as a Wi-Fi hotspot to share your internet connection with other devices.

The phone runs on KaiOS, which is a simple operating system that lets you use some popular apps like WhatsApp, Facebook, YouTube, and Google Maps. You can also use speech-to-text to type messages or search the web with your voice. The phone has a dedicated button to launch the KaiOS app store, where you can download more apps.

However, the phone also has some drawbacks. The phone has a 0.3MP camera, which is very low quality compared to most smartphones. You can take photos and videos, but they will not look very sharp or clear. The phone also has a 2.4-inch screen, which is small and has low resolution. This can make it hard to see the details or read the text on the screen.

The phone also has some software issues that many users have complained about. Some of the problems are: poor battery life, unintended keypresses, RAM management issues, and outdated operating system kernel. These issues can affect the performance and usability of the phone. The phone also lost the Google Assistant feature, which was a useful way to control the phone with your voice. Google stopped supporting the phone, so it no longer has the voice assistant.

- [In-Depth Review of the Nokia 6300 4G] on r/dumbphones
- [Nokia 6300 4G review after a month of use] by u/Successful_Craft_281 on r/dumbphones
- [One week of use, the good, bad and the ugly… (Nokia 6300 4G)] by u/nemanjitca on r/dumbphones -->

## Don't buy a counterfeit
Beware of ~~Wish.com~~ eBay listings if you plan to get yourself an 8110 4G, 2720 Flip or 6300 4G.

On eBay, Shopee and similar online shopping platforms, you'll see hundreds of listings for used 6300 4Gs in various conditions and claimed specifications. While the listings seem to look identical showing the phone's exterior, many of those are NOT genuine, but are cheap Chinese knock-offs with [terrible build quality] and software experience.

- Brand-new KaiOS phones, even off the shelf, don't cost less than 2/3 of their retail prices.
- Do keep in mind that HMD has never sold any of their devices in their KaiOS lineup 2G-only. All KaiOS phones from HMD are equipped with 4G LTE, Wi-Fi and Bluetooth. KaiOS devices are required to have 3G at minimum.
- If the seller only uploaded generic photos showing the phone's exterior, ask for additional photos of the box that it came in. A genuine phone's box would show all its features on fine print, as well as an information sticker that indicates its model number (which matches the device specification table above) and targeted regional market.
	- Check the printed model number on the packaging box with that on another information sticker under the back of the phone, and when dialing `*#0000#` in the operating system.
- Look for signs of the phone running KaiOS and not MRE: KaiOS uses the distinct [Open Sans] UI font and vibrant, properly aligned UI elements. KaiStore and related services should be available at all time. It does not natively run Opera Mini 4.4 or other Java/MRE apps.

### About Kosher phones
Kosher is a category of devices of which the software (sometimes hardware) is heavily modified to limit access or even to get rid of content and features deemed distracting or against religious values, including unnecessary apps, Internet access, and debugging over ADB, while maintaining other features and user interface to be identical to the original. Companies specializing in customizing Kosher phones exist, and you can easily come across Kosher phone listings on eBay advertising as productivity and focus improvement tools.

As the build and UI remain identical, it's difficult to tell Kosher phones and genuine ones apart. As of now, my key takeaways to differentiate them are:
- there might be logos embedded on hardware and/or splash screen displayed on the boot sequence to indicate a Kosher phone (original only shows 'enabled by KaiOS' followed by the Nokia logo and chime);
- Browser, KaiStore and other bloat games are missing even if you have an active cellular service; links in Messages don't work; options to manage KaiOS accounts in Settings are greyed out (depending on each variant, WhatsApp and APIs it relies on might go missing or be intentionally left in);
- no options to toggle ADB and DevTools access: dialing `*#*#debug#*#*` triggers nothing; cannot boot into Recovery mode; getting access to EDL mode varies

Kosher is indeed a great way to make your phone truly basic, but HMD already offers more basic phones with 4G in their feature phone lineup, so the choice is yours. **Double-check the description and pictures of the listings before you buy.**

*Photo provided by nuxx on r/KaiOS Discord server in October 2021.*

<p align="center">
	<img alt="A Kosher-customized Nokia 8000 4G with no Browser, KaiStore or third-party apps shown in 3-by-3 grid of apps. Center D-Pad key is engraved with a Hebrew symbol indicating Kosher phone" src="img/kosher-sparkler.jpg" width="400">
</p>

## Differences between US and international variants
"Buying Western-customized products will always give you the best quality possible" is unwise when it comes to consumer electronics, including mobile phones, and the 6300 4G is no exception. When buying the TA-1324 variant of this phone, you should expect:
- No cellular access: From the dawn of mobile phone technologies, for national security, the US has been using different cellular technologies from the rest of the world with little to no compatibility. On 4G LTE, the US variant receives different bands with little overlaps on international variants' bands, primarily band 7 (see the device specification table above). This means that you will have trouble making or receiving calls and texts on the US variant outside the country without roaming.
- Restricted device settings, notably device and T9 languages, as the phone software has to follow the FCC's regulations. On the US 6300 4G, the only languages available are English (US), español (US), Français (CA) and Português (BR).
- Tighten device security: US 6300 4G currently cannot be rooted due to different hash signature used for EDL handshake (see [Sideloading and debugging third-party applications] below).

Don't buy the US variant of 6300 4G unless you know what you're doing. Seek the availability of the phone in the closest place or nearby countries to where you are.

## Tips and tricks
- You can capture a screenshot by pressing both * and # keys at the same time.
- You don’t need a KaiOS account to use your phone or download apps from KaiStore, but you can create one in Settings, Accounts if you want to use Anti-Theft features.
- To stop getting ad notifications from KaiStore, go to *Settings → Personalization → Notices → App notices → Store* and turn off *Allow Notices*. Then go to *Store → Options → Settings & Account → Show rich content* and select *Do not show*.
		- To block KaiAds altogether, add `ssp.kaiads.com` in your Wi-Fi routers' blacklist or [the system's `hosts` file]. Be aware that this might prevent you from installing apps from KaiStore like WhatsApp.
- Speed Dial lets you call a contact quickly by pressing and holding a number key from 2 to 9 on the homescreen. To assign a contact to a number key, press and hold an empty key on the homescreen or go to *Contacts → Options → Settings → Set Speed Dial Contacts*. You can also change your voicemail number in the same menu.
		- *ICE (In Case of Emergency) Contacts, however, is an useless feature on this phone, since there's basically no way to activate it. On the 2720 Flip, you could hold or double-press the side button to trigger SOS Call.*
- You can use a GIF as your homescreen wallpaper, but it will drain your battery faster.
- If you prefer a different layout than the default 3-by-3 grid view, you can choose *Options → List view/Single view* and rearrange the items as you like.
- This phone has a hidden screen reader feature that might not work well with some third-party apps that have unlabeled buttons. To turn on the hidden Readout feature, open Browser, go to cyan-2048.github.io/kaios_scripts, use D-Pad to move the cursor and select *Screen Reader*.
- There's also a hidden call recording feature that has been made available on KaiOS 2.5.2 and later. To toggle, connect the phone to a WebIDE session (see [Sideloading and debugging third-party applications]), then open *Device Settings* in the right sidebar, search for `callrecording.mode` and set it to either `on` (press D-Pad Left to record), `auto` or `off`.
		- Alternatively you can sideload [CrossTweak] and press 3 to toggle call recording.

## Known issues
- The multiple clips holding the back panel can be stressed and quickly broken. Speaker is decent, but muffled on strong bass. *For tactile responses on keypad presses, turn on Keypad vibration under Settings → Device → Accessibility.*
		- *Note that phone shutting itself down or not receiving any charges often come down to loose or dirty battery connectors or charging port and not software problem. Happened to me once, got the phone checked and repaired for less than $10.*
- **[MAJOR]** Battery can drain heavily (from 5–7 days of 4G standby to 18 hours, or 2 hours in active usage) if you leave Wi-Fi or mobile data on at all time, e.g. to be immediately notified of incoming WhatsApp messages. It's better to turn them off if you don't plan to use Internet connection, and only turn them on periodically to check for notifications.
- **[MAJOR]** Keypad frequently registering multiple or no keystrokes instead of a single-press, because of keypad design and keypress timeout interval being too short in `keyboard.gaiamobile.org`. [BananaHackers' guide on fixing the keypad speed] may help
- **[MAJOR]** A-GPS failing to lock your current position on 4G LTE, possibly due to interferences with TDD bands ⇒ workaround: change your A-GPS APN settings under *Settings → Mobile network & data → APN settings* or switch to 2G/3G for the phone to retrieve GPS information properly (*Settings → Mobile network & data → Carrier - SIMx → Network type → 3G/2G*). Might be major issue for those in the US where 2G/3G has been shut down.
- **[MAJOR]** B2G takes up large chunk of memory, and RAM optimizations leading to the phone joining Doze deep sleep immediately and aggressive task killing after a few minutes, making opening or exiting apps horribly slow, and notifications—including incoming WhatsApp calls—being delayed.
		- Wi-Fi hotspot feature will stop transmitting data packets with your other devices when you put the phone into sleep. *As a workaround, you can have [a playlist of silent MP3s played in background] to prevent the phone from Doze sleep.*
		- *This can be permanently mitigated by modifying scripts in /boot to disable the Low Memory Killer module, which I'll mention in [Manual patching with Android Image Kitchen] below.*
- On certain network providers where this phone isn't yet certified, such as Jio Reliance in India, you may [temporarily mute yourself on phone calls with VoLTE/VoWiFi enabled]. Putting yourself on hold and off does ease the problem.
- Normally, you can wake up the phone from sleep by either pressing the Power, Volume up or Volume down buttons, regardless of whether keyguard is in place or not. On this phone there are no volume buttons, but some of their functions, such as triggering boot modes or waking the phone up, are mapped to * and # keys respectively. This can be problematic as those keys are located close to the bottom edge of the phone and can be randomly mashed if you store the phone in your front pockets, leading to [unintended screenshots].
- If you forgot your lockscreen passcode (not SIM or Anti-Theft ones), you can [bypass it] by holding down the top Power button, then select *Memory Cleaner → Deep Memory Cleaning*.
- *According to reports from GSMArena and Reddit, some call and text entries may not be registered in the log. I've not been able to replicate those during my usage however, could be related to other mentioned issues.*

### KaiOS-specific
- If you're setting up the phone for the first time with no SIM card, pre-installed apps such as WhatsApp, Facebook and Google apps may not appear in the app list or in KaiStore. After popping in a SIM, those apps will show up as normal.
		- *KaiStore will show up in all circumstances, regardless of whether there's a SIM card inserted or not.*
- This phone runs KaiOS 2.5, which itself is based on Gecko 48 from 2016, meaning without optimizations and new web technologies, some websites like Instagram and Uber just fall apart and the overall performance is unbearable.
		- No built-in Widevine DRM decoders, which means the phone is NOT capable of playing DRM-protected content from e.g. Spotify
- **[MAJOR]** Some built-in apps, such as Call logs, Contacts or Music, are written in a way that is performance-intensive and not optimized for the phone, causing slow rendering and system lags if you store a large number of contacts (technically infinite but 100 recommended), call logs (max 40), music files or other items in a list.
		- *Performance issues has been addressed on later versions, for now you should opt for alternatives such as [arma7x's K-Music] in KaiStore if possible.*
- **[MAJOR]** Sending text messages don't automatically convert to MMS in group chats. You'll have to add a message subject or file attachment before sending to manually do so, otherwise your message will be sent separately to each individual in the thread. Receiving works flawlessly. *Group messaging over MMS has been properly implemented as a feature on later versions.*
- **[MAJOR]** Alarms can be delayed, unable to go off or go off unexpectedly if the Clock app is killed. Before going to sleep, make sure to open the Clock app and lock the phone without pressing the End call key or closing the app.
- Predictive typing mode doesn't last between inputs, meaning if you switch between input boxes, it'll return to the normal T9 mode.
- Photos larger than 6000-by-4000 in size aren't viewable in the Gallery app due to restrictions in place preventing memory constraints. As a workaround, you can download [FabianOvrWrt's Explorer] and [mochaSoft Aps' Photo Zoom] from KaiStore to view them instead.
- Built-in File Manager app doesn't show folders in the internal storage other than pre-configured ones (audio, music, photos, books, videos, DCIM, downloads, others). This is [hardcoded within its code] as a measure to hide system files (such as DIC files for storing added T9 words), but can be easily misused. *To fully browse the internal storage, use third-party file managers from KaiStore, or turn on Settings → Storage → USB Storage and connect your phone to a computer.*
- Built-in email, calendar and contact syncing function with Google account may completely fail at times. Use IMAP and import contacts instead.
		- In December 2021, in order to [replace the expired Let's Encrypt root certificate DST Root CA X3 with the newer ISRG Root X1], KaiOS Technologies issued a Service Update over KaiStore to all active KaiOS devices. This caused the Contacts app on various phones, notably HMD/Nokia KaiOS phones and Alcatel MyFlip 2, to [freeze on open]. In some reported cases, apps such as E-Mail, Calendar and Settings failed to work as well, and Google accounts set up in the Settings app couldn't be removed. This was later classified as a bug of Google account syncing feature, and affected people were advised to wipe their devices as a fix.
		- T9 search in Contacts app is missing. For those missing the feature, there's a port called [FastContact] by Luxferre that you can sideload to use as an alternative.
		- E-Mail app lacks many crucial enterprise features, such as OAuth2 secure sign-in.
		- Speaking of built-in Calendar app, if you manage to opt for syncing your Google account with the phone, only the calendar *with your email address as its name* will sync.
- You cannot change message notification tone or alarm tone on the phone outside the defaults provided. This is because both are not managed by the system, but by the Messages and Clock app themselves.
		- To change them, you'll have to use ADB to pull `sms.gaiamobile.org` and `clock.gaiamobile.org` from `/system/b2g/webapps`, extract, edit the audio files and repackage the apps, then push them back under `/data/local/webapps` and edit the `basePath` in `/data/local/webapps/webapps.json` to reflect the change (see [BananaHackers' guide] for instructions)
- D-Pad shortcuts and app shortcuts in the carousel menu (when you press Left on the home screen) are not customizable. *The former has been addressed on later versions*, but to change them on this phone you'll have to edit `launcher.gaiamobile.org`.

### WhatsApp-specific
- (mostly 2720 Flip) Upon getting the confirmation code needed to set up WhatsApp, you may be indefinitely stuck at Connecting WhatsApp... regardless of whether you're on Wi-Fi/mobile data or had a SIM in. Some suggested that leftover pre-configuration files seem to have caused the issue and, in most cases, can be fixed with a factory reset.
		- *[Temporarily putting your SIM in another phone to receive the code] may help as well.*
- 8MB download/5MB upload limit: This is to avoid 'out of memory' errors with the nature of WhatsApp's end-to-end encryption. All things sent through the app's servers—including photos and videos—are encrypted on device. Decrypt them bit-by-bit would use too much memory for KaiOS devices, having only as much as 512MB of RAM.
- Pairing account with the WhatsApp Web interface or desktop applications is NOT possible. This is because KaiOS cannot hold background processes or handle battery life well enough to sync decryption keys and mirrors messages & calls from the phone.
		- Note that you cannot sign into another device, pair with WhatsApp Web and then sign into WhatsApp on KaiOS. This will cause the decryption keys to be renewed and all other devices to be logged off automatically.

## Secret codes
*Tip: You can save these codes as contacts for quick dialing later. When the phone suggests a saved code, you'll have to press Call to activate the code's function.*
- `*#*#33284#*#*`: Toggle debugging mode, allowing the phone to be accessed with ADB and DevTools. A bug icon will appear in the status bar letting you know debugging mode is on. This can also be turned on under *Settings → Device → Developer → Debugger → ADB and DevTools*.
- `*#06#`: Display the hidden International Mobile Equipment Identity numbers or IMEI(s) to uniquely identify a specific cell phone on GSM networks. Do not show them to anyone else: they're crucial for calling functions on the phone.
- `*#0606#` (TA-1324 only): Display the Mobile Equipment Identifier numbers or MEID(s) to uniquely identify a specific cell phone on CDMA networks. On international variants the MEIDs would be all zeroes, and thus this secret code doesn't apply.
- `*#0000#`: Display device information, such as firmware version, build date, model number, variant and CUID.
- `*#33#` (call): Check the [Call barring] service status from carrier for blocking or whitelisting calls, whether incoming or outgoing, domestic or international. Requires a 4-digit passcode to use. To toggle, go to *Settings → Network & Connectivity → Calling → Call barring*.
- `*#43#` (call): Check the [Call waiting] service status from carrier. To toggle, go to *Settings → Network & Connectivity → Calling → Call waiting*.
- `*#*#372733#*#*`: Open KaiOS MMI Test, an internal tool to test hardware performance of a KaiOS device through an automatic routine or manually by hand, including LCD backlight, T9 keyboard, camera, LED flash, RTC, speaker, microphone, vibrator, 3.5mm audio jack, SIM trays, Wi-Fi, Bluetooth, NFC, microSD and microUSB slots etc.
		- Throughout the manual speaker test, you'll hear some English and Chinese dialog from a female speaker, which transcribes to: *Hello. Please dial 110 for police, 119 for fire, 120 for ambulance, 122 for traffic accidents, and dial area code before 112 for six full obstacles.* [?]

### Codes that don't work
Most of these codes requires `userdebug` or `eng` versions to work.
- `*#07#`: Check the `ro.sar.enabled` property, if enabled check the current SAR level and display SAR-related health and safety information.
- `*#1219#`: Clear all userspace customizations, presumably for store display.
- `*#091#` (on)/`*#092#` (off): Toggle auto-answering on incoming call. This can be turned on via Device Settings interface in WebIDE.
- `*#2886#`: Should also open KaiOS MMI Test interface.
- `*#8378269#`/`*#*#2637643#*#*`: Open Testbox engineering menu with predecessor Firefox OS design, usually used by OEMs to test various hardware of the phone. This menu can be manually opened using [Luxferre's CrossTweak].
- `###2324#`: Open a menu, allowing to toggle Qualcomm diagnostic mode for fixing null/invalid IMEI or baseband via QPST.
- `*#*#212018#*#*`: Toggle privileged access (including rooted ADB shell) to the phone.
- `*#7223#`: Display internal firmware build and boot image versions.
- `*#*#0574#*#*`: Open LogManager utility which allows you to fully enable ADB and DevTools on Spreadtrum devices.
- `*#573564#`: Open T2M Log (jrdlog), a brief LogManager interface.
- `*#1314#`: Switch the `auto.send.crash.sms` property, whose purpose is still unknown.

## Special boot modes
- **Recovery mode**: With the device powered off, hold the top Power button and the * key, or type `adb reboot recovery` when connected to a computer. Allows you to factory reset the device by wiping /data and /cache, view boot and kernel logs, and install patches from `adb sideload` interface or SD card.
- **Fastboot mode**: Only accessible and automatically kick in when both /boot and /recovery is corrupted. Allows you to restore partitions under `fastboot` interface.
- **EDL mode**: With the device powered off, hold the top Power button and both the * and # keys, or type `adb reboot edl` when connected to a computer. Boots into a black screen, allows you to read and write partitions in low-level with proprietary Qualcomm tools. Remove the battery to exit.

<details markdown="block">
	<summary>What the heck is EDL mode?</summary>
<hr style="font-family:monospace;">

**Qualcomm Emergency Download mode**, commonly known as EDL mode, is a special engineering interface implemented on devices with Qualcomm chipsets. It lets you do special operations on the phone that only the device manufacturer can do, such as unlocking the bootloader, read and write firmwares on the phone's filesystem or recover from being a dead paperweight. Unlike bootloader or Fastboot mode, system files needed by the EDL mode resides on a separate 'primary bootloader' that aren't affected by software modifications.[^2]

Booting into this mode, the phone's screen will briefly show the 'enabled by KaiOS' logo, then turn almost black as if it's off, but in fact it's still listening to commands over Qualcomm's proprietary protocol called Sahara (or Firehose on newer devices). With a [suitable digitally-signed programmer in MBN/ELF file format] and some instruction-bundled tools, the most popular one being QFIL (Qualcomm Flash Image Loader), one can send commands from a computer to the phone over USB.

<hr style="font-family:monospace;"></details>

You can also **force reboot** the phone by holding the top Power button and the # key at any time.

EDL programmer for the international version of this phone (not TA-1324) can be found on BananaHackers' [EDL archive site] with hardware ID 0x009600e100420029 (a copy is available in the content of this repository). The US version of this phone has been signed with a different PK_HASH and needs a different firehose loader which we currently don't have in archive.

### UART debugging testpoint
[As discovered by atipls on Discord], on the mainboard of the 6300 4G, there are 3 UART testing points: TX, RX and GND just above the SIM2 slot. Shorting TX and GND takes you to Fastboot and Linux terminal interface.

<p align="center">
	<img alt="Mainboard of a TA-1307 Nokia 6300 4G, with the red arrow pointing to three gold contacts in the middle of the board, those being the UART testpoints in the order of TX, RX and ground" src="img/testpoint.png">
</p>

## Sideloading and debugging third-party applications
Don't want to download apps from KaiStore? Both the 6300 4G and 8000 4G have been classified as debug-enabled by the BananaHackers team. As with other KaiOS 2.5.4 devices, you can install and debug apps from outside sources on these phones, so long as they don't use 'forbidden' permissions, such as `engmode-extension`, `embed-apps` and `embed-widgets`, and you cannot debug pre-installed apps on the phone using WebIDE's Developer Tools (you're free to use `adb logcat` to view system logs instead).

Detailed instructions can be found at [Sideloading and debugging/WebIDE]. Feel free to check out apps made by the community on [BananaHackers Store], old [B-Hackers Store] and my personally curated [list of KaiOS apps].

**Do note that OmniSD, one of the methods used for on-device sideloading, and many Gerda-related apps requires the `navigator.mozApps.mgmt.import` API that has been removed from KaiOS 2.5.2.2, and therefore no longer work on this phone.** However, after permanently rooting the phone, the Privileged factory reset feature to gain privileged userspace session that could be used on KaiOS 2.5.2 and older can now be used again (see [Next steps]).

To remove unwanted apps from the phone, you can use [this fork of Luxferre's AppBuster] which lets you disable any apps you don't need and enable them again if you want.

## ROOT: Boot partition patching (non-US only)
On KaiOS 2.5.4 devices, such as the 6300 4G and 8000 4G, ADB and WebIDE can be used to install most third-party apps. However, apps with special ‘forbidden’ permissions are not allowed, including most BananaHackers apps with `engmode-extension` like Wallace Toolbox, which can be used to gain exclusive access of the phone. You also cannot make changes to the system. On the 2720 Flip and 800 Tough with KaiOS 2.5.2.2, with HMD/Nokia Mobile changing their release branches from `dev-keys` to `release-keys`, the situation is even worse as you cannot sideload at all.

This is because in order for WhatsApp's VoIP feature to work on these KaiOS versions, a security module called SELinux[^3] is now set to be `Enforced` which checks and reverts system modifications on boot. To get total read-write access to the devices, you'll now have to permanently root them by setting SELinux to `Permissive` mode.

The guide below is based on the main guide from BananaHackers website, but has been rewritten to make it easier to follow. The process will take somewhat considerable 30 minutes to an hour, so do this when you have enough time.

> [!IMPORTANT]
> **DISCLAIMER: This process will void your phone's warranty, disable its ability to receive WhatsApp calls and over-the-air updates, but you can undo this if you save a copy of the original boot partition. However, you might also brick your phone if you make a mistake in the process, so proceed at your own risk and with caution! I won't be responsible for any damages done to your phone by following these.**
>
> Remember, you don't have to root your phone to do things that usually need root access e.g. you can use [this fork of Luxferre's AppBuster] to disable apps from the launcher instead of deleting them with Wallace Toolbox. You can also install [Luxferre's CrossTweak], a Wallace Toolbox alternative also made by Luxferre that does not need `engmode-extension` and therefore can be easily installed on KaiOS 2.5.4 devices.

### Before proceeding: [back up] your data
> [Murphy's Law] states, "Anything that can go wrong, will go wrong". It's a proverb from the late 1940s, and while it may not be as academically accurate and influential as [Newton's laws of motion], it's generally acknowledged in technology as a rule of thumb. Even if you do the best you can, things can unexpectedly go south, so it's always good to prepare for the worst.

- To export your contacts from the built-in Contacts app, go to *Contacts → Options → Settings → Export contacts*. From there, choose to either save to a CSV file on your SD card, send through Bluetooth to other devices, or upload to Web services such as Google Contacts and Microsoft Outlook.
		- You can also use third-party backup services and tools such as [PhoneCopy in KaiStore] or D3SXX's [kaios-backup] etc.
- On debug-enabled devices, you can sideload `certified` apps such as D3SXX's [kaios-backup] or Fiachra1993's [kaios-sms-backup], which uses Firefox OS APIs to export your text messages to JSON or CSV files.
- Built-in Calendar app allows syncing events through Google, ActiveSync or CalDAV accounts. If you only have a small number of events, you can migrate each of those to your online calendars. strukturart's [greg] also allows syncing events with Nextcloud.
- Each entry in the Notes app can be shared over texts, Bluetooth or email.
- On internal storage or SD card (whichever you chose under *Settings → Storage → Default media location*), captured photos and videos are stored under `DCIM`; whereas recorded voice files are stored under `audio`.
		- To get your phone to show up on the computer as external storage, turn on *USB Storage* under *Settings

For backing up application data (excluding WhatsApp chats), system preferences and partition images, see [Backup].

### What we'll need
- a Nokia 6300 4G (excl. TA-1324), 8000 4G, 2720 Flip, 800 Tough or Alcatel Go Flip 3;
- an USB cable capable of data transferring (EDL cables work as well);
- MBN firehose programmer file: [8000 4G and 6300 4G], [2720 Flip], [800 Tough] or Go Flip 3 ([AT&T/Cricket], [T-Mobile/Metro/Rogers]);
- EDL utility to read and write system partitions: [bkerler's edl.py v3.1] for 8000 4G/6300 4G, [andybalholm's edl] for 2720 Flip/800 Tough/Go Flip 3;
- required for 6300 4G/8000 4G: [Gerda Recovery image file] ([backup]) for the Nokia 8110 4G, since the firehose loader above has a reading bug, we'll use this to access ADB from Recovery mode and get the boot partition from there;
- Python and `pip` for `edl.py` to work (setup guide can be found for each OS below);
		- Don't have an Internet connection? Download and install manually from PyPI: [pyusb], [pyserial], [keystone-engine], [capstone], [docopt]
- [Android Debug Bridge (ADB)] to read the boot image in Gerda Recovery (see [Sideloading and debugging/WebIDE])
- Windows: Qualcomm driver to contact the phone in EDL mode (included in the `edl.py` package)
- Windows: latest version of [Zadig] to configure `libusb-win32` driver; do NOT use the older version bundled as it has less chances of success
- macOS: [Homebrew] to quickly set up Python, ADB, `libusb` and configure the environment for EDL tools (setup guide can be found below)
		- *Python 2.7 bundled with macOS 10.8 to 12 is NOT recommended for following this guide.*
- **If you're going the automatic boot partition patching and compilation via Docker route (only recommended for 5-6 year old computers):**
		- [Git] to clone/download the repository of the patcher tool to your computer;
		- Docker Compose to provide the environment for the patcher tool to work (included in [Docker Desktop])
		- Windows: WSL2 with [Linux kernel update package] installed (to install WSL2, turn on Virtualization in BIOS, then open Command Prompt with administrative rights and type `wsl --install`)
- **If you're going the extracting and manual editing by hand route:**
		- Android Image Kitchen v3.8 ([Windows], [macOS/Linux])
		- on Windows 10 pre-1809: [Notepad++] to edit files while [preserving line endings]
		- (optional) [Java Runtime Environment] to properly sign the boot image with AVBv1

andybalholm's EDL cannot be used on 8000 4G and 6300 4G due to structural changes within GPT, which will result in an error `AttributeError: 'gpt' object has no attribute 'partentries'. Did you mean: 'num_part_entries'?`. **Do note that the command structures used between bkerler's and andybalholm's are different, which we'll mention below.**

For the sake of convenience, move the Gerda Recovery image and the MBN file into the root of `edl-3.1` or `edl-master` folder. If you need to have those in other folders, change the directory path accordingly.

*If you're on Linux, Python and ADB can be quickly set up by installing with your built-in package manager. We won't be covering this here, as each Linux distro has its own way of installing from package manager.*

### Part 1: Set up environment for EDL tools
> This portion of the guide was taken from [Development/EDL tools on BananaHackers Wiki] so that you don't have to switch tabs. Kudos to Cyan for the guides!

#### Linux
1. Install Python from your operating system's package manager e.g.
```
sudo apt-get install python pip3
```
2. Then, open Terminal and type this to install the dependencies for EDL tools:
```
sudo -H pip3 install pyusb pyserial capstone keystone-engine docopt
```
3. Switch your phone to EDL mode and connect it to your computer. Either:
	- if your phone is on, turn on debugging mode on your phone by dialing `*#*#33284#*#*`, connect it to your computer and type `adb reboot edl` in a command-line window.
	- if your phone is off, hold down `*` and `#` at the same time while inserting the USB cable to the phone.

In both cases, the phone's screen should blink with a 'enabled by KaiOS' logo then become blank. This is normal behaviour letting you know you're in EDL mode and you can proceed.

Additionally, if you have issue with device access:
- Open `/etc/modprobe.d/blacklist.conf` in a text editor and append `blacklist qcserial`.
- Copy both `51-edl.rules` and `50-android.rules` in the root of extracted EDL tools folder to `/etc/udev/rules.d`.

#### macOS
1. Follow the instructions to install [Homebrew] on its homepage. Basically just open Terminal and copy the long streak of code shown on the page, and type your password when prompted.

2. While you're in Terminal, type this into the command-line:
```
brew install python android-platform-tools libusb && pip3 install pyusb pyserial capstone keystone-engine docopt
```
3. Switch your phone to EDL mode and connect it to your computer. Either:
	- if your phone is on, turn on debugging mode on your phone by dialing `*#*#33284#*#*`, connect it to your computer and type `adb reboot edl` in a command-line window.
	- if your phone is off, hold down `*` and `#` at the same time while inserting the USB cable to the phone.

In both cases, the phone's screen should blink with a 'enabled by KaiOS' logo then become blank. This is normal behaviour letting you know you're in EDL mode and you can proceed.

#### Windows
1. Head over to [Python's official download page for Windows] and download the correct installer for your architecture, or download the latest version of Python from [Microsoft Store]. If you're downloading from Microsoft Store, skip to step 4.

2. Proceed with installing Python as usual. If you choose to customize your installation, include `pip`. Don't forget to tick the box next to "Add Python to PATH" to add Python as a global [environment variable], otherwise you'll have a hard time using Python to run scripts later on.
	- If you decide to customize your installation, choose to install `pip` and tick *Add Python to environment variables*.

<p align="center">
	<img alt="Screenshot of an installation window for Python 3.9 showing two options, 'Install Now' and 'Customize installation', with the checkbox for 'Add Python 3.9 to PATH' being selected" src="img/python.png">
</p>

3. On Windows 10/11, typing `python` or `python3` within Command Prompt/Windows Terminal will run the Microsoft Store version of Python. To override this default into running the locally installed version, toggle off App Installer (python.exe) and App Installer (python3.exe) under:
{:start="3"}
- Windows 10: Settings → Apps → [Apps & features] → App execution aliases
- Windows 11: Settings → Apps → Advanced app settings → App execution aliases

<img alt="Demostration of Settings app on Windows 11. User is already on Apps page and clicking on Advanced app settings. User then clicks on App execution aliases, where the toggles for App Installer (python.exe) and App Installer (python3.exe) are turned off. Description says Apps can declare a name used to run the app from a command prompt. If multiple apps use the same name, choose which one to use." src="img/settings-alias-off.gif">

4. Open Command Prompt/Windows Terminal with administrator privileges and run this command to install the required dependencies for EDL:
{:start="4"}

```
pip3 install pyusb pyserial capstone keystone-engine docopt
```

![Screenshot of a console window showing the successful process of collecting and downloading dependencies after typing the above command](img/pythoooon.png)

5. Extract the previously downloaded EDL package, open Drivers, Windows and run `Qualcomm_Diag_QD_Loader_2016_driver.exe` with administrator rights. Proceed with installation and leave everything as default, restart the computer if it prompts you to do so.
{:start="5"}

<p align="center">
	<img alt="Screenshot of an installation window for Qualcomm's diagnostic driver, in which two radio buttons are shown labelled 'WWAN-DHCP is not used to get IPAddress' and 'ETHERNET-DHCP is used to get IPAddress' respectively. The first button is selected." src="img/whatever.png">
</p>

6. Switch your phone to EDL mode and connect it to your computer. Either:
{:start="6"}
- if your phone is on, turn on debugging mode on your phone by dialing `*#*#33284#*#*`, connect it to your computer and type `adb reboot edl` in a command-line window.
- if your phone is off, hold down `*` and `#` at the same time while inserting the USB cable to the phone.

In both cases, the phone's screen should blink with an 'enabled by KaiOS' logo then become blank. This is normal behaviour letting you know you're in EDL mode and you can proceed.

7. To replace the installed `qcusbser` driver with `libusb-win32` for use with edl.py, download and open [Zadig] (do NOT use the version included in the EDL package). Tick Options, List All Devices and select `QHSUSB__BULK` (your device in EDL mode) in the main dropdown menu. In the target driver box—to which the green arrow is pointing—click the up/down arrows until you see `libusb-win32` and then click Replace Driver.
{:start="7"}

<p align="center">
	<img src="img/qhsusb-zadig.gif" alt="9-frame GIF demostrating Zadig's main interface, List All Devices option being selected from Option menu, QHSUSB_BULK being selected from the main dropdown list, followed by the second label box on the Drivers line, to which the green arrow points, changed to 'libusb-win32 (v1.2.6.0)'. Two smaller up/down arrows are right next to that box.">
</p>

> [!NOTE]
> Windows will automatically create restore points on driver installation, as Zadig suggests in its tooltips. On older PCs, this might cause issues with driver configuration process being lengthened past the 5-minute mark. If Zadig aborts the process and hangs, kill Zadig with Task Manager, remove and re-insert the battery on the phone to exit and re-enter EDL mode, then try to install again.

8. If you're configuring the driver for the first time, an "USB Device Not Recognised" pop-up may appear. Exit EDL mode by removing and re-inserting the battery, then turn on the phone in EDL mode again.
{:start="8"}

### Part 2: Obtaining the boot partition
#### Nokia 8000 4G and Nokia 6300 4G with bkerler's EDL
> Beware: due to the firehose loader being malfunctioned, the EDL tool only accepts one command each session, after which you'll have to disconnect the phone and restart the phone in EDL mode. If you try to throw a second command, it'll result in a `bytearray index out of range` error.

1. Turn on the phone in EDL mode.

2. Open the extracted EDL folder in a command-line shell. Flash the Gerda Recovery image to the recovery partition by typing:
```
python edl.py w recovery recovery-8110.img --loader=8k.mbn
```
*If the progress bar stops at 99% (and not earlier) and you get error `'usb.core.USBError: [Errno None] b'libusb0-dll:err [_usb_reap_async] timeout error\n'` or `usb.core.USBError: [Errno 60] Command timed out`, this is false. Don't mind the error and proceed with the next step.*

3. When finished, disconnect the phone from your computer and exit EDL mode by removing and re-inserting the battery.
4. Then, hold down the top Power button and `*` to turn on the phone in recovery mode. Connect the phone to your computer again.

> [!WARNING]
> Be careful not to boot into system at this point! As stated above, while SELinux is still in `Enforced` mode, it'll try to revert all system modifications on startup, in this case, the custom recovery image we've just flashed will be overwritten by the stock one. If you accidentally start into normal mode (with the usual Nokia chime), you'll have to start over from step 1.

Don't worry if this boots into a white screen: this is because the display driver for the Nokia 8110 4G included in the recovery image are not compatible with the display of 8000 4G/6300 4G.

Check if ADB can recognise the phone by typing `adb devices` into the command-line.

5. Navigate the command-line to the extracted `platform-tools` folder (if needed) and pull the boot image from the phone to the current directory on your computer with ADB by typing:
{:start="5"}

```
adb pull /dev/block/bootdevice/by-name/boot boot.img
```
You should now see `/dev/block/bootdevice/by-name/boot: 1 file pulled, 0 skipped.` and have a copy of the boot partition with the size of 32.0MB (32,768KB). Fetched boot image will be saved to the current directory.

6. Reboot the phone into normal mode by typing `adb reboot` into the command-line, or remove and re-insert the battery. Our custom Gerda Recovery partition will now be overwritten by the default one.
{:start="6"}

You can disconnect the phone from your computer for now.

#### Nokia 2720 Flip and Nokia 800 Tough with andybalholm's EDL
Unlike the 6300 4G and 8000 4G, our phones' EDL loader properly works with both reading and writing, so the steps are more straightforward.

1. Switch your phone to EDL mode and connect it to your computer. Either:
	- if your phone is on, turn on debugging mode on your phone by dialing `*#*#33284#*#*`, connect it to your computer and type `adb reboot edl` in a command-line window.
	- if your phone is off, hold down both side volume keys (2720 Flip) or both D-Pad Up and Down keys (800 Tough) at the same time while inserting the USB cable to the phone.

In both cases, the phone's screen should blink with a 'Powered by KaiOS' logo then become blank. This is normal behaviour letting you know you're in EDL mode and you can proceed.

2. Open the extracted EDL folder in a command-line shell. Extract the boot partition of the phone to the current directory on your computer =by typing either of these commands depending on which file you have:
{:start="2"}
```
python edl.py -r boot boot.img -loader 2720.mbn
```
```
python edl.py -r boot boot.img -loader 800t.mbn
```
3. When finished, reboot the phone into normal operation by typing either of these into the command-line, or remove and re-insert the battery:
{:start="3"}

```
python edl.py -reset -loader 2720.mbn
```
```
python edl.py -reset -loader 800t.mbn
```

You can disconnect the phone from your computer for now.

> [!WARNING]
> **Copy and keep the original boot partition somewhere safe in case you need to restore to the original state for over-the-air updates or re-enabling WhatsApp calls.**

### Part 3: Patching the boot partition
#### Automatic patching with `8k-boot-patcher`
1. Download and install [Docker Desktop]. Once set up, open the program, click Accept on this box and let the Docker Engine start before exiting.

![Screenshot of a window titled as 'Docker Subscription Service Agreement' which declares that you will have to accept Docker's Subscription Service Agreements, Data Processing Agreement and Data Privacy Policy in order to use the program, and the free scope of it is limited to personal and small business uses. The window also lists the options to view the full agreements, accept them or reject and close the program.](img/docker_abomination.png)

2. Use [Git] to clone/download the boot patcher toolkit by typing this into Command Prompt/Terminal. This will download the toolkit and have Docker set it up. Do not omit the dot/period at the end of this command, this tells Docker where our downloaded toolkit are located on the system.
{:start="2"}

```
git clone https://gitlab.com/suborg/8k-boot-patcher.git && cd 8k-boot-patcher && docker build -t 8kbootpatcher .
```
![Screenshot of a macOS Terminal window showing some logs in purple text after typing the command above](img/docker_build.png)

3. Copy the `boot.img` file we've just pulled from our phone to the desktop and do not change its name. Type this into Command Prompt/Terminal to run the patching process:
{:start="3"}
- Windows: `docker run --rm -it -v %cd%/Desktop:/image 8kbootpatcher`
- macOS/Linux: `docker run --rm -it -v ~/Desktop:/image 8kbootpatcher`

```
$ docker run --rm -it -v ~/Desktop:/image 8kbootpatcher

Boot image found, patching...
writing boot image config in bootimg.cfg
extracting kernel in zImage
extracting ramdisk in initrd.img
charger
data
[...]
ueventd.qcom.rc
ueventd.rc
verity_key
4037 blocks
4979 blocks
reading config file bootimg.cfg
reading ramdisk from myinitrd.img
Writing Boot Image boot.img
Boot image patched!
```
That's it! On your desktop there will be two new image files, the modified `boot.img` and the original `boot-orig.img`. You can now head to [part 4].

![Screenshot of boot.img and boot-orig.img files as shown on desktop](img/after_patch.png)

#### Manual patching with Android Image Kitchen
1. Extract the Android Image Kitchen tools and copy the boot image we've just obtained over to the root of the extracted folder.

![Screenshot of a list of folders and files contained in the extracted Android Image Kitchen folder](img/aik.png)

2. Open the folder in Command Prompt/Terminal and type `unpackimg boot.img`. This will split the image file and unpack the ramdisk to their subdirectories.
{:start="2"}

![Screenshot of a Windows Command Prompt window showing some logs of the boot partition extracting process after typing the command above](img/unpack.png)

> [!WARNING]
> **Be sure to edit the files correctly, else the phone won't boot!**

3. Let the editing begin! First, open `ramdisk/default.prop` using Notepad++ and change:
{:start="3"}
- line 7: `ro.secure=1` → `ro.secure=0`
- line 8: `security.perf_harden=1` → `security.perf_harden=0`
- line 10: `ro.debuggable=0` → `ro.debuggable=1`

```diff
@@ -4,9 +4,9 @@
  ro.sw.release.date=21-08-13
  ro.build.version.fih=20.00.17.01
  ro.build.elabel=false
- ro.secure=1
- security.perf_harden=1
+ ro.secure=0
+ security.perf_harden=0
  ro.allow.mock.location=0
- ro.debuggable=0
+ ro.debuggable=1
  ro.adb.secure=0
  ro.zygote=zygote32
```
<p align="center">
	<img src="img/default_prop_edited.png" alt="Screenshot of the modified content of the default.prop file">
</p>

4. Open `ramdisk/init.qcom.early_boot.sh` in Notepad++ and add `setenforce 0` as a new line at the end of the file.
{:start="4"}

```diff
@@ -312,14 +312,14 @@
  else
	  # nand configuration
	  if [ -e $nand_file ]
	  then
		  if grep ${partition_name} $nand_file
		  then
			  gps_enabled=false
		  fi
	  fi
  fi

  setprop ro.gps.enabled $gps_enabled
+ setenforce 0

```
<p align="center">
	<img src="img/setenforce.png" alt="Screenshot of the modified content of the init.qcom.early_boot.sh file">
</p>

5. Go back to the root Android Image Kitchen folder and open `split_img/boot.img-cmdline` in Notepad++. Without adding a new line, scroll to the end of the first line and append `androidboot.selinux=permissive enforcing=0`.
{:start="5"}

![Screenshot of the modified content of the boot.img-cmdline file](img/append.png)

6. Open `ramdisk/init.rc` (NOT `ramdisk/init`) and delete line 393 `setprop selinux.reload_policy 1` or mark a comment as shown. This will ultimately prevent SELinux from overwriting the policy changes we made above.
{:start="6"}

7. (Optional) If you wish to disable the Low Memory Killer function, now's a good time to do so! In the same `ramdisk/init.rc` file, after line 420, make a new line and add:
{:start="7"}

```
write /sys/module/lowmemorykiller/parameters/enable_lmk 0
```
Indent the new line to match up with other lines as shown.

```diff
@@ -390,7 +390,6 @@
  setusercryptopolicies /data/user

  # Reload policy from /data/security if present.
- setprop selinux.reload_policy 1

  # Set SELinux security contexts on upgrade or policy update.
  restorecon_recursive /data
@@ -418,9 +418,10 @@
  # Memory management.  Basic kernel parameters, and allow the high
  # level system server to be able to adjust the kernel OOM driver
  # parameters to match how it is managing things.
+ write /sys/module/lowmemorykiller/parameters/enable_lmk 0
  write /proc/sys/vm/overcommit_memory 1
  write /proc/sys/vm/min_free_order_shift 4
  chown root system /sys/module/lowmemorykiller/parameters/adj
  chmod 0664 /sys/module/lowmemorykiller/parameters/adj
  chown root system /sys/module/lowmemorykiller/parameters/minfree
  chmod 0664 /sys/module/lowmemorykiller/parameters/minfree
```
![Screenshot of the modified content of the init.rc file, with line 393 marked as comment which has the same effects as deleting the line altogether, and line 421 added to disable the Low Memory Killer module](img/f5-selinux.png)

8. And that's a wrap! Open the root Android Image Kitchen folder in a command-line window and type `repackimg` to package our modified boot partition.
{:start="8"}

![Screenshot of a Windows Command Prompt window showing some logs of the boot partition repacking process after typing the above command, but has a signing error at the end](img/repack_unsigned.png)

*If you happen to encounter an error during the signing process, that's likely because the process uses `java` to power the `boot-signer.jar` sequence and you don't have it installed. The image will still be packaged and ready for flashing, but if you're a perfectionist, you can install JRE and try again.*

![Screenshot of a Windows Command Prompt window showing some logs of the fully successful boot partition process](img/repackimg_signed.png)

If the newly packaged image is barely over 1/3 the size of the original image, it's a normal behaviour and you can proceed.

### Part 4: Flashing the modified boot partition
1. Turn on your phone in EDL mode and connect it to your computer.
2. Move the newly created `boot.img`, `unsigned-new.img` or `image-new.img` to the EDL tools folder and open Command Prompt/Terminal within it. From here type either of these commands depending on which image file you have:
```
python edl.py w boot boot.img --loader=8k.mbn
```
```
python edl.py w boot unsigned-new.img --loader=8k.mbn
```
```
python edl.py w boot image-new.img --loader=8k.mbn
```
For Nokia 2720 Flip and Nokia 800 Tough with andybalholm's EDL:
```
python edl.py -w boot boot.img -loader 2720.mbn
```
```
python edl.py -w boot boot.img -loader 800t.mbn
```
*Again, if the progress bar stops at 99% and you get a timeout error, this is because the phone doesn't send any indicator information back to the EDL tool when in fact the image has been successfully written. Don't mind the error and go on with the next step.*

3. Restart the phone to normal operation mode by typing `python edl.py reset`. And we're done!

#### Next steps
- Now that you've rooted your phone, to install applications with 'forbidden' permissions, connect your phone to a WebIDE session, open Device Preferences in the right pane, clear the value of `devtools.apps.forbidden-permissions`, then restart B2G by either reboot the phone or hold the top Power button and select *Memory Cleaner, Deep Clean Memory*.

![Screenshot of a WebIDE window in which the location of Device Preferences is highlighted in the right pane and the value of devTools.apps.forbiddenPermissions has been emptied](img/devpref.png)

- If you disabled the Low Memory Killer module, hooray, more tasks can run in background but are prone to crashes. [Increasing the swapfile size] prevents that from happening.
- If you wish to retain privileged permissions after restoring the phone to its unrooted state, before doing so, back up all data, sideload [Luxferre's CrossTweak] then press # to perform a privileged factory reset — this will wipe all data of the phone and let you set up with a privileged session. This session will last until an OTA update overrides or you choose to factory reset normally yourself.
- After rooting, you can spoof SELinux's Enforced status for WhatsApp VoIP by typing these commands one-by-one into the rooted ADB shell. This will last until a restart.
```
echo -n 1 > /data/enforce
mount -o bind /data/enforce /sys/fs/selinux/enforce
```

If you wish to revert all changes made, connect the phone to your computer in EDL mode, move the original boot image file to `edl-3.1` or `edl-master` folder, open Command Prompt/Terminal within the folder and type:
```
python edl.py w boot boot.img --loader=8k.mbn
python edl.py reset
```

## External links
- [Nokia 6300 4G product page] on Nokia Mobile's website
- [Nokia 8000 4G product page] on Nokia Mobile's website
- [Nokia 6300 4G review] by PC Magazine
- [Discussion: Nokia 6300 4G and Nokia 8000 4G] on 4PDA Forum (Russian)
- [Nokia 8000 4G and Nokia 6300 4G general discussion thread] on BananaHackers Google Groups
- [Nokia 8000 4G rooting research thread] on BananaHackers Google Groups
- [Nokia 6300 4G (nokia-leo)] on postmarketOS Wiki
- [Nokia 8000 4G (nokia-sparkler)] on postmarketOS Wiki
- [Affe Null's Bananian project repository], a Debian port for KaiOS devices

*GitHub Pages theme: MIT-licensed [riggraz/no-style-please](https://github.com/riggraz/no-style-please). Logo by [Vitaly Gorbachev](https://www.flaticon.com/free-icons/banana) from Flaticons*

<!-- these are called footnotes -->
[^1]: Taken from HMD's official [press release](https://www.hmdglobal.com/new-nokia-feature-phones-nokia-6300-4g-and-nokia-8000-4g) and [promotional video](https://www.youtube.com/watch?v=pub47YzYBJs).
[^2]: Aleph Security has a [deep-dive blog post](https://alephsecurity.com/2018/01/22/qualcomm-edl-1) into exploiting the nature of EDL mode on Qualcomm devices. If you're into the overall boot process, check out the breakdown of Qualcomm's Chain of Trust on [LineageOS Engineering Blog](https://lineageos.org/engineering/Qualcomm-Firmware).
[^3]: Read more about SELinux on [LineageOS team's Engineering Blog](https://lineageos.org/engineering/HowTo-SELinux).

<!-- Table of Contents -->
[provided by HMD]: https://nokiaphones-opensource.azureedge.net/download/phones/Nokia_6300_4G_20.00.17.01_OSS.tar.gz
[`leo-v20` branch of this repository]: https://github.com/minhduc-bui1/nokia-leo/tree/leo-v20
[Don’t buy a counterfeit]: #dont-buy-a-counterfeit
[About Kosher phones]: #about-kosher-phones
[Differences between US and international variants]: #differences-between-us-and-international-variants
[Tips and tricks]: #tips-and-tricks
[Known issues]: #known-issues
[KaiOS-specific]: #kaios-specific
[WhatsApp-specific]: #whatsapp-specific
[Secret codes]: #secret-codes
[Special boot modes]: #special-boot-modes
[UART debugging testpoint]: #uart-debugging-testpoint
[Sideloading and debugging third-party applications]: #sideloading-and-debugging-third-party-applications
[ROOT: Boot partition patching (non-US only)]: #root-boot-partition-patching-non-us-only
[Before proceeding: back up your data]: #before-proceeding-back-up-your-data
[What we’ll need]: #what-well-need
[Part 1: Set up environment for EDL tools]: #part-1-set-up-environment-for-edl-tools
[Part 2: Obtaining the boot partition]: #part-2-obtaining-the-boot-partition
[Nokia 8000 4G and Nokia 6300 4G with bkerler’s EDL]: #nokia-8000-4g-and-nokia-6300-4g-with-bkerlers-edl
[Nokia 2720 Flip and Nokia 800 Tough with andybalholm’s EDL]: #nokia-2720-flip-and-nokia-800-tough-with-andybalholms-edl
[Part 3: Patching the boot partition]: #part-3-patching-the-boot-partition
[Automatic patching with `8k-boot-patcher`]: #automatic-patching-with-8k-boot-patcher
[Manual patching with Android Image Kitchen]: #manual-patching-with-android-image-kitchen
[Part 4: Flashing the modified boot partition]: #part-4-flashing-the-modified-boot-partition
[External links]: #external-links

[COVID-19 pandemic]: https://en.wikipedia.org/wiki/COVID-19_pandemic
[In-Depth Review of the Nokia 6300 4G]: https://www.reddit.com/r/dumbphones/comments/t146b9/indepth_review_of_the_nokia_6300_4g/
[Nokia 6300 4G review after a month of use]: https://www.reddit.com/r/dumbphones/comments/15ghqx1/nokia_6300_4g_review_after_a_month_of_use/
[One week of use, the good, bad and the ugly… (Nokia 6300 4G)]: https://www.reddit.com/r/dumbphones/comments/oocmly/one_week_of_use_the_good_bad_and_the_ugly_nokia/

[terrible build quality]: https://www.reddit.com/r/KaiOS/comments/xglkr7/well_darn_it_i_just_received_a_counterfeit_nokia
[the system's `hosts` file]: https://ivan-hc.github.io/bananahackers/ADBlock.html
[CrossTweak]: https://gitlab.com/suborg/crosstweak
[BananaHackers' guide on fixing the keypad speed]: https://ivan-hc.github.io/bananahackers/fix-the-keypad-speed.html
[a playlist of silent MP3s played in background]: https://www.reddit.com/r/KaiOS/comments/15slovs/nokia_6300_4g_hotspot_drops_problem_workaround
[temporarily mute yourself on phone calls with VoLTE/VoWiFi enabled]: https://www.reddit.com/r/KaiOS/comments/15g5vo6/nokia_6300_4g_phone_calls_not_working_properly
[bypass it]: https://www.youtube.com/watch?v=0vTcQ_vY9LY
[unintended screenshots]: https://www.reddit.com/r/KaiOS/comments/vjnz83/screenshotting_every_time_i_sit_down_or_pedal_my
[arma7x's K-Music]: https://github.com/arma7x/kaimusic
[FabianOvrWrt's Explorer]: https://github.com/FabianOvrWrt/kaios-freshapps
[mochaSoft Aps' Photo Zoom]: https://www.kaiostech.com/store/apps/?bundle_id=dk.mochasoft.photozoom
[hardcoded within its code]: https://discordapp.com/channels/472006912846594048/472144586295345153/1173300055361458257
[replace the expired Let's Encrypt root certificate DST Root CA X3 with the newer ISRG Root X1]: https://letsencrypt.org/docs/dst-root-ca-x3-expiration-september-2021
[freeze on open]: https://www.reddit.com/r/KaiOS/comments/rawcu5/my_contacts_app_does_not_work
[FastContact]: https://gitlab.com/suborg/fastcontact
[BananaHackers' guide]: https://ivan-hc.github.io/bananahackers/clock-alarms.html#h.unmy3yif91xs
[Temporarily putting your SIM in another phone to receive the code]: https://www.reddit.com/r/KaiOS/comments/17rgyw5/comment/k8krpcd/?context=3
[Call barring]: https://www.communityphone.org/blogs/call-barring
[Call waiting]: https://en.wikipedia.org/wiki/Call_waiting
[Luxferre's CrossTweak]: https://gitlab.com/luxferre/crosstweak
[suitable digitally-signed programmer in MBN/ELF file format]: https://edl.bananahackers.net
[EDL archive site]: https://edl.bananahackers.net/loaders/8k.mbn
[As discovered by atipls on Discord]: https://discord.com/channels/472006912846594048/539074521580437504/1155993357206700205

<!-- Sideloading and debugging third-party applications -->
[Sideloading and debugging/WebIDE]: https://github.com/minhduc-bui1/nokia-leo/wiki/Sideloading-and-debugging
[BananaHackers Store]: https://store.bananahackers.net
[B-Hackers Store]: https://sites.google.com/view/b-hackers-store/home
[list of KaiOS apps]: https://github.com/stars/minhduc-bui1/lists/kaios-apps
[Next steps]: #next-steps
[this fork of Luxferre's AppBuster]: https://github.com/minhduc-bui1/AppBuster

<!-- Before proceeding: back up your data-->
[back up]: https://simple.wikipedia.org/wiki/Backup
[Murphy's Law]: https://en.wikipedia.org/wiki/Murphy%27s_law
[Newton's laws of motion]: https://www.britannica.com/science/Newtons-laws-of-motion
[PhoneCopy in KaiStore]: https://www.kaiostech.com/store/apps/?bundle_id=com.phonecopy.phonecopy
[kaios-backup]: https://github.com/D3SXX/kaios-backup
[kaios-sms-backup]: https://github.com/Fiachra1993/kaios-sms-backup
[greg]: https://github.com/strukturart/greg
[Backup]: https://github.com/minhduc-bui1/nokia-leo/wiki/Backup

<!-- ROOT -->
[8000 4G and 6300 4G]: https://edl.bananahackers.net/loaders/8k.mbn
[2720 Flip]: https://edl.bananahackers.net/loaders/2720.mbn
[800 Tough]: https://edl.bananahackers.net/loaders/800t.mbn
[AT&T/Cricket]: https://github.com/programmer-collection/alcatel/blob/master/Gflip3_ATT/Gflip3_ATT_NPRG.mbn
[T-Mobile/Metro/Rogers]: https://github.com/programmer-collection/alcatel/blob/master/Gflip3_TMO/Gflip3_TMO_NPRG.mbn
[Gerda Recovery image file]: https://cloud.disroot.org/s/3ojAfcF6J2jQrRg/download
[backup]: https://drive.google.com/open?id=1ot9rQDTYON8mZu57YWDy52brEhK3-PGh
[pyusb]: https://pypi.org/project/pyusb/
[pyserial]: https://pypi.org/project/pyserial/
[keystone-engine]: https://pypi.org/project/keystone-engine/
[capstone]: https://pypi.org/project/capstone/
[docopt]: https://pypi.org/project/docopt/
[bkerler's edl.py v3.1]: https://github.com/bkerler/edl/releases/tag/3.1
[andybalholm's edl]: https://github.com/andybalholm/edl
[Python's official download page for Windows]: https://www.python.org/downloads/windows
[Android Debug Bridge (ADB)]: https://developer.android.com/studio/releases/platform-tools
[Zadig]: https://github.com/pbatard/libwdi/releases/latest
[Homebrew]: https://brew.sh
[Git]: https://git-scm.com/book/en/v2/Getting-Started-Installing-Git
[Docker Desktop]: https://docs.docker.com/compose/install
[Linux kernel update package]: https://learn.microsoft.com/en-us/windows/wsl/install-manual#step-4---download-the-linux-kernel-update-package
[Windows]: https://github.com/osm0sis/Android-Image-Kitchen/tree/master
[macOS/Linux]: https://github.com/osm0sis/Android-Image-Kitchen/tree/AIK-Linux
[Notepad++]: https://notepad-plus-plus.org/downloads
[preserving line endings]: https://www.cs.toronto.edu/~krueger/csc209h/tut/line-endings.html
[Java Runtime Environment]: https://www.java.com/en/download
[Development/EDL tools on BananaHackers Wiki]: https://wiki.bananahackers.net/development/edl
[Microsoft Store]: https://apps.microsoft.com/search/publisher?name=Python+Software+Foundation
[environment variable]: https://en.wikipedia.org/wiki/Environment_variable
[Apps & features]: ms-settings:appsfeatures
[part 4]: #part-4-flashing-the-modified-boot-

<!-- External links -->
[Nokia 6300 4G product page]: https://www.nokia.com/phones/en_int/nokia-6300-4g
[Nokia 8000 4G product page]: https://www.nokia.com/phones/en_int/nokia-8000-4g
[Nokia 6300 4G review]: https://www.pcmag.com/reviews/nokia-6300-4g
[Discussion: Nokia 6300 4G and Nokia 8000 4G]: https://4pda.to/forum/index.php?showtopic=1009510
[Nokia 8000 4G and Nokia 6300 4G general discussion thread]: https://groups.google.com/g/bananahackers/c/jxEC3RVMYvI
[Nokia 8000 4G rooting research thread]: https://groups.google.com/g/bananahackers/c/8lCqP15zHXg
[Nokia 6300 4G (nokia-leo)]: https://wiki.postmarketos.org/wiki/Nokia_6300_4G_(nokia-leo)
[Nokia 8000 4G (nokia-sparkler)]: https://wiki.postmarketos.org/wiki/Nokia_8000_4G_(nokia-sparkler)
[Affe Null's Bananian project repository]: https://git.abscue.de/bananian/bananian

<style>
	.w { max-width: 1000px; padding: 4rem 1rem }
	li { margin-left: -0.5rem }
	body { font-family: system-ui, sans-serif; font-size: 14px }
</style>