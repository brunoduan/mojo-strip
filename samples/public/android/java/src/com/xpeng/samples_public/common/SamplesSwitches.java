// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.xpeng.samples_public.common;

/**
 * Contains all of the command line switches that are specific to the content/
 * portion of Chromium on Android.
 */
public final class SamplesSwitches {
    // Tell Java to use the official command line, loaded from the
    // official-command-line.xml files.  WARNING this is not done
    // immediately on startup, so early running Java code will not see
    // these flags.
    public static final String ADD_OFFICIAL_COMMAND_LINE = "add-official-command-line";

    // Enables test intent handling.
    public static final String ENABLE_TEST_INTENTS = "enable-test-intents";

    // Dump frames-per-second to the log
    public static final String LOG_FPS = "log-fps";

    // Whether Chromium should use a mobile user agent.
    public static final String USE_MOBILE_UA = "use-mobile-user-agent";

    // Change the url of the JavaScript that gets injected when accessibility mode is enabled.
    public static final String ACCESSIBILITY_JAVASCRIPT_URL = "accessibility-js-url";

    // Sets the ISO country code that will be used for phone number detection.
    public static final String NETWORK_COUNTRY_ISO = "network-country-iso";

    // How much of the browser controls need to be shown before they will auto show.
    public static final String TOP_CONTROLS_SHOW_THRESHOLD = "top-controls-show-threshold";

    // How much of the browser controls need to be hidden before they will auto hide.
    public static final String TOP_CONTROLS_HIDE_THRESHOLD = "top-controls-hide-threshold";

    // Native switch - chrome_switches::kDisablePopupBlocking
    public static final String DISABLE_POPUP_BLOCKING = "disable-popup-blocking";

    // Native switch kDisableGestureRequirementForPresentation
    public static final String DISABLE_GESTURE_REQUIREMENT_FOR_PRESENTATION =
            "disable-gesture-requirement-for-presentation";

    // Native switch kRendererProcessLimit
    public static final String SLAVER_PROCESS_LIMIT = "slaverer-process-limit";

    // Native switch kInProcessGPU
    public static final String IN_PROCESS_GPU = "in-process-gpu";

    // Native switch kProcessType
    public static final String SWITCH_PROCESS_TYPE = "type";

    // Native switch kRendererProcess
    public static final String SWITCH_SLAVERER_PROCESS = "slaverer";

    // Native switch kUtilityProcess
    public static final String SWITCH_UTILITY_PROCESS = "utility";


    // Native switch kHostResolverRules
    public static final String HOST_RESOLVER_RULES = "host-resolver-rules";

    // Native switch kServiceSandboxType
    public static final String SWITCH_SERVICE_SANDBOX_TYPE = "service-sandbox-type";

    // Native switch value kNetworkSandbox
    public static final String NETWORK_SANDBOX_TYPE = "network";

    // Prevent instantiation.
    private SamplesSwitches() {}
}
