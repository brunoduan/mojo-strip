// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/slaverer/in_process_slaverer_thread.h"

#include "build/build_config.h"
#include "samples/slaverer/slaver_process.h"
#include "samples/slaverer/slaver_process_impl.h"
#include "samples/slaverer/slaver_thread_impl.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#endif

namespace samples {

#if defined(OS_ANDROID)
extern bool g_master_main_loop_shutting_down;
#endif

InProcessSlavererThread::InProcessSlavererThread(
    const InProcessChildThreadParams& params)
    : Thread("Samples_InProcSlavererThread"), params_(params) {
}

InProcessSlavererThread::~InProcessSlavererThread() {
#if defined(OS_ANDROID)
  // Don't allow the slaver thread to be shut down in single process mode on
  // Android unless the master is shutting down.
  // Temporary CHECK() to debug http://crbug.com/514141
  CHECK(g_master_main_loop_shutting_down);
#endif

  Stop();
}

void InProcessSlavererThread::Init() {
  // Call AttachCurrentThreadWithName, before any other AttachCurrentThread()
  // calls. The latter causes Java VM to assign Thread-??? to the thread name.
  // Please note calls to AttachCurrentThreadWithName after AttachCurrentThread
  // will not change the thread name kept in Java VM.
#if defined(OS_ANDROID)
  base::android::AttachCurrentThreadWithName(thread_name());
  // Make sure we aren't somehow reinitialising the inprocess slaverer thread on
  // Android. Temporary CHECK() to debug http://crbug.com/514141
  CHECK(!slaver_process_);
#endif

  slaver_process_ = SlaverProcessImpl::Create();
  // SlaverThreadImpl doesn't currently support a proper shutdown sequence
  // and it's okay when we're running in multi-process mode because slaverers
  // get killed by the OS. In-process mode is used for test and debug only.
  new SlaverThreadImpl(params_);
}

void InProcessSlavererThread::CleanUp() {
#if defined(OS_ANDROID)
  // Don't allow the slaver thread to be shut down in single process mode on
  // Android unless the master is shutting down.
  // Temporary CHECK() to debug http://crbug.com/514141
  CHECK(g_master_main_loop_shutting_down);
#endif

  slaver_process_.reset();

  // It's a little lame to manually set this flag.  But the single process
  // SlavererThread will receive the WM_QUIT.  We don't need to assert on
  // this thread, so just force the flag manually.
  // If we want to avoid this, we could create the InProcSlavererThread
  // directly with _beginthreadex() rather than using the Thread class.
  // We used to set this flag in the Init function above. However there
  // other threads like WebThread which are created by this thread
  // which resets this flag. Please see Thread::StartWithOptions. Setting
  // this flag to true in Cleanup works around these problems.
  SetThreadWasQuitProperly(true);
}

base::Thread* CreateInProcessSlavererThread(
    const InProcessChildThreadParams& params) {
  return new InProcessSlavererThread(params);
}

}  // namespace samples
