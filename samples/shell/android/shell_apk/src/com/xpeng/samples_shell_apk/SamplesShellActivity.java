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

import com.xpeng.samples.common.ServiceManagerConnectionImpl;
import com.xpeng.samples.shell.ShellLauncher;
import com.xpeng.samples_public.master.MasterStartupController;

import org.chromium.base.CommandLine;
import org.chromium.base.MemoryPressureListener;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.echo.mojom.Echo;
import org.chromium.echo.mojom.EchoConstants;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.Pair;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.services.service_manager.Connector;

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

        connectEchoService();
    }

    private void initializationFailed() {
        Log.e(TAG, "ContentView initialization failed.");
        Toast.makeText(SamplesShellActivity.this,
                R.string.master_process_initialization_failed,
                Toast.LENGTH_SHORT).show();
        finish();
    }

    private static final String TEST_STRING = "abcdefghijklmnopqrstuvwxyz";
    private class EchoStringResponseImpl implements Echo.EchoStringResponse {
        @Override
        public void call(String str) {
            assert TEST_STRING.equals(str) == true;
        }
    }

    private void connectEchoService() {
        MessagePipeHandle handle =
                ServiceManagerConnectionImpl.getConnectorMessagePipeHandle();
        Core core = CoreImpl.getInstance();
        Pair<Echo.Proxy, InterfaceRequest<Echo>> pair =
                Echo.MANAGER.getInterfaceRequest(core);

        // Connect the Echo service via Connector.
        Connector connector = new Connector(handle);
        connector.bindInterface(
                EchoConstants.SERVICE_NAME, Echo.MANAGER.getName(), pair.second);

        // Fire the echoString() mojo call.
        Echo.EchoStringResponse callback = new EchoStringResponseImpl();
        pair.first.echoString(TEST_STRING, callback);
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
