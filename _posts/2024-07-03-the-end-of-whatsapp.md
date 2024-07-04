---
layout: post
category: news
title: The end of WhatsApp on KaiOS as we know it
---
<img src="{{ site.baseurl }}/assets/images/blog/2024-07-04-11-35-05.png" align="right" width="240" height="320" style="width:240px;margin:0 0 1rem 1rem" alt="Screenshot of the error message in WhatsApp, with a yellow warning sign and a text which reads Something went wrong">

It starts with the latest WhatsApp version preventing new users from signing up.

On 26 June, several Reddit users reported on r/KaiOS that they were unable to sign up for WhatsApp after they have bought their KaiOS phones. When they try to download and open WhatsApp for the first time, they see a warning sign that simply reads “Something went wrong.” with no further clarification.

This occurs after users update to the latest app version, 2.2329.13 and have yet to register their phone number in the app. People who previously signed into the app seem to be unaffected at the moment.

WhatsApp has yet to comment, nor has it publicly announced any plans to drop support for its KaiOS app.

Monitoring the phone’s network traffic over a proxy shows that this message is hardcoded as part of the update, with no data being sent to WhatsApp servers; which may indicate that the error is intentional and deliberate. All WhatsApp endpoints used by the app load and function as normal, even when updating the CA root store, so that excludes the possibility of any SSL certificate expirations.

In addition to that, on 3 July, the service decided to “nuke the time bomb” early for users who are staying on older versions, displaying a full screen message and [forcing users to update to the latest version via KaiStore](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lbelz97/).

All of which lead us to the possibility of when WhatsApp could eventually drop support for KaiOS devices.

Following this, a number of Reddit users have reached out to the customer support services of some parties involved, including WhatsApp, HMD, KaiOS Technologies and Jio. HMD’s online chat support stated that [there would be a hotfix OTA update “very soon”](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lanjcla/). WhatsApp and KaiOS Technologies have yet to comment. As of now, only Jio’s service centre confirmed that [WhatsApp would not be further supported on KaiOS 2.5](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lb9gej5/), there would be no upcoming app updates, and that the news will be publicly announced soon.

While WhatsApp has yet to deprecate its KaiOS app, Meta (previously Facebook Inc.) has been pausing development of WhatsApp on KaiOS for a long time, with no notable features to be seen; some of the last big updates were [voice calls (VoIP)](https://www.kaiostech.com/whatsapp-data-voice-calls-available-on-kaios-devices/) in June 2021 and [polls](https://www.reddit.com/r/KaiOS/comments/zf5z76/testing_features_removed_as_the_latest_version_of/), which was first teased in November 2022. Since then, app updates have been periodically distributed over CI/CD on a monthly basis to keep its backend operable and in line with its Android and iOS counterparts, so there is virtually no person involved.

Changing the time on your phone to the 15th of September 2024 at midnight UTC will [display the true payload message](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lbfjak6/):

> **WhatsApp is no longer available on KaiOS phones. Find more information by selecting Learn more.**

If you eventually are able to sign into the service, you will see a banner at the same time:

> WhatsApp will soon be unavailable on KaiOS phones.
> <br>
> WhatsApp will soon stop working on KaiOS phones in the coming months. You will no longer have access to your account and chats on this phone. Select **Learn more** for more information.

Finally, on the 10th of January 2025 at 8:00 UTC:

> **WhatsApp is no longer available on KaiOS phones. Find more information by selecting Learn more.**

Pressing Learn more redirects to the [About supported operating systems page on WhatsApp Help Center](https://faq.whatsapp.com/595164741332628).

If these are true, looks like WhatsApp has plans to drop support for KaiOS after all. It’s only a matter of time. Touché.

---

The good news is, while inspecting and comparing the code between the 2.2329.10 and 2.2329.13 versions, some people noticed a few new files which indicates that WhatsApp might be working on [some forms of device companion features on KaiOS](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lbfk6oz/) comparable to WhatsApp Web or multi-device on Android or iOS.

For now, if you want to continue setting up WhatsApp on KaiOS, you will have to install an older version of the app via either ADB and WebIDE or KaiStore Developer Portal, change the date of your phone to between 28th of June and 1st of July, sign up the service with your phone number, and finally update the app again to the latest version with KaiStore. [u/NoMoreUsernameLeak has a great guide for this.](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/comment/lahl3bo/)

[Join the discussion on Reddit](https://www.reddit.com/r/KaiOS/comments/1dp9ubt/whatsapp_stopped_working_nokia_6300_4g/)