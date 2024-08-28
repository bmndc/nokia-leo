---
layout: post
category: news
title: The end of WhatsApp on KaiOS as we know it
---
## TL;DR
**Did WhatsApp disappear from KaiStore?** It was deprecated on 26th June 2024. WhatsApp pulled it down KaiStore two weeks later.

**I have the previous version of WhatsApp downloaded on my phone, but it shows a Critical Error. What can I do to get it working again?** As of now, you can't anymore. There was a trick to bypass the Critical Error by setting the clock back, but when you get to the verification step, the app checks for the correct time on the server and throws the Critical Error again.

**I have already set up WhatsApp on my phone. Will this affect me?** On 15th September, you will see a banner notifying you that the app will stop working soon; the last moment will be on 10th January 2025 at 8am UTC.

---

It all started with the new version of WhatsApp preventing new users from signing up.

<img class="leo" src="{{ "/assets/images/blog/2024-07-04-11-35-05.png" | relative_url }}" align="right" width="240" height="100%" alt="Screenshot of the error message in WhatsApp, with a yellow warning sign and a text which reads Something went wrong">

26/6 was a rough day for people who bought a KaiOS phone to stay in touch with others over WhatsApp. Reddit users started flocking to the r/KaiOS subreddit, and reported that they were unable to log into the chat app. A few moments ago, they were installing the latest version, 2.2329.13, replacing the stub app on their new phones.

Now, whenever they try to download and open WhatsApp for the first time, they see a warning sign, below which simply reads “Something went wrong.” in bold, with no further clarification.

WhatsApp did not issue any official statements, nor has it publicly announced any plans to drop support for its KaiOS app.

Soon, the thread was flooded with new KaiOS users facing the same issue, although it didn’t seem to affect people who had already signed in with their phone numbers.

Some developers managed to look into the app’s source code and network traffic, even though much of it was minified and obfuscated. By monitoring the traffic through a proxy, they discovered that no data was being sent to WhatsApp servers, which suggests the error message was intentionally and deliberately hardcoded. Moreover, all WhatsApp endpoints loaded as usual, even after updating the CA root store, which rules out the possibility of any expired SSL certificates.

While comparing the source code of the previous version, 2.2319.10, with the latest one, they found some new files indicating that WhatsApp might be working on [some forms of device companion features](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lbfk6oz/), similar to WhatsApp Web or multi-device support on Android and iOS. This finding sparked hopes that this is just a temporary issue when implementing these new features.

