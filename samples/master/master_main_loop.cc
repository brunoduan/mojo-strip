// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/master_main_loop.h"

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/alias.h"
#include "base/deferred_sequenced_task_runner.h"
#include "base/feature_list.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/path_service.h"
#include "base/pending_task.h"
#include "base/process/process_metrics.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/synchronization/waitable_event.h"
#include "base/system/system_monitor.h"
#include "base/task/post_task.h"
#include "base/task/task_scheduler/initialization_util.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/timer/hi_res_timer_manager.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "components/tracing/common/trace_startup_config.h"
#include "components/tracing/common/trace_to_console.h"
#include "components/tracing/common/tracing_switches.h"
#include "samples/master/master_thread_impl.h"
#include "samples/master/child_process_security_policy_impl.h"
#include "samples/master/service_manager/service_manager_context.h"
#include "samples/master/slaverer_host/slaver_process_host_impl.h"
#include "samples/master/startup_data_impl.h"
#include "samples/master/startup_task_runner.h"
#include "samples/master/tracing/background_tracing_manager_impl.h"
#include "samples/master/tracing/tracing_controller_impl.h"
#include "samples/common/samples_switches_internal.h"
#include "samples/common/service_manager/service_manager_connection_impl.h"
#include "samples/common/task_scheduler.h"
#include "samples/public/master/master_main_parts.h"
#include "samples/public/master/master_task_traits.h"
#include "samples/public/master/master_thread.h"
#include "samples/public/master/samples_master_client.h"
#include "samples/public/master/slaver_process_host.h"
#include "samples/public/master/site_isolation_policy.h"
#include "samples/public/common/samples_client.h"
#include "samples/public/common/samples_features.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/main_function_params.h"
#include "samples/public/common/result_codes.h"
#include "samples/public/common/service_names.mojom.h"
#include "mojo/core/embedder/embedder.h"
#include "mojo/core/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/sync_call_restrictions.h"
#include "services/service_manager/runner/common/client_util.h"
#include "services/service_manager/zygote/common/zygote_buildflags.h"

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#include "samples/master/android/master_startup_controller.h"
#include "samples/master/android/launcher_thread.h"
#include "samples/master/android/tracing_controller_android.h"
#endif

// One of the linux specific headers defines this as a macro.
#ifdef DestroyAll
#undef DestroyAll
#endif

namespace samples {
namespace {

// Tell compiler not to inline this function so it's possible to tell what
// thread was unresponsive by inspecting the callstack.
NOINLINE void ResetThread_IO(
    std::unique_ptr<MasterProcessSubThread> io_thread) {
  const int line_number = __LINE__;
  io_thread.reset();
  base::debug::Alias(&line_number);
}

enum WorkerPoolType : size_t {
  BACKGROUND = 0,
  BACKGROUND_BLOCKING,
  FOREGROUND,
  FOREGROUND_BLOCKING,
  WORKER_POOL_COUNT  // Always last.
};

#if defined(ENABLE_IPC_FUZZER)
bool GetBuildDirectory(base::FilePath* result) {
  if (!base::PathService::Get(base::DIR_EXE, result))
    return false;

  return true;
}

void SetFileUrlPathAliasForIpcFuzzer() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kFileUrlPathAlias))
    return;

  base::FilePath build_directory;
  if (!GetBuildDirectory(&build_directory)) {
    LOG(ERROR) << "Failed to get build directory for /gen path alias.";
    return;
  }

  const base::CommandLine::StringType alias_switch =
      FILE_PATH_LITERAL("/gen=") + build_directory.AppendASCII("gen").value();
  base::CommandLine::ForCurrentProcess()->AppendSwitchNative(
      switches::kFileUrlPathAlias, alias_switch);
}
#endif

void OnStoppedStartupTracing(const base::FilePath& trace_file) {
  VLOG(0) << "Completed startup tracing to " << trace_file.value();
}

}  // namespace

// The currently-running MasterMainLoop.  There can be one or zero.
MasterMainLoop* g_current_master_main_loop = nullptr;

