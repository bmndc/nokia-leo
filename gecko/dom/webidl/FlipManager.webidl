/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

/**
 * FlipManager reports the current flip status, and dispatch a flipchange event
 * when device has flip opened or closed.
 */
[Pref="dom.flip.enabled", CheckAnyPermissions="flip", AvailableIn=CertifiedApps]
interface FlipManager : EventTarget {
    readonly attribute boolean flipOpened;

    attribute EventHandler onflipchange;
};
