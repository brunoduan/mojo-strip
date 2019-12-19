// Copyright 2013 The Chromium Authors. All rights reserved.                                                                                                                                                
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/android/master_startup_controller.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "samples/master/android/samples_startup_flags.h"
#include "samples/master/master_main_loop.h"

#include "jni/MasterStartupControllerImpl_jni.h"

using base::android::JavaParamRef;

namespace samples {

void MasterStartupComplete(int result) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_MasterStartupControllerImpl_masterStartupComplete(env, result);
}

void ServiceManagerStartupComplete() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_MasterStartupControllerImpl_serviceManagerStartupComplete(env);
}

static void JNI_MasterStartupControllerImpl_SetCommandLineFlags(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    jboolean single_process) {
  SetSamplesCommandLineFlags(static_cast<bool>(single_process));
}

static void JNI_MasterStartupControllerImpl_FlushStartupTasks(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  MasterMainLoop::GetInstance()->SynchronouslyFlushStartupTasks();
}

}  // namespace samples