#if defined(OS_ANDROID)
bool g_master_main_loop_shutting_down = false;
#endif

// MasterMainLoop construction / destruction =============================

MasterMainLoop* MasterMainLoop::GetInstance() {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
  return g_current_master_main_loop;
}

MasterMainLoop::MasterMainLoop(
    const MainFunctionParams& parameters,
    std::unique_ptr<base::TaskScheduler::ScopedExecutionFence>
        scoped_execution_fence)
    : parameters_(parameters),
      parsed_command_line_(parameters.command_line),
      result_code_(service_manager::RESULT_CODE_NORMAL_EXIT),
      created_threads_(false),
      scoped_execution_fence_(std::move(scoped_execution_fence)) {
  DCHECK(!g_current_master_main_loop);
  DCHECK(scoped_execution_fence_)
      << "TaskScheduler must be halted before kicking off samples.";
  g_current_master_main_loop = this;

  if (GetSamplesClient()->master()->ShouldCreateTaskScheduler()) {
    DCHECK(base::TaskScheduler::GetInstance());
  }
}

MasterMainLoop::~MasterMainLoop() {
  DCHECK_EQ(this, g_current_master_main_loop);
  g_current_master_main_loop = nullptr;
}

void MasterMainLoop::Init() {

  // |startup_data| is optional. If set, the thread owned by the data
  // will be registered as MasterThread::IO in CreateThreads() instead of
  // creating a brand new thread.
  if (parameters_.startup_data) {
    StartupDataImpl* startup_data =
        static_cast<StartupDataImpl*>(parameters_.startup_data);
    // This is always invoked before |io_thread_| is initialized (i.e. never
    // resets it).
    io_thread_ = std::move(startup_data->thread);
  }

  parts_.reset(
      GetSamplesClient()->master()->CreateMasterMainParts(parameters_));
}

// MasterMainLoop stages ==================================================

