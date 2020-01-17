// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Represents the master side of the master <--> slaverer communication
// channel. There will be one SlaverProcessHost per slaverer process.

#include "samples/master/slaverer_host/slaver_process_host_impl.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/containers/adapters.h"
#include "base/debug/alias.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/feature_list.h"
#include "base/files/file.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "base/memory/shared_memory_handle.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/persistent_histogram_allocator.h"
#include "base/metrics/persistent_memory_allocator.h"
#include "base/metrics/statistics_recorder.h"
#include "base/metrics/user_metrics.h"
#include "base/no_destructor.h"
#include "base/numerics/ranges.h"
#include "base/process/process_handle.h"
#include "base/rand_util.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/supports_user_data.h"
#include "base/synchronization/lock.h"
#include "base/sys_info.h"
#include "base/task/post_task.h"
#include "base/thread_annotations.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/trace_event/memory_allocator_dump.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_provider.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "samples/master/bad_message.h"
#include "samples/master/master_child_process_host_impl.h"
#include "samples/master/master_main.h"
#include "samples/master/master_main_loop.h"
#include "samples/master/child_process_security_policy_impl.h"
#include "samples/common/child_process_host_impl.h"
#include "samples/common/samples_switches_internal.h"
#include "samples/common/in_process_child_thread_params.h"
#include "samples/common/service_manager/child_connection.h"
#include "samples/common/service_manager/service_manager_connection_impl.h"
#include "samples/public/master/master_context.h"
#include "samples/public/master/master_message_filter.h"
#include "samples/public/master/master_task_traits.h"
#include "samples/public/master/notification_service.h"
#include "samples/public/master/notification_types.h"
#include "samples/public/master/samples_master_client.h"
#include "samples/public/master/slaver_process_host_factory.h"
#include "samples/public/master/slaver_process_host_observer.h"
#include "samples/public/master/site_isolation_policy.h"
#include "samples/public/common/bind_interface_helpers.h"
#include "samples/public/common/child_process_host.h"
#include "samples/public/common/connection_filter.h"
#include "samples/public/common/samples_constants.h"
#include "samples/public/common/samples_features.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/process_type.h"
#include "samples/public/common/resource_type.h"
#include "samples/public/common/result_codes.h"
#include "samples/public/common/sandboxed_process_launcher_delegate.h"
#include "samples/public/common/service_names.mojom.h"
#include "samples/public/common/url_constants.h"
#include "ipc/ipc.mojom.h"
#include "ipc/ipc_channel.h"
#include "ipc/ipc_channel_mojo.h"
#include "ipc/ipc_logging.h"
#include "ipc/ipc_message_macros.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/embedder/switches.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "services/service_manager/runner/common/client_util.h"
#include "services/service_manager/runner/common/switches.h"
#include "services/service_manager/sandbox/switches.h"
#include "services/service_manager/zygote/common/zygote_buildflags.h"
#include "third_party/blink/public/common/launching_process_state.h"

#if defined(OS_ANDROID)
#include "samples/public/master/android/java_interfaces.h"
#include "ipc/ipc_sync_channel.h"
#else
#include "samples/master/compositor/image_transport_factory.h"
#endif

#if BUILDFLAG(USE_ZYGOTE_HANDLE)
#include "services/service_manager/zygote/common/zygote_handle.h"  // nogncheck
#endif

namespace samples {

using CheckOriginLockResult =
    ChildProcessSecurityPolicyImpl::CheckOriginLockResult;

namespace {

// Stores the maximum number of slaverer processes the samples module can
// create. Only applies if it is set to a non-zero value.
size_t g_max_slaverer_count_override = 0;

bool g_run_slaverer_in_process = false;

SlavererMainThreadFactoryFunction g_slaverer_main_thread_factory = nullptr;

base::MessageLoop* g_in_process_thread;

const SlaverProcessHostFactory* g_slaver_process_host_factory_ = nullptr;
const char kSiteProcessMapKeyName[] = "samples_site_process_map";

SlaverProcessHost::AnalyzeHungSlavererFunction g_analyze_hung_slaverer =
    nullptr;

// the global list of all slaverer processes
base::LazyInstance<base::IDMap<SlaverProcessHost*>>::Leaky g_all_hosts =
    LAZY_INSTANCE_INITIALIZER;

// Map of site to process, to ensure we only have one SlaverProcessHost per
// site in process-per-site mode.  Each map is specific to a MasterContext.
class SiteProcessMap : public base::SupportsUserData::Data {
 public:
  typedef base::hash_map<std::string, SlaverProcessHost*> SiteToProcessMap;
  SiteProcessMap() {}

  void RegisterProcess(const std::string& site, SlaverProcessHost* process) {
    // There could already exist a site to process mapping due to races between
    // two WebSampless with blank SiteInstances. If that occurs, keeping the
    // exising entry and not overwriting it is a predictable behavior that is
    // safe.
    auto i = map_.find(site);
    if (i == map_.end())
      map_[site] = process;
  }

  SlaverProcessHost* FindProcess(const std::string& site) {
    auto i = map_.find(site);
    if (i != map_.end())
      return i->second;
    return nullptr;
  }

  void RemoveProcess(SlaverProcessHost* host) {
    // Find all instances of this process in the map, then separately remove
    // them.
    std::set<std::string> sites;
    for (SiteToProcessMap::const_iterator i = map_.begin(); i != map_.end();
         ++i) {
      if (i->second == host)
        sites.insert(i->first);
    }
    for (auto i = sites.begin(); i != sites.end(); ++i) {
      auto iter = map_.find(*i);
      if (iter != map_.end()) {
        DCHECK_EQ(iter->second, host);
        map_.erase(iter);
      }
    }
  }

 private:
  SiteToProcessMap map_;
};

// Find the SiteProcessMap specific to the given context.
SiteProcessMap* GetSiteProcessMapForMasterContext(MasterContext* context) {
  DCHECK(context);
  SiteProcessMap* existing_map = static_cast<SiteProcessMap*>(
      context->GetUserData(kSiteProcessMapKeyName));
  if (existing_map)
    return existing_map;

  auto new_map = std::make_unique<SiteProcessMap>();
  auto* new_map_ptr = new_map.get();
  context->SetUserData(kSiteProcessMapKeyName, std::move(new_map));
  return new_map_ptr;
}

// NOTE: changes to this class need to be reviewed by the security team.
class SlavererSandboxedProcessLauncherDelegate
    : public SandboxedProcessLauncherDelegate {
 public:
  SlavererSandboxedProcessLauncherDelegate() {}

  ~SlavererSandboxedProcessLauncherDelegate() override {}

#if BUILDFLAG(USE_ZYGOTE_HANDLE)
  service_manager::ZygoteHandle GetZygote() override {
    const base::CommandLine& master_command_line =
        *base::CommandLine::ForCurrentProcess();
    base::CommandLine::StringType slaverer_prefix =
        master_command_line.GetSwitchValueNative(switches::kSlavererCmdPrefix);
    if (!slaverer_prefix.empty())
      return nullptr;
    return service_manager::GetGenericZygote();
  }
#endif  // BUILDFLAG(USE_ZYGOTE_HANDLE)

  service_manager::SandboxType GetSandboxType() override {
    return service_manager::SANDBOX_TYPE_SLAVERER;
  }
};

// This class manages spare SlaverProcessHosts.
//
// There is a singleton instance of this class which manages a single spare
// slaverer (g_spare_slaver_process_host_manager, below). This class
// encapsulates the implementation of
// SlaverProcessHost::WarmupSpareSlaverProcessHost()
//
// SlaverProcessHostImpl should call
// SpareSlaverProcessHostManager::MaybeTakeSpareSlaverProcessHost when creating
// a new RPH. In this implementation, the spare slaverer is bound to a
// MasterContext and its default StoragePartition. If
// MaybeTakeSpareSlaverProcessHost is called with a MasterContext that does not
// match, the spare slaverer is discarded. Only the default StoragePartition
// will be able to use a spare slaverer. The spare slaverer will also not be
// used as a guest slaverer (is_for_guests_ == true).
//
// It is safe to call WarmupSpareSlaverProcessHost multiple times, although if
// called in a context where the spare slaverer is not likely to be used
// performance may suffer due to the unnecessary RPH creation.
class SpareSlaverProcessHostManager : public SlaverProcessHostObserver {
 public:
  SpareSlaverProcessHostManager() {}

  void WarmupSpareSlaverProcessHost(MasterContext* master_context) {
    if (spare_slaver_process_host_ &&
        spare_slaver_process_host_->GetMasterContext() == master_context) {
      return;  // Nothing to warm up.
    }

    CleanupSpareSlaverProcessHost();

    // Don't create a spare slaverer if we're using --single-process or if we've
    // got too many processes. See also ShouldTryToUseExistingProcessHost in
    // this file.
    if (SlaverProcessHost::run_slaverer_in_process() ||
        g_all_hosts.Get().size() >=
            SlaverProcessHostImpl::GetMaxSlavererProcessCount())
      return;

    spare_slaver_process_host_ = SlaverProcessHostImpl::CreateSlaverProcessHost(
        master_context,
        false /* is_for_guests_only */);
    spare_slaver_process_host_->AddObserver(this);
    spare_slaver_process_host_->Init();
  }

  SlaverProcessHost* MaybeTakeSpareSlaverProcessHost(
      MasterContext* master_context,
      bool is_for_guests_only) {
    // Give embedder a chance to disable using a spare SlaverProcessHost for
    // certain SiteInstances.  Some navigations, such as to NTP or extensions,
    // require passing command-line flags to the slaverer process at process
    // launch time, but this cannot be done for spare SlaverProcessHosts, which
    // are started before it is known which navigation might use them.  So, a
    // spare SlaverProcessHost should not be used in such cases.
    bool embedder_allows_spare_usage = false;

    using SpareProcessMaybeTakeAction =
        SlaverProcessHostImpl::SpareProcessMaybeTakeAction;
    SpareProcessMaybeTakeAction action =
        SpareProcessMaybeTakeAction::kNoSparePresent;
    if (!spare_slaver_process_host_)
      action = SpareProcessMaybeTakeAction::kNoSparePresent;
    else if (master_context != spare_slaver_process_host_->GetMasterContext())
      action = SpareProcessMaybeTakeAction::kMismatchedMasterContext;
    else if (!embedder_allows_spare_usage)
      action = SpareProcessMaybeTakeAction::kRefusedByEmbedder;
    else
      action = SpareProcessMaybeTakeAction::kSpareTaken;
    UMA_HISTOGRAM_ENUMERATION(
        "MasterSlaverProcessHost.SpareProcessMaybeTakeAction", action);

    // Decide whether to take or drop the spare process.
    SlaverProcessHost* returned_process = nullptr;
    if (spare_slaver_process_host_ &&
        master_context == spare_slaver_process_host_->GetMasterContext() &&
        !is_for_guests_only && embedder_allows_spare_usage) {
      CHECK(spare_slaver_process_host_->HostHasNotBeenUsed());

      // If the spare process ends up getting killed, the spare manager should
      // discard the spare RPH, so if one exists, it should always be live here.
      CHECK(spare_slaver_process_host_->IsInitializedAndNotDead());

      DCHECK_EQ(SpareProcessMaybeTakeAction::kSpareTaken, action);
      returned_process = spare_slaver_process_host_;
      ReleaseSpareSlaverProcessHost(spare_slaver_process_host_);
    } else if (!SlaverProcessHostImpl::IsSpareProcessKeptAtAllTimes()) {
      // If the spare shouldn't be kept around, then discard it as soon as we
      // find that the current spare was mismatched.
      CleanupSpareSlaverProcessHost();
    } else if (g_all_hosts.Get().size() >=
               SlaverProcessHostImpl::GetMaxSlavererProcessCount()) {
      // Drop the spare if we are at a process limit and the spare wasn't taken.
      // This helps avoid process reuse.
      CleanupSpareSlaverProcessHost();
    }

    return returned_process;
  }

