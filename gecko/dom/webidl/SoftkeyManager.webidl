/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Copyright (c) 2016 Acadine Technologies. All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at </code>
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

dictionary SoftkeyOptions {
    DOMString name = ""; // The name to show on screen.
    DOMString icon = ""; // The icon to draw on screen.
    boolean disabled = false; // Whether to draw the softkeys in disable state.
};

dictionary Softkey {
    DOMString code = ""; // The value that identifies a physical key.
    SoftkeyOptions options; // The options of this softkey.
};

[Pref="dom.softkey.enabled", CheckAnyPermissions="softkey", AvailableIn=CertifiedApps]
interface SoftkeyManager {
    /**
     * Show or hide the entire area of softkeys, including key names, icons and
     * the hud underneath. System App should receive a 'softkeyvisiblechange'
     * event if change has made to this attribute.
     */
    attribute boolean visible;

    /**
     * Register for softkeys. Reject if any of the key in register list doesn't
     * supported by current device. If the device doesn't support any softkey,
     * given an empty list should resolve. System App should receive a
     * 'softkeyregistered' event if this promise is going to be resolved.
     *
     * @param keys list of softkeys that wish to be registered.
     */
    [Throws]
    Promise<void> registerKeys(sequence<Softkey> keys);
};
