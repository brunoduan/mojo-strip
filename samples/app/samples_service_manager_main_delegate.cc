// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/app/samples_service_manager_main_delegate.h"

#include "base/command_line.h"
#include "samples/app/samples_main_runner_impl.h"
#include "samples/public/app/samples_main_delegate.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/service_names.mojom.h"
#include "services/service_manager/embedder/switches.h"
#include "services/service_manager/runner/common/client_util.h"

namespace samples {

SamplesServiceManagerMainDelegate::SamplesServiceManagerMainDelegate(
    const SamplesMainParams& params)
    : samples_main_params_(params),
      samples_main_runner_(SamplesMainRunnerImpl::Create()) {}

SamplesServiceManagerMainDelegate::~SamplesServiceManagerMainDelegate() =
    default;

int SamplesServiceManagerMainDelegate::Initialize(
    const InitializeParams& params) {
#if defined(OS_ANDROID)
  // May be called twice on Android due to the way master startup requests are
  // dispatched by the system.
  if (initialized_)
    return -1;
#endif

#if defined(OS_MACOSX)
  samples_main_params_.autorelease_pool = params.autorelease_pool;
#endif

  return samples_main_runner_->Initialize(samples_main_params_);
}

bool SamplesServiceManagerMainDelegate::IsEmbedderSubprocess() {
  auto type = base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
      switches::kProcessType);
  return type == switches::kSlavererProcess ||
         type == switches::kUtilityProcess ||
         type == service_manager::switches::kZygoteProcess;
}

int SamplesServiceManagerMainDelegate::RunEmbedderProcess() {
  return samples_main_runner_->Run(start_service_manager_only_);
}

void SamplesServiceManagerMainDelegate::ShutDownEmbedderProcess() {
#if !defined(OS_ANDROID)
  samples_main_runner_->Shutdown();
#endif
}

service_manager::ProcessType
SamplesServiceManagerMainDelegate::OverrideProcessType() {
  return samples_main_params_.delegate->OverrideProcessType();
}

void SamplesServiceManagerMainDelegate::OverrideMojoConfiguration(
    mojo::core::Configuration* config) {
  // If this is the master process and there's no remote service manager, we
  // will serve as the global Mojo broker.
  if (!service_manager::ServiceManagerIsRemote() &&
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kProcessType))
    config->is_broker_process = true;
}

std::unique_ptr<base::Value>
SamplesServiceManagerMainDelegate::CreateServiceCatalog() {
  return nullptr;
}

bool SamplesServiceManagerMainDelegate::ShouldLaunchAsServiceProcess(
    const service_manager::Identity& identity) {
  return identity.name() != mojom::kPackagedServicesServiceName;
}

void SamplesServiceManagerMainDelegate::AdjustServiceProcessCommandLine(
    const service_manager::Identity& identity,
    base::CommandLine* command_line) {
  base::CommandLine::StringVector args_without_switches;
  if (identity.name() == mojom::kPackagedServicesServiceName) {
    // Ensure other arguments like URL are not lost.
    args_without_switches = command_line->GetArgs();

    // When launching the master process, ensure that we don't inherit any
    // process type flag. When samples embeds Service Manager, a process with no
    // type is launched as a master process.
    base::CommandLine::SwitchMap switches = command_line->GetSwitches();
    switches.erase(switches::kProcessType);
    *command_line = base::CommandLine(command_line->GetProgram());
    for (const auto& sw : switches)
      command_line->AppendSwitchNative(sw.first, sw.second);
  }

  samples_main_params_.delegate->AdjustServiceProcessCommandLine(identity,
                                                                 command_line);

  // Append other arguments back to |command_line| after the second call to
  // delegate as long as it can still remove all the arguments without switches.
  for (const auto& arg : args_without_switches)
    command_line->AppendArgNative(arg);
}

void SamplesServiceManagerMainDelegate::OnServiceManagerInitialized(
    const base::Closure& quit_closure,
    service_manager::BackgroundServiceManager* service_manager) {
  return samples_main_params_.delegate->OnServiceManagerInitialized(
      quit_closure, service_manager);
}

std::unique_ptr<service_manager::Service>
SamplesServiceManagerMainDelegate::CreateEmbeddedService(
    const std::string& service_name) {
  // TODO

  return nullptr;
}

void SamplesServiceManagerMainDelegate::SetStartServiceManagerOnly(
    bool start_service_manager_only) {
  start_service_manager_only_ = start_service_manager_only;
}

}  // namespace samples
