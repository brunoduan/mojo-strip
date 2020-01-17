// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.xpeng.samples_affiliate_shell_apk;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import com.xpeng.samples.common.ServiceManagerConnectionImpl;

import org.chromium.base.CommandLine;
import org.chromium.base.MemoryPressureListener;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.ProcessInitException;
import org.chromium.echo_master.mojom.EchoMaster;
import org.chromium.echo_master.mojom.EchoMasterConstants;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.Core;
import org.chromium.mojo.system.MessagePipeHandle;
import org.chromium.mojo.system.Pair;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.services.service_manager.Connector;

public class SamplesAffiliateShellActivity extends Activity {

    private static final String TAG = "SamplesAffiliateShellActivity";
    public static final String COMMAND_LINE_ARGS_KEY = "commandLineArgs";

    @Override
    protected void onCreate(final Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.samples_shell_activity);

        if (!CommandLine.isInitialized()) {
            ((SamplesAffiliateShellApplication) getApplication()).initCommandLine();
        }

        String[] commandLineParams = getCommandLineParamsFromIntent(getIntent());
        if (commandLineParams != null) {
            CommandLine.getInstance().appendSwitchesAndArguments(commandLineParams);
        }

        try {
            LibraryLoader.getInstance().ensureInitialized(LibraryProcessType.PROCESS_MASTER);
        } catch (ProcessInitException e) {
            Log.e(TAG, "ContentView initialization failed.", e);
            System.exit(-1);
            return;
        }

        ServiceManagerConnectionImpl.setConnectionReadyCallback(
                new ServiceManagerConnectionImpl.ConnectionReadyCallback() {
            @Override
            public void ready() {
                //connectEchoMasterService();
            }
        });

        //sendBootstrapBroadcast();
    }

    private void sendBootstrapBroadcast() {
        Intent intent = new Intent();
        intent.setAction("com.xpeng.samples_shell_apk.BOOTSTRAP_CHILD_PROCESS");
        intent.setPackage("com.xpeng.samples_shell_apk");
        intent.putExtra("pkg_name", getPackageName());

        sendBroadcast(intent);
    }

    private static final String TEST_STRING = "abcdefghijklmnopqrstuvwxyz";
    private class EchoStringResponseImpl implements EchoMaster.EchoStringResponse {
        @Override
        public void call(String str) {
            assert TEST_STRING.equals(str) == true;
        }
    }

    private void connectEchoMasterService() {
        MessagePipeHandle handle =
                ServiceManagerConnectionImpl.getConnectorMessagePipeHandle();
        Core core = CoreImpl.getInstance();
        Pair<EchoMaster.Proxy, InterfaceRequest<EchoMaster>> pair =
                EchoMaster.MANAGER.getInterfaceRequest(core);

        // Connect the Echo service via Connector.
        Connector connector = new Connector(handle);
        connector.bindInterface(
                EchoMasterConstants.SERVICE_NAME, EchoMaster.MANAGER.getName(), pair.second);

        // Fire the echoString() mojo call.
        EchoMaster.EchoStringResponse callback = new EchoStringResponseImpl();
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