But a week later, WhatsApp decided to fuse the "time bomb" early, displaying a full-screen Critical Error message for users who had not updated to the latest version, [forcing them to update via KaiStore](https://www.reddit.com/r/KaiOS/comments/1du50sr/i_found_the_solution_guys_you_just_have_to_set/).

Some people [discovered a workaround](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lahl3bo/) by installing an older version of the app, 2.2329.10, changing the date of the phone back between June 28 and July 1, logging in and then updating again to the latest version through KaiStore. But when users receive a SMS verification code for their phone number, the app checks the time and latest version, then throws the Critical Error message and prevents them from signing up.

---

Normally, each WhatsApp version is associated with a "time bomb" that displays a reminder, telling users to update the app past a certain date. However, changing the time on your phone to the 15th of September 2024 at midnight UTC [displays another payload message](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lbfjak6/):

> **WhatsApp is no longer available on KaiOS phones. Find more information by selecting Learn more.**

If you're using the app, at the same time you will see a banner: *WhatsApp will soon be unavailable on KaiOS phones.* On selecting the banner:

> WhatsApp will soon stop working on KaiOS phones in the coming months. You will no longer have access to your account and chats on this phone. Select **Learn more** for more information.

On the 10th of January 2025 at 8:00 UTC, the app stops working and displays the payload message.

Pressing Learn more redirects to the [About supported operating systems page on WhatsApp Help Center](https://faq.whatsapp.com/595164741332628).

---

Following this, Reddit users reached out to customer support services of WhatsApp, HMD, KaiOS Technologies, Jio and others. While WhatsApp and KaiOS Technologies have yet to comment, HMD’s online chat support promised that [there would be a hotfix OTA update “very soon.”](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lanjcla/) So far, only Jio’s service center stated that [WhatsApp would no longer be supported on KaiOS 2.5](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lb9gej5/), there would be no upcoming app updates, and this news would be publicly announced soon.

All of which lead to speculation that WhatsApp might eventually drop support for KaiOS phones.

2018 was a significant year for KaiOS, with the global release of the Nokia 8110 4G and the rising popularity of affordable JioPhones in India&#8212;a country where mobile payments among small and medium businesses are rapidly growing. A few months before, [Google invested $22 million USD](https://www.theverge.com/2018/6/28/17513036/google-kaios-investment-feature-phones-firefox-os-apps-services-strategy) and announced it would bring its services, Search, Maps, YouTube and Assistant onto the platform. Meta Platforms Inc., then known as Facebook Inc., and WhatsApp anticipated this and partnered with KaiOS Technologies, by [bringing WhatsApp](https://www.theverge.com/2019/7/22/20703872/whatsapp-kaios-nokia-8110-jio-phone-feature-phones) and Facebook to KaiOS devices, hoping to attract more users to their services.

Since then, these partnerships have been unstable and at times appear to have parted ways: Google and Jio collaborated to [develop the JioPhone Next](https://www.theverge.com/2021/10/29/22752388/india-google-smartphone-jiophone-pixel), which runs Android Go but hasn't gained much recognition; KaiOS Technologies has been [investing more on African nations and the US](https://www.kaiostech.com/kaios-was-at-ces-tech-event-2024/) in recent years; and HMD hasn't released any new KaiOS devices globally since 2020. KaiOS 2.5 phones have not been well-received by the public, being seen as laggy and lacking popular apps. The codebase, based on Gecko 48 from 2016, has also gradually introduced more vulnerabilities and technical limitations.

In 2021, KaiOS Technologies [collaborated with Mozilla to update the Gecko codebase](https://www.ghacks.net/2020/03/14/firefox-os-successor-mozilla-and-kaios-announce-partnership/) to a more recent Gecko 84 from 2020, which should introduce new web technologies to KaiOS. Despite the potential, KaiOS 3 phones are largely exclusive to the United States. Although applications from older versions can be easily updated and tested for compatibility with v3, app developers are reluctant due to the lack of debug-enabled devices, and device manufacturers are hesitant to produce v3 devices due to the lack of sales and interest from carriers.

Google [started dropping support for KaiOS](https://9to5google.com/2021/08/30/google-assistant-kaios-text/) in August of the same year, as Google Assistant lost the abilities to call, text, or toggle controls on KaiOS phones. In the following year, device manufacturers released over-the-air updates to [entirely remove Assistant from KaiOS](https://www.reddit.com/r/KaiOS/comments/w9tv0p/goodbye_google_assistant_and_speech_to_text/). On newer versions of the operating system, the YouTube app became just a shortcut to the YouTube website. To combat this, KaiOS Technologies developed its own Assistant spinoff, KaiVA, but [it didn't take off](https://www.reddit.com/r/KaiOS/comments/x6oaa5/kaiva_kai_voice_assistant_service_on_nokia_v_flip/) [beyond some newly released devices](https://www.reddit.com/r/KaiOS/comments/18uoux5/kaiva_in_kaios_25/).

While WhatsApp has yet to take action, Meta has been pausing development of WhatsApp on KaiOS for a long time. Notably, the last major updates were [voice calls (VoIP)](https://www.androidauthority.com/whatsapp-calls-kaios-feature-phones-1233576/) in June 2021 and [polls](https://www.reddit.com/r/KaiOS/comments/zf5z76/testing_features_removed_as_the_latest_version_of/), first teased in November 2022. Since then, app updates have been periodically distributed on a monthly basis, often to maintain backend functionality in alignment with the Android and iOS versions.

---

Another week went past, and WhatsApp pulled their app off KaiStore to not let people download the broken build.

TODO: add Blackview's and KaiOS's statements

---

While these are only my observations, if these are true, looks like WhatsApp has plans to drop support for KaiOS after all. It’s only a matter of time. Touché.

Good news, however: as people inspect and compare the code between versions 2.2329.10 and 2.2329.13, they noticed new files suggesting that WhatsApp might be working on [some forms of device companion features](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lbfk6oz/), similar to WhatsApp Web or multi-device support on Android and iOS. If all of these are fluke, we might get to see this come into light in the near future.

*Kudos to [@fabricedesre](https://github.com/fabricedesre) and [@Tombarr](https://barrasso.me/) for fact-checking information in this blog post.*

[Join the discussion on Reddit](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/whatsapp_stopped_working_nokia_6300_4g/)


<!-- img class="leo" src="{{ site.baseurl }}/assets/images/blog/2024-07-04-11-35-05.png" align="right" width="240" height="100%" alt="Screenshot of the error message in WhatsApp, with a yellow warning sign and a text which reads Something went wrong">

<p style="text-align:center">
    <img src="{{ site.baseurl }}/assets/images/blog/wa-hmd.png" width="300" height="100%" style="display:inline-block" alt="Screenshot of HMD customer support chat">
    <img src="{{ site.baseurl }}/assets/images/blog/wa-support.jpeg" width="388" height="100%" style="display:inline-block" alt="Screenshot of WhatsApp support chat, telling user to keep themselves up-to-date with WhatsApp Help Centre and WhatsApp Blog">
</p>

<p style="text-align:center">
    <img src="{{ site.baseurl }}/assets/images/blog/2024-09-15-00-00-07.png" width="240" height="100%" style="display:inline-block" alt="Screenshot of the payload message in WhatsApp, with a yellow warning sign">
    <img src="{{ site.baseurl }}/assets/images/blog/2024-10-01-00-00-18.png" width="240" height="100%" style="display:inline-block" alt="Screenshot of the yellow banner with a warning sign in WhatsApp's chat list, text says WhatsApp will soon be unavailable on KaiOS phones.">
    <img src="{{ site.baseurl }}/assets/images/blog/2024-10-01-00-00-49.png" width="240" height="100%" style="display:inline-block" alt="Screenshot of a popup with the message when selecting the banner">
</p>

<img src="{{ site.baseurl }}/assets/images/blog/wa-vscode.png" width="100%" height="100%" alt="Screenshot of a full-screen Visual Studio Code window. In the left sidebar there is a Only in compared folder section, which lists several files, one of which is a JavaScript named addCompanionDeviceScreen" -->