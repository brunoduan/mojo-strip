// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/slaverer/slaver_thread_impl.h"

#include <algorithm>
#include <limits>
#include <map>
#include <utility>
#include <vector>

#include "base/allocator/allocator_extension.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/discardable_memory_allocator.h"
#include "base/memory/shared_memory.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/process/process_metrics.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/threading/simple_thread.h"
#include "base/threading/thread_local.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/trace_event.h"
#include "base/values.h"
#include "build/build_config.h"
#include "samples/child/thread_safe_sender.h"
#include "samples/common/buildflags.h"
#include "samples/common/samples_constants_internal.h"
#include "samples/public/common/samples_constants.h"
#include "samples/public/common/samples_features.h"
#include "samples/public/common/samples_paths.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/service_manager_connection.h"
#include "samples/public/common/service_names.mojom.h"
#include "samples/public/common/simple_connection_filter.h"
#include "samples/public/common/url_constants.h"
#include "samples/public/slaverer/samples_slaverer_client.h"
#include "samples/public/slaverer/slaver_thread_observer.h"
#include "samples/slaverer/slaver_process_impl.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_channel_mojo.h"
#include "ipc/ipc_platform_file.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/web_string.h"

#if defined(OS_ANDROID)
#include <cpu-features.h>
#endif

#if defined(ENABLE_IPC_FUZZER)
#include "samples/common/external_ipc_dumper.h"
#include "mojo/public/cpp/bindings/message_dumper.h"
#endif

#if defined(OS_MACOSX)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

using base::ThreadRestrictions;
using blink::WebString;

namespace samples {

namespace {

// An implementation of mojom::SlaverMessageFilter which can be mocked out
// for tests which may indirectly send messages over this interface.
mojom::SlaverMessageFilter* g_slaver_message_filter_for_testing;


// Keep the global SlaverThreadImpl in a TLS slot so it is impossible to access
// incorrectly from the wrong thread.
base::LazyInstance<base::ThreadLocalPointer<SlaverThreadImpl>>::DestructorAtExit
    lazy_tls = LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<scoped_refptr<base::SingleThreadTaskRunner>>::
    DestructorAtExit g_main_task_runner = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
SlaverThreadImpl* SlaverThreadImpl::current() {
  return lazy_tls.Pointer()->Get();
}

// static
mojom::SlaverMessageFilter* SlaverThreadImpl::current_slaver_message_filter() {
  if (g_slaver_message_filter_for_testing)
    return g_slaver_message_filter_for_testing;
  DCHECK(current());
  return current()->slaver_message_filter();
}

// static
void SlaverThreadImpl::SetSlaverMessageFilterForTesting(
    mojom::SlaverMessageFilter* slaver_message_filter) {
  g_slaver_message_filter_for_testing = slaver_message_filter;
}

// static
scoped_refptr<base::SingleThreadTaskRunner>
SlaverThreadImpl::DeprecatedGetMainTaskRunner() {
  return g_main_task_runner.Get();
}

// In single-process mode used for debugging, we don't pass a slaverer client
// ID via command line because SlaverThreadImpl lives in the same process as
// the master
SlaverThreadImpl::SlaverThreadImpl(
    const InProcessChildThreadParams& params)
    : ChildThreadImpl(base::DoNothing(),
                      Options::Builder()
                          .InMasterProcess(params)
                          .AutoStartServiceManagerConnection(false)
                          .ConnectToMaster(true)
                          .IPCTaskRunner(base::ThreadTaskRunnerHandle::Get())
                          .Build()),
      slaverer_binding_(this),
      client_id_(1),
      weak_factory_(this) {
  Init();
}

// Multi-process mode.
SlaverThreadImpl::SlaverThreadImpl(
    base::RepeatingClosure quit_closure)
    : ChildThreadImpl(std::move(quit_closure),
                      Options::Builder()
                          .AutoStartServiceManagerConnection(false)
                          .ConnectToMaster(true)
                          .IPCTaskRunner(base::ThreadTaskRunnerHandle::Get())
                          .Build()),
      slaverer_binding_(this),
      weak_factory_(this) {
  DCHECK(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kSlavererClientId));
  base::StringToInt(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                        switches::kSlavererClientId),
                    &client_id_);
  Init();
}

