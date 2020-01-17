// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/master_child_process_host_impl.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/metrics/persistent_memory_allocator.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "components/tracing/common/trace_startup_config.h"
#include "components/tracing/common/tracing_switches.h"
#include "samples/master/bad_message.h"
#include "samples/master/master_main_loop.h"
#include "samples/master/service_manager/service_manager_context.h"
#include "samples/master/tracing/trace_message_filter.h"
#include "samples/common/child_process_host_impl.h"
#include "samples/common/service_manager/child_connection.h"
#include "samples/public/master/master_child_process_host_delegate.h"
#include "samples/public/master/master_child_process_observer.h"
#include "samples/public/master/master_message_filter.h"
#include "samples/public/master/master_task_traits.h"
#include "samples/public/master/master_thread.h"
#include "samples/public/master/child_process_data.h"
#include "samples/public/master/samples_master_client.h"
#include "samples/public/common/connection_filter.h"
#include "samples/public/common/samples_features.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/process_type.h"
#include "samples/public/common/result_codes.h"
#include "samples/public/common/sandboxed_process_launcher_delegate.h"
#include "samples/public/common/service_manager_connection.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/embedder/switches.h"
#include "samples/public/common/service_names.mojom.h"

namespace samples {
namespace {

static base::LazyInstance<
    MasterChildProcessHostImpl::MasterChildProcessList>::DestructorAtExit
    g_child_process_list = LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<base::ObserverList<MasterChildProcessObserver>::Unchecked>::
    DestructorAtExit g_master_child_process_observers =
        LAZY_INSTANCE_INITIALIZER;

void NotifyProcessLaunchedAndConnected(const ChildProcessData& data) {
  for (auto& observer : g_master_child_process_observers.Get())
    observer.MasterChildProcessLaunchedAndConnected(data);
}

void NotifyProcessHostConnected(const ChildProcessData& data) {
  for (auto& observer : g_master_child_process_observers.Get())
    observer.MasterChildProcessHostConnected(data);
}

void NotifyProcessHostDisconnected(const ChildProcessData& data) {
  for (auto& observer : g_master_child_process_observers.Get())
    observer.MasterChildProcessHostDisconnected(data);
}

#if !defined(OS_ANDROID)
void NotifyProcessCrashed(const ChildProcessData& data,
                          const ChildProcessTerminationInfo& info) {
  for (auto& observer : g_master_child_process_observers.Get())
    observer.MasterChildProcessCrashed(data, info);
}
#endif

void NotifyProcessKilled(const ChildProcessData& data,
                         const ChildProcessTerminationInfo& info) {
  for (auto& observer : g_master_child_process_observers.Get())
    observer.MasterChildProcessKilled(data, info);
}

}  // namespace

MasterChildProcessHost* MasterChildProcessHost::Create(
    samples::ProcessType process_type,
    MasterChildProcessHostDelegate* delegate) {
  return Create(process_type, delegate, std::string());
}

MasterChildProcessHost* MasterChildProcessHost::Create(
    samples::ProcessType process_type,
    MasterChildProcessHostDelegate* delegate,
    const std::string& service_name) {
  return new MasterChildProcessHostImpl(process_type, delegate, service_name);
}

MasterChildProcessHost* MasterChildProcessHost::FromID(int child_process_id) {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  MasterChildProcessHostImpl::MasterChildProcessList* process_list =
      g_child_process_list.Pointer();
  for (MasterChildProcessHostImpl* host : *process_list) {
    if (host->GetData().id == child_process_id)
      return host;
  }
  return nullptr;
}

#if defined(OS_MACOSX)
base::PortProvider* MasterChildProcessHost::GetPortProvider() {
  return MachBroker::GetInstance();
}
#endif

// static
MasterChildProcessHostImpl::MasterChildProcessList*
    MasterChildProcessHostImpl::GetIterator() {
  return g_child_process_list.Pointer();
}

// static
void MasterChildProcessHostImpl::AddObserver(
    MasterChildProcessObserver* observer) {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
  g_master_child_process_observers.Get().AddObserver(observer);
}

// static
void MasterChildProcessHostImpl::RemoveObserver(
    MasterChildProcessObserver* observer) {
  // TODO(phajdan.jr): Check thread after fixing http://crbug.com/167126.
  g_master_child_process_observers.Get().RemoveObserver(observer);
}

MasterChildProcessHostImpl::MasterChildProcessHostImpl(
    samples::ProcessType process_type,
    MasterChildProcessHostDelegate* delegate,
    const std::string& service_name)
    : data_(process_type),
      delegate_(delegate),
      channel_(nullptr),
      is_channel_connected_(false),
      notify_child_disconnected_(false),
      weak_factory_(this) {
  data_.id = ChildProcessHostImpl::GenerateChildProcessUniqueId();

  child_process_host_.reset(ChildProcessHost::Create(this));

  AddFilter(new TraceMessageFilter(data_.id));

  g_child_process_list.Get().push_back(this);
  GetSamplesClient()->master()->MasterChildProcessHostCreated(this);

  if (!service_name.empty()) {
    DCHECK_CURRENTLY_ON(MasterThread::IO);
    service_manager::Identity child_identity(
        service_name, service_manager::mojom::kInheritUserID,
        base::StringPrintf("%d", data_.id));
    child_connection_.reset(
        new ChildConnection(child_identity, &mojo_invitation_,
                            ServiceManagerContext::GetConnectorForIOThread(),
                            base::ThreadTaskRunnerHandle::Get()));
    data_.metrics_name = service_name;
  }

}

MasterChildProcessHostImpl::~MasterChildProcessHostImpl() {
  g_child_process_list.Get().remove(this);

  if (notify_child_disconnected_) {
    base::PostTaskWithTraits(
        FROM_HERE, {MasterThread::UI},
        base::BindOnce(&NotifyProcessHostDisconnected, data_.Duplicate()));
  }
}

// static
void MasterChildProcessHostImpl::TerminateAll() {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  // Make a copy since the MasterChildProcessHost dtor mutates the original
  // list.
  MasterChildProcessList copy = g_child_process_list.Get();
  for (auto it = copy.begin(); it != copy.end(); ++it) {
    delete (*it)->delegate();  // ~*HostDelegate deletes *HostImpl.
  }
}

//static
void MasterChildProcessHostImpl::CopyTraceStartupFlags(                                                                                                                                                    
    base::CommandLine* cmd_line) {
  if (tracing::TraceStartupConfig::GetInstance()->IsEnabled()) {
    const auto trace_config =
        tracing::TraceStartupConfig::GetInstance()->GetTraceConfig();
    if (!trace_config.IsArgumentFilterEnabled()) {
      // The only trace option that we can pass through switches is the record
      // mode. Other trace options should have the default value.
      //
      // TODO(chiniforooshan): Add other trace options to switches if, for
      // example, they are used in a telemetry test that needs startup trace
      // events from renderer processes.
      cmd_line->AppendSwitchASCII(switches::kTraceStartup,
                                  trace_config.ToCategoryFilterString());
      cmd_line->AppendSwitchASCII(
          switches::kTraceStartupRecordMode,
          base::trace_event::TraceConfig::TraceRecordModeToStr(
              trace_config.GetTraceRecordMode()));
    }
  }
}

// static
void MasterChildProcessHostImpl::CopyFeatureAndFieldTrialFlags(
    base::CommandLine* cmd_line) {
  // If we run base::FieldTrials, we want to pass to their state to the
  // child process so that it can act in accordance with each state.
  base::FieldTrialList::CopyFieldTrialStateToFlags(
      switches::kFieldTrialHandle, switches::kEnableFeatures,
      switches::kDisableFeatures, cmd_line);
}

void MasterChildProcessHostImpl::Launch(
    std::unique_ptr<SandboxedProcessLauncherDelegate> delegate,
    std::unique_ptr<base::CommandLine> cmd_line,
    bool terminate_on_shutdown) {
  DCHECK_CURRENTLY_ON(MasterThread::IO);

  GetSamplesClient()->master()->AppendExtraCommandLineSwitches(cmd_line.get(),
                                                                data_.id);

  const base::CommandLine& samples_command_line =
      *base::CommandLine::ForCurrentProcess();
  static const char* const kForwardSwitches[] = {
      service_manager::switches::kDisableInProcessStackTraces,
      switches::kDisableBackgroundTasks,
      switches::kDisableLogging,
      switches::kEnableLogging,
      switches::kIPCConnectionTimeout,
      switches::kLoggingLevel,
      switches::kTraceToConsole,
      switches::kV,
      switches::kVModule,
  };
  cmd_line->CopySwitchesFrom(samples_command_line, kForwardSwitches,
                             arraysize(kForwardSwitches));

  if (child_connection_) {
    cmd_line->AppendSwitchASCII(
        service_manager::switches::kServiceRequestChannelToken,
        child_connection_->service_token());
  }

  if (!data_.package_name.empty()) {
    cmd_line->AppendSwitchASCII(switches::kPackageName, data_.package_name);
  }

  if (data_.metrics_name.empty()) {
    data_.metrics_name = mojom::kUtilityServiceName;
  }
  // All processes should have a non-empty metrics name.
  DCHECK(!data_.metrics_name.empty());

  notify_child_disconnected_ = true;
  child_process_.reset(new ChildProcessLauncher(
      std::move(delegate), std::move(cmd_line), data_.id, this,
      std::move(mojo_invitation_),
      base::Bind(&MasterChildProcessHostImpl::OnMojoError,
                 weak_factory_.GetWeakPtr(),
                 base::ThreadTaskRunnerHandle::Get()),
      terminate_on_shutdown));
}

const ChildProcessData& MasterChildProcessHostImpl::GetData() const {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  return data_;
}

ChildProcessHost* MasterChildProcessHostImpl::GetHost() const {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  return child_process_host_.get();
}

const base::Process& MasterChildProcessHostImpl::GetProcess() const {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  DCHECK(child_process_.get())
      << "Requesting a child process handle before launching.";
  DCHECK(child_process_->GetProcess().IsValid())
      << "Requesting a child process handle before launch has completed OK.";
  return child_process_->GetProcess();
}

void MasterChildProcessHostImpl::SetName(const base::string16& name) {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  data_.name = name;
}

void MasterChildProcessHostImpl::SetPackageName(const std::string& name) {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  data_.package_name = name;
}

void MasterChildProcessHostImpl::SetMetricsName(
    const std::string& metrics_name) {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  data_.metrics_name = metrics_name;
}

void MasterChildProcessHostImpl::SetHandle(base::ProcessHandle handle) {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  data_.SetHandle(handle);
}

service_manager::mojom::ServiceRequest
MasterChildProcessHostImpl::TakeInProcessServiceRequest() {
  auto invitation = std::move(mojo_invitation_);
  return service_manager::mojom::ServiceRequest(
      invitation.ExtractMessagePipe(child_connection_->service_token()));
}

void MasterChildProcessHostImpl::ForceShutdown() {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  g_child_process_list.Get().remove(this);
  child_process_host_->ForceShutdown();
}

void MasterChildProcessHostImpl::AddFilter(MasterMessageFilter* filter) {
  child_process_host_->AddFilter(filter->GetFilter());
}

void MasterChildProcessHostImpl::BindInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  if (!child_connection_)
    return;

