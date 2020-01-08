// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_SAMPLES_MASTER_CLIENT_H_
#define SAMPLES_PUBLIC_MASTER_SAMPLES_MASTER_CLIENT_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/optional.h"
#include "base/task/task_scheduler/task_scheduler.h"
#include "base/values.h"
#include "build/build_config.h"
#include "samples/public/master/global_request_id.h"
#include "samples/public/common/samples_client.h"
#include "samples/public/common/resource_type.h"
#include "services/service_manager/embedder/embedded_service_info.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/mojom/service.mojom.h"
#include "services/service_manager/sandbox/sandbox_type.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

#if (defined(OS_POSIX) && !defined(OS_MACOSX)) || defined(OS_FUCHSIA)
#include "base/posix/global_descriptors.h"
#endif

#if defined(OS_POSIX) || defined(OS_FUCHSIA)
#include "samples/public/master/posix_file_descriptor_info.h"
#endif

class GURL;

namespace base {
class CommandLine;
class FilePath;
}

namespace mojo {
class ScopedInterfaceEndpointHandle;
}

namespace service_manager {
class Identity;
class Service;
struct BindSourceInfo;
}

namespace sandbox {
class TargetPolicy;
}

namespace samples {

enum class PermissionType;
class AuthenticatorRequestClientDelegate;
class MasterChildProcessHost;
class MasterContext;
class MasterMainParts;
class MasterURLHandler;
class ControllerPresentationServiceDelegate;
class MemoryCoordinatorDelegate;
class QuotaPermissionContext;
class ReceiverPresentationServiceDelegate;
class SlaverProcessHost;
class SlaverViewHost;
class ResourceContext;
class ServiceManagerConnection;
class TracingDelegate;
struct MainFunctionParams;
struct Referrer;

SAMPLES_EXPORT void OverrideOnBindInterface(
    const service_manager::BindSourceInfo& remote_info,
    const std::string& name,
    mojo::ScopedMessagePipeHandle* handle);

// Embedder API (or SPI) for participating in master logic, to be implemented
// by the client of the samples master. See ChromeSamplesMasterClient for the
// principal implementation. The methods are assumed to be called on the UI
// thread unless otherwise specified. Use this "escape hatch" sparingly, to
// avoid the embedder interface ballooning and becoming very specific to Chrome.
// (Often, the call out to the client can happen in a different part of the code
// that either already has a hook out to the embedder, or calls out to one of
// the observer interfaces.)
class SAMPLES_EXPORT SamplesMasterClient {
 public:
  virtual ~SamplesMasterClient() {}

  // Allows the embedder to set any number of custom MasterMainParts
  // implementations for the master startup code. See comments in
  // master_main_parts.h.
  virtual MasterMainParts* CreateMasterMainParts(
      const MainFunctionParams& parameters);

  // Allows the embedder to change the default behavior of
  // MasterThread::PostAfterStartupTask to better match whatever
  // definition of "startup" the embedder has in mind. This may be
  // called on any thread.
  // Note: see related MasterThread::PostAfterStartupTask.
  virtual void PostAfterStartupTask(
      const base::Location& from_here,
      const scoped_refptr<base::TaskRunner>& task_runner,
      base::OnceClosure task);

  // Allows the embedder to indicate whether it considers startup to be
  // complete. May be called on any thread. This should be called on a one-off
  // basis; if you need to poll this function constantly, use the above
  // PostAfterStartupTask() API instead.
  virtual bool IsMasterStartupComplete();

  // Allows the embedder to handle a request from unit tests running in the
  // samples layer to consider startup complete (for the sake of
  // PostAfterStartupTask()).
  virtual void SetMasterStartupIsCompleteForTesting();

  // Notifies that a render process will be created. This is called before
  // the samples layer adds its own MasterMessageFilters, so that the
  // embedder's IPC filters have priority.
  //
  // If the client provides a service request, the samples layer will ask the
  // corresponding embedder renderer-side component to bind it to an
  // implementation at the appropriate moment during initialization.
  virtual void SlaverProcessWillLaunch(
      SlaverProcessHost* host,
      service_manager::mojom::ServiceRequest* service_request) {}

