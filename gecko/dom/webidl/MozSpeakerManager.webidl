/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * SpeakerPolicy is used to configure the force speaker policy of a SpeakerManager.
 * - "foreground-or-playing" (default value):
 *   SpeakerManagerService will try to respect our "forcespeaker" setting when our
 *   SpeakerManager is in the foreground, or our APP is playing. Note that foreground
 *   state has a higher priority than playing state. If we are in the foreground and
 *   not playing, but some other background APP is playing, then SpeakerManagerService
 *   will respect our "forcespeaker" setting.
 * - "playing":
 *   SpeakerManagerService will try to respect our "forcespeaker" setting only when
 *   our APP is playing.
 * - "query":
 *   Configure SpeakerManager as query mode. In this mode, SpeakerManager can only be
 *   used to query "speakerforced" status, but our "forcespeaker" setting is never
 *   applied. The event "onspeakerforcedchange" is still effective.
 */
enum SpeakerPolicy {
  "foreground-or-playing",
  "playing",
  "query"
};

/*
 * Allow application can control acoustic sound output through speaker.
 * Reference https://wiki.mozilla.org/WebAPI/SpeakerManager
 */
[Constructor(optional SpeakerPolicy policy)]
interface MozSpeakerManager : EventTarget {
  /* query the speaker status */
  readonly attribute boolean speakerforced;
  /* force device device's acoustic sound output through speaker */
  attribute boolean forcespeaker;
  /* this event will be fired when device's speaker forced status change */
  attribute EventHandler onspeakerforcedchange;
};
