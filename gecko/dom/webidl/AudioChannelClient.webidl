/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2018 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

[AvailableIn="PrivilegedApps",
 Constructor(AudioChannel audioChannelType)]
interface AudioChannelClient : EventTarget {

    // Called when APP wants to occupy the audio channel which is specified in the
    // constructor. After calling this function, it doesn't mean that this channel is
    // immediately unmuted. APP may want to hook onstatechange handler in order to be
    // notified when muted state changes.
    // This function is useful for APP which wants background music/FM to be muted.
    [Throws]
    void requestChannel();

    // Called when APP no longer wants to occupy the audio channel. APP must call this
    // function if it has called requestChannel().
    [Throws]
    void abandonChannel();

    // This event is dispatched when muted state changes. APP should check channelMuted
    // attribute to know current muted state.
    attribute EventHandler onstatechange;

    // When channelMuted is false, it means APP is allowed to play content through this
    // channel. It also means the competing channel may be muted, or its volume may be
    // reduced.
    // If the channel owned by us is interrupted (muted), or APP did not request channel,
    // or APP has abandoned the channel, channelMuted is set to true.
    readonly attribute boolean channelMuted;
};