  child_connection_->BindInterface(interface_name, std::move(interface_pipe));
}

ChildProcessTerminationInfo MasterChildProcessHostImpl::GetTerminationInfo(
    bool known_dead) {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  if (!child_process_) {
    // If the delegate doesn't use Launch() helper.
    ChildProcessTerminationInfo info;
    info.status =
        base::GetTerminationStatus(data_.GetHandle(), &info.exit_code);
    return info;
  }
  return child_process_->GetChildTerminationInfo(known_dead);
}

bool MasterChildProcessHostImpl::OnMessageReceived(
    const IPC::Message& message) {
  return delegate_->OnMessageReceived(message);
}

void MasterChildProcessHostImpl::OnChannelConnected(int32_t peer_pid) {
  DCHECK_CURRENTLY_ON(MasterThread::IO);

  is_channel_connected_ = true;
  notify_child_disconnected_ = true;

  base::PostTaskWithTraits(
      FROM_HERE, {MasterThread::UI},
      base::BindOnce(&NotifyProcessHostConnected, data_.Duplicate()));

  delegate_->OnChannelConnected(peer_pid);

  if (IsProcessLaunched()) {
    base::PostTaskWithTraits(
        FROM_HERE, {MasterThread::UI},
        base::BindOnce(&NotifyProcessLaunchedAndConnected, data_.Duplicate()));
  }
}