int MasterMainLoop::EarlyInitialization() {

#if BUILDFLAG(USE_ZYGOTE_HANDLE)
  // The initialization of the sandbox host ends up with forking the Zygote
  // process and requires no thread been forked. The initialization has happened
  // by now since a thread to start the ServiceManager has been created
  // before the master main loop starts.
  DCHECK(SandboxHostLinux::GetInstance()->IsInitialized());
#endif

  if (parts_) {
    const int pre_early_init_error_code = parts_->PreEarlyInitialization();
    if (pre_early_init_error_code != service_manager::RESULT_CODE_NORMAL_EXIT)
      return pre_early_init_error_code;
  }

#if defined(OS_ANDROID)
  // Up the priority of the UI thread unless it was already high (since recent
  // versions of Android (O+) do this automatically).
  if (base::PlatformThread::GetCurrentThreadPriority() <
      base::ThreadPriority::DISPLAY) {
    base::PlatformThread::SetCurrentThreadPriority(
        base::ThreadPriority::DISPLAY);
  }
#endif  // defined(OS_ANDROID)

#if defined(OS_ANDROID)
  // We use quite a few file descriptors for our IPC as well as disk the disk
  // cache,and the default limit on the Mac is low (256), so bump it up.

  // Same for Linux. The default various per distro, but it is 1024 on Fedora.
  // Low soft limits combined with liberal use of file descriptors means power
  // users can easily hit this limit with many open tabs. Bump up the limit to
  // an arbitrarily high number. See https://crbug.com/539567
  base::IncreaseFdLimitTo(8192);
#endif  // defined(OS_ANDROID)

  if (parsed_command_line_.HasSwitch(switches::kSlavererProcessLimit)) {
    std::string limit_string = parsed_command_line_.GetSwitchValueASCII(
        switches::kSlavererProcessLimit);
    size_t process_limit;
    if (base::StringToSizeT(limit_string, &process_limit)) {
      SlaverProcessHost::SetMaxSlavererProcessCount(process_limit);
    }
  }

  if (parts_)
    parts_->PostEarlyInitialization();

  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void MasterMainLoop::PreMainMessageLoopStart() {
  if (parts_) {
    parts_->PreMainMessageLoopStart();
  }
}

void MasterMainLoop::MainMessageLoopStart() {
  // DO NOT add more code here. Use PreMainMessageLoopStart() above or
  // PostMainMessageLoopStart() below.

  if (!base::MessageLoopCurrentForUI::IsSet())
    main_message_loop_ = std::make_unique<base::MessageLoopForUI>();

  InitializeMainThread();
}

void MasterMainLoop::PostMainMessageLoopStart() {
  {
    hi_res_timer_manager_.reset(new base::HighResolutionTimerManager);
  }

  {
    base::SetRecordActionTaskRunner(
        base::CreateSingleThreadTaskRunnerWithTraits({MasterThread::UI}));
  }

  if (parts_)
    parts_->PostMainMessageLoopStart();

}

int MasterMainLoop::PreCreateThreads() {
  if (parts_) {
    result_code_ = parts_->PreCreateThreads();
  }

#if !defined(GOOGLE_CHROME_BUILD) || defined(OS_ANDROID)
  // Single-process is an unsupported and not fully tested mode, so
  // don't enable it for official Chrome builds (except on Android).
  if (parsed_command_line_.HasSwitch(switches::kSingleProcess))
    SlaverProcessHost::SetRunSlavererInProcess(true);
#endif

  // Initialize origins that are whitelisted for process isolation.  Must be
  // done after base::FeatureList is initialized, but before any navigations
  // can happen.
  ChildProcessSecurityPolicyImpl* policy =
      ChildProcessSecurityPolicyImpl::GetInstance();
  policy->AddIsolatedOrigins(SiteIsolationPolicy::GetIsolatedOrigins());

  // Record metrics about which site isolation flags have been turned on.
  SiteIsolationPolicy::StartRecordingSiteIsolationFlagUsage();

  return result_code_;
}

void MasterMainLoop::PreShutdown() {
  parts_->PreShutdown();
}

void MasterMainLoop::CreateStartupTasks() {
  DCHECK(!startup_task_runner_);
#if defined(OS_ANDROID)
  startup_task_runner_ = std::make_unique<StartupTaskRunner>(
      base::BindOnce(&MasterStartupComplete),
      base::ThreadTaskRunnerHandle::Get());
#else
  startup_task_runner_ = std::make_unique<StartupTaskRunner>(
      base::OnceCallback<void(int)>(), base::ThreadTaskRunnerHandle::Get());
#endif
  StartupTask pre_create_threads =
      base::Bind(&MasterMainLoop::PreCreateThreads, base::Unretained(this));
  startup_task_runner_->AddTask(std::move(pre_create_threads));

  StartupTask create_threads =
      base::Bind(&MasterMainLoop::CreateThreads, base::Unretained(this));
  startup_task_runner_->AddTask(std::move(create_threads));

  StartupTask post_create_threads =
      base::Bind(&MasterMainLoop::PostCreateThreads, base::Unretained(this));
  startup_task_runner_->AddTask(std::move(post_create_threads));

  StartupTask master_thread_started = base::Bind(
      &MasterMainLoop::MasterThreadsStarted, base::Unretained(this));
  startup_task_runner_->AddTask(std::move(master_thread_started));

  StartupTask pre_main_message_loop_run = base::Bind(
      &MasterMainLoop::PreMainMessageLoopRun, base::Unretained(this));
  startup_task_runner_->AddTask(std::move(pre_main_message_loop_run));

#if defined(OS_ANDROID)
  if (parameters_.ui_task) {
    // Running inside master tests, which relies on synchronous start.
    startup_task_runner_->RunAllTasksNow();
  } else {
    startup_task_runner_->StartRunningTasksAsync();
  }
#else
  startup_task_runner_->RunAllTasksNow();
#endif
}

scoped_refptr<base::SingleThreadTaskRunner>
MasterMainLoop::GetResizeTaskRunner() {
  return base::ThreadTaskRunnerHandle::Get();
}

#if defined(OS_ANDROID)
void MasterMainLoop::SynchronouslyFlushStartupTasks() {
  startup_task_runner_->RunAllTasksNow();
}
#endif  // OS_ANDROID

int MasterMainLoop::CreateThreads() {
  // Release the TaskScheduler's threads.
  scoped_execution_fence_.reset();

  // The |io_thread| can have optionally been injected into Init(), but if not,
  // create it here. Thre thread is only tagged as MasterThread::IO here in
  // order to prevent any code from statically posting to it before
  // CreateThreads() (as such maintaining the invariant that PreCreateThreads()
  // et al. "happen-before" MasterThread::IO is "brought up").
  if (!io_thread_) {
    io_thread_ = MasterProcessSubThread::CreateIOThread();
  }
  io_thread_->RegisterAsMasterThread();

  created_threads_ = true;
  return result_code_;
}

int MasterMainLoop::PostCreateThreads() {
  if (parts_) {
    parts_->PostCreateThreads();
  }

  return result_code_;
}

int MasterMainLoop::PreMainMessageLoopRun() {
  if (parts_) {
    parts_->PreMainMessageLoopRun();
  }

  // If the UI thread blocks, the whole UI is unresponsive.
  // Do not allow unresponsive tasks from the UI thread.
  base::DisallowUnresponsiveTasks();
  return result_code_;
}

void MasterMainLoop::RunMainMessageLoopParts() {
  bool ran_main_loop = false;
  if (parts_)
    ran_main_loop = parts_->MainMessageLoopRun(&result_code_);

  if (!ran_main_loop)
    MainMessageLoopRun();
}

void MasterMainLoop::ShutdownThreadsAndCleanUp() {
  if (!created_threads_) {
    // Called early, nothing to do
    return;
  }

  // Teardown may start in PostMainMessageLoopRun, and during teardown we
  // need to be able to perform IO.
  base::ThreadRestrictions::SetIOAllowed(true);
  base::PostTaskWithTraits(
      FROM_HERE, {MasterThread::IO},
      base::BindOnce(
          base::IgnoreResult(&base::ThreadRestrictions::SetIOAllowed), true));

#if defined(OS_ANDROID)
  g_master_main_loop_shutting_down = true;
#endif

  if (SlaverProcessHost::run_slaverer_in_process())
    SlaverProcessHostImpl::ShutDownInProcessSlaverer();

  if (parts_) {
    parts_->PostMainMessageLoopRun();
  }

  // Shutdown the Service Manager and IPC.
  service_manager_context_.reset();
  mojo_ipc_support_.reset();

  {
    base::ThreadRestrictions::ScopedAllowWait allow_wait_for_join;
    {
      ResetThread_IO(std::move(io_thread_));
    }

    {
      base::TaskScheduler::GetInstance()->Shutdown();
    }
  }

  if (parts_) {
    parts_->PostDestroyThreads();
  }
}

void MasterMainLoop::InitializeMainThread() {
  base::PlatformThread::SetName("CrMasterMain");

  // Register the main thread. The main thread's task runner should already have
  // been initialized in MainMessageLoopStart() (or before if
  // MessageLoopCurrent::Get() was externally provided).
  DCHECK(base::ThreadTaskRunnerHandle::IsSet());
  main_thread_.reset(new MasterThreadImpl(
      MasterThread::UI, base::ThreadTaskRunnerHandle::Get()));
}

int MasterMainLoop::MasterThreadsStarted() {
  // Bring up Mojo IPC and the embedded Service Manager as early as possible.
  // Initializaing mojo requires the IO thread to have been initialized first,
  // so this cannot happen any earlier than now.
  InitializeMojo();

#if defined(ENABLE_IPC_FUZZER)
  SetFileUrlPathAliasForIpcFuzzer();
#endif
  return result_code_;
}

bool MasterMainLoop::InitializeToolkit() {
  if (parts_)
    parts_->ToolkitInitialized();

  return true;
}

void MasterMainLoop::MainMessageLoopRun() {
#if defined(OS_ANDROID)
  // Android's main message loop is the Java message loop.
  NOTREACHED();
#else
  DCHECK(base::MessageLoopCurrentForUI::IsSet());
  if (parameters_.ui_task) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  *parameters_.ui_task);
  }

  base::RunLoop run_loop;
  parts_->PreDefaultMainMessageLoopRun(run_loop.QuitClosure());
  run_loop.Run();
