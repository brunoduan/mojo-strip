// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_APP_SAMPLES_JNI_ONLOAD_H_
#define SAMPLES_PUBLIC_APP_SAMPLES_JNI_ONLOAD_H_

#include <jni.h>

#include "base/android/base_jni_onload.h"
#include "samples/common/export.h"

namespace samples {
namespace android {

// Returns true if initialization succeeded.
SAMPLES_EXPORT bool OnJNIOnLoadInit();

}  // namespace android
}  // namespace samples

#endif  // SAMPLES_PUBLIC_APP_SAMPLES_JNI_ONLOAD_H_