  // Notifies that a MasterChildProcessHost has been created.
  virtual void MasterChildProcessHostCreated(MasterChildProcessHost* host) {}

  // Returns whether all instances of the specified effective URL should be
  // rendered by the same process, rather than using process-per-site-instance.
  virtual bool ShouldUseProcessPerSite(MasterContext* master_context,
                                       const GURL& effective_url);

  // Returns whether a new view for a given |site_url| can be launched in a
  // given |process_host|.
  virtual bool IsSuitableHost(SlaverProcessHost* process_host,
                              const GURL& site_url);

  // Returns whether a new view for a new site instance can be added to a
  // given |process_host|.
  virtual bool MayReuseHost(SlaverProcessHost* process_host);

  // Returns whether a new process should be created or an existing one should
  // be reused based on the URL we want to load. This should return false,
  // unless there is a good reason otherwise.
  virtual bool ShouldTryToUseExistingProcessHost(
      MasterContext* master_context, const GURL& url);

  // Allows the embedder to programmatically control whether the
  // --site-per-process mode of Site Isolation should be used.
  virtual std::vector<url::Origin> GetOriginsRequiringDedicatedProcess();

  // Note that for correctness, the same value should be consistently returned.
  // See also https://crbug.com/825369
  virtual bool ShouldEnableStrictSiteIsolation();

  // Indicates whether a file path should be accessible via file URL given a
  // request from a master context which lives within |profile_path|.
  virtual bool IsFileAccessAllowed(const base::FilePath& path,
                                   const base::FilePath& absolute_path,
                                   const base::FilePath& profile_path);

  // Allows the embedder to pass extra command line flags.
  // switches::kProcessType will already be set at this point.
  virtual void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                              int child_process_id) {}

  // Allows the samples embedder to adjust the command line arguments for
  // a utility process started to run a service. This is called on a background
  // thread.
  virtual void AdjustUtilityServiceProcessCommandLine(
      const service_manager::Identity& identity,
      base::CommandLine* command_line) {}

  // Returns the locale used by the application.
  // This is called on the UI and IO threads.
  virtual std::string GetApplicationLocale();

  // Returns the fully qualified path to the log file name, or an empty path.
  // This function is used by the sandbox to allow write access to the log.
  virtual base::FilePath GetLoggingFileName(
      const base::CommandLine& command_line);

  // Notifies that MasterURLHandler has been created, so that the embedder can
  // optionally add their own handlers.
  virtual void MasterURLHandlerCreated(MasterURLHandler* handler) {}

  // Returns the path to the master shader disk cache root.
  virtual base::FilePath GetShaderDiskCacheDirectory();

  // Returns the path to the shader disk cache root for shaders generated by
  // skia.
  virtual base::FilePath GetGrShaderDiskCacheDirectory();

