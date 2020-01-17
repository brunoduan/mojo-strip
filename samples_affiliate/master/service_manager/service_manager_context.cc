// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/service_manager/service_manager_context.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/deferred_sequenced_task_runner.h"
#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/no_destructor.h"
#include "base/process/process_handle.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "build/build_config.h"
#include "samples/master/master_main_loop.h"
#include "samples/master/child_process_launcher.h"
#include "samples/master/service_manager/common_master_interfaces.h"
#include "samples/master/utility_process_host.h"
#include "samples/master/utility_process_host_client.h"
#include "samples/common/service_manager/service_manager_connection_impl.h"
#include "samples/grit/samples_resources.h"
#include "samples/public/master/master_thread.h"
#include "samples/public/master/child_process_data.h"
#include "samples/public/master/samples_master_client.h"
#include "samples/public/common/bind_interface_helpers.h"
#include "samples/public/common/samples_client.h"
#include "samples/public/common/samples_features.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/service_manager_connection.h"
#include "samples/public/common/service_names.mojom.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "mojo/public/cpp/system/invitation.h"
#include "services/catalog/manifest_provider.h"
#include "services/catalog/public/cpp/manifest_parsing_util.h"
#include "services/catalog/public/mojom/constants.mojom.h"
#include "services/data_decoder/public/mojom/constants.mojom.h"
#include "services/service_manager/connect_params.h"
#include "services/service_manager/embedder/manifest_utils.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/mojom/service.mojom.h"
#include "services/service_manager/runner/common/client_util.h"
#include "services/service_manager/runner/host/service_process_launcher.h"
#include "services/service_manager/sandbox/sandbox_type.h"
#include "services/service_manager/service_manager.h"
#include "services/tracing/public/mojom/constants.mojom.h"
#include "services/tracing/tracing_service.h"

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#endif

namespace samples {

namespace {

base::LazyInstance<std::unique_ptr<service_manager::Connector>>::Leaky
    g_io_thread_connector = LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<std::map<std::string, base::WeakPtr<UtilityProcessHost>>>::
    Leaky g_active_process_groups;

void DestroyConnectorOnIOThread() { g_io_thread_connector.Get().reset(); }

// Launch a process for a service once its sandbox type is known.
void StartServiceInUtilityProcess(
    const std::string& service_name,
    const SamplesMasterClient::ProcessNameCallback& process_name_callback,
    base::Optional<std::string> process_group,
    service_manager::mojom::ServiceRequest request,
    service_manager::mojom::PIDReceiverPtr pid_receiver,
    service_manager::mojom::ConnectResult query_result,
    const std::string& sandbox_string) {
  service_manager::SandboxType sandbox_type =
      service_manager::UtilitySandboxTypeFromString(sandbox_string);

  // Look for an existing process group.
  base::WeakPtr<UtilityProcessHost>* weak_host = nullptr;
  if (process_group)
    weak_host = &g_active_process_groups.Get()[*process_group];

  UtilityProcessHost* process_host = nullptr;
  if (weak_host && *weak_host) {
    // Start service in an existing process.
    process_host = weak_host->get();
  } else {
    // Start a new process for this service.
    UtilityProcessHost* impl = new UtilityProcessHost(nullptr, nullptr);
    base::string16 process_name = process_name_callback.Run();
    DCHECK(!process_name.empty());
    impl->SetName(process_name);
    impl->SetMetricsName(service_name);
    impl->SetServiceIdentity(service_manager::Identity(service_name));
    impl->SetSandboxType(sandbox_type);
    impl->Start();
    impl->SetLaunchCallback(
        base::BindOnce([](service_manager::mojom::PIDReceiverPtr pid_receiver,
                          base::ProcessId pid) { pid_receiver->SetPID(pid); },
                       std::move(pid_receiver)));
    if (weak_host)
      *weak_host = impl->AsWeakPtr();
    process_host = impl;
  }

  service_manager::mojom::ServiceFactoryPtr service_factory;
  BindInterface(process_host, mojo::MakeRequest(&service_factory));

  // CreateService expects a non-null PIDReceiverPtr, but we don't actually
  // expect the utility process to report anything on it. Send a dead-end proxy.
  service_manager::mojom::PIDReceiverPtr dead_pid_receiver;
  mojo::MakeRequest(&dead_pid_receiver);
  service_factory->CreateService(std::move(request), service_name,
                                 std::move(dead_pid_receiver));
}

// Determine a sandbox type for a service and launch a process for it.
void QueryAndStartServiceInUtilityProcess(
    const std::string& service_name,
    const SamplesMasterClient::ProcessNameCallback& process_name_callback,
    base::Optional<std::string> process_group,
    service_manager::mojom::ServiceRequest request,
    service_manager::mojom::PIDReceiverPtr pid_receiver) {
  ServiceManagerContext::GetConnectorForIOThread()->QueryService(
      service_manager::Identity(service_name),
      base::BindOnce(&StartServiceInUtilityProcess, service_name,
                     process_name_callback, std::move(process_group),
                     std::move(request), std::move(pid_receiver)));
}

// A ManifestProvider which resolves application names to builtin manifest
// resources for the catalog service to consume.
class BuiltinManifestProvider : public catalog::ManifestProvider {
 public:
  BuiltinManifestProvider() {}
  ~BuiltinManifestProvider() override {}