void MasterChildProcessHostImpl::OnChannelError() {
  delegate_->OnChannelError();
}

void MasterChildProcessHostImpl::OnBadMessageReceived(
    const IPC::Message& message) {
  std::string log_message = "Bad message received of type: ";
  if (message.IsValid()) {
    log_message += std::to_string(message.type());
  } else {
    log_message += "unknown";
  }
  TerminateOnBadMessageReceived(log_message);
}

void MasterChildProcessHostImpl::TerminateOnBadMessageReceived(
    const std::string& error) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableKillAfterBadIPC)) {
    return;
  }
  LOG(ERROR) << "Terminating child process for bad IPC message: " << error;
  // Create a memory dump. This will contain enough stack frames to work out
  // what the bad message was.
  base::debug::DumpWithoutCrashing();

  child_process_->Terminate(RESULT_CODE_KILLED_BAD_MESSAGE);
}

void MasterChildProcessHostImpl::OnChannelInitialized(IPC::Channel* channel) {
  channel_ = channel;
}

void MasterChildProcessHostImpl::OnChildDisconnected() {
  DCHECK_CURRENTLY_ON(MasterThread::IO);
  if (child_process_.get() || data_.GetHandle()) {
    ChildProcessTerminationInfo info =
        GetTerminationInfo(true /* known_dead */);
#if defined(OS_ANDROID)
    delegate_->OnProcessCrashed(info.exit_code);
    base::PostTaskWithTraits(
        FROM_HERE, {MasterThread::UI},
        base::BindOnce(&NotifyProcessKilled, data_.Duplicate(), info));
#else  // OS_ANDROID
    switch (info.status) {
      case base::TERMINATION_STATUS_PROCESS_CRASHED:
      case base::TERMINATION_STATUS_ABNORMAL_TERMINATION: {
        delegate_->OnProcessCrashed(info.exit_code);
        base::PostTaskWithTraits(
            FROM_HERE, {MasterThread::UI},
            base::BindOnce(&NotifyProcessCrashed, data_.Duplicate(), info));
        UMA_HISTOGRAM_ENUMERATION("ChildProcess.Crashed2",
                                  static_cast<ProcessType>(data_.process_type),
                                  PROCESS_TYPE_MAX);
        break;
      }
#if defined(OS_CHROMEOS)
      case base::TERMINATION_STATUS_PROCESS_WAS_KILLED_BY_OOM:
#endif
      case base::TERMINATION_STATUS_PROCESS_WAS_KILLED: {
        delegate_->OnProcessCrashed(info.exit_code);
        base::PostTaskWithTraits(
            FROM_HERE, {MasterThread::UI},
            base::BindOnce(&NotifyProcessKilled, data_.Duplicate(), info));
        // Report that this child process was killed.
        UMA_HISTOGRAM_ENUMERATION("ChildProcess.Killed2",
                                  static_cast<ProcessType>(data_.process_type),
                                  PROCESS_TYPE_MAX);
        break;
      }
      case base::TERMINATION_STATUS_STILL_RUNNING: {
        UMA_HISTOGRAM_ENUMERATION("ChildProcess.DisconnectedAlive2",
                                  static_cast<ProcessType>(data_.process_type),
                                  PROCESS_TYPE_MAX);
        break;
      }
      default:
        break;
    }
#endif  // OS_ANDROID
    UMA_HISTOGRAM_ENUMERATION("ChildProcess.Disconnected2",
                              static_cast<ProcessType>(data_.process_type),
                              PROCESS_TYPE_MAX);
#if defined(OS_CHROMEOS)
    if (info.status == base::TERMINATION_STATUS_PROCESS_WAS_KILLED_BY_OOM) {
      UMA_HISTOGRAM_ENUMERATION("ChildProcess.Killed2.OOM",
                                static_cast<ProcessType>(data_.process_type),
                                PROCESS_TYPE_MAX);
    }
#endif
  }
  channel_ = nullptr;
  delete delegate_;  // Will delete us
}

