---
layout: post
title: Android Oreo now ported to leo; experimental root script for Arch users
category: news
---
Despite a long list of issues and complaints, two things about the 6300 4G that kept me aroused with HMD back in the day were the phone's affordability, and how the company acknowledged the developer community. And as such, since it released the 6300 4G's kernel source code, there have been tons of contributions made by the community to keep the phone alive. Most notably, Affe Null and asslan have together made a [postmarketOS port](https://wiki.postmarketos.org/wiki/Nokia_6300_4G_(nokia-leo)) for the 6300 4G, putting the cutting-edge of desktop Linux onto the device.

Now, thanks to @Llixuma, we now have another operating system port for the 6300 4G. This time, it's Android 8.1! It is very much experimental at the time of writing, as they're working on getting the most of kernel logs from the UART port and fine-tuning the device tree. But I see this as an oppotunity to push the limits of what we can do with this versatility of a phone.

You can check out and compile leo's Android Oreo device tree for yourself [here](https://github.com/Llixuma/Nokia-Leo-device-tree). Contributions are welcome; if you're proficient in building Android device trees, you can have a chat with the developer on Telegram.

Also, TIL that the Arch User Repository (AUR) hosts a version of bkerler's EDL, sparking the possibility of automating partition works in EDL mode, and thus automating 6300 4G's rooting process as well. With the help of @Llixuma's [documentation](https://github.com/Llixuma/Nokia-6300-4G-root-2024/blob/main/Tutorial.md) and inspiration from the team at ibus-bamboo, I decided to write an experimental Bash script with some interactive prompts to automatically root the phone, which you can find [here](https://github.com/minhduc-bui1/nokia-leo/blob/docs/root.sh). Rooting the phone is now as easy as:
```console
bash -c "$(curl -fsSL https://raw.githubusercontent.com/minhduc-bui1/nokia-leo/docs/root.sh)"
```
Please use this at your own risk, though.