  void AddServiceManifest(base::StringPiece name, int resource_id) {
    std::string contents =
        GetSamplesClient()
            ->GetDataResource(resource_id, ui::ScaleFactor::SCALE_FACTOR_NONE)
            .as_string();
    DCHECK(!contents.empty());

    std::unique_ptr<base::Value> manifest_value =
        base::JSONReader::Read(contents);
    DCHECK(manifest_value);

    std::unique_ptr<base::Value> overlay_value =
        GetSamplesClient()->master()->GetServiceManifestOverlay(name);

    service_manager::MergeManifestWithOverlay(manifest_value.get(),
                                              overlay_value.get());

    base::Optional<catalog::RequiredFileMap> required_files =
        catalog::RetrieveRequiredFiles(*manifest_value);
    if (required_files) {
      ChildProcessLauncher::SetRegisteredFilesForService(
          name.as_string(), std::move(*required_files));
    }

    auto result = manifests_.insert(
        std::make_pair(name.as_string(), std::move(manifest_value)));
    DCHECK(result.second) << "Duplicate manifest entry: " << name;
  }

 private:
  // catalog::ManifestProvider:
  std::unique_ptr<base::Value> GetManifest(const std::string& name) override {
    auto it = manifests_.find(name);
    return it != manifests_.end() ? it->second->CreateDeepCopy() : nullptr;
  }

  std::map<std::string, std::unique_ptr<base::Value>> manifests_;

