// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/master_process_sub_thread.h"

#include "base/compiler_specific.h"
#include "base/debug/alias.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/memory_dump_manager.h"
#include "samples/master/master_child_process_host_impl.h"
#include "samples/master/master_thread_impl.h"
#include "samples/master/notification_service_impl.h"
#include "samples/master/utility_process_host.h"
#include "samples/common/child_process_host_impl.h"
#include "samples/public/master/master_child_process_host_iterator.h"
#include "samples/public/master/master_thread_delegate.h"
#include "samples/public/common/process_type.h"

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#endif

namespace samples {

namespace {
MasterThreadDelegate* g_io_thread_delegate = nullptr;
}  // namespace

// static
void MasterThread::SetIOThreadDelegate(MasterThreadDelegate* delegate) {
  // |delegate| can only be set/unset while MasterThread::IO isn't up.
  DCHECK(!MasterThread::IsThreadInitialized(MasterThread::IO));
  // and it cannot be set twice.
  DCHECK(!g_io_thread_delegate || !delegate);

  g_io_thread_delegate = delegate;
}

MasterProcessSubThread::MasterProcessSubThread(MasterThread::ID identifier)
    : base::Thread(MasterThreadImpl::GetThreadName(identifier)),
      identifier_(identifier) {
  // Not bound to creation thread.
  DETACH_FROM_THREAD(master_thread_checker_);
}

MasterProcessSubThread::~MasterProcessSubThread() {
  Stop();
}

void MasterProcessSubThread::RegisterAsMasterThread() {
  DCHECK(IsRunning());

  DCHECK(!master_thread_);
  master_thread_.reset(new MasterThreadImpl(identifier_, task_runner()));

  // Unretained(this) is safe as |this| outlives its underlying thread.
  task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &MasterProcessSubThread::CompleteInitializationOnMasterThread,
          Unretained(this)));
}

void MasterProcessSubThread::AllowBlockingForTesting() {
  DCHECK(!IsRunning());
  is_blocking_allowed_for_testing_ = true;
}

// static
std::unique_ptr<MasterProcessSubThread>
MasterProcessSubThread::CreateIOThread() {
  TRACE_EVENT0("startup", "MasterProcessSubThread::CreateIOThread");
  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
#if defined(OS_ANDROID) || defined(OS_CHROMEOS)
  // Up the priority of the |io_thread_| as some of its IPCs relate to
  // display tasks.
  options.priority = base::ThreadPriority::DISPLAY;
#endif
  std::unique_ptr<MasterProcessSubThread> io_thread(
      new MasterProcessSubThread(MasterThread::IO));
  if (!io_thread->StartWithOptions(options))
    LOG(FATAL) << "Failed to start MasterThread:IO";
  return io_thread;
}

void MasterProcessSubThread::Init() {
  DCHECK_CALLED_ON_VALID_THREAD(master_thread_checker_);

  if (!is_blocking_allowed_for_testing_) {
    base::DisallowUnresponsiveTasks();
  }
}

void MasterProcessSubThread::Run(base::RunLoop* run_loop) {
  DCHECK_CALLED_ON_VALID_THREAD(master_thread_checker_);

#if defined(OS_ANDROID)
  // Not to reset thread name to "Thread-???" by VM, attach VM with thread name.
  // Though it may create unnecessary VM thread objects, keeping thread name
  // gives more benefit in debugging in the platform.
  if (!thread_name().empty()) {
    base::android::AttachCurrentThreadWithName(thread_name());
  }
#endif

  switch (identifier_) {
    case MasterThread::UI:
      // The main thread is usually promoted as the UI thread and doesn't go
      // through Run() but some tests do run a separate UI thread.
      UIThreadRun(run_loop);
      break;
    case MasterThread::IO:
      IOThreadRun(run_loop);
      return;
    case MasterThread::ID_COUNT:
      NOTREACHED();
      break;
  }
}

void MasterProcessSubThread::CleanUp() {
  DCHECK_CALLED_ON_VALID_THREAD(master_thread_checker_);

  // Run extra cleanup if this thread represents MasterThread::IO.
  if (MasterThread::CurrentlyOn(MasterThread::IO))
    IOThreadCleanUp();

  if (identifier_ == MasterThread::IO && g_io_thread_delegate)
    g_io_thread_delegate->CleanUp();

  notification_service_.reset();

}

void MasterProcessSubThread::CompleteInitializationOnMasterThread() {
  DCHECK_CALLED_ON_VALID_THREAD(master_thread_checker_);

  notification_service_ = std::make_unique<NotificationServiceImpl>();

  if (identifier_ == MasterThread::IO && g_io_thread_delegate) {
    // Allow blocking calls while initializing the IO thread.
    base::ScopedAllowBlocking allow_blocking_for_init;
    g_io_thread_delegate->Init();
  }
}

// Mark following two functions as NOINLINE so the compiler doesn't merge
// them together.

NOINLINE void MasterProcessSubThread::UIThreadRun(base::RunLoop* run_loop) {
  const int line_number = __LINE__;
  Thread::Run(run_loop);
  base::debug::Alias(&line_number);
}

NOINLINE void MasterProcessSubThread::IOThreadRun(base::RunLoop* run_loop) {
  const int line_number = __LINE__;
  Thread::Run(run_loop);
  base::debug::Alias(&line_number);
}

void MasterProcessSubThread::IOThreadCleanUp() {
  DCHECK_CALLED_ON_VALID_THREAD(master_thread_checker_);

  // If any child processes are still running, terminate them and
  // and delete the MasterChildProcessHost instances to release whatever
  // IO thread only resources they are referencing.
  MasterChildProcessHostImpl::TerminateAll();
}

}  // namespace samples