void SlaverThreadImpl::Init() {
  TRACE_EVENT0("startup", "SlaverThreadImpl::Init");

  GetSamplesClient()->slaverer()->PostIOThreadCreated(GetIOTaskRunner().get());

  base::trace_event::TraceLog::GetInstance()->SetThreadSortIndex(
      base::PlatformThread::CurrentId(),
      kTraceEventSlavererMainThreadSortIndex);

  lazy_tls.Pointer()->Set(this);
  g_main_task_runner.Get() = base::ThreadTaskRunnerHandle::Get();

  // Register this object as the main thread.
  ChildProcess::current()->set_main_thread(this);

  auto registry = std::make_unique<service_manager::BinderRegistry>();

  // In single process the single process is all there is.

  GetServiceManagerConnection()->AddConnectionFilter(
      std::make_unique<SimpleConnectionFilter>(std::move(registry)));

  {
    auto registry_with_source_info =
        std::make_unique<service_manager::BinderRegistryWithArgs<
            const service_manager::BindSourceInfo&>>();
    GetServiceManagerConnection()->AddConnectionFilter(
        std::make_unique<SimpleConnectionFilterWithSourceInfo>(
            std::move(registry_with_source_info)));
  }

  GetSamplesClient()->slaverer()->SlaverThreadStarted();

  StartServiceManagerConnection();

#if defined(ENABLE_IPC_FUZZER)
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(switches::kIpcDumpDirectory)) {
    base::FilePath dump_directory =
        command_line.GetSwitchValuePath(switches::kIpcDumpDirectory);
    IPC::ChannelProxy::OutgoingMessageFilter* filter =
        LoadExternalIPCDumper(dump_directory);
    GetChannel()->set_outgoing_message_filter(filter);
    mojo::MessageDumper::SetMessageDumpDirectory(dump_directory);
  }
#endif

}

SlaverThreadImpl::~SlaverThreadImpl() {
  g_main_task_runner.Get() = nullptr;
}

void SlaverThreadImpl::Shutdown() {
  ChildThreadImpl::Shutdown();
  // In a multi-process mode, we immediately exit the slaverer.
  // Historically we had a graceful shutdown sequence here but it was
  // 1) a waste of performance and 2) a source of lots of complicated
  // crashes caused by shutdown ordering. Immediate exit eliminates
  // those problems.


  // In a single-process mode, we cannot call _exit(0) in Shutdown() because
  // it will exit the process before the master side is ready to exit.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSingleProcess))
    base::Process::TerminateCurrentProcessImmediately(0);
}

bool SlaverThreadImpl::ShouldBeDestroyed() {
  DCHECK(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kSingleProcess));
  // In a single-process mode, it is unsafe to destruct this slaverer thread
  // because we haven't run the shutdown sequence. Hence we leak the slaver
  // thread.
  //
  // In this case, we also need to disable at-exit callbacks because some of
  // the at-exit callbacks are expected to run after the slaverer thread
  // has been destructed.
  base::AtExitManager::DisableAllAtExitManagers();
  return false;
}

bool SlaverThreadImpl::Send(IPC::Message* msg) {
  // There are cases where we want to pump asynchronous messages while waiting
  // synchronously for the replies to the message to be sent here. However, this
  // may create an opportunity for re-entrancy into WebKit and other subsystems,
  // so we need to take care to disable callbacks, timers, and pending network
  // loads that could trigger such callbacks.
  bool pumping_events = false;
  if (msg->is_sync()) {
    if (msg->is_caller_pumping_messages()) {
      pumping_events = true;
    }
  }

  bool rv = ChildThreadImpl::Send(msg);

  return rv;
}

IPC::SyncChannel* SlaverThreadImpl::GetChannel() {
  return channel();
}

std::string SlaverThreadImpl::GetLocale() {
  // The master process should have passed the locale to the slaverer via the
  // --lang command line flag.
  const base::CommandLine& parsed_command_line =
      *base::CommandLine::ForCurrentProcess();
  const std::string& lang =
      parsed_command_line.GetSwitchValueASCII(switches::kLang);
  DCHECK(!lang.empty());
  return lang;
}

IPC::SyncMessageFilter* SlaverThreadImpl::GetSyncMessageFilter() {
  return sync_message_filter();
}

