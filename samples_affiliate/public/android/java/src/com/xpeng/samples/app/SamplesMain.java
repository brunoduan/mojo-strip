package com.xpeng.samples.app;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.MainDex;

@JNINamespace("samples")
@MainDex
public class SamplesMain {
  /**
   * Start the SamplesMainRunner in native side.
   *
   * @param startServiceManagerOnly Whether to start only the ServiceManager.
   **/
  public static int start(boolean startServiceManagerOnly) {
      return nativeStart(startServiceManagerOnly);
  }

  public static void startAffiliateThread() {
      nativeStartAffiliateThread();
  }

  private static native int nativeStart(boolean startServiceManagerOnly);
  private static native void nativeStartAffiliateThread();
}
