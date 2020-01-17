// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_APP_ANDROID_LIBRARY_LOADER_HOOKS_H_
#define SAMPLES_APP_ANDROID_LIBRARY_LOADER_HOOKS_H_

#include <jni.h>


namespace samples {

// Do the intialization of samples needed immediately after the native library
// has loaded.
// This is designed to be used as a hook function to be passed to
// base::android::SetLibraryLoadedHook
bool LibraryLoaded(JNIEnv* env, jclass clazz);

}  // namespace samples

#endif  // SAMPLES_APP_ANDROID_LIBRARY_LOADER_HOOKS_H_
