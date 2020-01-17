// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTRE_ANDROID_SAMPLES_STARTUP_FLAGS_H_
#define SAMPLES_MASTRE_ANDROID_SAMPLES_STARTUP_FLAGS_H_

#include <string>

namespace samples {

// Force-appends flags to the command line turning on Android-specific
// features owned by Samples. This is called as soon as possible during
// initialization to make sure code sees the new flags.
void SetSamplesCommandLineFlags(bool single_process);

}  // namespace samples

#endif  // SAMPLES_MASTRE_ANDROID_SAMPLES_STARTUP_FLAGS_H_
