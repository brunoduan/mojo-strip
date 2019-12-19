// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/utility/utility_service_factory.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/trace_event/trace_log.h"
#include "build/build_config.h"
#include "samples/child/child_process.h"
#include "samples/public/common/samples_client.h"
#include "samples/public/common/samples_features.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/service_names.mojom.h"
#include "samples/public/utility/samples_utility_client.h"
#include "samples/public/utility/utility_thread.h"
#include "samples/utility/utility_thread_impl.h"
#include "services/data_decoder/data_decoder_service.h"
#include "services/data_decoder/public/mojom/constants.mojom.h"
#include "services/service_manager/public/mojom/service.mojom.h"

namespace samples {

namespace {

std::unique_ptr<service_manager::Service> CreateDataDecoderService() {
  samples::UtilityThread::Get()->EnsureBlinkInitialized();
  return data_decoder::DataDecoderService::Create();
}

}  // namespace

UtilityServiceFactory::UtilityServiceFactory() {}

UtilityServiceFactory::~UtilityServiceFactory() {}

void UtilityServiceFactory::CreateService(
    service_manager::mojom::ServiceRequest request,
    const std::string& name,
    service_manager::mojom::PIDReceiverPtr pid_receiver) {
  auto* trace_log = base::trace_event::TraceLog::GetInstance();
  if (trace_log->IsProcessNameEmpty())
    trace_log->set_process_name("Service: " + name);
  ServiceFactory::CreateService(std::move(request), name,
                                std::move(pid_receiver));
}

void UtilityServiceFactory::RegisterServices(ServiceMap* services) {
  GetSamplesClient()->utility()->RegisterServices(services);

  service_manager::EmbeddedServiceInfo data_decoder_info;
  data_decoder_info.factory = base::Bind(&CreateDataDecoderService);
  services->insert(
      std::make_pair(data_decoder::mojom::kServiceName, data_decoder_info));
}

void UtilityServiceFactory::OnServiceQuit() {
  UtilityThread::Get()->ReleaseProcess();
}

void UtilityServiceFactory::OnLoadFailed() {
  UtilityThreadImpl* utility_thread =
      static_cast<UtilityThreadImpl*>(UtilityThread::Get());
  utility_thread->Shutdown();
  utility_thread->ReleaseProcess();
}

}  // namespace samples
