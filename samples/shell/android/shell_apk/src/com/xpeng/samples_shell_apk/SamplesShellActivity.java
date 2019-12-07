// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.xpeng.samples_shell_apk;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.Toast;

import org.chromium.base.MemoryPressureListener;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;

public class SamplesShellActivity extends Activity {

    private static final String TAG = "SamplesShellActivity";

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

	setContentView(R.layout.samples_shell_activity);

	try {
            LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_BROWSER);
        } catch (ProcessInitException e) {
            Log.e(TAG, "ContentView initialization failed.", e);
            System.exit(-1);
            return;
        }
    }

    @Override
    protected void onStart() {
        super.onStart();

    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void startActivity(Intent i) {
        super.startActivity(i);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }
}
