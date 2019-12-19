// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_SLAVERER_HOST_SLAVER_PROCESS_HOST_IMPL_H_
#define SAMPLES_MASTER_SLAVERER_HOST_SLAVER_PROCESS_HOST_IMPL_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/process/process.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/post_task.h"
#include "build/build_config.h"
#include "samples/master/child_process_launcher.h"
#include "samples/common/associated_interfaces.mojom.h"
#include "samples/common/child_control.mojom.h"
#include "samples/common/export.h"
#include "samples/common/slaverer.mojom.h"
#include "samples/common/slaverer_host.mojom.h"
#include "samples/public/master/master_task_traits.h"
#include "samples/public/master/master_thread.h"
#include "samples/public/master/slaver_process_host.h"
#include "samples/public/common/service_manager_connection.h"
#include "ipc/ipc_channel_proxy.h"
#include "ipc/ipc_platform_file.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/associated_binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "mojo/public/cpp/system/invitation.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/mojom/service.mojom.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/mojom/associated_interfaces/associated_interfaces.mojom.h"
//#include "third_party/blink/public/mojom/filesystem/file_system.mojom.h"

#if defined(OS_ANDROID)
#include "samples/public/master/android/child_process_importance.h"
#endif

namespace base {
class CommandLine;
class MessageLoop;
class SharedPersistentMemoryAllocator;
}

