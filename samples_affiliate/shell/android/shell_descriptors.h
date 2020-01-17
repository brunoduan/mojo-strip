// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SHELL_ANDROID_SHELL_DESCRIPTORS_H_
#define SAMPLES_SHELL_ANDROID_SHELL_DESCRIPTORS_H_

#include "samples/public/common/samples_descriptors.h"

// This is a list of global descriptor keys to be used with the
// base::GlobalDescriptors object (see base/posix/global_descriptors.h)
enum {
  kShellPakDescriptor = kSamplesIPCDescriptorMax + 1,
  kAndroidMinidumpDescriptor,
};

#endif  // SAMPLES_SHELL_ANDROID_SHELL_DESCRIPTORS_H_