  DISALLOW_COPY_AND_ASSIGN(BuiltinManifestProvider);
};

class NullServiceProcessLauncherFactory
    : public service_manager::ServiceProcessLauncherFactory {
 public:
  NullServiceProcessLauncherFactory() {}
  ~NullServiceProcessLauncherFactory() override {}

 private:
  std::unique_ptr<service_manager::ServiceProcessLauncher> Create(
      const base::FilePath& service_path) override {
    // There are innocuous races where master code may attempt to connect
    // to a specific renderer instance through the Service Manager after that
    // renderer has been terminated. These result in this code path being hit
    // fairly regularly and the resulting log spam causes confusion. We suppress
    // this message only for "samples_renderer".
    const base::FilePath::StringType kRendererServiceFilename =
        base::FilePath().AppendASCII(mojom::kSlavererServiceName).value();
    const base::FilePath::StringType service_executable =
        service_path.BaseName().value();
    if (service_executable.find(kRendererServiceFilename) ==
        base::FilePath::StringType::npos) {
      LOG(ERROR) << "Attempting to run unsupported native service: "
                 << service_path.value();
    }
    return nullptr;
  }

  DISALLOW_COPY_AND_ASSIGN(NullServiceProcessLauncherFactory);
};

// This class is intended for tests that want to load service binaries (rather
// than via the utility process). Production code uses
// NullServiceProcessLauncherFactory.
class ServiceBinaryLauncherFactory
    : public service_manager::ServiceProcessLauncherFactory {
 public:
  ServiceBinaryLauncherFactory() = default;
  ~ServiceBinaryLauncherFactory() override = default;

 private:
  std::unique_ptr<service_manager::ServiceProcessLauncher> Create(
      const base::FilePath& service_path) override {
    return std::make_unique<service_manager::ServiceProcessLauncher>(
        nullptr, service_path);
  }

  DISALLOW_COPY_AND_ASSIGN(ServiceBinaryLauncherFactory);
};

}  // namespace

// State which lives on the IO thread and drives the ServiceManager.
class ServiceManagerContext::InProcessServiceManagerContext
    : public base::RefCountedThreadSafe<InProcessServiceManagerContext> {
 public:
  InProcessServiceManagerContext(scoped_refptr<base::SingleThreadTaskRunner>
                                     service_manager_thread_task_runner)
      : service_manager_thread_task_runner_(
            service_manager_thread_task_runner) {}

  void Start(
      service_manager::mojom::ServicePtrInfo packaged_services_service_info,
      std::unique_ptr<BuiltinManifestProvider> manifest_provider) {
    service_manager_thread_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &InProcessServiceManagerContext::StartOnServiceManagerThread, this,
            std::move(manifest_provider),
            std::move(packaged_services_service_info),
            base::ThreadTaskRunnerHandle::Get()));
  }

  void ShutDown() {
    service_manager_thread_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &InProcessServiceManagerContext::ShutDownOnServiceManagerThread,
            this));
  }

  void StartServices(std::vector<service_manager::Identity> identities) {
    service_manager_thread_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&InProcessServiceManagerContext ::
                                      StartServicesOnServiceManagerThread,
                                  this, std::move(identities)));
  }

 private:
  friend class base::RefCountedThreadSafe<InProcessServiceManagerContext>;

  ~InProcessServiceManagerContext() {}

  void StartOnServiceManagerThread(
      std::unique_ptr<BuiltinManifestProvider> manifest_provider,
      service_manager::mojom::ServicePtrInfo packaged_services_service_info,
      scoped_refptr<base::SequencedTaskRunner> ui_thread_task_runner) {
    manifest_provider_ = std::move(manifest_provider);
    std::unique_ptr<service_manager::ServiceProcessLauncherFactory>
        service_process_launcher_factory;
    if (base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kEnableServiceBinaryLauncher)) {
      service_process_launcher_factory =
          std::make_unique<ServiceBinaryLauncherFactory>();
    } else {
      service_process_launcher_factory =
          std::make_unique<NullServiceProcessLauncherFactory>();
    }
    service_manager_ = std::make_unique<service_manager::ServiceManager>(
        std::move(service_process_launcher_factory), nullptr,
        manifest_provider_.get());

    service_manager::mojom::ServicePtr packaged_services_service;
    packaged_services_service.Bind(std::move(packaged_services_service_info));
    service_manager_->RegisterService(
        service_manager::Identity(mojom::kPackagedServicesServiceName,
                                  service_manager::mojom::kRootUserID),
        std::move(packaged_services_service), nullptr);
    service_manager_->SetInstanceQuitCallback(
        base::Bind(&OnInstanceQuitOnServiceManagerThread,
                   std::move(ui_thread_task_runner)));
  }

  static void OnInstanceQuitOnServiceManagerThread(
      scoped_refptr<base::SequencedTaskRunner> ui_thread_task_runner,
      const service_manager::Identity& id) {
    ui_thread_task_runner->PostTask(FROM_HERE,
                                    base::BindOnce(&OnInstanceQuit, id));
  }

  static void OnInstanceQuit(const service_manager::Identity& id) {
    if (GetSamplesClient()->master()->ShouldTerminateOnServiceQuit(id)) {
      // Don't LOG(FATAL) because we don't want a master crash report.
      LOG(ERROR) << "Terminating because service '" << id.name()
                 << "' quit unexpectedly.";
      // Skip shutdown to reduce the risk that other code in the master will
      // respond to the service pipe closing.
      exit(1);
    }
  }

  void ShutDownOnServiceManagerThread() {
    service_manager_.reset();
    manifest_provider_.reset();
  }

  void StartServicesOnServiceManagerThread(
      std::vector<service_manager::Identity> identities) {
    if (!service_manager_)
      return;

    for (const auto& identity : identities)
      service_manager_->StartService(identity);
  }

  scoped_refptr<base::SingleThreadTaskRunner>
      service_manager_thread_task_runner_;
  std::unique_ptr<BuiltinManifestProvider> manifest_provider_;
  std::unique_ptr<service_manager::ServiceManager> service_manager_;

  DISALLOW_COPY_AND_ASSIGN(InProcessServiceManagerContext);
};

