---
layout: post
category: news
title: The end of WhatsApp on KaiOS as we know it
---
It starts with the latest WhatsApp version preventing new users from signing up.

<img src="{{ site.baseurl }}/assets/images/blog/2024-07-04-11-35-05.png" align="right" width="240" height="320" style="width:240px;margin:0 0 1rem 1rem" alt="Screenshot of the error message in WhatsApp, with a yellow warning sign and a text which reads Something went wrong">

On 26 June, several Reddit users reported on r/KaiOS that they were unable to sign up for WhatsApp after they have bought their KaiOS phones. When they try to download and open WhatsApp for the first time, they see a warning sign that simply reads “Something went wrong.” with no further clarification.

This occurs after users update WhatsApp on their phone to the latest version, 2.2329.13, and have yet to register their phone number. People who previously signed into the app seem to be unaffected at the moment.

WhatsApp has yet to comment, nor has it publicly announced any plans to drop support for its KaiOS app.

Monitoring the phone’s network traffic over a proxy shows that this message is hardcoded as part of the update, with no data being sent to WhatsApp servers; which may indicate that the error is intentional and deliberate. All WhatsApp endpoints used by the app load and function as normal, even when updating the CA root store, so that excludes the possibility of any SSL certificate expirations.

Then, on 3 July, WhatsApp decided to fuse the "time bomb” early for users who are staying on older versions, displaying a full screen Critical Error message, [forcing users to update to the latest version via KaiStore](https://www.reddit.com/r/KaiOS/comments/1du50sr/i_found_the_solution_guys_you_just_have_to_set/).

Following this, Reddit users have reached out to customer support services of WhatsApp, HMD, KaiOS Technologies, Jio and others. While WhatsApp and KaiOS Technologies have yet to comment, HMD’s online chat support stated that [there would be a hotfix OTA update “very soon”](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lanjcla/). Only Jio’s service centre has confirmed that [WhatsApp would not be further supported on KaiOS 2.5](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lb9gej5/), there would be no upcoming app updates, and that the news will be publicly announced soon.

Some users [discovered a workaround](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lahl3bo/) by installing an older version of the client, 2.2329.10, change the date of the phone back between 28 June and 1 July, set up their accounts and then update again to the latest version though KaiStore. But a day later, when users receive a verification code for their phone number over SMS, the app checks the time and latest version number, then throws the Critical Error message and prevents them from signing up.

All of which lead us to the possibility of whether WhatsApp could eventually drop support for KaiOS devices.

2018 was a great year of KaiOS, with the global release of the Nokia 8110 4G, and the affordable JioPhones started to get more popularity in India&#8212;the land where the trend of mobile payments among small and medium businesses is accelerating. Meta Platforms Inc., at the time was Facebook Inc., and WhatsApp foresaw this and partnered with KaiOS Technologies, going after Google to bring WhatsApp and Facebook onto KaiOS devices, hoping to get more users onto their services.

Since then, these partnerships have been quite unstable, and at times seem like they have parted ways: Google and Jio worked directly to develop the JioPhone Next, which runs Android Go and was not greatly known; KaiOS Technologies investing more in the African nations and the US in recent years; HMD hasn't released any new KaiOS devices globally since 2020... KaiOS 2.5 phones have not been well received by the public, having been seen as laggy and missing popular apps. The codebase, which is based on Gecko 48 from 2016, also keeps gradually introducing vulnerabilities and technical limitations, one after another.

In 2021, KaiOS Technologies collaborated with Mozilla to update the Gecko codebase to a more recent Gecko 84 from 2020, which should also bring new web technologies to KaiOS. KaiOS 3 phones, while having potential, are almost exclusive to the United States. Although applications from older versions can be easily brought up and tested to be compatible with v3, app developers are reluctant to do so with the lack of debug-enabled devices outside the US, and device manufacturers are reluctant to v3 devices with the lack of apps...

While WhatsApp has yet to deprecate its KaiOS app, Meta Platforms Inc. (formerly Facebook Inc.) has been pausing development of WhatsApp on KaiOS for a long time. Notably, some of the last major updates were [voice calls (VoIP)](https://www.kaiostech.com/whatsapp-data-voice-calls-available-on-kaios-devices/) in June 2021 and [polls](https://www.reddit.com/r/KaiOS/comments/zf5z76/testing_features_removed_as_the_latest_version_of/), which was first teased in November 2022. Since then, app updates have been periodically distributed over CI/CD on a monthly basis to maintain backend functionality in alignment with the Android and iOS versions, with minimal human involvement.

Normally, each WhatsApp version is associated with a "time bomb" that would display a reminder, telling users to update the app past a certain date. But changing the time on your phone to the 15th of September 2024 at midnight UTC will [display another payload message](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lbfjak6/):

> **WhatsApp is no longer available on KaiOS phones. Find more information by selecting Learn more.**

If you eventually are able to sign into the service, you will see a banner at the same time: *WhatsApp will soon be unavailable on KaiOS phones.*

> WhatsApp will soon stop working on KaiOS phones in the coming months. You will no longer have access to your account and chats on this phone. Select **Learn more** for more information.

On the 10th of January 2025 at 8:00 UTC, the app stops working and displays the payload message.

Pressing Learn more redirects to the [About supported operating systems page on WhatsApp Help Center](https://faq.whatsapp.com/595164741332628).

If these are true, looks like WhatsApp has plans to drop support for KaiOS after all. It’s only a matter of time. Touché.

---
While these are only observations, the good news is, as people inspect and compare the code between the 2.2329.10 and 2.2329.13 versions, some noticed a few new files which indicates that WhatsApp might be working on some forms of device companion features on KaiOS comparable to WhatsApp Web or multi-device on Android or iOS.

<a href="https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lbfk6oz/">
    <img src="{{ site.baseurl }}/assets/images/blog/compare-wa-version.png" alt="Screenshot of a full-screen Visual Studio Code window. In the left sidebar there is a Only in compared folder section, which lists several files, one of which is a JavaScript named addCompanionDeviceScreen">
</a>

For now, if you want to continue setting up WhatsApp on KaiOS, you will have to install an older version of the app via either ADB and WebIDE or KaiStore Developer Portal, change the date of your phone to between 28th of June and 1st of July, sign up the service with your phone number, and finally update the app again to the latest version with KaiStore. [u/NoMoreUsernameLeak has a great guide for this.]()

*(Note: this might not work anymore as WhatsApp checks for the server time when sending SMS verification code.)*

[Join the discussion on Reddit](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/whatsapp_stopped_working_nokia_6300_4g/)