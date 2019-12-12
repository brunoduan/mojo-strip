package com.xpeng.samples.shell;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

@JNINamespace("samples::shell")
public class ShellLauncher {
  private static final ShellLauncher sInstance = new ShellLauncher();

  public static ShellLauncher getInstance() {
    return sInstance;
  }
  
  private ShellLauncher() {
    nativeInit(this);
  }

  public void launchShell() {
    nativeLaunchShell();
  }

  @CalledByNative
  public void destroy() {
  }

  private static native void nativeInit(Object shellLauncherInstance);
  private static native void nativeLaunchShell();
}