bool MasterChildProcessHostImpl::Send(IPC::Message* message) {
  return child_process_host_->Send(message);
}

void MasterChildProcessHostImpl::OnProcessLaunchFailed(int error_code) {
  delegate_->OnProcessLaunchFailed(error_code);
  notify_child_disconnected_ = false;
  delete delegate_;  // Will delete us
}

void MasterChildProcessHostImpl::OnProcessLaunched() {
  DCHECK_CURRENTLY_ON(MasterThread::IO);

  const base::Process& process = child_process_->GetProcess();
  DCHECK(process.IsValid());

  if (child_connection_)
    child_connection_->SetProcessHandle(process.Handle());

  data_.SetHandle(process.Handle());
  delegate_->OnProcessLaunched();

  if (is_channel_connected_) {
    base::PostTaskWithTraits(
        FROM_HERE, {MasterThread::UI},
        base::BindOnce(&NotifyProcessLaunchedAndConnected, data_.Duplicate()));
  }
}

bool MasterChildProcessHostImpl::IsProcessLaunched() const {
  DCHECK_CURRENTLY_ON(MasterThread::IO);

  return child_process_.get() && child_process_->GetProcess().IsValid();
}

// static
void MasterChildProcessHostImpl::OnMojoError(
    base::WeakPtr<MasterChildProcessHostImpl> process,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    const std::string& error) {
  if (!task_runner->BelongsToCurrentThread()) {
    task_runner->PostTask(
        FROM_HERE, base::BindOnce(&MasterChildProcessHostImpl::OnMojoError,
                                  process, task_runner, error));
    return;
  }
  if (!process)
    return;
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableKillAfterBadIPC)) {
    return;
  }
  LOG(ERROR) << "Terminating child process for bad Mojo message: " << error;

  // Create a memory dump with the error message captured in a crash key value.
  // This will make it easy to determine details about what interface call
  // failed.
  base::debug::ScopedCrashKeyString scoped_error_key(
      bad_message::GetMojoErrorCrashKey(), error);
  base::debug::DumpWithoutCrashing();
  process->child_process_->Terminate(RESULT_CODE_KILLED_BAD_MESSAGE);
}

}  // namespace samples
