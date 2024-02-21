---
layout: post
title: Migration to GitHub-flavoured Markdown; w2d.md archival and QoL changes
category: news
---
After a long time figuring out why GitHub Pages refused to read my SCSS code (`main.scss` not `style.scss`!), about 50 commits, most of which were just correcting mistakes and citing as "minor changes", and half a cup of coffee, today I made the decision to switch the nokia-leo documentation to use GitHub-flavoured Markdown (GfM) instead of `kramdown`.

`kramdown`, while being more versatile with support for more HTML tags and inline JavaScript code (GfM refuses to do so for "security reasons"), does not respect ordered and nested lists, which I presume 90% of the documentation is. I've tried to temporarily fix the problem with StackOverflow workarounds and CSS tags throughout the rooting guide, which made the guide unpleasant to follow if you read directly on github.com. GfM doesn't have this problem and render the guide as it's supposed to be. Moving to GfM means that the README viewing experience on github.com and GitHub Pages would be identical and easier to maintain. Considering the advantage, I decided to go for GfM.

Due to GfM not allowing HTML script tags, this change means that my fork of W2D on BananaHackers website, w2d.md is now obsolete. @cyan-2048 has since made a fork of the site directly at https://wiki.bananahackers.net/w2d that you can visit to turn on Wi-Fi hotspot functionality on unsupported devices.

Also: scrollable code on smaller screens! By default, no-style-please wraps the code snippets, which means they take more real screen estate and following some steps to modify code on a smaller screen with large font scale is difficult. This quality-of-life change should make it more easier by allowing swiping or scrolling to see the rest of the code.