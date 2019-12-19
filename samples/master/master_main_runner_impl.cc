// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/master_main_runner_impl.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug/debugger.h"
#include "base/debug/leak_annotations.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/atomic_flag.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "samples/master/master_main_loop.h"
#include "samples/common/samples_switches_internal.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/main_function_params.h"

namespace samples {
namespace {

base::LazyInstance<base::AtomicFlag>::Leaky g_exited_main_message_loop;

}  // namespace

// static
MasterMainRunnerImpl* MasterMainRunnerImpl::Create() {
  return new MasterMainRunnerImpl();
}

MasterMainRunnerImpl::MasterMainRunnerImpl()
    : initialization_started_(false),
      is_shutdown_(false),
      scoped_execution_fence_(
          std::make_unique<base::TaskScheduler::ScopedExecutionFence>()) {}

MasterMainRunnerImpl::~MasterMainRunnerImpl() {
  if (initialization_started_ && !is_shutdown_)
    Shutdown();
}

int MasterMainRunnerImpl::Initialize(const MainFunctionParams& parameters) {
  // On Android we normally initialize the master in a series of UI thread
  // tasks. While this is happening a second request can come from the OS or
  // another application to start the master. If this happens then we must
  // not run these parts of initialization twice.
  if (!initialization_started_) {
    initialization_started_ = true;

    if (parameters.command_line.HasSwitch(switches::kWaitForDebugger))
      base::debug::WaitForDebugger(60, true);

    if (parameters.command_line.HasSwitch(switches::kMasterStartupDialog))
      WaitForDebugger("Master");

    main_loop_.reset(
        new MasterMainLoop(parameters, std::move(scoped_execution_fence_)));

    main_loop_->Init();

    if (parameters.created_main_parts_closure) {
      parameters.created_main_parts_closure->Run(main_loop_->parts());
      delete parameters.created_main_parts_closure;
    }

    const int early_init_error_code = main_loop_->EarlyInitialization();
    if (early_init_error_code > 0)
      return early_init_error_code;

    // Must happen before we try to use a message loop or display any UI.
    if (!main_loop_->InitializeToolkit())
      return 1;

    main_loop_->PreMainMessageLoopStart();
    main_loop_->MainMessageLoopStart();
    main_loop_->PostMainMessageLoopStart();

  }
  main_loop_->CreateStartupTasks();
  int result_code = main_loop_->GetResultCode();
  if (result_code > 0)
    return result_code;

  // Return -1 to indicate no early termination.
  return -1;
}

#if defined(OS_ANDROID)
void MasterMainRunnerImpl::SynchronouslyFlushStartupTasks() {
  main_loop_->SynchronouslyFlushStartupTasks();
}
#endif

int MasterMainRunnerImpl::Run() {
  DCHECK(initialization_started_);
  DCHECK(!is_shutdown_);
  main_loop_->RunMainMessageLoopParts();
  return main_loop_->GetResultCode();
}

void MasterMainRunnerImpl::Shutdown() {
  DCHECK(initialization_started_);
  DCHECK(!is_shutdown_);

#ifdef LEAK_SANITIZER
  // Invoke leak detection now, to avoid dealing with shutdown-only leaks.
  // Normally this will have already happened in
  // BroserProcessImpl::ReleaseModule(), so this call has no effect. This is
  // only for processes which do not instantiate a MasterProcess.
  // If leaks are found, the process will exit here.
  __lsan_do_leak_check();
#endif

  main_loop_->PreShutdown();

  {
    g_exited_main_message_loop.Get().Set();

    main_loop_->ShutdownThreadsAndCleanUp();

#if defined(OS_ANDROID)
    // Forcefully terminates the RunLoop inside MessagePumpForUI, ensuring
    // proper shutdown for samples_mastertests. Shutdown() is not used by
    // the actual master.
    if (base::RunLoop::IsRunningOnCurrentThread())
      base::RunLoop::QuitCurrentDeprecated();
#endif
    main_loop_.reset(nullptr);

    is_shutdown_ = true;
  }
}

// static
MasterMainRunner* MasterMainRunner::Create() {
  return MasterMainRunnerImpl::Create();
}

// static
bool MasterMainRunner::ExitedMainMessageLoop() {
  return g_exited_main_message_loop.IsCreated() &&
         g_exited_main_message_loop.Get().IsSet();
}

}  // namespace samples
