// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/app/samples_jni_onload.h"

#include <vector>

#include "base/android/base_jni_onload.h"
#include "base/android/jni_android.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "base/bind.h"
#include "samples/app/android/library_loader_hooks.h"
#include "samples/public/app/samples_main.h"

namespace samples {
namespace android {

bool OnJNIOnLoadInit() {
  if (!base::android::OnJNIOnLoadInit())
    return false;

  base::android::SetLibraryLoadedHook(&samples::LibraryLoaded);
  return true;
}

}  // namespace android
}  // namespace samples
