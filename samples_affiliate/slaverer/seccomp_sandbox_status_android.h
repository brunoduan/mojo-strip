// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SLAVERER_SECCOMP_SANDBOX_STATUS_ANDROID_H_
#define SAMPLES_SLAVERER_SECCOMP_SANDBOX_STATUS_ANDROID_H_

#include "sandbox/linux/seccomp-bpf-helpers/seccomp_starter_android.h"

namespace samples {

void SetSeccompSandboxStatus(sandbox::SeccompSandboxStatus status);

}  // namespace samples

#endif  // SAMPLES_SLAVERER_SECCOMP_SANDBOX_STATUS_ANDROID_H_
