package com.xpeng.samples_shell_apk;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;

import com.xpeng.samples.master.AffiliateProcessHostImpl;

import org.chromium.base.Log;

public class SamplesBootstrapReceiver extends BroadcastReceiver {
    private static final String TAG = "Samples";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null) {
            return;
        }
        String pkg_name = intent.getStringExtra("pkg_name");
        if (TextUtils.isEmpty(pkg_name)) {
            return;
        }

        AffiliateProcessHostImpl.createAffiliateProcess(pkg_name);
    }
}
