package com.xpeng.samples.shell;

import org.chromium.services.service_manager.InterfaceProvider;

interface ShellHost {
  InterfaceProvider getRemoteInterfaces();
}
