// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <android/native_window_jni.h>
#include <cpu-features.h>

#include "base/android/jni_array.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "base/android/memory_pressure_listener_android.h"
#include "base/android/unguessable_token_android.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/unguessable_token.h"
#include "samples/child/child_thread_impl.h"
#include "samples/public/common/samples_descriptors.h"
#include "samples/public/common/samples_switches.h"
#include "jni/SamplesChildProcessServiceDelegate_jni.h"
#include "services/service_manager/embedder/shared_file_util.h"
#include "services/service_manager/embedder/switches.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;

namespace samples {

namespace {

// Chrome actually uses the renderer code path for all of its child
// processes such as renderers, plugins, etc.
void JNI_SamplesChildProcessServiceDelegate_InternalInitChildProcess(
    JNIEnv* env,
    const JavaParamRef<jobject>& service_impl,
    jint cpu_count,
    jlong cpu_features) {
  // Set the CPU properties.
  android_setCpu(cpu_count, cpu_features);
}

}  // namespace

void JNI_SamplesChildProcessServiceDelegate_InitChildProcess(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint cpu_count,
    jlong cpu_features) {
  JNI_SamplesChildProcessServiceDelegate_InternalInitChildProcess(
      env, obj, cpu_count, cpu_features);
}

void JNI_SamplesChildProcessServiceDelegate_ShutdownMainThread(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  ChildThreadImpl::ShutdownThread();
}

void JNI_SamplesChildProcessServiceDelegate_RetrieveFileDescriptorsIdsToKeys(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  std::map<int, std::string> ids_to_keys;
  std::string file_switch_value =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          service_manager::switches::kSharedFiles);

  std::vector<int> ids;
  std::vector<std::string> keys;
  if (!file_switch_value.empty()) {
    base::Optional<std::map<int, std::string>> ids_to_keys_from_command_line =
        service_manager::ParseSharedFileSwitchValue(file_switch_value);
    if (ids_to_keys_from_command_line) {
      for (auto iter : *ids_to_keys_from_command_line) {
        ids.push_back(iter.first);
        keys.push_back(iter.second);
      }
    }
  }

  Java_SamplesChildProcessServiceDelegate_setFileDescriptorsIdsToKeys(
      env, obj, base::android::ToJavaIntArray(env, ids),
      base::android::ToJavaArrayOfStrings(env, keys));
}

}  // namespace samples
