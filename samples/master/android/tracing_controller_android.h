// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_ANDROID_TRACING_CONTROLLER_ANDROID_H_
#define SAMPLES_MASTER_ANDROID_TRACING_CONTROLLER_ANDROID_H_

#include <set>

#include "base/android/jni_weak_ref.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace samples {

// This class implements the native methods of TracingControllerAndroid.java
class TracingControllerAndroid {
 public:
  TracingControllerAndroid(JNIEnv* env, jobject obj);
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  bool StartTracing(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj,
                    const base::android::JavaParamRef<jstring>& categories,
                    const base::android::JavaParamRef<jstring>& trace_options);
  void StopTracing(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& obj,
                   const base::android::JavaParamRef<jstring>& jfilepath);
  bool GetKnownCategoryGroupsAsync(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  static void GenerateTracingFilePath(base::FilePath* file_path);

 private:
  ~TracingControllerAndroid();
  void OnTracingStopped();
  void OnKnownCategoriesReceived(
      const std::set<std::string>& categories_received);

  JavaObjectWeakGlobalRef weak_java_object_;
  base::WeakPtrFactory<TracingControllerAndroid> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TracingControllerAndroid);
};

}  // namespace samples

#endif  // SAMPLES_MASTER_ANDROID_TRACING_CONTROLLER_ANDROID_H_
