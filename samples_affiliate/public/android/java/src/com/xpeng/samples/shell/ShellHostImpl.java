package com.xpeng.samples.shell;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.mojo.system.impl.CoreImpl;
import org.chromium.services.service_manager.InterfaceProvider;


@JNINamespace("samples")
public class ShellHostImpl implements ShellHost {
  private long mNativeShellHostAndroid;
  private final InterfaceProvider mInterfaceProvider;

  private ShellHostImpl(long nativeShellHostAndroid, int nativeInterfaceProviderHandle) {
    mNativeShellHostAndroid = nativeShellHostAndroid;
    mInterfaceProvider = 
        new InterfaceProvider(CoreImpl.getInstance()
                                      .acquireNativeHandle(nativeInterfaceProviderHandle)
                                      .toMessagePipeHandle());
  }

  @CalledByNative
  private static ShellHostImpl create(long nativeShellHostAndroid,
                                      int nativeInterfaceProviderHandle) {
    return new ShellHostImpl(nativeShellHostAndroid, nativeInterfaceProviderHandle);
  }

  @CalledByNative
  private void clearNativePtr() {
    mNativeShellHostAndroid = 0;
  }

  @Override
  public InterfaceProvider getRemoteInterfaces() {
    return mInterfaceProvider;
  }

}
