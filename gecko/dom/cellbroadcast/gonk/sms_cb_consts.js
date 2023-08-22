/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

/**
 * Preference key for whether to enable emergency notifications (default enabled).
 */
this.KEY_ENABLE_EMERGENCY_ALERTS = "enable_emergency_alerts";

/**
 * Whether to display CMAS extreme threat notifications (default is enabled).
 */
this.KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS = "enable_cmas_extreme_threat_alerts";

/**
 * Whether to display CMAS severe threat notifications (default is enabled).
 */
this.KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS = "enable_cmas_severe_threat_alerts";

/**
 * Whether to display CMAS amber alert messages (default is enabled).
 */
this.KEY_ENABLE_CMAS_AMBER_ALERTS = "enable_cmas_amber_alerts";

/**
 * The default flag specifying whether ETWS/CMAS test setting is forcibly disabled in
 * Settings->More->Emergency broadcasts menu even though developer options is turned on.
 * (default is disabled)
 */
this.KEY_CARRIER_FORCE_DISABLE_ETWS_CMAS_TEST_BOOL = "carrier_force_disable_etws_cmas_test_bool";

/**
 * Whether to display ETWS test messages (default is disabled).
 */
this.KEY_ENABLE_ETWS_TEST_ALERTS = "enable_etws_test_alerts";

/**
 * Whether to display CMAS monthly test messages (default is disabled).
 */
this.KEY_ENABLE_CMAS_TEST_ALERTS = "enable_cmas_test_alerts";

/**
 * For any other custom GSM cb channel ranges.
 * Format: [left, right,...], it will enable id from lef to (right-1).
 */
this.KEY_OTHER_CHANNEL_RANGES = "cb_other_gsm_channel_ranges";

// Allow this file to be imported via Components.utils.import().
this.EXPORTED_SYMBOLS = Object.keys(this);