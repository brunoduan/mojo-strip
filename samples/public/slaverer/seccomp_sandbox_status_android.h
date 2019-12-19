// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_RENDERER_SECCOMP_SANDBOX_STATUS_ANDROID_H_
#define SAMPLES_PUBLIC_RENDERER_SECCOMP_SANDBOX_STATUS_ANDROID_H_

#include "samples/common/export.h"

#include "sandbox/linux/seccomp-bpf-helpers/seccomp_starter_android.h"

namespace samples {

// Gets the SeccompSandboxStatus of the current process.
SAMPLES_EXPORT sandbox::SeccompSandboxStatus GetSeccompSandboxStatus();

}  // namespace samples

#endif  // SAMPLES_PUBLIC_RENDERER_SECCOMP_SANDBOX_STATUS_ANDROID_H_