  // Prepares for future requests (with an assumption that a future navigation
  // might require a new process for |master_context|).
  //
  // Note that depending on the caller PrepareForFutureRequests can be called
  // after the spare_slaver_process_host_ has either been 1) matched and taken
  // or 2) mismatched and ignored or 3) matched and ignored.
  void PrepareForFutureRequests(MasterContext* master_context) {
    if (SlaverProcessHostImpl::IsSpareProcessKeptAtAllTimes()) {
      // Always keep around a spare process for the most recently requested
      // |master_context|.
      WarmupSpareSlaverProcessHost(master_context);
    } else {
      // Discard the ignored (probably non-matching) spare so as not to waste
      // resources.
      CleanupSpareSlaverProcessHost();
    }
  }

  // Gracefully remove and cleanup a spare SlaverProcessHost if it exists.
  void CleanupSpareSlaverProcessHost() {
    if (spare_slaver_process_host_) {
      // Stop observing the process, to avoid getting notifications as a
      // consequence of the Cleanup call below - such notification could call
      // back into CleanupSpareSlaverProcessHost leading to stack overflow.
      spare_slaver_process_host_->RemoveObserver(this);

      // Make sure the SlaverProcessHost object gets destroyed.
      if (!spare_slaver_process_host_->IsKeepAliveRefCountDisabled())
        spare_slaver_process_host_->Cleanup();

      // Drop reference to the SlaverProcessHost object.
      spare_slaver_process_host_ = nullptr;
    }
  }

  SlaverProcessHost* spare_slaver_process_host() {
    return spare_slaver_process_host_;
  }

 private:
  // Release ownership of |host| as a possible spare slaverer.  Called when
  // |host| has either been 1) claimed to be used in a navigation or 2) shutdown
  // somewhere else.
  void ReleaseSpareSlaverProcessHost(SlaverProcessHost* host) {
    if (spare_slaver_process_host_ && spare_slaver_process_host_ == host) {
      spare_slaver_process_host_->RemoveObserver(this);
      spare_slaver_process_host_ = nullptr;
    }
  }

  void SlaverProcessExited(SlaverProcessHost* host,
                           const ChildProcessTerminationInfo& info) override {
    if (host == spare_slaver_process_host_)
      CleanupSpareSlaverProcessHost();
  }

  void SlaverProcessHostDestroyed(SlaverProcessHost* host) override {
    ReleaseSpareSlaverProcessHost(host);
  }

  // This is a bare pointer, because SlaverProcessHost manages the lifetime of
  // all its instances; see g_all_hosts, above.
  SlaverProcessHost* spare_slaver_process_host_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SpareSlaverProcessHostManager);
};

base::LazyInstance<SpareSlaverProcessHostManager>::Leaky
    g_spare_slaver_process_host_manager = LAZY_INSTANCE_INITIALIZER;

const void* const kDefaultSubframeProcessHostHolderKey =
    &kDefaultSubframeProcessHostHolderKey;

class DefaultSubframeProcessHostHolder : public base::SupportsUserData::Data,
                                         public SlaverProcessHostObserver {
 public:
  explicit DefaultSubframeProcessHostHolder(MasterContext* master_context)
      : master_context_(master_context) {}
  ~DefaultSubframeProcessHostHolder() override {}

  // Gets the correct slaver process to use for this SiteInstance.
  SlaverProcessHost* GetProcessHost(bool is_for_guests_only) {
    // Is this the default storage partition? If it isn't, then just give it its
    // own non-shared process.
    if (is_for_guests_only) {
      SlaverProcessHost* host = SlaverProcessHostImpl::CreateSlaverProcessHost(
          master_context_, is_for_guests_only);
      host->SetIsNeverSuitableForReuse();
      return host;
    }

    // If we already have a shared host for the default storage partition, use
    // it.
    if (host_)
      return host_;

    host_ = SlaverProcessHostImpl::CreateSlaverProcessHost(
        master_context_,
        false /* is for guests only */);
    host_->SetIsNeverSuitableForReuse();
    host_->AddObserver(this);

    return host_;
  }

  // Implementation of SlaverProcessHostObserver.
  void SlaverProcessHostDestroyed(SlaverProcessHost* host) override {
    DCHECK_EQ(host_, host);
    host_->RemoveObserver(this);
    host_ = nullptr;
  }

 private:
  MasterContext* master_context_;

  // The default subframe slaver process used for the default storage partition
  // of this MasterContext.
  SlaverProcessHost* host_ = nullptr;
};

// Forwards service requests to Service Manager since the slaverer cannot launch
// out-of-process services on is own.
template <typename Interface>
void ForwardRequest(const char* service_name,
                    mojo::InterfaceRequest<Interface> request) {
  // TODO(beng): This should really be using the per-profile connector.
  service_manager::Connector* connector =
      ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface(service_name, std::move(request));
}

class SlaverProcessHostIsReadyObserver : public SlaverProcessHostObserver {
 public:
  SlaverProcessHostIsReadyObserver(SlaverProcessHost* slaver_process_host,
                                   base::OnceClosure task)
      : slaver_process_host_(slaver_process_host),
        task_(std::move(task)),
        weak_factory_(this) {
    slaver_process_host_->AddObserver(this);
    if (slaver_process_host_->IsReady())
      PostTask();
  }

  ~SlaverProcessHostIsReadyObserver() override {
    slaver_process_host_->RemoveObserver(this);
  }

  void SlaverProcessReady(SlaverProcessHost* host) override { PostTask(); }

  void SlaverProcessHostDestroyed(SlaverProcessHost* host) override {
    delete this;
  }

 private:
  void PostTask() {
    base::PostTaskWithTraits(
        FROM_HERE, {MasterThread::UI},
        base::BindOnce(&SlaverProcessHostIsReadyObserver::CallTask,
                       weak_factory_.GetWeakPtr()));
  }

  void CallTask() {
    DCHECK_CURRENTLY_ON(MasterThread::UI);
    if (slaver_process_host_->IsReady())
      std::move(task_).Run();

    delete this;
  }

  SlaverProcessHost* slaver_process_host_;
  base::OnceClosure task_;
  base::WeakPtrFactory<SlaverProcessHostIsReadyObserver> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SlaverProcessHostIsReadyObserver);
};

void CopyFeatureSwitch(const base::CommandLine& src,
                       base::CommandLine* dest,
                       const char* switch_name) {
  std::vector<std::string> features = FeaturesFromSwitch(src, switch_name);
  if (!features.empty())
    dest->AppendSwitchASCII(switch_name, base::JoinString(features, ","));
}

}  // namespace

// Held by the RPH and used to control an (unowned) ConnectionFilterImpl from
// any thread.
class SlaverProcessHostImpl::ConnectionFilterController
    : public base::RefCountedThreadSafe<ConnectionFilterController> {
 public:
  // |filter| is not owned by this object.
  explicit ConnectionFilterController(ConnectionFilterImpl* filter)
      : filter_(filter) {}

  void DisableFilter();

 private:
  friend class base::RefCountedThreadSafe<ConnectionFilterController>;
  friend class ConnectionFilterImpl;

  ~ConnectionFilterController() {}

  void Detach() {
    base::AutoLock lock(lock_);
    filter_ = nullptr;
  }

  base::Lock lock_;
  ConnectionFilterImpl* filter_ PT_GUARDED_BY(lock_);
};

// Held by the RPH's MasterContext's ServiceManagerConnection, ownership
// transferred back to RPH upon RPH destruction.
class SlaverProcessHostImpl::ConnectionFilterImpl : public ConnectionFilter {
 public:
  ConnectionFilterImpl(
      const service_manager::Identity& child_identity,
      std::unique_ptr<service_manager::BinderRegistry> registry)
      : child_identity_(child_identity),
        registry_(std::move(registry)),
        controller_(new ConnectionFilterController(this)),
        weak_factory_(this) {
    // Registration of this filter may race with master shutdown, in which case
    // it's possible for this filter to be destroyed on the main thread. This
    // is fine as long as the filter hasn't been used on the IO thread yet. We
    // detach the ThreadChecker initially and the first use of the filter will
    // bind it.
    thread_checker_.DetachFromThread();
  }

  ~ConnectionFilterImpl() override {
    DCHECK(thread_checker_.CalledOnValidThread());
    controller_->Detach();
  }

  scoped_refptr<ConnectionFilterController> controller() { return controller_; }

  void Disable() {
    base::AutoLock lock(enabled_lock_);
    enabled_ = false;
  }

 private:
  // ConnectionFilter:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle* interface_pipe,
                       service_manager::Connector* connector) override {
    DCHECK(thread_checker_.CalledOnValidThread());
    DCHECK_CURRENTLY_ON(MasterThread::IO);
    // We only fulfill connections from the slaverer we host.
    if (child_identity_.name() != source_info.identity.name() ||
        child_identity_.instance() != source_info.identity.instance()) {
      return;
    }

    base::AutoLock lock(enabled_lock_);
    if (!enabled_)
      return;

    registry_->TryBindInterface(interface_name, interface_pipe);
  }

  base::ThreadChecker thread_checker_;
  service_manager::Identity child_identity_;
  std::unique_ptr<service_manager::BinderRegistry> registry_;
  scoped_refptr<ConnectionFilterController> controller_;

  base::Lock enabled_lock_;
  bool enabled_ GUARDED_BY(enabled_lock_) = true;

  base::WeakPtrFactory<ConnectionFilterImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionFilterImpl);
};

void SlaverProcessHostImpl::ConnectionFilterController::DisableFilter() {
  base::AutoLock lock(lock_);
  if (filter_)
    filter_->Disable();
}