#endif
}

void MasterMainLoop::InitializeMojo() {
  if (!parsed_command_line_.HasSwitch(switches::kSingleProcess)) {
    // Disallow mojo sync calls in the master process. Note that we allow sync
    // calls in single-process mode since slaverer IPCs are made from a master
    // thread.
    mojo::SyncCallRestrictions::DisallowSyncCall();
  }

  mojo_ipc_support_.reset(new mojo::core::ScopedIPCSupport(
      base::CreateSingleThreadTaskRunnerWithTraits({MasterThread::IO}),
      mojo::core::ScopedIPCSupport::ShutdownPolicy::FAST));

  service_manager_context_.reset(
      new ServiceManagerContext(io_thread_->task_runner()));
  ServiceManagerContext::StartMasterConnection();
  GetSamplesClient()->OnServiceManagerConnected(
      ServiceManagerConnection::GetForProcess());

  tracing_controller_ = std::make_unique<samples::TracingControllerImpl>();
  samples::BackgroundTracingManagerImpl::GetInstance()
      ->AddMetadataGeneratorFunction();

  // Start startup tracing through TracingController's interface. TraceLog has
  // been enabled in samples_main_runner where threads are not available. Now We
  // need to start tracing for all other tracing agents, which require threads.
  auto* trace_startup_config = tracing::TraceStartupConfig::GetInstance();
  if (trace_startup_config->IsEnabled()) {
    // This checks kTraceConfigFile switch.
    TracingController::GetInstance()->StartTracing(
        trace_startup_config->GetTraceConfig(),
        TracingController::StartTracingDoneCallback());
  } else if (parsed_command_line_.HasSwitch(switches::kTraceToConsole)) {
    TracingController::GetInstance()->StartTracing(
        tracing::GetConfigForTraceToConsole(),
        TracingController::StartTracingDoneCallback());
  }
  // Start tracing to a file for certain duration if needed. Only do this after
  // starting the main message loop to avoid calling
  // MessagePumpForUI::ScheduleWork() before MessagePumpForUI::Start() as it
  // will crash the browser.
  if (trace_startup_config->IsTracingStartupForDuration()) {
    TRACE_EVENT0("startup", "MasterMainLoop::InitStartupTracingForDuration");
    InitStartupTracingForDuration();
  }

  if (parts_) {
    parts_->ServiceManagerConnectionStarted(
        ServiceManagerConnection::GetForProcess());
  }
}