ServiceManagerContext::ServiceManagerContext(
    scoped_refptr<base::SingleThreadTaskRunner>
        service_manager_thread_task_runner)
    : service_manager_thread_task_runner_(
          std::move(service_manager_thread_task_runner)) {
  // The |service_manager_thread_task_runner_| must have been created before
  // starting the ServiceManager.
  DCHECK(service_manager_thread_task_runner_);
  service_manager::mojom::ServiceRequest packaged_services_request;
  if (service_manager::ServiceManagerIsRemote()) {
    auto endpoint = mojo::PlatformChannel::RecoverPassedEndpointFromCommandLine(
        *base::CommandLine::ForCurrentProcess());
    auto invitation = mojo::IncomingInvitation::Accept(std::move(endpoint));
    packaged_services_request =
        service_manager::GetServiceRequestFromCommandLine(&invitation);
  } else {
    std::unique_ptr<BuiltinManifestProvider> manifest_provider =
        std::make_unique<BuiltinManifestProvider>();
    static const struct ManifestInfo {
      const char* name;
      int resource_id;
    } kManifests[] = {
        {mojom::kMasterServiceName, IDR_MOJO_SAMPLES_MASTER_MANIFEST},
        {mojom::kPackagedServicesServiceName, IDR_MOJO_SAMPLES_PACKAGED_SERVICES_MANIFEST},
        {mojom::kSlavererServiceName, IDR_MOJO_SAMPLES_SLAVERER_MANIFEST},
        {mojom::kUtilityServiceName, IDR_MOJO_SAMPLES_UTILITY_MANIFEST},
        {catalog::mojom::kServiceName, IDR_MOJO_CATALOG_MANIFEST},
    };

    for (size_t i = 0; i < arraysize(kManifests); ++i) {
      manifest_provider->AddServiceManifest(kManifests[i].name,
                                            kManifests[i].resource_id);
    }
    for (const auto& manifest :
         GetSamplesClient()->master()->GetExtraServiceManifests()) {
      manifest_provider->AddServiceManifest(manifest.name,
                                            manifest.resource_id);
    }

    in_process_context_ =
        new InProcessServiceManagerContext(service_manager_thread_task_runner_);

    service_manager::mojom::ServicePtr packaged_services_service;
    packaged_services_request = mojo::MakeRequest(&packaged_services_service);
    in_process_context_->Start(packaged_services_service.PassInterface(),
                               std::move(manifest_provider));
  }

  packaged_services_connection_ =
      ServiceManagerConnection::Create(std::move(packaged_services_request),
                                       service_manager_thread_task_runner_);

  service_manager::mojom::ServicePtr root_master_service;
  ServiceManagerConnection::SetForProcess(
      ServiceManagerConnection::Create(mojo::MakeRequest(&root_master_service),
                                       service_manager_thread_task_runner_));
  auto* master_connection = ServiceManagerConnection::GetForProcess();

  service_manager::mojom::PIDReceiverPtr pid_receiver;
  packaged_services_connection_->GetConnector()->StartService(
      service_manager::Identity(mojom::kMasterServiceName,
                                service_manager::mojom::kRootUserID),
      std::move(root_master_service), mojo::MakeRequest(&pid_receiver));
  pid_receiver->SetPID(base::GetCurrentProcId());

  {
    service_manager::EmbeddedServiceInfo info;
    info.factory = base::Bind(&tracing::TracingService::Create);
    packaged_services_connection_->AddEmbeddedService(
        tracing::mojom::kServiceName, info);
  }

  SamplesMasterClient::StaticServiceMap services;
  GetSamplesClient()->master()->RegisterInProcessServices(
      &services, packaged_services_connection_.get());
  for (const auto& entry : services) {
    packaged_services_connection_->AddEmbeddedService(entry.first,
                                                      entry.second);
  }

  // This is safe to assign directly from any thread, because
  // ServiceManagerContext must be constructed before anyone can call
  // GetConnectorForIOThread().
  g_io_thread_connector.Get() = master_connection->GetConnector()->Clone();

  SamplesMasterClient::OutOfProcessServiceMap out_of_process_services;
  GetSamplesClient()->master()->RegisterOutOfProcessServices(
      &out_of_process_services);

  out_of_process_services[data_decoder::mojom::kServiceName] =
      base::BindRepeating(&base::ASCIIToUTF16, "Data Decoder Service");

  for (const auto& service : out_of_process_services) {
    packaged_services_connection_->AddServiceRequestHandlerWithPID(
        service.first,
        base::BindRepeating(&QueryAndStartServiceInUtilityProcess,
                            service.first, service.second.process_name_callback,
                            service.second.process_group));
  }

  packaged_services_connection_->Start();

  in_process_context_->StartServices(
      GetSamplesClient()->master()->GetStartupServices());
}

ServiceManagerContext::~ServiceManagerContext() {
  // NOTE: The in-process ServiceManager MUST be destroyed before the master
  // process-wide ServiceManagerConnection. Otherwise it's possible for the
  // ServiceManager to receive connection requests for service:samples_master
  // which it may attempt to service by launching a new instance of the master.
  if (in_process_context_)
    in_process_context_->ShutDown();
  if (ServiceManagerConnection::GetForProcess())
    ServiceManagerConnection::DestroyForProcess();
  service_manager_thread_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&DestroyConnectorOnIOThread));
}

// static
service_manager::Connector* ServiceManagerContext::GetConnectorForIOThread() {
  DCHECK(MasterThread::CurrentlyOn(MasterThread::IO));
  return g_io_thread_connector.Get().get();
}

// static
bool ServiceManagerContext::HasValidProcessForProcessGroup(
    const std::string& process_group_name) {
  auto iter = g_active_process_groups.Get().find(process_group_name);
  if (iter == g_active_process_groups.Get().end() || !iter->second)
    return false;
  return iter->second->GetData().IsHandleValid();
}

// static
void ServiceManagerContext::StartMasterConnection() {
  auto* master_connection = ServiceManagerConnection::GetForProcess();
  RegisterCommonMasterInterfaces(master_connection);
  master_connection->Start();
}

}  // namespace samples