base::MessageLoop*
SlaverProcessHostImpl::GetInProcessSlavererThreadForTesting() {
  return g_in_process_thread;
}

// static
size_t SlaverProcessHost::GetMaxSlavererProcessCount() {
  if (g_max_slaverer_count_override)
    return g_max_slaverer_count_override;

#if defined(OS_ANDROID)
  // On Android we don't maintain a limit of slaverer process hosts - we are
  // happy with keeping a lot of these, as long as the number of live slaverer
  // processes remains reasonable, and on Android the OS takes care of that.
  return std::numeric_limits<size_t>::max();
#endif
#if defined(OS_CHROMEOS)
  // On Chrome OS new slaverer processes are very cheap and there's no OS
  // driven constraint on the number of processes, and the effectiveness
  // of the tab discarder is very poor when we have tabs sharing a
  // slaverer process.  So, set a high limit, and based on UMA stats
  // for CrOS the 99.9th percentile of Tabs.MaxTabsInADay is around 100.
  return 100;
#endif

  // On other platforms, calculate the maximum number of slaverer process hosts
  // according to the amount of installed memory as reported by the OS, along
  // with some hard-coded limits. The calculation assumes that the slaverers
  // will use up to half of the installed RAM and assumes that each WebSampless
  // uses |kEstimatedWebSamplessMemoryUsage| MB. If this assumption changes, the
  // ThirtyFourTabs test needs to be adjusted to match the expected number of
  // processes.
  //
  // Using the above assumptions, with the given amounts of installed memory
  // below on a 64-bit CPU, the maximum slaverer count based on available RAM
  // alone will be as follows:
  //
  //   128 MB -> 1
  //   512 MB -> 4
  //  1024 MB -> 8
  //  4096 MB -> 34
  // 16384 MB -> 136
  //
  // Then the calculated value will be clamped by |kMinSlavererProcessCount| and
  // |kMaxSlavererProcessCount|.

  static size_t max_count = 0;
  if (!max_count) {
    static constexpr size_t kEstimatedWebSamplessMemoryUsage =
#if defined(ARCH_CPU_64_BITS)
        60;  // In MB
#else
        40;  // In MB
#endif
    max_count = base::SysInfo::AmountOfPhysicalMemoryMB() / 2;
    max_count /= kEstimatedWebSamplessMemoryUsage;

    static constexpr size_t kMinSlavererProcessCount = 3;
    max_count = base::ClampToRange(max_count, kMinSlavererProcessCount,
                                   kMaxSlavererProcessCount);
  }
  return max_count;
}

// static
void SlaverProcessHost::SetMaxSlavererProcessCount(size_t count) {
  g_max_slaverer_count_override = count;
  if (g_all_hosts.Get().size() > count)
    g_spare_slaver_process_host_manager.Get().CleanupSpareSlaverProcessHost();
}

// static
int SlaverProcessHost::GetCurrentSlaverProcessCountForTesting() {
  SlaverProcessHost::iterator it = SlaverProcessHost::AllHostsIterator();
  int count = 0;
  while (!it.IsAtEnd()) {
    SlaverProcessHost* host = it.GetCurrentValue();
    if (host->IsInitializedAndNotDead() &&
        host != SlaverProcessHostImpl::GetSpareSlaverProcessHostForTesting()) {
      count++;
    }
    it.Advance();
  }
  return count;
}

// static
SlaverProcessHost* SlaverProcessHostImpl::CreateSlaverProcessHost(
    MasterContext* master_context,
    bool is_for_guests_only) {
  if (g_slaver_process_host_factory_) {
    return g_slaver_process_host_factory_->CreateSlaverProcessHost(
        master_context);
  }

  return new SlaverProcessHostImpl(master_context,
                                   is_for_guests_only);
}

// static
const unsigned int SlaverProcessHostImpl::kMaxFrameDepthForPriority =
    std::numeric_limits<unsigned int>::max();

SlaverProcessHostImpl::SlaverProcessHostImpl(
    MasterContext* master_context,
    bool is_for_guests_only)
    : fast_shutdown_started_(false),
      deleting_soon_(false),
#ifndef NDEBUG
      is_self_deleted_(false),
#endif
      pending_views_(0),
      keep_alive_ref_count_(0),
      is_keep_alive_ref_count_disabled_(false),
      route_provider_binding_(this),
      visible_clients_(0),
      priority_(!blink::kLaunchingProcessIsBackgrounded,
                frame_depth_,
                false /* intersects_viewport */,
#if defined(OS_ANDROID)
                ChildProcessImportance::NORMAL
#endif
                ),
      id_(ChildProcessHostImpl::GenerateChildProcessUniqueId()),
      master_context_(master_context),
      sudden_termination_allowed_(true),
      ignore_input_events_(false),
      is_for_guests_only_(is_for_guests_only),
      is_unused_(true),
      delayed_cleanup_needed_(false),
      within_process_died_observer_(false),
      channel_connected_(false),
      sent_slaver_process_ready_(false),
#if defined(OS_ANDROID)
      never_signaled_(base::WaitableEvent::ResetPolicy::MANUAL,
                      base::WaitableEvent::InitialState::NOT_SIGNALED),
#endif
      slaverer_host_binding_(this),
      instance_weak_factory_(
          new base::WeakPtrFactory<SlaverProcessHostImpl>(this)),
      weak_factory_(this) {
  for (size_t i = 0; i < kNumKeepAliveClients; i++)
    keep_alive_client_count_[i] = 0;

  ChildProcessSecurityPolicyImpl::GetInstance()->Add(GetID());

  CHECK(!MasterMainRunner::ExitedMainMessageLoop());
  RegisterHost(GetID(), this);
  g_all_hosts.Get().set_check_on_null_data(true);
  // Initialize |child_process_activity_time_| to a reasonable value.
  mark_child_process_activity_time();

  InitializeChannelProxy();
}

// static
void SlaverProcessHostImpl::ShutDownInProcessSlaverer() {
  DCHECK(g_run_slaverer_in_process);

  switch (g_all_hosts.Pointer()->size()) {
    case 0:
      return;
    case 1: {
      SlaverProcessHostImpl* host = static_cast<SlaverProcessHostImpl*>(
          AllHostsIterator().GetCurrentValue());
      for (auto& observer : host->observers_)
        observer.SlaverProcessHostDestroyed(host);
#ifndef NDEBUG
      host->is_self_deleted_ = true;
#endif
      delete host;
      return;
    }
    default:
      NOTREACHED() << "There should be only one SlaverProcessHost when running "
                   << "in-process.";
  }
}

void SlaverProcessHostImpl::RegisterSlavererMainThreadFactory(
    SlavererMainThreadFactoryFunction create) {
  g_slaverer_main_thread_factory = create;
}

SlaverProcessHostImpl::~SlaverProcessHostImpl() {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
#ifndef NDEBUG
  DCHECK(is_self_deleted_)
      << "SlaverProcessHostImpl is destroyed by something other than itself";
#endif

  // Make sure to clean up the in-process slaverer before the channel, otherwise
  // it may still run and have its IPCs fail, causing asserts.
  in_process_slaverer_.reset();

  ChildProcessSecurityPolicyImpl::GetInstance()->Remove(GetID());

  is_dead_ = true;

  UnregisterHost(GetID());
}

bool SlaverProcessHostImpl::Init() {
  // calling Init() more than once does nothing, this makes it more convenient
  // for the view host which may not be sure in some cases
  if (IsInitializedAndNotDead())
    return true;

  base::CommandLine::StringType slaverer_prefix;
  // A command prefix is something prepended to the command line of the spawned
  // process.
  const base::CommandLine& master_command_line =
      *base::CommandLine::ForCurrentProcess();
  slaverer_prefix =
      master_command_line.GetSwitchValueNative(switches::kSlavererCmdPrefix);

#if defined(OS_LINUX)
  int flags = slaverer_prefix.empty() ? ChildProcessHost::CHILD_ALLOW_SELF
                                      : ChildProcessHost::CHILD_NORMAL;
#else
  int flags = ChildProcessHost::CHILD_NORMAL;
#endif

  // Find the slaverer before creating the channel so if this fails early we
  // return without creating the channel.
  base::FilePath slaverer_path = ChildProcessHost::GetChildPath(flags);
  if (slaverer_path.empty())
    return false;

  is_initialized_ = true;
  is_dead_ = false;
  sent_slaver_process_ready_ = false;

  // We may reach Init() during process death notification (e.g.
  // SlaverProcessExited on some observer). In this case the Channel may be
  // null, so we re-initialize it here.
  if (!channel_)
    InitializeChannelProxy();

  // Unpause the Channel briefly. This will be paused again below if we launch a
  // real child process. Note that messages may be sent in the short window
  // between now and then (e.g. in response to SlaverProcessWillLaunch) and we
  // depend on those messages being sent right away.
  //
  // |channel_| must always be non-null here: either it was initialized in
  // the constructor, or in the most recent call to ProcessDied().
  channel_->Unpause(false /* flush */);

  // Call the embedder first so that their IPC filters have priority.
  service_manager::mojom::ServiceRequest service_request;
  GetSamplesClient()->master()->SlaverProcessWillLaunch(this,
                                                         &service_request);
  if (service_request.is_pending()) {
    GetSlavererInterface()->CreateEmbedderSlavererService(
        std::move(service_request));
  }

  CreateMessageFilters();
  RegisterMojoInterfaces();

  if (run_slaverer_in_process()) {
    DCHECK(g_slaverer_main_thread_factory);
    // Crank up a thread and run the initialization there.  With the way that
    // messages flow between the master and slaverer, this thread is required
    // to prevent a deadlock in single-process mode.  Since the primordial
    // thread in the slaverer process runs the WebKit code and can sometimes
    // make blocking calls to the UI thread (i.e. this thread), they need to run
    // on separate threads.
    in_process_slaverer_.reset(
        g_slaverer_main_thread_factory(InProcessChildThreadParams(
            base::CreateSingleThreadTaskRunnerWithTraits({MasterThread::IO}),
            &mojo_invitation_, child_connection_->service_token())));

    base::Thread::Options options;
    // We can't have multiple UI loops on Linux and Android, so we don't support
    // in-process plugins.
    options.message_loop_type = base::MessageLoop::TYPE_DEFAULT;
    // As for execution sequence, this callback should have no any dependency
    // on starting in-process-slaver-thread.
    // So put it here to trigger ChannelMojo initialization earlier to enable
    // in-process-slaver-thread using ChannelMojo there.
    OnProcessLaunched();  // Fake a callback that the process is ready.

    in_process_slaverer_->StartWithOptions(options);

    g_in_process_thread = in_process_slaverer_->message_loop();

    // Make sure any queued messages on the channel are flushed in the case
    // where we aren't launching a child process.
    channel_->Flush();
  } else {
    // Build command line for slaverer.  We call AppendSlavererCommandLine()
    // first so the process type argument will appear first.
    std::unique_ptr<base::CommandLine> cmd_line =
        std::make_unique<base::CommandLine>(slaverer_path);
    if (!slaverer_prefix.empty())
      cmd_line->PrependWrapper(slaverer_prefix);
    AppendSlavererCommandLine(cmd_line.get());

    // Spawn the child process asynchronously to avoid blocking the UI thread.
    // As long as there's no slaverer prefix, we can use the zygote process
    // at this stage.
    child_process_launcher_ = std::make_unique<ChildProcessLauncher>(
        std::make_unique<SlavererSandboxedProcessLauncherDelegate>(),
        std::move(cmd_line), GetID(), this, std::move(mojo_invitation_),
        base::BindRepeating(&SlaverProcessHostImpl::OnMojoError, id_));
    channel_->Pause();

    fast_shutdown_started_ = false;
  }

  init_time_ = base::TimeTicks::Now();
  return true;
}

