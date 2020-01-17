// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/test/echo_master/echo_master_service.h"

#include "services/service_manager/public/cpp/service_context.h"

namespace echo_master {

std::unique_ptr<service_manager::Service> CreateEchoMasterService() {
  return std::make_unique<EchoMasterService>();
}

EchoMasterService::EchoMasterService() {
  registry_.AddInterface<mojom::EchoMaster>(
      base::Bind(&EchoMasterService::BindEchoMasterRequest, base::Unretained(this)));
}

EchoMasterService::~EchoMasterService() {}

void EchoMasterService::OnStart() {}

void EchoMasterService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void EchoMasterService::BindEchoMasterRequest(mojom::EchoMasterRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void EchoMasterService::EchoString(const std::string& input,
                             EchoStringCallback callback) {
  std::move(callback).Run(input);
}

void EchoMasterService::Quit() {
  context()->CreateQuitClosure().Run();
}

}  // namespace echo_master
