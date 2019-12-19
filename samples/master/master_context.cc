// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/master/master_context.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/guid.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "base/supports_user_data.h"
#include "base/task/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/unguessable_token.h"
#include "build/build_config.h"
#include "samples/master/samples_service_delegate_impl.h"
#include "samples/master/service_manager/common_master_interfaces.h"
#include "samples/common/child_process_host_impl.h"
#include "samples/public/master/master_task_traits.h"
#include "samples/public/master/master_thread.h"
#include "samples/public/master/samples_master_client.h"
#include "samples/public/master/slaver_process_host.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/service_manager_connection.h"
#include "samples/public/common/service_names.mojom.h"
#include "services/file/file_service.h"
#include "services/file/public/mojom/constants.mojom.h"
#include "services/file/user_id_map.h"
#include "services/samples/public/mojom/constants.mojom.h"
#include "services/samples/service.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/mojom/service.mojom.h"

using base::UserDataAdapter;

namespace samples {

namespace {

base::LazyInstance<std::map<std::string, MasterContext*>>::DestructorAtExit
    g_user_id_to_context = LAZY_INSTANCE_INITIALIZER;

class ServiceUserIdHolder : public base::SupportsUserData::Data {
 public:
  explicit ServiceUserIdHolder(const std::string& user_id)
      : user_id_(user_id) {}
  ~ServiceUserIdHolder() override {}

  const std::string& user_id() const { return user_id_; }

 private:
  std::string user_id_;

  DISALLOW_COPY_AND_ASSIGN(ServiceUserIdHolder);
};

class SamplesServiceDelegateHolder : public base::SupportsUserData::Data {
 public:
  explicit SamplesServiceDelegateHolder(MasterContext* master_context)
      : delegate_(master_context) {}
  ~SamplesServiceDelegateHolder() override = default;

  SamplesServiceDelegateImpl* delegate() { return &delegate_; }

 private:
  SamplesServiceDelegateImpl delegate_;

