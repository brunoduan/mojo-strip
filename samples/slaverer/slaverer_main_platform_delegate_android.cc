// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/slaverer/slaverer_main_platform_delegate.h"

#include "base/android/build_info.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "samples/slaverer/seccomp_sandbox_status_android.h"
#include "sandbox/linux/seccomp-bpf-helpers/seccomp_starter_android.h"
#include "sandbox/sandbox_buildflags.h"

#if BUILDFLAG(USE_SECCOMP_BPF)
#include "sandbox/linux/seccomp-bpf-helpers/baseline_policy_android.h"
#endif

namespace samples {

SlavererMainPlatformDelegate::SlavererMainPlatformDelegate(
    const MainFunctionParams& parameters) {}

SlavererMainPlatformDelegate::~SlavererMainPlatformDelegate() {
}

void SlavererMainPlatformDelegate::PlatformInitialize() {
}

void SlavererMainPlatformDelegate::PlatformUninitialize() {
}

bool SlavererMainPlatformDelegate::EnableSandbox() {
  TRACE_EVENT0("startup", "SlavererMainPlatformDelegate::EnableSandbox");
  auto* info = base::android::BuildInfo::GetInstance();
  sandbox::SeccompStarterAndroid starter(info->sdk_int(), info->device());
  // The policy compiler is only available if USE_SECCOMP_BPF is enabled.
#if BUILDFLAG(USE_SECCOMP_BPF)
  starter.set_policy(std::make_unique<sandbox::BaselinePolicyAndroid>());
#endif
  starter.StartSandbox();

  SetSeccompSandboxStatus(starter.status());
  UMA_HISTOGRAM_ENUMERATION("Android.SeccompStatus.SlavererSandbox",
                            starter.status(),
                            sandbox::SeccompSandboxStatus::STATUS_MAX);

  return true;
}

}  // namespace samples