void SlaverThreadImpl::AddRoute(int32_t routing_id, IPC::Listener* listener) {
  ChildThreadImpl::GetRouter()->AddRoute(routing_id, listener);
}

void SlaverThreadImpl::RemoveRoute(int32_t routing_id) {
  ChildThreadImpl::GetRouter()->RemoveRoute(routing_id);
}

mojom::SlavererHost* SlaverThreadImpl::GetSlavererHost() {
  if (!slaverer_host_) {
    GetChannel()->GetRemoteAssociatedInterface(&slaverer_host_);
  }
  return slaverer_host_.get();
}

int SlaverThreadImpl::GenerateRoutingID() {
  int32_t routing_id = MSG_ROUTING_NONE;
  slaver_message_filter()->GenerateRoutingID(&routing_id);
  return routing_id;
}

void SlaverThreadImpl::AddFilter(IPC::MessageFilter* filter) {
  channel()->AddFilter(filter);
}

void SlaverThreadImpl::RemoveFilter(IPC::MessageFilter* filter) {
  channel()->RemoveFilter(filter);
}

void SlaverThreadImpl::AddObserver(SlaverThreadObserver* observer) {
  observers_.AddObserver(observer);
  observer->RegisterMojoInterfaces(&associated_interfaces_);
}

void SlaverThreadImpl::RemoveObserver(SlaverThreadObserver* observer) {
  observer->UnregisterMojoInterfaces(&associated_interfaces_);
  observers_.RemoveObserver(observer);
}

base::WaitableEvent* SlaverThreadImpl::GetShutdownEvent() {
  return ChildProcess::current()->GetShutDownEvent();
}

int32_t SlaverThreadImpl::GetClientId() {
  return client_id_;
}

void SlaverThreadImpl::SetSlavererProcessType(
    blink::scheduler::SlavererProcessType type) {
}

void SlaverThreadImpl::OnAssociatedInterfaceRequest(
    const std::string& name,
    mojo::ScopedInterfaceEndpointHandle handle) {
  if (!associated_interfaces_.TryBindInterface(name, &handle))
    ChildThreadImpl::OnAssociatedInterfaceRequest(name, std::move(handle));
}

scoped_refptr<base::SingleThreadTaskRunner>
SlaverThreadImpl::GetIOTaskRunner() {
  return ChildProcess::current()->io_task_runner();
}

bool SlaverThreadImpl::IsMainThread() {
  return !!current();
}

void SlaverThreadImpl::OnChannelError() {
  // In single-process mode, the slaverer can't be restarted after shutdown.
  // So, if we get a channel error, crash the whole process right now to get a
  // more informative stack, since we will otherwise just crash later when we
  // try to restart it.
  CHECK(!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSingleProcess));
  ChildThreadImpl::OnChannelError();
}

void SlaverThreadImpl::OnProcessFinalRelease() {
  // Do not shutdown the process. The master process is the only one
  // responsible for slaverer shutdown.
  //
  // Slaverer process used to request self shutdown. It has been removed. It
  // caused race conditions, where the master process was reusing slaverer
  // processes that were shutting down.
  // See https://crbug.com/535246 or https://crbug.com/873541/#c8.
  NOTREACHED();
}

bool SlaverThreadImpl::OnControlMessageReceived(const IPC::Message& msg) {
  return false;
}

void SlaverThreadImpl::SetProcessBackgrounded(bool backgrounded) {
}

void SlaverThreadImpl::ProcessPurgeAndSuspend() {
  if (!base::FeatureList::IsEnabled(features::kPurgeAndSuspend))
    return;

}

blink::AssociatedInterfaceRegistry*
SlaverThreadImpl::GetAssociatedInterfaceRegistry() {
  return &associated_interfaces_;
}

mojom::SlaverMessageFilter* SlaverThreadImpl::slaver_message_filter() {
  if (!slaver_message_filter_)
    GetChannel()->GetRemoteAssociatedInterface(&slaver_message_filter_);
  return slaver_message_filter_.get();
}

void SlaverThreadImpl::CreateEmbedderSlavererService(
    service_manager::mojom::ServiceRequest service_request) {
  GetSamplesClient()->slaverer()->CreateSlavererService(
      std::move(service_request));
}

}  // namespace samples
