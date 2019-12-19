// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/common/sandboxed_process_launcher_delegate.h"

#include "build/build_config.h"
#include "services/service_manager/zygote/common/zygote_buildflags.h"

namespace samples {

#if BUILDFLAG(USE_ZYGOTE_HANDLE)
service_manager::ZygoteHandle SandboxedProcessLauncherDelegate::GetZygote() {
  return nullptr;
}
#endif  // BUILDFLAG(USE_ZYGOTE_HANDLE)

#if defined(OS_POSIX)
base::EnvironmentMap SandboxedProcessLauncherDelegate::GetEnvironment() {
  return base::EnvironmentMap();
}
#endif  // defined(OS_POSIX)

}  // namespace samples
