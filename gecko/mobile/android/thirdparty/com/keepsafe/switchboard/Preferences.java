/*
   Copyright 2012 KeepSafe Software Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
package com.keepsafe.switchboard;


import android.content.Context;
import android.content.SharedPreferences;
import android.support.annotation.Nullable;

/**
 * Application preferences for SwitchBoard.
 * @author Philipp Berner
 *
 */
public class Preferences {

    private static final String switchBoardSettings = "com.keepsafe.switchboard.settings";

    private static final String kDynamicConfigServerUrl = "dynamic-config-server-url";
    private static final String kDynamicConfig = "dynamic-config";

    /**
     * Returns the stored config server URL.
     * @param c Context
     * @return URL for config endpoint.
     */
    @Nullable public static String getDynamicConfigServerUrl(Context c) {
        final SharedPreferences prefs = c.getApplicationContext().getSharedPreferences(switchBoardSettings, Context.MODE_PRIVATE);
        return prefs.getString(kDynamicConfigServerUrl, null);
    }

    /**
     * Stores the config servers URL.
     * @param c Context
     * @param configServerUrl URL for config endpoint.
     */
    public static void setDynamicConfigServerUrl(Context c, String configServerUrl) {
        final SharedPreferences.Editor editor = c.getApplicationContext().
                getSharedPreferences(switchBoardSettings, Context.MODE_PRIVATE).edit();
        editor.putString(kDynamicConfigServerUrl, configServerUrl);
        editor.apply();
    }

    /**
     * Gets the user config as a JSON string.
     * @param c Context
     * @return Config JSON
     */
    @Nullable public static String getDynamicConfigJson(Context c) {
        final SharedPreferences prefs = c.getApplicationContext().getSharedPreferences(switchBoardSettings, Context.MODE_PRIVATE);
        return prefs.getString(kDynamicConfig, null);
    }

    /**
     * Saves the user config as a JSON sting.
     * @param c Context
     * @param configJson Config JSON
     */
    public static void setDynamicConfigJson(Context c, String configJson) {
        final SharedPreferences.Editor editor = c.getApplicationContext().
                getSharedPreferences(switchBoardSettings, Context.MODE_PRIVATE).edit();
        editor.putString(kDynamicConfig, configJson);
        editor.apply();
    }
}
