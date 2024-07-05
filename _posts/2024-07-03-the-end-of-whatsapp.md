---
layout: post
category: news
title: The end of WhatsApp on KaiOS as we know it
---
It starts with the latest WhatsApp version preventing new users from signing up.

<img src="{{ site.baseurl }}/assets/images/blog/2024-07-04-11-35-05.png" align="right" width="240" height="320" style="width:240px;margin:0 0 1rem 1rem" alt="Screenshot of the error message in WhatsApp, with a yellow warning sign and a text which reads Something went wrong">

On June 26, several Reddit users reported on r/KaiOS that they were unable to sign up for WhatsApp after buying their KaiOS phones. When they tried to download and open WhatsApp for the first time, they saw a warning sign that simply read “Something went wrong.” with no further clarification.

This issue occurs after users update WhatsApp on their phones to the latest version, 2.2329.13, and have yet to register their phone numbers. People who previously signed into the app seem to be unaffected at the moment.

WhatsApp has yet to comment, nor has it publicly announced any plans to drop support for its KaiOS app.

Monitoring the phone’s network traffic over a proxy shows that this message is hardcoded as part of the update, with no data being sent to WhatsApp servers; this may indicate that the error is intentional and deliberate. All WhatsApp endpoints used by the app load and function as normal, even when updating the CA root store, so that excludes the possibility of any SSL certificate expirations.

Then, on July 3, WhatsApp decided to trigger the "time bomb" early for users staying on older versions, displaying a full-screen Critical Error message and [forcing users to update to the latest version via KaiStore](https://www.reddit.com/r/KaiOS/comments/1du50sr/i_found_the_solution_guys_you_just_have_to_set/).

Following this, Reddit users reached out to customer support services of WhatsApp, HMD, KaiOS Technologies, Jio, and others. While WhatsApp and KaiOS Technologies have yet to comment, HMD’s online chat support stated that [there would be a hotfix OTA update “very soon.”](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lanjcla/) Only Jio’s service center has confirmed that [WhatsApp would no longer be supported on KaiOS 2.5](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lb9gej5/), there would be no upcoming app updates, and this news will be publicly announced soon.

Some users [discovered a workaround](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lahl3bo/) by installing an older version of the client, 2.2329.10, changing the date of the phone back between June 28 and July 1, setting up their accounts, and then updating again to the latest version through KaiStore. But a day later, when users received a verification code for their phone number via SMS, the app checked the time and latest version number, then threw the Critical Error message and prevented them from signing up.

All of which lead us to the possibility that WhatsApp could eventually drop support for KaiOS devices.

2018 was a significant year for KaiOS, with the global release of the Nokia 8110 4G and the rising popularity of affordable JioPhones in India&#8212;a country where mobile payments among small and medium businesses are rapidly growing. Meta Platforms Inc., then known as Facebook Inc., and WhatsApp anticipated this and partnered with KaiOS Technologies, going after Google by [bringing WhatsApp](https://www.theverge.com/2019/7/22/20703872/whatsapp-kaios-nokia-8110-jio-phone-feature-phones) and Facebook to KaiOS devices, hoping to attract more users to their services.

Since then, these partnerships have been unstable and at times appear to have parted ways: Google and Jio collaborated to develop the JioPhone Next, which runs Android Go but hasn't gained much recognition; KaiOS Technologies has been investing more on African nations and the US in recent years; and HMD hasn't released any new KaiOS devices globally since 2020. KaiOS 2.5 phones have not been well-received by the public, being seen as laggy and lacking popular apps. The codebase, based on Gecko 48 from 2016, has also gradually introduced more vulnerabilities and technical limitations.

In 2021, KaiOS Technologies collaborated with Mozilla to update the Gecko codebase to a more recent Gecko 84 from 2020, which should introduce new web technologies to KaiOS. KaiOS 3 phones, despite their potential, are almost exclusive to the United States. Although applications from older versions can be easily updated and tested for compatibility with v3, app developers are reluctant due to the lack of debug-enabled devices outside the US, and device manufacturers are hesitant to produce v3 devices due to the lack of apps and interest.

Google [started dropping support for KaiOS](https://9to5google.com/2021/08/30/google-assistant-kaios-text/) in August of the same year, as Google Assistant lost the abilities to call, text, or toggle controls on your phone. The following year, device manufacturers released updates to entirely remove Assistant from KaiOS phones. On newer KaiOS versions, the YouTube app became just a shortcut to the YouTube website. To combat this, KaiOS Technologies developed its own version of Assistant, KaiVA, but it didn't take off beyond some newly released devices.

While WhatsApp has yet to take action, Meta has been pausing development of WhatsApp on KaiOS for a long time. Notably, some of the last major updates were [voice calls (VoIP)](https://www.androidauthority.com/whatsapp-calls-kaios-feature-phones-1233576/) in June 2021 and [polls](https://www.reddit.com/r/KaiOS/comments/zf5z76/testing_features_removed_as_the_latest_version_of/), first teased in November 2022. Since then, app updates have been periodically distributed over CI/CD on a monthly basis to maintain backend functionality in alignment with the Android and iOS versions, with minimal human involvement.

Normally, each WhatsApp version is associated with a "time bomb" that would display a reminder, telling users to update the app past a certain date. But changing the time on your phone to the 15th of September 2024 at midnight UTC will [display another payload message](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lbfjak6/):

> **WhatsApp is no longer available on KaiOS phones. Find more information by selecting Learn more.**

If you eventually are able to sign into the service, you will see a banner at the same time: *WhatsApp will soon be unavailable on KaiOS phones.* On selecting the banner:

> WhatsApp will soon stop working on KaiOS phones in the coming months. You will no longer have access to your account and chats on this phone. Select **Learn more** for more information.

On the 10th of January 2025 at 8:00 UTC, the app stops working and displays the payload message.

Pressing Learn more redirects to the [About supported operating systems page on WhatsApp Help Center](https://faq.whatsapp.com/595164741332628).

<p style="display:inline">
    <img src="{{ site.baseurl }}/assets/images/blog/2024-09-15-00-00-07.png" width="240" height="320" style="width:240px" alt="Screenshot of the payload message in WhatsApp, with a yellow warning sign">
    <img src="{{ site.baseurl }}/assets/images/blog/2024-10-01-00-00-18.png" width="240" height="320" style="width:240px" alt="Screenshot of the yellow banner with a warning sign in WhatsApp's chat list, text says WhatsApp will soon be unavailable on KaiOS phones.">
    <img src="{{ site.baseurl }}/assets/images/blog/2024-10-01-00-00-49.png" width="240" height="320" style="width:240px" alt="Screenshot of a popup with the message when selecting the banner">
</p>

If these are true, looks like WhatsApp has plans to drop support for KaiOS after all. It’s only a matter of time. Touché.

While these are only observations, the good news is, as people inspect and compare the code between the 2.2329.10 and 2.2329.13 versions, some noticed a few new files which indicates that WhatsApp might be working on some forms of device companion features on KaiOS comparable to WhatsApp Web or multi-device on Android or iOS.

<a href="https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lbfk6oz/">
    <img src="{{ site.baseurl }}/assets/images/blog/compare-wa-version.png" alt="Screenshot of a full-screen Visual Studio Code window. In the left sidebar there is a Only in compared folder section, which lists several files, one of which is a JavaScript named addCompanionDeviceScreen">
</a>

*OpenAI's GPT-4o was used to edit this blog post for natural wording.*

[Join the discussion on Reddit](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/whatsapp_stopped_working_nokia_6300_4g/)