base::FilePath MasterMainLoop::GetStartupTraceFileName() const {
  base::FilePath trace_file;

  trace_file = tracing::TraceStartupConfig::GetInstance()->GetResultFile();
  if (trace_file.empty()) {
#if defined(OS_ANDROID)
    TracingControllerAndroid::GenerateTracingFilePath(&trace_file);
#else
    // Default to saving the startup trace into the current dir.
    trace_file = base::FilePath().AppendASCII("chrometrace.log");
#endif
  }

  return trace_file;
}

void MasterMainLoop::InitStartupTracingForDuration() {
  DCHECK(tracing::TraceStartupConfig::GetInstance()
             ->IsTracingStartupForDuration());

  startup_trace_file_ = GetStartupTraceFileName();

  startup_trace_timer_.Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(
          tracing::TraceStartupConfig::GetInstance()->GetStartupDuration()),
      this, &MasterMainLoop::EndStartupTracing);
}

void MasterMainLoop::EndStartupTracing() {
  // Do nothing if startup tracing is already stopped.
  if (!tracing::TraceStartupConfig::GetInstance()->IsEnabled())
    return;

  TracingController::GetInstance()->StopTracing(
      TracingController::CreateFileEndpoint(
          startup_trace_file_,
          base::Bind(OnStoppedStartupTracing, startup_trace_file_)));
}

void MasterMainLoop::StopStartupTracingTimer() {
  startup_trace_timer_.Stop();
}

}  // namespace samples