void SlaverProcessHostImpl::EnableSendQueue() {
  if (!channel_)
    InitializeChannelProxy();
}

void SlaverProcessHostImpl::InitializeChannelProxy() {
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner =
      base::CreateSingleThreadTaskRunnerWithTraits({MasterThread::IO});

  // Acquire a Connector which will route connections to a new instance of the
  // slaverer service.
  service_manager::Connector* connector =
      MasterContext::GetConnectorFor(master_context_);
  if (!connector) {
    // Note that some embedders (e.g. Android WebView) may not initialize a
    // Connector per MasterContext. In those cases we fall back to the
    // master-wide Connector.
    if (!ServiceManagerConnection::GetForProcess()) {
      // Additionally, some test code may not initialize the process-wide
      // ServiceManagerConnection prior to this point. This class of test code
      // doesn't care about slaver processes, so we can initialize a dummy
      // connection.
      ServiceManagerConnection::SetForProcess(ServiceManagerConnection::Create(
          mojo::MakeRequest(&test_service_), io_task_runner));
    }
    connector = ServiceManagerConnection::GetForProcess()->GetConnector();
  }

  // Establish a ServiceManager connection for the new slaver service instance.
  mojo_invitation_ = {};
  service_manager::Identity child_identity(
      mojom::kSlavererServiceName,
      MasterContext::GetServiceUserIdFor(GetMasterContext()),
      base::StringPrintf("%d_%d", id_, instance_id_++));
  child_connection_ = std::make_unique<ChildConnection>(
      child_identity, &mojo_invitation_, connector, io_task_runner);

  // Send an interface request to bootstrap the IPC::Channel. Note that this
  // request will happily sit on the pipe until the process is launched and
  // connected to the ServiceManager. We take the other end immediately and
  // plug it into a new ChannelProxy.
  mojo::MessagePipe pipe;
  BindInterface(IPC::mojom::ChannelBootstrap::Name_, std::move(pipe.handle1));
  std::unique_ptr<IPC::ChannelFactory> channel_factory =
      IPC::ChannelMojo::CreateServerFactory(
          std::move(pipe.handle0), io_task_runner,
          base::ThreadTaskRunnerHandle::Get());

  samples::BindInterface(this, &child_control_interface_);

  ResetChannelProxy();

  // Do NOT expand ifdef or run time condition checks here! Synchronous
  // IPCs from master process are banned. It is only narrowly allowed
  // for Android WebView to maintain backward compatibility.
  // See crbug.com/526842 for details.
#if defined(OS_ANDROID)
  if (GetSamplesClient()->UsingSynchronousCompositing()) {
    channel_ = IPC::SyncChannel::Create(this, io_task_runner.get(),
                                        base::ThreadTaskRunnerHandle::Get(),
                                        &never_signaled_);
  }
#endif  // OS_ANDROID
  if (!channel_) {
    channel_ = std::make_unique<IPC::ChannelProxy>(
        this, io_task_runner.get(), base::ThreadTaskRunnerHandle::Get());
  }
  channel_->Init(std::move(channel_factory), true /* create_pipe_now */);

  // Note that Channel send is effectively paused and unpaused at various points
  // during startup, and existing code relies on a fragile relative message
  // ordering resulting from some early messages being queued until process
  // launch while others are sent immediately. See https://goo.gl/REW75h for
  // details.
  //
  // We acquire a few associated interface proxies here -- before the channel is
  // paused -- to ensure that subsequent initialization messages on those
  // interfaces behave properly. Specifically, this avoids the risk of an
  // interface being requested while the Channel is paused, which could
  // effectively and undesirably block the transmission of a subsequent message
  // on that interface while the Channel is unpaused.
  //
  // See OnProcessLaunched() for some additional details of this somewhat
  // surprising behavior.
  channel_->GetRemoteAssociatedInterface(&remote_route_provider_);
  channel_->GetRemoteAssociatedInterface(&slaverer_interface_);

  // We start the Channel in a paused state. It will be briefly unpaused again
  // in Init() if applicable, before process launch is initiated.
  channel_->Pause();
}

void SlaverProcessHostImpl::ResetChannelProxy() {
  if (!channel_)
    return;

  channel_.reset();
  channel_connected_ = false;
}

void SlaverProcessHostImpl::CreateMessageFilters() {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
}

void SlaverProcessHostImpl::CancelProcessShutdownDelayForUnload() {
  if (IsKeepAliveRefCountDisabled())
    return;
  DecrementKeepAliveRefCount(SlaverProcessHost::KeepAliveClientType::kUnload);
}

void SlaverProcessHostImpl::DelayProcessShutdownForUnload(
    const base::TimeDelta& timeout) {
  // No need to delay shutdown if the process is already shutting down.
  if (IsKeepAliveRefCountDisabled() || deleting_soon_ || fast_shutdown_started_)
    return;

  IncrementKeepAliveRefCount(SlaverProcessHost::KeepAliveClientType::kUnload);
  base::PostDelayedTaskWithTraits(
      FROM_HERE, {MasterThread::UI},
      base::BindOnce(
          &SlaverProcessHostImpl::CancelProcessShutdownDelayForUnload,
          weak_factory_.GetWeakPtr()),
      timeout);
}

void SlaverProcessHostImpl::RegisterMojoInterfaces() {
  auto registry = std::make_unique<service_manager::BinderRegistry>();

  associated_interfaces_ =
      std::make_unique<blink::AssociatedInterfaceRegistry>();
  blink::AssociatedInterfaceRegistry* associated_registry =
      associated_interfaces_.get();
  associated_registry->AddInterface(base::Bind(
      &SlaverProcessHostImpl::BindRouteProvider, base::Unretained(this)));
  associated_registry->AddInterface(base::Bind(
      &SlaverProcessHostImpl::CreateSlavererHost, base::Unretained(this)));

  // ---- Please do not register interfaces below this line ------
  //
  // This call should be done after registering all interfaces above, so that
  // embedder can override any interfaces. The fact that registry calls
  // the last registration for the name allows us to easily override interfaces.
  GetSamplesClient()->master()->ExposeInterfacesToSlaverer(
      registry.get(), associated_interfaces_.get(), this);

  ServiceManagerConnection* service_manager_connection =
      MasterContext::GetServiceManagerConnectionFor(master_context_);
  if (connection_filter_id_ !=
      ServiceManagerConnection::kInvalidConnectionFilterId) {
    connection_filter_controller_->DisableFilter();
    service_manager_connection->RemoveConnectionFilter(connection_filter_id_);
  }
  std::unique_ptr<ConnectionFilterImpl> connection_filter =
      std::make_unique<ConnectionFilterImpl>(
          child_connection_->child_identity(), std::move(registry));
  connection_filter_controller_ = connection_filter->controller();
  connection_filter_id_ = service_manager_connection->AddConnectionFilter(
      std::move(connection_filter));
}

void SlaverProcessHostImpl::BindRouteProvider(
    mojom::RouteProviderAssociatedRequest request) {
  if (route_provider_binding_.is_bound())
    return;
  route_provider_binding_.Bind(std::move(request));
}

void SlaverProcessHostImpl::GetRoute(
    int32_t routing_id,
    blink::mojom::AssociatedInterfaceProviderAssociatedRequest request) {
  DCHECK(request.is_pending());
  associated_interface_provider_bindings_.AddBinding(
      this, std::move(request), routing_id);
}

void SlaverProcessHostImpl::GetAssociatedInterface(
    const std::string& name,
    blink::mojom::AssociatedInterfaceAssociatedRequest request) {
  int32_t routing_id =
      associated_interface_provider_bindings_.dispatch_context();
  IPC::Listener* listener = listeners_.Lookup(routing_id);
  if (listener)
    listener->OnAssociatedInterfaceRequest(name, request.PassHandle());
}

void SlaverProcessHostImpl::CreateSlavererHost(
    mojom::SlavererHostAssociatedRequest request) {
  slaverer_host_binding_.Bind(std::move(request));
}

void SlaverProcessHostImpl::BindInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  child_connection_->BindInterface(interface_name, std::move(interface_pipe));
}

const service_manager::Identity& SlaverProcessHostImpl::GetChildIdentity()
    const {
  // GetChildIdentity should only be called if the RPH is (or soon will be)
  // backed by an actual slaverer process.  This helps prevent leaks similar to
  // the ones raised in https://crbug.com/813045.
  DCHECK(IsInitializedAndNotDead());

  return child_connection_->child_identity();
}

bool SlaverProcessHostImpl::IsProcessBackgrounded() const {
  return priority_.is_background();
}

void SlaverProcessHostImpl::IncrementKeepAliveRefCount(
    SlaverProcessHost::KeepAliveClientType client) {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
  DCHECK(!is_keep_alive_ref_count_disabled_);
  base::TimeTicks now = base::TimeTicks::Now();
  size_t client_type = static_cast<size_t>(client);
  keep_alive_client_count_[client_type]++;
  if (keep_alive_client_count_[client_type] == 1)
    keep_alive_client_start_time_[client_type] = now;

  ++keep_alive_ref_count_;
  if (keep_alive_ref_count_ == 1) {
  }
}

void SlaverProcessHostImpl::DecrementKeepAliveRefCount(
    SlaverProcessHost::KeepAliveClientType client) {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
  DCHECK(!is_keep_alive_ref_count_disabled_);
  DCHECK_GT(keep_alive_ref_count_, 0U);
  base::TimeTicks now = base::TimeTicks::Now();
  size_t client_type = static_cast<size_t>(client);
  keep_alive_client_count_[client_type]--;
  if (keep_alive_client_count_[client_type] == 0) {
    RecordKeepAliveDuration(client, keep_alive_client_start_time_[client_type],
                            now);
  }

  --keep_alive_ref_count_;
  if (keep_alive_ref_count_ == 0) {
    Cleanup();
  }
}