  // Returns additional allowed scheme set which can access files in
  // FileSystem API.
  virtual void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_schemes) {}

  // |schemes| is a return value parameter that gets a whitelist of schemes that
  // should bypass the Is Privileged Context check.
  // See http://www.w3.org/TR/powerful-features/#settings-privileged
  virtual void GetSchemesBypassingSecureContextCheckWhitelist(
      std::set<std::string>* schemes) {}

  // Generate a Service user-id for the supplied master context. Defaults to
  // returning a random GUID.
  virtual std::string GetServiceUserIdForMasterContext(
      MasterContext* master_context);

  // Allows to register master interfaces exposed through the
  // SlaverProcessHost. Note that interface factory callbacks added to
  // |registry| will by default be run immediately on the IO thread, unless a
  // task runner is provided.
  virtual void ExposeInterfacesToSlaverer(
      service_manager::BinderRegistry* registry,
      blink::AssociatedInterfaceRegistry* associated_registry,
      SlaverProcessHost* render_process_host) {}

  // (Currently called only from GPUProcessHost, move somewhere more central).
  // Called when a request to bind |interface_name| on |interface_pipe| is
  // received from |source_info.identity|. If the request is bound,
  // |interface_pipe| will become invalid (taken by the client).
  virtual void BindInterfaceRequest(
      const service_manager::BindSourceInfo& source_info,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) {}

  using StaticServiceMap =
      std::map<std::string, service_manager::EmbeddedServiceInfo>;

  // Registers services to be loaded in the master process by the Service
  // Manager. |connection| is the ServiceManagerConnection service are
  // registered with.
  virtual void RegisterInProcessServices(StaticServiceMap* services,
                                         ServiceManagerConnection* connection) {
  }

  virtual void OverrideOnBindInterface(
      const service_manager::BindSourceInfo& remote_info,
      const std::string& name,
      mojo::ScopedMessagePipeHandle* handle) {}

  using ProcessNameCallback = base::RepeatingCallback<base::string16()>;

  struct SAMPLES_EXPORT OutOfProcessServiceInfo {
    OutOfProcessServiceInfo();
    OutOfProcessServiceInfo(const ProcessNameCallback& process_name_callback);
    OutOfProcessServiceInfo(const ProcessNameCallback& process_name_callback,
                            const std::string& process_group);
    ~OutOfProcessServiceInfo();

    // The callback function to get the display name of the service process
    // launched for the service.
    ProcessNameCallback process_name_callback;

    // If provided, a string which groups this service into a process shared
    // by other services using the same string.
    base::Optional<std::string> process_group;
  };

  using OutOfProcessServiceMap = std::map<std::string, OutOfProcessServiceInfo>;

  // Registers services to be loaded out of the master process in an
  // utility process. The value of each map entry should be a process name,
  // to use for the service's host process when launched.
  virtual void RegisterOutOfProcessServices(OutOfProcessServiceMap* services) {}

  // Allows the embedder to terminate the master if a specific service instance
  // quits or crashes.
  virtual bool ShouldTerminateOnServiceQuit(
      const service_manager::Identity& id);

  // Allow the embedder to provide a dictionary loaded from a JSON file
  // resembling a service manifest whose capabilities section will be merged
  // with samples's own for |name|. Additional entries will be appended to their
  // respective sections.
  virtual std::unique_ptr<base::Value> GetServiceManifestOverlay(
      base::StringPiece name);

  struct ServiceManifestInfo {
    // The name of the service.
    std::string name;

    // The resource ID of the manifest.
    int resource_id;
  };

  // Allows the embedder to provide extra service manifests to be registered
  // with the service manager context.
  virtual std::vector<ServiceManifestInfo> GetExtraServiceManifests();

  // Allows the embedder to have a list of services started after the
  // in-process Service Manager has been initialized.
  virtual std::vector<service_manager::Identity> GetStartupServices();

  // Populates |mappings| with all files that need to be mapped before launching
  // a child process.
#if (defined(OS_POSIX) && !defined(OS_MACOSX)) || defined(OS_FUCHSIA)
  virtual void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      samples::PosixFileDescriptorInfo* mappings) {}
#endif  // defined(OS_POSIX) && !defined(OS_MACOSX) || defined(OS_FUCHSIA)

  // Provides parameters for initializing the global task scheduler. Default
  // params are used if this returns nullptr.
  virtual std::unique_ptr<base::TaskScheduler::InitParams>
  GetTaskSchedulerInitParams();

  // Returns whether a base::TaskScheduler should be created when
  // MasterMainLoop starts.
  // If false, a task scheduler has been created by the embedder, and
  // MasterMainLoop should skip creating a second one.
  // Note: the embedder should *not* start the TaskScheduler for
  // MasterMainLoop, MasterMainLoop itself is responsible for that.
  virtual bool ShouldCreateTaskScheduler();

  // Creates a new TracingDelegate. The caller owns the returned value.
  // It's valid to return nullptr.
  virtual TracingDelegate* GetTracingDelegate();

};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_SAMPLES_MASTER_CLIENT_H_
