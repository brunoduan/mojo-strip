// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/utility/utility_thread_impl.h"

#include <utility>

#include "base/command_line.h"
#include "build/build_config.h"
#include "samples/child/child_process.h"
#include "samples/public/common/service_manager_connection.h"
#include "samples/public/common/simple_connection_filter.h"
#include "samples/public/utility/samples_utility_client.h"
#include "samples/utility/utility_service_factory.h"
#include "ipc/ipc_sync_channel.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/sandbox/switches.h"

namespace samples {

UtilityThreadImpl::UtilityThreadImpl(base::RepeatingClosure quit_closure)
    : ChildThreadImpl(std::move(quit_closure),
                      ChildThreadImpl::Options::Builder()
                          .AutoStartServiceManagerConnection(false)
                          .Build()) {
  Init();
}

UtilityThreadImpl::UtilityThreadImpl(const InProcessChildThreadParams& params)
    : ChildThreadImpl(base::DoNothing(),
                      ChildThreadImpl::Options::Builder()
                          .AutoStartServiceManagerConnection(false)
                          .InMasterProcess(params)
                          .Build()) {
  Init();
}

UtilityThreadImpl::~UtilityThreadImpl() = default;

void UtilityThreadImpl::Shutdown() {
  ChildThreadImpl::Shutdown();
}

void UtilityThreadImpl::ReleaseProcess() {
  if (!IsInMasterProcess()) {
    ChildProcess::current()->ReleaseProcess();
    return;
  }

  // Close the channel to cause the UtilityProcessHost to be deleted. We need to
  // take a different code path than the multi-process case because that case
  // depends on the child process going away to close the channel, but that
  // can't happen when we're in single process mode.
  channel()->Close();
}

void UtilityThreadImpl::EnsureBlinkInitialized() {
  EnsureBlinkInitializedInternal(/*sandbox_support=*/false);
}

void UtilityThreadImpl::EnsureBlinkInitializedInternal(bool sandbox_support) {
  // We can only initialize Blink on one thread, and in single process mode
  // we run the utility thread on a separate thread. This means that if any
  // code needs Blink initialized in the utility process, they need to have
  // another path to support single process mode.
  if (IsInMasterProcess())
    return;

}

void UtilityThreadImpl::Init() {
  ChildProcess::current()->AddRefProcess();

  auto registry = std::make_unique<service_manager::BinderRegistry>();
  registry->AddInterface(
      base::Bind(&UtilityThreadImpl::BindServiceFactoryRequest,
                 base::Unretained(this)),
      base::ThreadTaskRunnerHandle::Get());

  samples::ServiceManagerConnection* connection = GetServiceManagerConnection();
  if (connection) {
    connection->AddConnectionFilter(
        std::make_unique<SimpleConnectionFilter>(std::move(registry)));
  }

  GetSamplesClient()->utility()->UtilityThreadStarted();

  service_factory_.reset(new UtilityServiceFactory);

  if (connection) {
    connection->Start();
    GetSamplesClient()->OnServiceManagerConnected(connection);
  }
}

bool UtilityThreadImpl::OnControlMessageReceived(const IPC::Message& msg) {
  return GetSamplesClient()->utility()->OnMessageReceived(msg);
}

void UtilityThreadImpl::BindServiceFactoryRequest(
    service_manager::mojom::ServiceFactoryRequest request) {
  DCHECK(service_factory_);
  service_factory_bindings_.AddBinding(service_factory_.get(),
                                       std::move(request));
}

}  // namespace samples