namespace samples {
class ChildConnection;
class InProcessChildThreadParams;
class PermissionServiceContext;
class SlaverProcessHostFactory;
struct ChildProcessTerminationInfo;

typedef base::Thread* (*SlavererMainThreadFactoryFunction)(
    const InProcessChildThreadParams& params);

// Implements a concrete SlaverProcessHost for the master process for talking
// to actual slaverer processes (as opposed to mocks).
//
// Represents the master side of the master <--> slaverer communication
// channel. There will be one SlaverProcessHost per slaverer process.
//
// This object is refcounted so that it can release its resources when all
// hosts using it go away.
//
// This object communicates back and forth with the SlaverProcess object
// running in the slaverer process. Each SlaverProcessHost and SlaverProcess
// keeps a list of SlaverView (slaverer) and WebContentsImpl (master) which
// are correlated with IDs. This way, the Views and the corresponding ViewHosts
// communicate through the two process objects.
//
class SAMPLES_EXPORT SlaverProcessHostImpl
    : public SlaverProcessHost,
      public ChildProcessLauncher::Client,
      public mojom::RouteProvider,
      public blink::mojom::AssociatedInterfaceProvider,
      public mojom::SlavererHost {
 public:
  // Special depth used when there are no PriorityClients.
  static const unsigned int kMaxFrameDepthForPriority;

  static SlaverProcessHost* CreateSlaverProcessHost(
      MasterContext* master_context,
      bool is_for_guests_only);

  ~SlaverProcessHostImpl() override;

  // SlaverProcessHost implementation (public portion).
  bool Init() override;
  void EnableSendQueue() override;
  void AddRoute(int32_t routing_id, IPC::Listener* listener) override;
  void RemoveRoute(int32_t routing_id) override;
  void AddObserver(SlaverProcessHostObserver* observer) override;
  void RemoveObserver(SlaverProcessHostObserver* observer) override;
  void ShutdownForBadMessage(CrashReportMode crash_report_mode) override;
  void UpdateClientPriority(PriorityClient* client) override;
  int VisibleClientCount() const override;
  unsigned int GetFrameDepth() const override;
  bool GetIntersectsViewport() const override;
  bool IsForGuestsOnly() const override;
  bool Shutdown(int exit_code) override;
  bool FastShutdownIfPossible(size_t page_count = 0,
                              bool skip_unload_handlers = false) override;
  const base::Process& GetProcess() const override;
  bool IsReady() const override;
  MasterContext* GetMasterContext() const override;
  int GetID() const override;
  bool IsInitializedAndNotDead() const override;
  void Cleanup() override;
#if defined(OS_ANDROID)
  ChildProcessImportance GetEffectiveImportance() override;
#endif
  void SetSuddenTerminationAllowed(bool enabled) override;
  bool SuddenTerminationAllowed() const override;
  IPC::ChannelProxy* GetChannel() override;
  void AddFilter(MasterMessageFilter* filter) override;
  bool FastShutdownStarted() const override;
  base::TimeDelta GetChildProcessIdleTime() const override;
  void FilterURL(bool empty_allowed, GURL* url) override;
  void BindInterface(const std::string& interface_name,
                     mojo::ScopedMessagePipeHandle interface_pipe) override;
  const service_manager::Identity& GetChildIdentity() const override;
  bool IsProcessBackgrounded() const override;
  void IncrementKeepAliveRefCount(
      SlaverProcessHost::KeepAliveClientType) override;
  void DecrementKeepAliveRefCount(
      SlaverProcessHost::KeepAliveClientType) override;
  void DisableKeepAliveRefCount() override;
  bool IsKeepAliveRefCountDisabled() override;
  void PurgeAndSuspend() override;
  void Resume() override;
  mojom::Slaverer* GetSlavererInterface() override;

  void SetIsNeverSuitableForReuse() override;
  bool MayReuseHost() override;
  bool IsUnused() override;
  void SetIsUsed() override;

  bool HostHasNotBeenUsed() override;
  void LockToOrigin(const GURL& lock_url) override;

  mojom::RouteProvider* GetRemoteRouteProvider();

  // IPC::Sender via SlaverProcessHost.
  bool Send(IPC::Message* msg) override;

  // IPC::Listener via SlaverProcessHost.
  bool OnMessageReceived(const IPC::Message& msg) override;
  void OnAssociatedInterfaceRequest(
      const std::string& interface_name,
      mojo::ScopedInterfaceEndpointHandle handle) override;
  void OnChannelConnected(int32_t peer_pid) override;
  void OnChannelError() override;
  void OnBadMessageReceived(const IPC::Message& message) override;

  // ChildProcessLauncher::Client implementation.
  void OnProcessLaunched() override;
  void OnProcessLaunchFailed(int error_code) override;

  // Call this function when it is evident that the child process is actively
  // performing some operation, for example if we just received an IPC message.
  void mark_child_process_activity_time() {
    child_process_activity_time_ = base::TimeTicks::Now();
  }

  // Used to extend the lifetime of the sessions until the slaver view
  // in the slaverer is fully closed. This is static because its also called
  // with mock hosts as input in test cases. The SlaverWidget routing associated
  // with the view is used as the key since the WidgetMsg_Close and
  // WidgetHostMsg_Close_ACK logic is centered around SlaverWidgets.
  static void ReleaseOnCloseACK(SlaverProcessHost* host,
                                int widget_route_id);

  // Register/unregister the host identified by the host id in the global host
  // list.
  static void RegisterHost(int host_id, SlaverProcessHost* host);
  static void UnregisterHost(int host_id);

  // Implementation of FilterURL below that can be shared with the mock class.
  static void FilterURL(SlaverProcessHost* rph, bool empty_allowed, GURL* url);

  // Returns true if |host| is suitable for slavering a page in the given
  // |master_context|, where the page would utilize |site_url| as its
  // SiteInstance site URL, and its process would be locked to |lock_url|.
  // |site_url| and |lock_url| may differ in cases where an effective URL is
  // not the actual site that the process is locked to, which happens for
  // hosted apps.
  static bool IsSuitableHost(SlaverProcessHost* host,
                             MasterContext* master_context,
                             const GURL& site_url,
                             const GURL& lock_url);

  // Variant of the above that takes in a SiteInstance site URL and the
  // process's origin lock URL, when they are known.
  static SlaverProcessHost* GetSoleProcessHostForSite(
      MasterContext* master_context,
      const GURL& site_url,
      const GURL& lock_url);

  // Should be called when |master_context| is used in a navigation.
  //
  // The SpareSlaverProcessHostManager can decide how to respond (for example,
  // by shutting down the spare process to conserve resources, or alternatively
  // by making sure that the spare process belongs to the same MasterContext as
  // the most recent navigation).
  static void NotifySpareManagerAboutRecentlyUsedMasterContext(
      MasterContext* master_context);

  // This enum backs a histogram, so do not change the order of entries or
  // remove entries and update enums.xml if adding new entries.
  enum class SpareProcessMaybeTakeAction {
    kNoSparePresent = 0,
    kMismatchedMasterContext = 1,
    kMismatchedStoragePartition = 2,
    kRefusedByEmbedder = 3,
    kSpareTaken = 4,
    kRefusedBySiteInstance = 5,
    kMaxValue = kRefusedBySiteInstance
  };

  static base::MessageLoop* GetInProcessSlavererThreadForTesting();

  // This forces a slaverer that is running "in process" to shut down.
  static void ShutDownInProcessSlaverer();

  static void RegisterSlavererMainThreadFactory(
      SlavererMainThreadFactoryFunction create);

  void SetMasterPluginMessageFilterSubFilterForTesting(
      scoped_refptr<MasterMessageFilter> message_filter) const;

  void set_is_for_guests_only_for_testing(bool is_for_guests_only) {
    is_for_guests_only_ = is_for_guests_only;
  }

#if defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_MACOSX)
  // Launch the zygote early in the master startup.
  static void EarlyZygoteLaunch();
#endif  // defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_MACOSX)

  void RecomputeAndUpdateWebKitPreferences();

  static void set_slaver_process_host_factory_for_testing(
      const SlaverProcessHostFactory* rph_factory);
  // Gets the global factory used to create new SlaverProcessHosts in unit
  // tests.
  static const SlaverProcessHostFactory*
  get_slaver_process_host_factory_for_testing();

  // globally-used spare SlaverProcessHost at any time.
  static SlaverProcessHost* GetSpareSlaverProcessHostForTesting();

  // Discards the spare SlaverProcessHost.  After this call,
  // GetSpareSlaverProcessHostForTesting will return nullptr.
  static void DiscardSpareSlaverProcessHostForTesting();

  // Returns true if a spare SlaverProcessHost should be kept at all times.
  static bool IsSpareProcessKeptAtAllTimes();

  bool is_initialized() const { return is_initialized_; }

  // Ensures that this process is kept alive for the specified amount of time.
  // This is used to ensure that unload handlers have a chance to execute
  // before the process shuts down.
  void DelayProcessShutdownForUnload(const base::TimeDelta& timeout);

 protected:
  // A proxy for our IPC::Channel that lives on the IO thread.
  std::unique_ptr<IPC::ChannelProxy> channel_;

  // True if fast shutdown has been performed on this RPH.
  bool fast_shutdown_started_;

  // True if we've posted a DeleteTask and will be deleted soon.
  bool deleting_soon_;

#ifndef NDEBUG
  // True if this object has deleted itself.
  bool is_self_deleted_;
#endif

  // The count of currently swapped out but pending SlaverViews.  We have
  // started to swap these in, so the slaverer process should not exit if
  // this count is non-zero.
  int32_t pending_views_;

 private:
  friend class ChildProcessLauncherMasterTest_ChildSpawnFail_Test;
  friend class VisitRelayingSlaverProcessHost;
  class ConnectionFilterController;
  class ConnectionFilterImpl;

  // Use CreateSlaverProcessHost() instead of calling this constructor
  // directly.
  SlaverProcessHostImpl(MasterContext* master_context,
                        bool is_for_guests_only);

  // Initializes a new IPC::ChannelProxy in |channel_|, which will be connected
  // to the next child process launched for this host, if any.
  void InitializeChannelProxy();

  // Resets |channel_|, removing it from the attachment broker if necessary.
  // Always call this in lieu of directly resetting |channel_|.
  void ResetChannelProxy();

  // Creates and adds the IO thread message filters.
  void CreateMessageFilters();

  // Registers Mojo interfaces to be exposed to the slaverer.
  void RegisterMojoInterfaces();

  // mojom::RouteProvider:
  void GetRoute(int32_t routing_id,
                blink::mojom::AssociatedInterfaceProviderAssociatedRequest
                    request) override;

  // blink::mojom::AssociatedInterfaceProvider:
  void GetAssociatedInterface(
      const std::string& name,
      blink::mojom::AssociatedInterfaceAssociatedRequest request) override;

  void SuddenTerminationChanged(bool enabled) override;

  void BindRouteProvider(mojom::RouteProviderAssociatedRequest request);

  void CreateSlavererHost(mojom::SlavererHostAssociatedRequest request);

  // Control message handlers.
  void OnUserMetricsRecordAction(const std::string& action);
  void OnCloseACK(int closed_widget_route_id);

  // Generates a command line to be used to spawn a slaverer and appends the
  // results to |*command_line|.
  void AppendSlavererCommandLine(base::CommandLine* command_line);

  // Copies applicable command line switches from the given |master_cmd| line
  // flags to the output |slaverer_cmd| line flags. Not all switches will be
  // copied over.
  void PropagateMasterCommandLineToSlaverer(
      const base::CommandLine& master_cmd,
      base::CommandLine* slaverer_cmd);

  // Recompute |visible_clients_| and |effective_importance_| from
  // |priority_clients_|.
  void UpdateProcessPriorityInputs();

  // Inspects the current object state and sets/removes background priority if
  // appropriate. Should be called after any of the involved data members
  // change.
  void UpdateProcessPriority();

  // Handle termination of our process.
  void ProcessDied(bool already_dead,
                   ChildProcessTerminationInfo* known_details);

  // Destroy all objects that can cause methods to be invoked on this object or
  // any other that hang off it.
  void ResetIPC();

  void RecordKeepAliveDuration(SlaverProcessHost::KeepAliveClientType,
                               base::TimeTicks start,
                               base::TimeTicks end);

  void NotifySlavererIfLockedToSite();

  static void OnMojoError(int slaver_process_id, const std::string& error);

  template <typename InterfaceType>
  using AddInterfaceCallback =
      base::Callback<void(mojo::InterfaceRequest<InterfaceType>)>;

  template <typename CallbackType>
  struct InterfaceGetter;

  template <typename InterfaceType>
  struct InterfaceGetter<AddInterfaceCallback<InterfaceType>> {
    static void GetInterfaceOnUIThread(
        base::WeakPtr<SlaverProcessHostImpl> weak_host,
        const AddInterfaceCallback<InterfaceType>& callback,
        mojo::InterfaceRequest<InterfaceType> request) {
      if (!weak_host)
        return;
      callback.Run(std::move(request));
    }
  };

  // Helper to bind an interface callback whose lifetime is limited to that of
  // the slaver process currently hosted by the RPHI. Callbacks added by this
  // method will never run beyond the next invocation of Cleanup().
  template <typename CallbackType>
  void AddUIThreadInterface(service_manager::BinderRegistry* registry,
                            const CallbackType& callback) {
    registry->AddInterface(
        base::Bind(&InterfaceGetter<CallbackType>::GetInterfaceOnUIThread,
                   instance_weak_factory_->GetWeakPtr(), callback),
        base::CreateSingleThreadTaskRunnerWithTraits({MasterThread::UI}));
  }

  // Callback to unblock process shutdown after waiting for unload handlers to
  // execute.
  void CancelProcessShutdownDelayForUnload();

  mojo::OutgoingInvitation mojo_invitation_;

  std::unique_ptr<ChildConnection> child_connection_;
  int connection_filter_id_ =
      ServiceManagerConnection::kInvalidConnectionFilterId;
  scoped_refptr<ConnectionFilterController> connection_filter_controller_;
  service_manager::mojom::ServicePtr test_service_;

  size_t keep_alive_ref_count_;

  // TODO(panicker): Remove these after investigation in
  // https://crbug.com/823482.
  static const size_t kNumKeepAliveClients = 4;
  size_t keep_alive_client_count_[kNumKeepAliveClients];
  base::TimeTicks keep_alive_client_start_time_[kNumKeepAliveClients];

  // Set in DisableKeepAliveRefCount(). When true, |keep_alive_ref_count_| must
  // no longer be modified.
  bool is_keep_alive_ref_count_disabled_;

  // Whether this host is never suitable for reuse as determined in the
  // MayReuseHost() function.
  bool is_never_suitable_for_reuse_ = false;

  // The registered IPC listener objects. When this list is empty, we should
  // delete ourselves.
  base::IDMap<IPC::Listener*> listeners_;

  // Mojo interfaces provided to the child process are registered here if they
  // need consistent delivery ordering with legacy IPC, and are process-wide in
  // nature (e.g. metrics, memory usage).
  std::unique_ptr<blink::AssociatedInterfaceRegistry> associated_interfaces_;

  mojo::AssociatedBinding<mojom::RouteProvider> route_provider_binding_;
  mojo::AssociatedBindingSet<blink::mojom::AssociatedInterfaceProvider, int32_t>
      associated_interface_provider_bindings_;

  // These fields are cached values that are updated in
  // UpdateProcessPriorityInputs, and are used to compute priority sent to
  // ChildProcessLauncher.
  // |visible_clients_| is the count of currently visible clients.
  int32_t visible_clients_;
  // |frame_depth_| can be used to rank processes of the same visibility, ie it
  // is the lowest depth of all visible clients, or if there are no visible
  // widgets the lowest depth of all hidden clients. Initialized to max depth
  // when there are no clients.
  unsigned int frame_depth_ = kMaxFrameDepthForPriority;
  // |intersects_viewport_| similar to |frame_depth_| can be used to rank
  // processes of same visibility. It indicates process has frames that
  // intersect with the viewport.
  bool intersects_viewport_ = false;
#if defined(OS_ANDROID)
  // Highest importance of all clients that contribute priority.
  ChildProcessImportance effective_importance_ = ChildProcessImportance::NORMAL;
#endif

  // Clients that contribute priority to this proces.
  base::flat_set<PriorityClient*> priority_clients_;

  ChildProcessLauncherPriority priority_;

  // Used in single-process mode.
  std::unique_ptr<base::Thread> in_process_slaverer_;

  // True after Init() has been called.
  bool is_initialized_ = false;

  // True after ProcessDied(), until the next call to Init().
  bool is_dead_ = false;

  // PlzNavigate
  // Stores the time at which the first call to Init happened.
  base::TimeTicks init_time_;

  // Used to launch and terminate the process without blocking the UI thread.
  std::unique_ptr<ChildProcessLauncher> child_process_launcher_;

  // The globally-unique identifier for this RPH.
  const int id_;

  // A secondary ID used by the Service Manager to distinguish different
  // incarnations of the same RPH from each other. Unlike |id_| this is not
  // globally unique, but it is guaranteed to change every time ProcessDied() is
  // called.
  int instance_id_ = 1;

  MasterContext* const master_context_;

  // The observers watching our lifetime.
  base::ObserverList<SlaverProcessHostObserver>::Unchecked observers_;

  // True if the process can be shut down suddenly.  If this is true, then we're
  // sure that all the SlaverViews in the process can be shutdown suddenly.  If
  // it's false, then specific SlaverViews might still be allowed to be shutdown
  // suddenly by checking their SuddenTerminationAllowed() flag.  This can occur
  // if one WebContents has an unload event listener but another WebContents in
  // the same process doesn't.
  bool sudden_termination_allowed_;

  // Set to true if we shouldn't send input events.  We actually do the
  // filtering for this at the slaver widget level.
  bool ignore_input_events_;

  // Records the last time we regarded the child process active.
  base::TimeTicks child_process_activity_time_;

  // Indicates whether this SlaverProcessHost is exclusively hosting guest
  // SlaverFrames.
  bool is_for_guests_only_;

  // Indicates whether this SlaverProcessHost is unused, meaning that it has
  // not committed any web samples, and it has not been given to a SiteInstance
  // that has a site assigned.
  bool is_unused_;

  // Set if a call to Cleanup is required once the SlaverProcessHostImpl is no
  // longer within the SlaverProcessHostObserver::SlaverProcessExited callbacks.
  bool delayed_cleanup_needed_;

  // Indicates whether SlaverProcessHostImpl is currently iterating and calling
  // through SlaverProcessHostObserver::SlaverProcessExited.
  bool within_process_died_observer_;

  // Records the time when the process starts surviving for workers for UMA.
  base::TimeTicks keep_alive_start_time_;

  bool channel_connected_;
  bool sent_slaver_process_ready_;

#if defined(OS_ANDROID)
  // UI thread is the source of sync IPCs and all shutdown signals.
  // Therefore a proper shutdown event to unblock the UI thread is not
  // possible without massive refactoring shutdown code.
  // Luckily Android never performs a clean shutdown. So explicitly
  // ignore this problem.
  base::WaitableEvent never_signaled_;
#endif

  mojom::ChildControlPtr child_control_interface_;
  mojom::RouteProviderAssociatedPtr remote_route_provider_;
  mojom::SlavererAssociatedPtr slaverer_interface_;
  mojo::AssociatedBinding<mojom::SlavererHost> slaverer_host_binding_;

  // A WeakPtrFactory which is reset every time Cleanup() runs. Used to vend
  // WeakPtrs which are invalidated any time the RPHI is recycled.
  std::unique_ptr<base::WeakPtrFactory<SlaverProcessHostImpl>>
      instance_weak_factory_;

  base::WeakPtrFactory<SlaverProcessHostImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SlaverProcessHostImpl);
};

}  // namespace samples

#endif  // SAMPLES_MASTER_SLAVERER_HOST_SLAVER_PROCESS_HOST_IMPL_H_
