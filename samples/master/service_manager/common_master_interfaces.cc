// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/service_manager/common_master_interfaces.h"

#include <map>
#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "base/task/post_task.h"
#include "base/task_runner.h"
#include "build/build_config.h"
#include "samples/master/master_main_loop.h"
#include "samples/common/child_process_host_impl.h"
#include "samples/public/master/master_task_traits.h"
#include "samples/public/master/master_thread.h"
#include "samples/public/common/connection_filter.h"
#include "samples/public/common/service_manager_connection.h"
#include "samples/public/common/service_names.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace samples {

namespace {

class ConnectionFilterImpl : public ConnectionFilter {
 public:
  ConnectionFilterImpl() {
  }

  ~ConnectionFilterImpl() override { DCHECK_CURRENTLY_ON(MasterThread::IO); }

 private:
  template <typename Interface>
  using InterfaceBinder =
      base::Callback<void(mojo::InterfaceRequest<Interface>,
                          const service_manager::BindSourceInfo&)>;

  // ConnectionFilter:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle* interface_pipe,
                       service_manager::Connector* connector) override {
    if (source_info.identity.name() == mojom::kSlavererServiceName)
      return;

    registry_.TryBindInterface(interface_name, interface_pipe, source_info);
  }

  template <typename Interface>
  static void BindOnTaskRunner(
      const scoped_refptr<base::TaskRunner>& task_runner,
      const InterfaceBinder<Interface>& binder,
      mojo::InterfaceRequest<Interface> request,
      const service_manager::BindSourceInfo& source_info) {
    task_runner->PostTask(
        FROM_HERE, base::BindOnce(binder, std::move(request), source_info));
  }

  service_manager::BinderRegistryWithArgs<
      const service_manager::BindSourceInfo&>
      registry_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionFilterImpl);
};

}  // namespace

void RegisterCommonMasterInterfaces(ServiceManagerConnection* connection) {
  connection->AddConnectionFilter(std::make_unique<ConnectionFilterImpl>());
}

}  // namespace samples
