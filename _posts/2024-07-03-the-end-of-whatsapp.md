---
layout: post
category: news
title: The end of WhatsApp on KaiOS is nigh
---
...and it starts with the latest WhatsApp version preventing new users from signing up.

On the 26th of June, a large number of Reddit users reported on r/KaiOS of not being able to sign up for WhatsApp on KaiOS. If you try to download and open WhatsApp for the first time, you may see a warning sign that reads "Something went wrong." This message appears after people update to the latest version of the app, 2.2329.13, which came as part of the service's monthly app update releases.

KaiOS 2.5 devices which have yet to sign up for the service are all affected, with no exceptions so far have been recorded. Fortunately, it does seem to not affect users who have previously signed into the app at the moment. WhatsApp has not publicly announced any plans of deprecating its KaiOS app.

Monitoring the phone's network traffic over a proxy shows that this message is hardcoded as part of the update, with no data being sent to any of WhatsApp servers; which may indicate that the error is intentional and deliberate. All WhatsApp endpoints used by the app load and function as normal, even when updating the CA root store, which excludes the possibility of any SSL certificate expirations.

In addition to that, on the 3rd of July, the service decided to "nuke the time bomb" early for users who are staying on older versions, displaying a full screen message to force them to update to the latest version.

For now, if you want to continue setting up WhatsApp on KaiOS, you will have to install an older version of the app via either ADB and WebIDE or KaiStore Developer Portal, change the date of your phone to between 28th of June and 1st of July, sign up the service with your phone number, and finally update the app again to the latest version with KaiStore.

...which leads us to the point of when WhatsApp will eventually drop support for KaiOS devices.

Following this, a number of Reddit users have reached out to the customer support services of some parties involved, including WhatsApp, HMD, KaiOS Technologies and Jio. As of now, Jio's service centre confirmed that WhatsApp would not be further supported on KaiOS 2.5, there would be no upcoming app updates and that the news will be publicly announced soon. Meanwhile, HMD's online chat support stated that there would be a hotfix OTA update "very soon". WhatsApp and KaiOS Technologies have yet to comment.

While WhatsApp has yet to deprecate its KaiOS app, Meta (previously Facebook Inc.) has been pausing development of WhatsApp on KaiOS for a long time, with no notable features to be seen; some of the last big updates were VoIP in June 2021 and polls, first teased in November 2022. Since then, app updates have been periodically distributed over CI/CD on a monthly basis to keep its backend operable and in line with its Android and iOS counterparts, so there is virtually no person involved.

Changing the time on your phone to the 15th of September 2024 at midnight UTC will display the true payload message:

> **WhatsApp is no longer available on KaiOS phones. Find more information by selecting Learn more.**

If you eventually are able to sign into the service, you will see a banner at the same time:

> WhatsApp will soon be unavailable on KaiOS phones.

> WhatsApp will soon stop working on KaiOS phones in the coming months. You will no longer have access to your account and chats on this phone. Select **Learn more** for more information.

Finally, on the 10th of January 2025 at 8:00 UTC:

> **WhatsApp is no longer available on KaiOS phones. Find more information by selecting Learn more.**

Pressing Learn more redirects to the [About supported operating systems page on WhatsApp Help Center](https://faq.whatsapp.com/595164741332628).

Looks like WhatsApp has plans after all. It's only a matter of time. Touché.

If you have just bought a brand new KaiOS phone today and plan to set up WhatsApp to connect with friends and family... today is not the day for you.