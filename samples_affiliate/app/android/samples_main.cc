// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/lazy_instance.h"
#include "base/trace_event/trace_event.h"
#include "samples/app/samples_service_manager_main_delegate.h"
#include "samples/public/app/samples_main.h"
#include "samples/public/app/samples_main_delegate.h"
#include "jni/SamplesMain_jni.h"
#include "services/service_manager/embedder/main.h"

using base::LazyInstance;
using base::android::JavaParamRef;

namespace samples {

namespace {

LazyInstance<std::unique_ptr<service_manager::MainDelegate>>::DestructorAtExit
    g_service_manager_main_delegate = LAZY_INSTANCE_INITIALIZER;

LazyInstance<std::unique_ptr<SamplesMainDelegate>>::DestructorAtExit
    g_samples_main_delegate = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// TODO(qinmin/hanxi): split this function into 2 separate methods: One to
// start the ServiceManager and one to start the remainder of the master
// process. The first method should always be called upon master start, and
// the second method can be deferred. See http://crbug.com/854209.
static jint JNI_SamplesMain_Start(JNIEnv* env,
                                  const JavaParamRef<jclass>& clazz,
                                  jboolean start_service_manager_only) {
  TRACE_EVENT0("startup", "samples::Start");

  DCHECK(!g_service_manager_main_delegate.Get() || !start_service_manager_only);

  if (!g_service_manager_main_delegate.Get()) {
    g_service_manager_main_delegate.Get() =
        std::make_unique<SamplesServiceManagerMainDelegate>(
            SamplesMainParams(g_samples_main_delegate.Get().get()));
  }

  static_cast<SamplesServiceManagerMainDelegate*>(
      g_service_manager_main_delegate.Get().get())
      ->SetStartServiceManagerOnly(start_service_manager_only);

  service_manager::MainParams main_params(
      g_service_manager_main_delegate.Get().get());

  return service_manager::Main(main_params);
}

static void JNI_SamplesMain_StartAffiliateThread(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
}

void SetSamplesMainDelegate(SamplesMainDelegate* delegate) {
  DCHECK(!g_samples_main_delegate.Get().get());
  g_samples_main_delegate.Get().reset(delegate);
}

}  // namespace samples