void SlaverProcessHostImpl::RecordKeepAliveDuration(
    SlaverProcessHost::KeepAliveClientType client,
    base::TimeTicks start,
    base::TimeTicks end) {
  switch (client) {
    case SlaverProcessHost::KeepAliveClientType::kServiceWorker:
      UMA_HISTOGRAM_LONG_TIMES(
          "MasterSlaverProcessHost.KeepAliveDuration.ServiceWorker",
          end - start);
      break;
    case SlaverProcessHost::KeepAliveClientType::kSharedWorker:
      UMA_HISTOGRAM_LONG_TIMES(
          "MasterSlaverProcessHost.KeepAliveDuration.SharedWorker",
          end - start);
      break;
    case SlaverProcessHost::KeepAliveClientType::kFetch:
      UMA_HISTOGRAM_LONG_TIMES(
          "MasterSlaverProcessHost.KeepAliveDuration.Fetch", end - start);
      break;
    case SlaverProcessHost::KeepAliveClientType::kUnload:
      UMA_HISTOGRAM_LONG_TIMES(
          "MasterSlaverProcessHost.KeepAliveDuration.Unload", end - start);
      break;
  }
}

void SlaverProcessHostImpl::DisableKeepAliveRefCount() {
  DCHECK_CURRENTLY_ON(MasterThread::UI);

  if (is_keep_alive_ref_count_disabled_)
    return;
  is_keep_alive_ref_count_disabled_ = true;

  keep_alive_ref_count_ = 0;
  base::TimeTicks now = base::TimeTicks::Now();
  for (size_t i = 0; i < kNumKeepAliveClients; i++) {
    if (keep_alive_client_count_[i] > 0) {
      RecordKeepAliveDuration(
          static_cast<SlaverProcessHost::KeepAliveClientType>(i),
          keep_alive_client_start_time_[i], now);
      keep_alive_client_count_[i] = 0;
    }
  }

  // Cleaning up will also remove this from the SpareSlaverProcessHostManager.
  // (in this case |keep_alive_ref_count_| would be 0 even before).
  Cleanup();
}

bool SlaverProcessHostImpl::IsKeepAliveRefCountDisabled() {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
  return is_keep_alive_ref_count_disabled_;
}

void SlaverProcessHostImpl::PurgeAndSuspend() {
  GetSlavererInterface()->ProcessPurgeAndSuspend();
}

void SlaverProcessHostImpl::Resume() {}

mojom::Slaverer* SlaverProcessHostImpl::GetSlavererInterface() {
  return slaverer_interface_.get();
}

void SlaverProcessHostImpl::SetIsNeverSuitableForReuse() {
  is_never_suitable_for_reuse_ = true;
}

bool SlaverProcessHostImpl::MayReuseHost() {
  if (is_never_suitable_for_reuse_)
    return false;

  return GetSamplesClient()->master()->MayReuseHost(this);
}

bool SlaverProcessHostImpl::IsUnused() {
  return is_unused_;
}

void SlaverProcessHostImpl::SetIsUsed() {
  is_unused_ = false;
}

mojom::RouteProvider* SlaverProcessHostImpl::GetRemoteRouteProvider() {
  return remote_route_provider_.get();
}

void SlaverProcessHostImpl::AddRoute(int32_t routing_id,
                                     IPC::Listener* listener) {
  CHECK(!listeners_.Lookup(routing_id)) << "Found Routing ID Conflict: "
                                        << routing_id;
  listeners_.AddWithID(listener, routing_id);
}

void SlaverProcessHostImpl::RemoveRoute(int32_t routing_id) {
  DCHECK(listeners_.Lookup(routing_id) != nullptr);
  listeners_.Remove(routing_id);
  Cleanup();
}

void SlaverProcessHostImpl::AddObserver(SlaverProcessHostObserver* observer) {
  observers_.AddObserver(observer);
}

void SlaverProcessHostImpl::RemoveObserver(
    SlaverProcessHostObserver* observer) {
  observers_.RemoveObserver(observer);
}

void SlaverProcessHostImpl::ShutdownForBadMessage(
    CrashReportMode crash_report_mode) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kDisableKillAfterBadIPC))
    return;

  if (run_slaverer_in_process()) {
    // In single process mode it is better if we don't suicide but just
    // crash.
    CHECK(false);
  }

  // We kill the slaverer but don't include a NOTREACHED, because we want the
  // master to try to survive when it gets illegal messages from the slaverer.
  Shutdown(RESULT_CODE_KILLED_BAD_MESSAGE);

  if (crash_report_mode == CrashReportMode::GENERATE_CRASH_DUMP) {
    // Set crash keys to understand slaverer kills related to site isolation.
    auto* policy = ChildProcessSecurityPolicyImpl::GetInstance();
    std::string lock_url = policy->GetOriginLock(GetID()).spec();
    base::debug::SetCrashKeyString(bad_message::GetKilledProcessOriginLockKey(),
                                   lock_url.empty() ? "(none)" : lock_url);

    std::string site_isolation_mode;
    if (SiteIsolationPolicy::UseDedicatedProcessesForAllSites())
      site_isolation_mode += "spp ";
    if (SiteIsolationPolicy::AreIsolatedOriginsEnabled())
      site_isolation_mode += "io ";
    if (site_isolation_mode.empty())
      site_isolation_mode = "(none)";

    static auto* isolation_mode_key = base::debug::AllocateCrashKeyString(
        "site_isolation_mode", base::debug::CrashKeySize::Size32);
    base::debug::SetCrashKeyString(isolation_mode_key, site_isolation_mode);

    // Report a crash, since none will be generated by the killed slaverer.
    base::debug::DumpWithoutCrashing();
  }
}

void SlaverProcessHostImpl::UpdateClientPriority(PriorityClient* client) {
  DCHECK(client);
  DCHECK_EQ(1u, priority_clients_.count(client));
  UpdateProcessPriorityInputs();
}

int SlaverProcessHostImpl::VisibleClientCount() const {
  return visible_clients_;
}

unsigned int SlaverProcessHostImpl::GetFrameDepth() const {
  return frame_depth_;
}

bool SlaverProcessHostImpl::GetIntersectsViewport() const {
  return intersects_viewport_;
}

#if defined(OS_ANDROID)
ChildProcessImportance SlaverProcessHostImpl::GetEffectiveImportance() {
  return effective_importance_;
}
#endif

// static
void SlaverProcessHostImpl::set_slaver_process_host_factory_for_testing(
    const SlaverProcessHostFactory* rph_factory) {
  g_slaver_process_host_factory_ = rph_factory;
}

// static
const SlaverProcessHostFactory*
SlaverProcessHostImpl::get_slaver_process_host_factory_for_testing() {
  return g_slaver_process_host_factory_;
}

// static
void SlaverProcessHostImpl::NotifySpareManagerAboutRecentlyUsedMasterContext(
    MasterContext* master_context) {
  g_spare_slaver_process_host_manager.Get().PrepareForFutureRequests(
      master_context);
}

// static
SlaverProcessHost*
SlaverProcessHostImpl::GetSpareSlaverProcessHostForTesting() {
  return g_spare_slaver_process_host_manager.Get().spare_slaver_process_host();
}

// static
void SlaverProcessHostImpl::DiscardSpareSlaverProcessHostForTesting() {
  g_spare_slaver_process_host_manager.Get().CleanupSpareSlaverProcessHost();
}

// static
bool SlaverProcessHostImpl::IsSpareProcessKeptAtAllTimes() {
  if (!SiteIsolationPolicy::UseDedicatedProcessesForAllSites())
    return false;

  if (!base::FeatureList::IsEnabled(features::kSpareSlavererForSitePerProcess))
    return false;

  // Spare slaverer actually hurts performance on low-memory devices.  See
  // https://crbug.com/843775 for more details.
  //
  // The comparison below is using 1077 rather than 1024 because 1) this helps
  // ensure that devices with exactly 1GB of RAM won't get included because of
  // inaccuracies or off-by-one errors and 2) this is the bucket boundary in
  // Memory.Stats.Win.TotalPhys2.
  if (base::SysInfo::AmountOfPhysicalMemoryMB() <= 1077)
    return false;

  return true;
}

bool SlaverProcessHostImpl::HostHasNotBeenUsed() {
  return IsUnused() && listeners_.IsEmpty() && keep_alive_ref_count_ == 0 &&
         pending_views_ == 0;
}

void SlaverProcessHostImpl::LockToOrigin(const GURL& lock_url) {
  ChildProcessSecurityPolicyImpl::GetInstance()->LockToOrigin(GetID(),
                                                              lock_url);

  // Note that LockToOrigin is only called once per SlaverProcessHostImpl (when
  // committing a navigation into an empty slaverer).  Therefore, the call to
  // NotifySlavererIfLockedToSite below is insufficient for setting up slaverers
  // respawned after crashing - this is handled by another call to
  // NotifySlavererIfLockedToSite from OnProcessLaunched.
  NotifySlavererIfLockedToSite();
}

void SlaverProcessHostImpl::NotifySlavererIfLockedToSite() {
  GURL lock_url =
      ChildProcessSecurityPolicyImpl::GetInstance()->GetOriginLock(GetID());
  if (!lock_url.is_valid())
    return;
}

bool SlaverProcessHostImpl::IsForGuestsOnly() const {
  return is_for_guests_only_;
}

void SlaverProcessHostImpl::AppendSlavererCommandLine(
    base::CommandLine* command_line) {
  // Pass the process type first, so it shows first in process listings.
  command_line->AppendSwitchASCII(switches::kProcessType,
                                  switches::kSlavererProcess);

  // Now send any options from our own command line we want to propagate.
  const base::CommandLine& master_command_line =
      *base::CommandLine::ForCurrentProcess();
  PropagateMasterCommandLineToSlaverer(master_command_line, command_line);

  // Pass on the master locale.
  const std::string locale =
      GetSamplesClient()->master()->GetApplicationLocale();
  command_line->AppendSwitchASCII(switches::kLang, locale);

  // A non-empty SlavererCmdPrefix implies that Zygote is disabled.
  if (!base::CommandLine::ForCurrentProcess()
           ->GetSwitchValueNative(switches::kSlavererCmdPrefix)
           .empty()) {
    command_line->AppendSwitch(switches::kNoZygote);
  }

  GetSamplesClient()->master()->AppendExtraCommandLineSwitches(command_line,
                                                                GetID());

  command_line->AppendSwitchASCII(
      service_manager::switches::kServiceRequestChannelToken,
      child_connection_->service_token());
  command_line->AppendSwitchASCII(switches::kSlavererClientId,
                                  std::to_string(GetID()));

  if (SiteIsolationPolicy::UseDedicatedProcessesForAllSites()) {
    // Disable V8 code mitigations if slaverer processes are site-isolated.
    command_line->AppendSwitch(switches::kNoV8UntrustedCodeMitigations);
  }
}

