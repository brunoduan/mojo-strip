// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.xpeng.samples_shell_apk;

import android.app.Application;
import android.content.Context;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.BuildConfig;
import org.chromium.base.CommandLine;
import org.chromium.base.ContextUtils;
import org.chromium.base.PathUtils;

public class SamplesShellApplication extends Application {
    private static final String PRIVATE_DATA_DIRECTORY_SUFFIX = "samples_shell";

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);

        boolean isBrowserProcess = !ContextUtils.getProcessName().contains(":");
        ContextUtils.initApplicationContext(this);
        PathUtils.setPrivateDataDirectorySuffix(PRIVATE_DATA_DIRECTORY_SUFFIX);
        ApplicationStatus.initialize(this);
    }
}