  DISALLOW_COPY_AND_ASSIGN(SamplesServiceDelegateHolder);
};

// Key names on MasterContext.
const char kSamplesServiceDelegateKey[] = "samples-service-delegate";
const char kMojoWasInitialized[] = "mojo-was-initialized";
const char kServiceManagerConnection[] = "service-manager-connection";
const char kServiceUserId[] = "service-user-id";

#if defined(OS_CHROMEOS)
const char kMountPointsKey[] = "mount_points";
#endif  // defined(OS_CHROMEOS)

void RemoveMasterContextFromUserIdMap(MasterContext* master_context) {
  ServiceUserIdHolder* holder = static_cast<ServiceUserIdHolder*>(
      master_context->GetUserData(kServiceUserId));
  if (holder) {
    auto it = g_user_id_to_context.Get().find(holder->user_id());
    if (it != g_user_id_to_context.Get().end())
      g_user_id_to_context.Get().erase(it);
  }
}

class MasterContextServiceManagerConnectionHolder
    : public base::SupportsUserData::Data {
 public:
  explicit MasterContextServiceManagerConnectionHolder(
      service_manager::mojom::ServiceRequest request)
      : service_manager_connection_(ServiceManagerConnection::Create(
            std::move(request),
            base::CreateSingleThreadTaskRunnerWithTraits(
                {MasterThread::IO}))) {}
  ~MasterContextServiceManagerConnectionHolder() override {}

  ServiceManagerConnection* service_manager_connection() {
    return service_manager_connection_.get();
  }

 private:
  std::unique_ptr<ServiceManagerConnection> service_manager_connection_;

  DISALLOW_COPY_AND_ASSIGN(MasterContextServiceManagerConnectionHolder);
};

}  // namespace

// static
void MasterContext::NotifyWillBeDestroyed(MasterContext* master_context) {
  // Make sure NotifyWillBeDestroyed is idempotent.  This helps facilitate the
  // pattern where NotifyWillBeDestroyed is called from *both*
  // ShellMasterContext and its derived classes (e.g.
  // LayoutTestMasterContext).
  if (master_context->was_notify_will_be_destroyed_called_)
    return;
  master_context->was_notify_will_be_destroyed_called_ = true;

  // Subclasses of MasterContext may expect there to be no more
  // SlaverProcessHosts using them by the time this function returns. We
  // therefore explicitly tear down embedded Samples Service instances now to
  // ensure that all their WebSampless (and therefore RPHs) are torn down too.
  master_context->RemoveUserData(kSamplesServiceDelegateKey);


  // Shared workers also keep slaver process hosts alive, and are expected to
  // return ref counts to 0 after documents close. However, to ensure that
  // hosts are destructed now, forcibly release their ref counts here.
  for (SlaverProcessHost::iterator host_iterator =
           SlaverProcessHost::AllHostsIterator();
       !host_iterator.IsAtEnd(); host_iterator.Advance()) {
    SlaverProcessHost* host = host_iterator.GetCurrentValue();
    if (host->GetMasterContext() == master_context) {
      // This will also clean up spare RPH references.
      host->DisableKeepAliveRefCount();
    }
  }
}

// static
void MasterContext::Initialize(
    MasterContext* master_context,
    const base::FilePath& path) {
  std::string new_id;
  if (GetSamplesClient() && GetSamplesClient()->master()) {
    new_id = GetSamplesClient()->master()->GetServiceUserIdForMasterContext(
        master_context);
  } else {
    // Some test scenarios initialize a MasterContext without a samples client.
    new_id = base::GenerateGUID();
  }

  ServiceUserIdHolder* holder = static_cast<ServiceUserIdHolder*>(
      master_context->GetUserData(kServiceUserId));
  if (holder)
    file::ForgetServiceUserIdUserDirAssociation(holder->user_id());
  file::AssociateServiceUserIdWithUserDir(new_id, path);
  RemoveMasterContextFromUserIdMap(master_context);
  g_user_id_to_context.Get()[new_id] = master_context;
  master_context->SetUserData(kServiceUserId,
                               std::make_unique<ServiceUserIdHolder>(new_id));

  master_context->SetUserData(
      kMojoWasInitialized, std::make_unique<base::SupportsUserData::Data>());

  ServiceManagerConnection* service_manager_connection =
      ServiceManagerConnection::GetForProcess();
  if (service_manager_connection && base::ThreadTaskRunnerHandle::IsSet()) {
    // NOTE: Many unit tests create a TestMasterContext without initializing
    // Mojo or the global service manager connection.

    service_manager::mojom::ServicePtr service;
    auto service_request = mojo::MakeRequest(&service);

    service_manager::mojom::PIDReceiverPtr pid_receiver;
    service_manager::Identity identity(mojom::kMasterServiceName, new_id);
    service_manager_connection->GetConnector()->StartService(
        identity, std::move(service), mojo::MakeRequest(&pid_receiver));
    pid_receiver->SetPID(base::GetCurrentProcId());

    service_manager_connection->GetConnector()->StartService(identity);
    MasterContextServiceManagerConnectionHolder* connection_holder =
        new MasterContextServiceManagerConnectionHolder(
            std::move(service_request));
    master_context->SetUserData(kServiceManagerConnection,
                                 base::WrapUnique(connection_holder));

    ServiceManagerConnection* connection =
        connection_holder->service_manager_connection();

    // New embedded service factories should be added to |connection| here.

    {
      service_manager::EmbeddedServiceInfo info;
      info.factory = base::BindRepeating(&file::CreateFileService);
      connection->AddEmbeddedService(file::mojom::kServiceName, info);
    }

    master_context->SetUserData(
        kSamplesServiceDelegateKey,
        std::make_unique<SamplesServiceDelegateHolder>(master_context));

    {
      service_manager::EmbeddedServiceInfo info;
      info.task_runner = base::SequencedTaskRunnerHandle::Get();
      info.factory = base::BindRepeating(
          [](MasterContext* context)
              -> std::unique_ptr<service_manager::Service> {
            auto* holder = static_cast<SamplesServiceDelegateHolder*>(
                context->GetUserData(kSamplesServiceDelegateKey));
            auto* delegate = holder->delegate();
            auto service = std::make_unique<samples::Service>(delegate);
            delegate->AddService(service.get());
            return service;
          },
          master_context);
      connection->AddEmbeddedService(samples::mojom::kServiceName, info);
    }

    SamplesMasterClient::StaticServiceMap services;
    master_context->RegisterInProcessServices(&services);
    for (const auto& entry : services) {
      connection->AddEmbeddedService(entry.first, entry.second);
    }

    RegisterCommonMasterInterfaces(connection);
    connection->Start();
  }
}

// static
const std::string& MasterContext::GetServiceUserIdFor(
    MasterContext* master_context) {
  CHECK(master_context->GetUserData(kMojoWasInitialized))
      << "Attempting to get the mojo user id for a MasterContext that was "
      << "never Initialize()ed.";

  ServiceUserIdHolder* holder = static_cast<ServiceUserIdHolder*>(
      master_context->GetUserData(kServiceUserId));
  return holder->user_id();
}

// static
MasterContext* MasterContext::GetMasterContextForServiceUserId(
    const std::string& user_id) {
  auto it = g_user_id_to_context.Get().find(user_id);
  return it != g_user_id_to_context.Get().end() ? it->second : nullptr;
}

// static
service_manager::Connector* MasterContext::GetConnectorFor(
    MasterContext* master_context) {
  ServiceManagerConnection* connection =
      GetServiceManagerConnectionFor(master_context);
  return connection ? connection->GetConnector() : nullptr;
}

// static
ServiceManagerConnection* MasterContext::GetServiceManagerConnectionFor(
    MasterContext* master_context) {
  MasterContextServiceManagerConnectionHolder* connection_holder =
      static_cast<MasterContextServiceManagerConnectionHolder*>(
          master_context->GetUserData(kServiceManagerConnection));
  return connection_holder ? connection_holder->service_manager_connection()
                           : nullptr;
}

MasterContext::MasterContext()
    : unique_id_(base::UnguessableToken::Create().ToString()) {
}

MasterContext::~MasterContext() {
  CHECK(GetUserData(kMojoWasInitialized))
      << "Attempting to destroy a MasterContext that never called "
      << "Initialize()";

  RemoveMasterContextFromUserIdMap(this);

}

const std::string& MasterContext::UniqueId() const {
  return unique_id_;
}

}  // namespace samples