void SlaverProcessHostImpl::PropagateMasterCommandLineToSlaverer(
    const base::CommandLine& master_cmd,
    base::CommandLine* slaverer_cmd) {
  // Propagate the following switches to the slaverer command line (along
  // with any associated values) if present in the master command line.
  static const char* const kSwitchNames[] = {
    service_manager::switches::kDisableInProcessStackTraces,
    service_manager::switches::kDisableSeccompFilterSandbox,
    service_manager::switches::kNoSandbox,
    switches::kAllowLoopbackInPeerConnection,
    switches::kAndroidFontsPath,
    switches::kBlinkSettings,
    switches::kDefaultTileWidth,
    switches::kDefaultTileHeight,
    switches::kDisable2dCanvasImageChromium,
    switches::kDisableAcceleratedJpegDecoding,
    switches::kDisableAcceleratedVideoDecode,
    switches::kDisableBackgroundTasks,
    switches::kDisableBackgroundTimerThrottling,
    switches::kDisableBreakpad,
    switches::kDisableCompositorUkmForTests,
    switches::kDisablePreferCompositingToLCDText,
    switches::kDisableDatabases,
    switches::kDisableFileSystem,
    switches::kDisableImageAnimationResync,
    switches::kDisableLowResTiling,
    switches::kDisableHistogramCustomizer,
    switches::kDisableLCDText,
    switches::kDisableLogging,
    switches::kDisableNotifications,
    switches::kDisableOopRasterization,
    switches::kDisableOriginTrialControlledBlinkFeatures,
    switches::kDisablePepper3DImageChromium,
    switches::kDisablePermissionsAPI,
    switches::kDisablePresentationAPI,
    switches::kDisableRGBA4444Textures,
    switches::kDisableSharedWorkers,
    switches::kDisableSpeechAPI,
    switches::kDisableThreadedCompositing,
    switches::kDisableThreadedScrolling,
    switches::kDisableV8IdleTasks,
    switches::kDisableWebGLImageChromium,
    switches::kDomAutomationController,
    switches::kEnableAccessibilityObjectModel,
    switches::kEnableAutomation,
    switches::kEnableBlinkGenPropertyTrees,
    switches::kEnableExperimentalWebPlatformFeatures,
    switches::kEnableLowResTiling,
    switches::kEnableLogging,
    switches::kEnableNetworkInformationDownlinkMax,
    switches::kEnableOopRasterization,
    switches::kEnablePluginPlaceholderTesting,
    switches::kEnablePreciseMemoryInfo,
    switches::kEnablePrintMaster,
    switches::kEnablePreferCompositingToLCDText,
    switches::kEnableRGBA4444Textures,
    switches::kEnableSlimmingPaintV2,
    switches::kEnableThreadedCompositing,
    switches::kEnableViewport,
    switches::kEnableVtune,
    switches::kEnableWebGL2ComputeContext,
    switches::kEnableWebGLDraftExtensions,
    switches::kEnableWebGLImageChromium,
    switches::kEnableWebVR,
    switches::kExplicitlyAllowedPorts,
    switches::kFileUrlPathAlias,
    switches::kForceOverlayFullscreenVideo,
    switches::kFullMemoryCrashReport,
    switches::kIPCConnectionTimeout,
    switches::kJavaScriptFlags,
    switches::kLoggingLevel,
    switches::kMaxUntiledLayerWidth,
    switches::kMaxUntiledLayerHeight,
    switches::kNoZygote,
    switches::kOverridePluginPowerSaverForTesting,
    switches::kPassiveListenersDefault,
    switches::kReducedReferrerGranularity,
    switches::kSamplingHeapProfiler,
    switches::kShowPaintRects,
    switches::kStatsCollectionController,
    switches::kTestType,
    switches::kTouchEventFeatureDetection,
    switches::kTouchTextSelectionStrategy,
    switches::kUseFakeUIForMediaStream,
    // Please keep these in alphabetical order. Compositor switches here should
    // also be added to chrome/master/chromeos/login/chrome_restart_request.cc.

    switches::kEnableLowEndDeviceMode,
    switches::kDisableLowEndDeviceMode,
#if defined(OS_ANDROID)
    switches::kOrderfileMemoryOptimization,
    switches::kSlavererWaitForJavaDebugger,
#endif
#if defined(ENABLE_IPC_FUZZER)
    switches::kIpcDumpDirectory,
    switches::kIpcFuzzerTestcase,
#endif
  };
  slaverer_cmd->CopySwitchesFrom(master_cmd, kSwitchNames,
                                 arraysize(kSwitchNames));

  MasterChildProcessHostImpl::CopyFeatureAndFieldTrialFlags(slaverer_cmd);

  // Add kWaitForDebugger to let slaverer process wait for a debugger.
  if (master_cmd.HasSwitch(switches::kWaitForDebuggerChildren)) {
    // Look to pass-on the kWaitForDebugger flag.
    std::string value =
        master_cmd.GetSwitchValueASCII(switches::kWaitForDebuggerChildren);
    if (value.empty() || value == switches::kSlavererProcess) {
      slaverer_cmd->AppendSwitch(switches::kWaitForDebugger);
    }
  }

  DCHECK(child_connection_);
  slaverer_cmd->AppendSwitchASCII(service_manager::switches::kServicePipeToken,
                                  child_connection_->service_token());

  CopyFeatureSwitch(master_cmd, slaverer_cmd, switches::kEnableBlinkFeatures);
  CopyFeatureSwitch(master_cmd, slaverer_cmd, switches::kDisableBlinkFeatures);
}

const base::Process& SlaverProcessHostImpl::GetProcess() const {
  if (run_slaverer_in_process()) {
    // This is a sentinel object used for this process in single process mode.
    static const base::NoDestructor<base::Process> self(
        base::Process::Current());
    return *self;
  }

  if (!child_process_launcher_.get() || child_process_launcher_->IsStarting()) {
    // This is a sentinel for "no process".
    static const base::NoDestructor<base::Process> null_process;
    return *null_process;
  }

  return child_process_launcher_->GetProcess();
}

bool SlaverProcessHostImpl::IsReady() const {
  // The process launch result (that sets GetHandle()) and the channel
  // connection (that sets channel_connected_) can happen in either order.
  return GetProcess().Handle() && channel_connected_;
}

bool SlaverProcessHostImpl::Shutdown(int exit_code) {
  if (run_slaverer_in_process())
    return false;  // Single process mode never shuts down the slaverer.

  if (!child_process_launcher_.get())
    return false;

  return child_process_launcher_->Terminate(exit_code);
}

bool SlaverProcessHostImpl::FastShutdownIfPossible(size_t page_count,
                                                   bool skip_unload_handlers) {
  if (run_slaverer_in_process())
    return false;  // Single process mode never shuts down the slaverer.

  if (!child_process_launcher_.get())
    return false;  // Slaver process hasn't started or is probably crashed.

  // Test if there's an unload listener.
  // NOTE: It's possible that an onunload listener may be installed
  // while we're shutting down, so there's a small race here.  Given that
  // the window is small, it's unlikely that the web page has much
  // state that will be lost by not calling its unload handlers properly.
  if (!skip_unload_handlers && !SuddenTerminationAllowed())
    return false;

  if (keep_alive_ref_count_ != 0) {
    if (keep_alive_start_time_.is_null())
      keep_alive_start_time_ = base::TimeTicks::Now();
    return false;
  }

  // Set this before ProcessDied() so observers can tell if the slaver process
  // died due to fast shutdown versus another cause.
  fast_shutdown_started_ = true;

  ProcessDied(false /* already_dead */, nullptr);
  return true;
}

bool SlaverProcessHostImpl::Send(IPC::Message* msg) {
  TRACE_EVENT2("slaverer_host", "SlaverProcessHostImpl::Send", "class",
               IPC_MESSAGE_ID_CLASS(msg->type()), "line",
               IPC_MESSAGE_ID_LINE(msg->type()));

  std::unique_ptr<IPC::Message> message(msg);

  // |channel_| is only null after Cleanup(), at which point we don't care about
  // delivering any messages.
  if (!channel_)
    return false;

#if !defined(OS_ANDROID)
  DCHECK(!message->is_sync());
#else
  if (message->is_sync()) {
    // If Init() hasn't been called yet since construction or the last
    // ProcessDied() we avoid blocking on sync IPC.
    if (!IsInitializedAndNotDead())
      return false;

    // Likewise if we've done Init(), but process launch has not yet completed,
    // we avoid blocking on sync IPC.
    if (child_process_launcher_.get() && child_process_launcher_->IsStarting())
      return false;
  }
#endif

  return channel_->Send(message.release());
}

bool SlaverProcessHostImpl::OnMessageReceived(const IPC::Message& msg) {
  // If we're about to be deleted, or have initiated the fast shutdown sequence,
  // we ignore incoming messages.

  if (deleting_soon_ || fast_shutdown_started_)
    return false;

  mark_child_process_activity_time();
  if (msg.routing_id() == MSG_ROUTING_CONTROL) {
    // Dispatch control messages.
    IPC_BEGIN_MESSAGE_MAP(SlaverProcessHostImpl, msg)
    // Adding single handlers for your service here is fine, but once your
    // service needs more than one handler, please extract them into a new
    // message filter and add that filter to CreateMessageFilters().
    IPC_END_MESSAGE_MAP()

    return true;
  }

  // Dispatch incoming messages to the appropriate IPC::Listener.
  IPC::Listener* listener = listeners_.Lookup(msg.routing_id());
  if (!listener) {
    if (msg.is_sync()) {
      // The listener has gone away, so we must respond or else the caller will
      // hang waiting for a reply.
      IPC::Message* reply = IPC::SyncMessage::GenerateReply(&msg);
      reply->set_reply_error();
      Send(reply);
    }
    return true;
  }
  return listener->OnMessageReceived(msg);
}

void SlaverProcessHostImpl::OnAssociatedInterfaceRequest(
    const std::string& interface_name,
    mojo::ScopedInterfaceEndpointHandle handle) {
  if (associated_interfaces_ &&
      !associated_interfaces_->TryBindInterface(interface_name, &handle)) {
    LOG(ERROR) << "Request for unknown Channel-associated interface: "
               << interface_name;
  }
}

void SlaverProcessHostImpl::OnChannelConnected(int32_t peer_pid) {
  channel_connected_ = true;
  if (IsReady()) {
    DCHECK(!sent_slaver_process_ready_);
    sent_slaver_process_ready_ = true;
    // Send SlaverProcessReady only if we already received the process handle.
    for (auto& observer : observers_)
      observer.SlaverProcessReady(this);
  }

#if BUILDFLAG(IPC_MESSAGE_LOG_ENABLED)
  child_control_interface_->SetIPCLoggingEnabled(
      IPC::Logging::GetInstance()->Enabled());
#endif
}

