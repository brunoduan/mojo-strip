package com.xpeng.samples.master;

import org.chromium.base.annotations.JNINamespace;

@JNINamespace("samples")
public class AffiliateProcessHostImpl {
    public static void createAffiliateProcess(String pkgName) {
        nativeCreateAffiliateProcess(pkgName);
    }

    private static native void nativeCreateAffiliateProcess(String pkgName);
}
