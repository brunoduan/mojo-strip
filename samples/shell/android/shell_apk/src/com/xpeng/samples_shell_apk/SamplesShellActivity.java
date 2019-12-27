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

import com.xpeng.samples.shell.ShellLauncher;
import com.xpeng.samples_public.master.MasterStartupController;

import org.chromium.base.CommandLine;
import org.chromium.base.MemoryPressureListener;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;

public class SamplesShellActivity extends Activity {

    private static final String TAG = "SamplesShellActivity";
    public static final String COMMAND_LINE_ARGS_KEY = "commandLineArgs";

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

      if (!CommandLine.isInitialized()) {
        ((SamplesShellApplication) getApplication()).initCommandLine();
        String[] commandLineParams = getCommandLineParamsFromIntent(getIntent());
        if (commandLineParams != null) {
          CommandLine.getInstance().appendSwitchesAndArguments(commandLineParams);
        }
        String [] sp = new String[] {"--single-process"};
        CommandLine.getInstance().appendSwitchesAndArguments(sp);
      }

	    setContentView(R.layout.samples_shell_activity);

	    try {
            LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_MASTER);
      } catch (ProcessInitException e) {
        Log.e(TAG, "ContentView initialization failed.", e);
        System.exit(-1);
        return;
      }

      try {
          MasterStartupController.get(LibraryProcessType.PROCESS_MASTER)
                  .startMasterProcessesAsync(
                          false,
                          new MasterStartupController.StartupCallback() {
                              @Override
                              public void onSuccess() {
                                  finishInitialization(savedInstanceState);
                              }

                              @Override
                              public void onFailure() {
                                    initializationFailed();
                                }
                          });
      } catch (ProcessInitException e) {
          Log.e(TAG, "Unable to load native library.", e);
          System.exit(-1);
      }
    }

    private void finishInitialization(Bundle savedInstanceState) {
        ShellLauncher.getInstance().launchShell();
    }

    private void initializationFailed() {
        Log.e(TAG, "ContentView initialization failed.");
        Toast.makeText(SamplesShellActivity.this,
                R.string.master_process_initialization_failed,
                Toast.LENGTH_SHORT).show();
        finish();
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

    @Override
    protected void onNewIntent(Intent intent) {
      if (getCommandLineParamsFromIntent(intent) != null) {
        Log.i(TAG, "Ignoring command line params: can only be set when creating the activity.");
      }

      if (MemoryPressureListener.handleDebugIntent(this, intent.getAction())) {
        return;
      }
    }

    private static String[] getCommandLineParamsFromIntent(Intent intent) {
        return intent != null ? intent.getStringArrayExtra(COMMAND_LINE_ARGS_KEY) : null;
    }
}