void SlaverProcessHostImpl::OnChannelError() {
  UMA_HISTOGRAM_BOOLEAN("MasterSlaverProcessHost.OnChannelError", true);
  ProcessDied(true /* already_dead */, nullptr);
}

void SlaverProcessHostImpl::OnBadMessageReceived(const IPC::Message& message) {
  // Message de-serialization failed. We consider this a capital crime. Kill the
  // slaverer if we have one.
  auto type = message.type();
  LOG(ERROR) << "bad message " << type << " terminating slaverer.";

  // The ReceivedBadMessage call below will trigger a DumpWithoutCrashing. Alias
  // enough information here so that we can determine what the bad message was.
  base::debug::Alias(&type);

  bad_message::ReceivedBadMessage(this,
                                  bad_message::RPH_DESERIALIZATION_FAILED);
}

MasterContext* SlaverProcessHostImpl::GetMasterContext() const {
  return master_context_;
}

int SlaverProcessHostImpl::GetID() const {
  return id_;
}

bool SlaverProcessHostImpl::IsInitializedAndNotDead() const {
  return is_initialized_ && !is_dead_;
}

void SlaverProcessHostImpl::Cleanup() {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
  // Keep the one slaverer thread around forever in single process mode.
  if (run_slaverer_in_process())
    return;

  // If within_process_died_observer_ is true, one of our observers performed an
  // action that caused us to die (e.g. http://crbug.com/339504). Therefore,
  // delay the destruction until all of the observer callbacks have been made,
  // and guarantee that the SlaverProcessHostDestroyed observer callback is
  // always the last callback fired.
  if (within_process_died_observer_) {
    delayed_cleanup_needed_ = true;
    return;
  }
  delayed_cleanup_needed_ = false;

  // Records the time when the process starts kept alive by the ref count for
  // UMA.
  if (listeners_.IsEmpty() && keep_alive_ref_count_ > 0 &&
      keep_alive_start_time_.is_null()) {
    keep_alive_start_time_ = base::TimeTicks::Now();
  }

  // Until there are no other owners of this object, we can't delete ourselves.
  if (!listeners_.IsEmpty() || keep_alive_ref_count_ != 0)
    return;

  if (!keep_alive_start_time_.is_null()) {
    UMA_HISTOGRAM_LONG_TIMES("MasterSlaverProcessHost.KeepAliveDuration",
                             base::TimeTicks::Now() - keep_alive_start_time_);
  }

  // We cannot clean up twice; if this fails, there is an issue with our
  // control flow.
  DCHECK(!deleting_soon_);

  DCHECK_EQ(0, pending_views_);

  // If the process associated with this SlaverProcessHost is still alive,
  // notify all observers that the process has exited cleanly, even though it
  // will be destroyed a bit later. Observers shouldn't rely on this process
  // anymore.
  if (IsInitializedAndNotDead()) {
    // Populates Android-only fields and closes the underlying base::Process.
    ChildProcessTerminationInfo info =
        child_process_launcher_->GetChildTerminationInfo(
            false /* already_dead */);
    info.status = base::TERMINATION_STATUS_NORMAL_TERMINATION;
    info.exit_code = 0;
    for (auto& observer : observers_) {
      observer.SlaverProcessExited(this, info);
    }
  }
  for (auto& observer : observers_)
    observer.SlaverProcessHostDestroyed(this);
  NotificationService::current()->Notify(
      NOTIFICATION_SLAVERER_PROCESS_TERMINATED,
      Source<SlaverProcessHost>(this), NotificationService::NoDetails());

  if (connection_filter_id_ !=
        ServiceManagerConnection::kInvalidConnectionFilterId) {
    ServiceManagerConnection* service_manager_connection =
        MasterContext::GetServiceManagerConnectionFor(master_context_);
    connection_filter_controller_->DisableFilter();
    service_manager_connection->RemoveConnectionFilter(connection_filter_id_);
    connection_filter_id_ =
        ServiceManagerConnection::kInvalidConnectionFilterId;
  }

#ifndef NDEBUG
  is_self_deleted_ = true;
#endif
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
  deleting_soon_ = true;

  // Destroy all mojo bindings and IPC channels that can cause calls to this
  // object, to avoid method invocations that trigger usages of profile.
  ResetIPC();

  DCHECK(!channel_);

  // Remove ourself from the list of slaverer processes so that we can't be
  // reused in between now and when the Delete task runs.
  UnregisterHost(GetID());

  instance_weak_factory_ =
      std::make_unique<base::WeakPtrFactory<SlaverProcessHostImpl>>(this);
}

void SlaverProcessHostImpl::SetSuddenTerminationAllowed(bool enabled) {
  sudden_termination_allowed_ = enabled;
}

bool SlaverProcessHostImpl::SuddenTerminationAllowed() const {
  return sudden_termination_allowed_;
}

base::TimeDelta SlaverProcessHostImpl::GetChildProcessIdleTime() const {
  return base::TimeTicks::Now() - child_process_activity_time_;
}

void SlaverProcessHostImpl::FilterURL(bool empty_allowed, GURL* url) {
  FilterURL(this, empty_allowed, url);
}

IPC::ChannelProxy* SlaverProcessHostImpl::GetChannel() {
  return channel_.get();
}

void SlaverProcessHostImpl::AddFilter(MasterMessageFilter* filter) {
  filter->RegisterAssociatedInterfaces(channel_.get());
  channel_->AddFilter(filter->GetFilter());
}

bool SlaverProcessHostImpl::FastShutdownStarted() const {
  return fast_shutdown_started_;
}

// static
void SlaverProcessHostImpl::RegisterHost(int host_id, SlaverProcessHost* host) {
  g_all_hosts.Get().AddWithID(host, host_id);
}

// static
void SlaverProcessHostImpl::UnregisterHost(int host_id) {
  SlaverProcessHost* host = g_all_hosts.Get().Lookup(host_id);
  if (!host)
    return;

  g_all_hosts.Get().Remove(host_id);

  // Look up the map of site to process for the given master_context,
  // in case we need to remove this process from it.  It will be registered
  // under any sites it slavered that use process-per-site mode.
  SiteProcessMap* map =
      GetSiteProcessMapForMasterContext(host->GetMasterContext());
  map->RemoveProcess(host);
}

// static
void SlaverProcessHostImpl::FilterURL(SlaverProcessHost* rph,
                                      bool empty_allowed,
                                      GURL* url) {
  ChildProcessSecurityPolicyImpl* policy =
      ChildProcessSecurityPolicyImpl::GetInstance();

  if (empty_allowed && url->is_empty())
    return;

  if (!url->is_valid()) {
    // Have to use about:blank for the denied case, instead of an empty GURL.
    // This is because the master treats navigation to an empty GURL as a
    // navigation to the home page. This is often a privileged page
    // (chrome://newtab/) which is exactly what we don't want.
    *url = GURL(url::kAboutBlankURL);
    return;
  }

  if (!policy->CanRequestURL(rph->GetID(), *url)) {
    // If this slaverer is not permitted to request this URL, we invalidate the
    // URL.  This prevents us from storing the blocked URL and becoming confused
    // later.
    VLOG(1) << "Blocked URL " << url->spec();
    *url = GURL(url::kAboutBlankURL);
  }
}

// static
void SlaverProcessHost::WarmupSpareSlaverProcessHost(
    samples::MasterContext* master_context) {
  g_spare_slaver_process_host_manager.Get().WarmupSpareSlaverProcessHost(
      master_context);
}

// static
bool SlaverProcessHost::run_slaverer_in_process() {
  return g_run_slaverer_in_process;
}

// static
void SlaverProcessHost::SetRunSlavererInProcess(bool value) {
  g_run_slaverer_in_process = value;

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (value) {
    if (!command_line->HasSwitch(switches::kLang)) {
      // Modify the current process' command line to include the master locale,
      // as the slaverer expects this flag to be set.
      const std::string locale =
          GetSamplesClient()->master()->GetApplicationLocale();
      command_line->AppendSwitchASCII(switches::kLang, locale);
    }
  }
}

// static
SlaverProcessHost::iterator SlaverProcessHost::AllHostsIterator() {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
  return iterator(g_all_hosts.Pointer());
}

// static
SlaverProcessHost* SlaverProcessHost::FromID(int slaver_process_id) {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
  return g_all_hosts.Get().Lookup(slaver_process_id);
}

// static
SlaverProcessHost* SlaverProcessHost::FromSlavererIdentity(
    const service_manager::Identity& identity) {
  for (SlaverProcessHost::iterator i(SlaverProcessHost::AllHostsIterator());
       !i.IsAtEnd(); i.Advance()) {
    SlaverProcessHost* process = i.GetCurrentValue();
    if (process->GetChildIdentity() == identity)
      return process;
  }
  return nullptr;
}

// static
bool SlaverProcessHost::ShouldTryToUseExistingProcessHost(
    MasterContext* master_context,
    const GURL& url) {
  if (run_slaverer_in_process())
    return true;

  // NOTE: Sometimes it's necessary to create more slaver processes than
  //       GetMaxSlavererProcessCount(), for instance when we want to create
  //       a slaverer process for a master context that has no existing
  //       slaverers. This is OK in moderation, since the
  //       GetMaxSlavererProcessCount() is conservative.
  if (g_all_hosts.Get().size() >= GetMaxSlavererProcessCount())
    return true;

  return GetSamplesClient()->master()->ShouldTryToUseExistingProcessHost(
      master_context, url);
}

// static
bool SlaverProcessHost::ShouldUseProcessPerSite(MasterContext* master_context,
                                                const GURL& url) {
  // Returns true if we should use the process-per-site model.  This will be
  // the case if the --process-per-site switch is specified, or in
  // process-per-site-instance for particular sites (e.g., WebUI).
  // Note that --single-process is handled in ShouldTryToUseExistingProcessHost.
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kProcessPerSite))
    return true;

  // Error pages should use process-per-site model, as it is useful to
  // consolidate them to minimize resource usage and there is no security
  // drawback to combining them all in the same process.
  if (url.SchemeIs(kChromeErrorScheme))
    return true;

  // Otherwise let the samples client decide, defaulting to false.
  return GetSamplesClient()->master()->ShouldUseProcessPerSite(master_context,
                                                                url);
}

void SlaverProcessHostImpl::ProcessDied(
    bool already_dead,
    ChildProcessTerminationInfo* known_info) {
  // Our child process has died.  If we didn't expect it, it's a crash.
  // In any case, we need to let everyone know it's gone.
  // The OnChannelError notification can fire multiple times due to nested sync
  // calls to a slaverer. If we don't have a valid channel here it means we
  // already handled the error.

  // It should not be possible for us to be called re-entrantly.
  DCHECK(!within_process_died_observer_);

  // It should not be possible for a process death notification to come in while
  // we are dying.
  DCHECK(!deleting_soon_);

  // child_process_launcher_ can be NULL in single process mode or if fast
  // termination happened.
  ChildProcessTerminationInfo info;
  info.exit_code = 0;
  if (known_info) {
    info = *known_info;
  } else if (child_process_launcher_.get()) {
    info = child_process_launcher_->GetChildTerminationInfo(already_dead);
    if (already_dead && info.status == base::TERMINATION_STATUS_STILL_RUNNING) {
      // May be in case of IPC error, if it takes long time for slaverer
      // to exit. Child process will be killed in any case during
      // child_process_launcher_.reset(). Make sure we will not broadcast
      // FrameHostMsg_SlaverProcessGone with status
      // TERMINATION_STATUS_STILL_RUNNING, since this will break WebSamplessImpl
      // logic.
      info.status = base::TERMINATION_STATUS_PROCESS_CRASHED;
    }
  }

  child_process_launcher_.reset();
  is_dead_ = true;
  // Make sure no IPCs or mojo calls from the old process get dispatched after
  // it has died.
  ResetIPC();

  UpdateProcessPriority();

  within_process_died_observer_ = true;
  NotificationService::current()->Notify(
      NOTIFICATION_SLAVERER_PROCESS_CLOSED, Source<SlaverProcessHost>(this),
      Details<ChildProcessTerminationInfo>(&info));
  for (auto& observer : observers_)
    observer.SlaverProcessExited(this, info);
  within_process_died_observer_ = false;

  // Initialize a new ChannelProxy in case this host is re-used for a new
  // process. This ensures that new messages can be sent on the host ASAP (even
  // before Init()) and they'll eventually reach the new process.
  //
  // Note that this may have already been called by one of the above observers
  EnableSendQueue();

  // It's possible that one of the calls out to the observers might have caused
  // this object to be no longer needed.
  if (delayed_cleanup_needed_)
    Cleanup();
}

void SlaverProcessHostImpl::ResetIPC() {
  if (slaverer_host_binding_.is_bound())
    slaverer_host_binding_.Unbind();
  if (route_provider_binding_.is_bound())
    route_provider_binding_.Close();
  associated_interface_provider_bindings_.CloseAllBindings();
  associated_interfaces_.reset();

  // It's important not to wait for the DeleteTask to delete the channel
  // proxy. Kill it off now. That way, in case the profile is going away, the
  // rest of the objects attached to this SlaverProcessHost start going
  // away first, since deleting the channel proxy will post a
  // OnChannelClosed() to IPC::ChannelProxy::Context on the IO thread.
  ResetChannelProxy();
}

void SlaverProcessHost::PostTaskWhenProcessIsReady(base::OnceClosure task) {
  DCHECK_CURRENTLY_ON(MasterThread::UI);
  DCHECK(!task.is_null());
  new SlaverProcessHostIsReadyObserver(this, std::move(task));
}

// static
void SlaverProcessHost::SetHungSlavererAnalysisFunction(
    AnalyzeHungSlavererFunction analyze_hung_slaverer) {
  g_analyze_hung_slaverer = analyze_hung_slaverer;
}

void SlaverProcessHostImpl::SuddenTerminationChanged(bool enabled) {
  SetSuddenTerminationAllowed(enabled);
}

void SlaverProcessHostImpl::UpdateProcessPriorityInputs() {
  int32_t new_visible_widgets_count = 0;
  unsigned int new_frame_depth = kMaxFrameDepthForPriority;
  bool new_intersects_viewport = false;
#if defined(OS_ANDROID)
  ChildProcessImportance new_effective_importance =
      ChildProcessImportance::NORMAL;
#endif
  for (auto* client : priority_clients_) {
    Priority priority = client->GetPriority();

    // Compute the lowest depth of widgets with highest visibility priority.
    // See comment on |frame_depth_| for more details.
    if (priority.is_hidden) {
      if (!new_visible_widgets_count) {
        new_frame_depth = std::min(new_frame_depth, priority.frame_depth);
        new_intersects_viewport =
            new_intersects_viewport || priority.intersects_viewport;
      }
    } else {
      if (new_visible_widgets_count) {
        new_frame_depth = std::min(new_frame_depth, priority.frame_depth);
        new_intersects_viewport =
            new_intersects_viewport || priority.intersects_viewport;
      } else {
        new_frame_depth = priority.frame_depth;
        new_intersects_viewport = priority.intersects_viewport;
      }
      new_visible_widgets_count++;
    }

#if defined(OS_ANDROID)
    new_effective_importance =
        std::max(new_effective_importance, priority.importance);
#endif
  }

  bool inputs_changed = new_visible_widgets_count != visible_clients_;
  visible_clients_ = new_visible_widgets_count;
  frame_depth_ = new_frame_depth;
  intersects_viewport_ = new_intersects_viewport;
#if defined(OS_ANDROID)
  inputs_changed =
      inputs_changed || new_effective_importance != effective_importance_;
  effective_importance_ = new_effective_importance;
#endif
  if (inputs_changed)
    UpdateProcessPriority();
}

void SlaverProcessHostImpl::UpdateProcessPriority() {
  if (!run_slaverer_in_process() && (!child_process_launcher_.get() ||
                                     child_process_launcher_->IsStarting())) {
    // This path can be hit early (no-op) or on ProcessDied(). Reset |priority_|
    // to defaults in case this SlaverProcessHostImpl is re-used.
    priority_.visible = !blink::kLaunchingProcessIsBackgrounded;
    return;
  }

  const ChildProcessLauncherPriority priority(
      visible_clients_ > 0 || base::CommandLine::ForCurrentProcess()->HasSwitch(
                                  switches::kDisableSlavererBackgrounding),
      frame_depth_, intersects_viewport_,
#if defined(OS_ANDROID)
      GetEffectiveImportance()
#endif
          );

  const bool should_background_changed =
      priority_.is_background() != priority.is_background();
  if (priority_ == priority)
    return;

  TRACE_EVENT2("slaverer_host", "SlaverProcessHostImpl::UpdateProcessPriority",
               "should_background", priority.is_background(),
               "has_pending_views", priority.boost_for_pending_views);
  priority_ = priority;

  // Control the background state from the master process, otherwise the task
  // telling the slaverer to "unbackground" itself may be preempted by other
  // tasks executing at lowered priority ahead of it or simply by not being
  // swiftly scheduled by the OS per the low process priority
  // (http://crbug.com/398103).
  if (!run_slaverer_in_process()) {
    DCHECK(child_process_launcher_.get());
    DCHECK(!child_process_launcher_->IsStarting());
    child_process_launcher_->SetProcessPriority(priority_);
  }

  // Notify the child process of background state.
  if (should_background_changed) {
    GetSlavererInterface()->SetProcessBackgrounded(priority_.is_background());
  }
}

void SlaverProcessHostImpl::OnProcessLaunched() {
  // No point doing anything, since this object will be destructed soon.  We
  // especially don't want to send the SLAVERER_PROCESS_CREATED notification,
  // since some clients might expect a SLAVERER_PROCESS_TERMINATED afterwards to
  // properly cleanup.
  if (deleting_soon_)
    return;

  if (child_process_launcher_) {
    DCHECK(child_process_launcher_->GetProcess().IsValid());
    // TODO(https://crbug.com/875933): This should be based on
    // |priority_.is_background()|, see similar check below.
    DCHECK_EQ(blink::kLaunchingProcessIsBackgrounded, !priority_.visible);

    // Unpause the channel now that the process is launched. We don't flush it
    // yet to ensure that any initialization messages sent here (e.g., things
    // done in response to NOTIFICATION_RENDER_PROCESS_CREATED; see below)
    // preempt already queued messages.
    channel_->Unpause(false /* flush */);

    if (child_connection_) {
      child_connection_->SetProcessHandle(
          child_process_launcher_->GetProcess().Handle());
    }

// Not all platforms launch processes in the same backgrounded state. Make
// sure |priority_.visible| reflects this platform's initial process
// state.
#if defined(OS_MACOSX)
    priority_.visible =
        !child_process_launcher_->GetProcess().IsProcessBackgrounded(
            MachBroker::GetInstance());
#elif defined(OS_ANDROID)
    // Android child process priority works differently and cannot be queried
    // directly from base::Process.
    // TODO(https://crbug.com/875933): Fix initial priority on Android to
    // reflect |priority_.is_background()|.
    DCHECK_EQ(blink::kLaunchingProcessIsBackgrounded, !priority_.visible);
#else
    priority_.visible =
        !child_process_launcher_->GetProcess().IsProcessBackgrounded();
#endif  // defined(OS_MACOSX)

    // Only update the priority on startup if boosting is enabled (to avoid
    // reintroducing https://crbug.com/560446#c13 while pending views only
    // experimentally result in a boost).
    if (priority_.boost_for_pending_views)
      UpdateProcessPriority();

  }

  NotifySlavererIfLockedToSite();

  // NOTE: This needs to be before flushing queued messages, because
  // ExtensionService uses this notification to initialize the slaverer process
  // with state that must be there before any JavaScript executes.
  //
  // The queued messages contain such things as "navigate". If this notification
  // was after, we can end up executing JavaScript before the initialization
  // happens.
  NotificationService::current()->Notify(NOTIFICATION_SLAVERER_PROCESS_CREATED,
                                         Source<SlaverProcessHost>(this),
                                         NotificationService::NoDetails());

  if (child_process_launcher_)
    channel_->Flush();

  if (IsReady()) {
    DCHECK(!sent_slaver_process_ready_);
    sent_slaver_process_ready_ = true;
    // Send SlaverProcessReady only if the channel is already connected.
    for (auto& observer : observers_)
      observer.SlaverProcessReady(this);
  }

}

void SlaverProcessHostImpl::OnProcessLaunchFailed(int error_code) {
  // If this object will be destructed soon, then observers have already been
  // sent a SlaverProcessHostDestroyed notification, and we must observe our
  // contract that says that will be the last call.
  if (deleting_soon_)
    return;

  ChildProcessTerminationInfo info;
  info.status = base::TERMINATION_STATUS_LAUNCH_FAILED;
  info.exit_code = error_code;
  ProcessDied(true, &info);
}

void SlaverProcessHostImpl::OnUserMetricsRecordAction(
    const std::string& action) {
  base::RecordComputedAction(action);
}

// static
void SlaverProcessHostImpl::OnMojoError(int slaver_process_id,
                                        const std::string& error) {
  LOG(ERROR) << "Terminating slaver process for bad Mojo message: " << error;

  // The ReceivedBadMessage call below will trigger a DumpWithoutCrashing.
  // Capture the error message in a crash key value.
  base::debug::ScopedCrashKeyString error_key_value(
      bad_message::GetMojoErrorCrashKey(), error);
  bad_message::ReceivedBadMessage(slaver_process_id,
                                  bad_message::RPH_MOJO_PROCESS_ERROR);
}

}  // namespace samples
