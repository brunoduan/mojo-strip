// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_SLAVER_PROCESS_HOST_H_
#define SAMPLES_PUBLIC_MASTER_SLAVER_PROCESS_HOST_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>
#include <string>

#include "base/containers/id_map.h"
#include "base/process/kill.h"
#include "base/process/process.h"
#include "base/supports_user_data.h"
#include "build/build_config.h"
#include "samples/common/export.h"
#include "samples/public/common/bind_interface_helpers.h"
#include "ipc/ipc_channel_proxy.h"
#include "ipc/ipc_sender.h"

#if defined(OS_ANDROID)
#include "samples/public/master/android/child_process_importance.h"
#endif

class GURL;

namespace base {
class SharedPersistentMemoryAllocator;
class TimeDelta;
}

namespace service_manager {
class Identity;
}

namespace samples {
class MasterContext;
class MasterMessageFilter;
class SlaverProcessHostObserver;

#if defined(OS_ANDROID)
enum class ChildProcessImportance;
#endif

namespace mojom {
class Slaverer;
}

// Interface that represents the master side of the master <-> slaverer
// communication channel. There will generally be one SlaverProcessHost per
// slaverer process.
class SAMPLES_EXPORT SlaverProcessHost : public IPC::Sender,
                                         public IPC::Listener,
                                         public base::SupportsUserData {
 public:
  using iterator = base::IDMap<SlaverProcessHost*>::iterator;

  // Priority (or on Android, the importance) that a client contributes to this
  // SlaverProcessHost. Eg a SlaverProcessHost with a visible client has higher
  // priority / importance than a SlaverProcessHost with hidden clients only.
  struct Priority {
    bool is_hidden;
    unsigned int frame_depth;
    bool intersects_viewport;
#if defined(OS_ANDROID)
    ChildProcessImportance importance;
#endif
  };

  // Interface for a client that contributes Priority to this
  // SlaverProcessHost. Clients can call UpdateClientPriority when their
  // Priority changes.
  class PriorityClient {
   public:
    virtual Priority GetPriority() = 0;

   protected:
    virtual ~PriorityClient() {}
  };

  // Crash reporting mode for ShutdownForBadMessage.
  enum class CrashReportMode {
    NO_CRASH_DUMP,
    GENERATE_CRASH_DUMP,
  };

  // General functions ---------------------------------------------------------

  ~SlaverProcessHost() override {}

  // Initialize the new slaverer process, returning true on success. This must
  // be called once before the object can be used, but can be called after
  // that with no effect. Therefore, if the caller isn't sure about whether
  // the process has been created, it should just call Init().
  virtual bool Init() = 0;

  // Ensures that a Channel exists and is at least queueing outgoing messages
  // if there isn't a slaver process connected to it yet. This may be used to
  // ensure that in the event of a slaverer crash and restart, subsequent
  // messages sent via Send() will eventually reach the new process.
  virtual void EnableSendQueue() = 0;

  // These methods add or remove listener for a specific message routing ID.
  // Used for refcounting, each holder of this object must AddRoute and
  // RemoveRoute. This object should be allocated on the heap; when no
  // listeners own it any more, it will delete itself.
  virtual void AddRoute(int32_t routing_id, IPC::Listener* listener) = 0;
  virtual void RemoveRoute(int32_t routing_id) = 0;

  // Add and remove observers for lifecycle events. The order in which
  // notifications are sent to observers is undefined. Observers must be sure to
  // remove the observer before they go away.
  virtual void AddObserver(SlaverProcessHostObserver* observer) = 0;
  virtual void RemoveObserver(SlaverProcessHostObserver* observer) = 0;

  // Called when a received message cannot be decoded. Terminates the slaverer.
  // Most callers should not call this directly, but instead should call
  // bad_message::BadMessageReceived() or an equivalent method outside of the
  // samples module.
  //
  // If |crash_report_mode| is GENERATE_CRASH_DUMP, then a master crash dump
  // will be reported as well.
  virtual void ShutdownForBadMessage(CrashReportMode crash_report_mode) = 0;

  // Recompute Priority state. PriorityClient should call this when their
  // individual priority changes.
  virtual void UpdateClientPriority(PriorityClient* client) = 0;

  // Number of visible (ie |!is_hidden|) PriorityClients.
  virtual int VisibleClientCount() const = 0;

  // Get computed frame depth from PriorityClients.
  virtual unsigned int GetFrameDepth() const = 0;

  // Get computed viewport intersection state from PriorityClients.
  virtual bool GetIntersectsViewport() const = 0;

  // Indicates whether the current SlaverProcessHost is exclusively hosting
  // guest SlaverFrames. Not all guest SlaverFrames are created equal.  A guest,
  // as indicated by MasterPluginGuest::IsGuest, may coexist with other
  // non-guest SlaverFrames in the same process if IsForGuestsOnly() is false.
  virtual bool IsForGuestsOnly() const = 0;

  // Try to shut down the associated slaverer process without running unload
  // handlers, etc, giving it the specified exit code.  Returns true
  // if it was able to shut down.  On Windows, this must not be called before
  // SlaverProcessReady was called on a SlaverProcessHostObserver, otherwise
  // SlaverProcessExited may never be called.
  virtual bool Shutdown(int exit_code) = 0;

  // Try to shut down the associated slaverer process as fast as possible.
  // If a non-zero |page_count| value is provided, then a fast shutdown will
  // only happen if the count matches the active view count. If
  // |skip_unload_handlers| is false and this slaverer has any SlaverViews with
  // unload handlers, then this function does nothing. Otherwise, the function
  // will ingnore checking for those handlers. Returns true if it was able to do
  // fast shutdown.
  virtual bool FastShutdownIfPossible(size_t page_count = 0,
                                      bool skip_unload_handlers = false) = 0;

  // Returns true if fast shutdown was started for the slaverer.
  virtual bool FastShutdownStarted() const = 0;

  // Returns the process object associated with the child process.  In certain
  // tests or single-process mode, this will actually represent the current
  // process.
  //
  // NOTE: this is not necessarily valid immediately after calling Init, as
  // Init starts the process asynchronously.  It's guaranteed to be valid after
  // the first IPC arrives or SlaverProcessReady was called on a
  // SlaverProcessHostObserver for this. At that point, IsReady() returns true.
  virtual const base::Process& GetProcess() const = 0;

  // Returns whether the process is ready. The process is ready once both
  // conditions (which can happen in arbitrary order) are true:
  // 1- the launcher reported a successful launch
  // 2- the channel is connected.
  //
  // After that point, GetHandle() is valid, and deferred messages have been
  // sent.
  virtual bool IsReady() const = 0;

  // Returns the user master context associated with this slaverer process.
  virtual samples::MasterContext* GetMasterContext() const = 0;

  // Returns the unique ID for this child process host. This can be used later
  // in a call to FromID() to get back to this object (this is used to avoid
  // sending non-threadsafe pointers to other threads).
  //
  // This ID will be unique across all child process hosts, including workers,
  // plugins, etc.
  //
  // This will never return ChildProcessHost::kInvalidUniqueID.
  virtual int GetID() const = 0;

  // Returns true iff the Init() was called and the process hasn't died yet.
  //
  // Note that even if IsInitializedAndNotDead() returns true, then (for a short
  // duration after calling Init()) the process might not be fully spawned
  // *yet*.  For example - IsReady() might return false and GetProcess() might
  // still return an invalid process with a null handle.
  virtual bool IsInitializedAndNotDead() const = 0;

  // Returns the slaverer channel.
  virtual IPC::ChannelProxy* GetChannel() = 0;

  // Adds a message filter to the IPC channel.
  virtual void AddFilter(MasterMessageFilter* filter) = 0;

  // Schedules the host for deletion and removes it from the all_hosts list.
  virtual void Cleanup() = 0;

#if defined(OS_ANDROID)
  // Return the highest importance of all widgets in this process.
  virtual ChildProcessImportance GetEffectiveImportance() = 0;
#endif

  // Sets a flag indicating that the process can be abnormally terminated.
  virtual void SetSuddenTerminationAllowed(bool allowed) = 0;
  // Returns true if the process can be abnormally terminated.
  virtual bool SuddenTerminationAllowed() const = 0;

  // Returns how long the child has been idle. The definition of idle
  // depends on when a derived class calls mark_child_process_activity_time().
  // This is a rough indicator and its resolution should not be better than
  // 10 milliseconds.
  virtual base::TimeDelta GetChildProcessIdleTime() const = 0;

  // Checks that the given slaverer can request |url|, if not it sets it to
  // about:blank.
  // |empty_allowed| must be set to false for navigations for security reasons.
  virtual void FilterURL(bool empty_allowed, GURL* url) = 0;

  // Binds interfaces exposed to the master process from the slaverer.
  virtual void BindInterface(const std::string& interface_name,
                             mojo::ScopedMessagePipeHandle interface_pipe) = 0;

  virtual const service_manager::Identity& GetChildIdentity() const = 0;

  // Returns true if this process currently has backgrounded priority.
  virtual bool IsProcessBackgrounded() const = 0;

  enum class KeepAliveClientType {
    kServiceWorker = 0,
    kSharedWorker = 1,
    kFetch = 2,
    kUnload = 3,
  };
  // "Keep alive ref count" represents the number of the customers of this
  // slaver process who wish the slaverer process to be alive. While the ref
  // count is positive, |this| object will keep the slaverer process alive,
  // unless DisableKeepAliveRefCount() is called.
  //
  // Here is the list of users:
  //  - Service Worker:
  //    While there are service workers who live in this process, they wish
  //    the slaverer process to be alive. The ref count is incremented when this
  //    process is allocated to the worker, and decremented when worker's
  //    shutdown sequence is completed.
  //  - Shared Worker:
  //    While there are shared workers who live in this process, they wish
  //    the slaverer process to be alive. The ref count is incremented when
  //    a shared worker is created in the process, and decremented when
  //    it is terminated (it self-destructs when it no longer has clients).
  //  - Keepalive request (if the KeepAliveSlavererForKeepaliveRequests
  //    feature is enabled):
  //    When a fetch request with keepalive flag
  //    (https://fetch.spec.whatwg.org/#request-keepalive-flag) specified is
  //    pending, it wishes the slaverer process to be kept alive.
  //  - Unload handlers:
  //    Keeps the process alive briefly to give subframe unload handlers a
  //    chance to execute after their parent frame navigates or is detached.
  //    See https://crbug.com/852204.
  virtual void IncrementKeepAliveRefCount(KeepAliveClientType) = 0;
  virtual void DecrementKeepAliveRefCount(KeepAliveClientType) = 0;

  // Sets keep alive ref counts to zero. Called when the master context will be
  // destroyed so this SlaverProcessHost can immediately die.
  //
  // After this is called, the Increment/DecrementKeepAliveRefCount() functions
  // must not be called.
  virtual void DisableKeepAliveRefCount() = 0;

  // Returns true if DisableKeepAliveRefCount() was called.
  virtual bool IsKeepAliveRefCountDisabled() = 0;

  // Purges and suspends the slaverer process.
  virtual void PurgeAndSuspend() = 0;

  // Resumes the slaverer process.
  virtual void Resume() = 0;

  // Acquires the |mojom::Slaverer| interface to the slaver process. This is for
  // internal use only, and is only exposed here to support
  // MockSlaverProcessHost usage in tests.
  virtual mojom::Slaverer* GetSlavererInterface() = 0;

  // Whether this process is locked out from ever being reused for sites other
  // than the ones it currently has.
  virtual void SetIsNeverSuitableForReuse() = 0;
  virtual bool MayReuseHost() = 0;

  // Indicates whether this SlaverProcessHost is "unused".  This starts out as
  // true for new processes and becomes false after one of the following:
  // (1) This process commits any page.
  // (2) This process is given to a SiteInstance that already has a site
  //     assigned.
  // Note that a process hosting ServiceWorkers will be implicitly handled by
  // (2) during ServiceWorker initialization, and SharedWorkers will be handled
  // by (1) since a page needs to commit before it can create a SharedWorker.
  //
  // While a process is unused, it is still suitable to host a URL that
  // requires a dedicated process.
  virtual bool IsUnused() = 0;
  virtual void SetIsUsed() = 0;

  // Return true if the host has not been used. This is stronger than IsUnused()
  // in that it checks if this RPH has ever been used to slaver at all, rather
  // than just no being suitable to host a URL that requires a dedicated
  // process.
  // TODO(alexmos): can this be unified with IsUnused()? See also
  // crbug.com/738634.
  virtual bool HostHasNotBeenUsed() = 0;

  // Locks this SlaverProcessHost to the 'origin' |lock_url|. This method is
  // public so that it can be called from SiteInstanceImpl, and used by
  // MockSlaverProcessHost. It isn't meant to be called outside of samples.
  // TODO(creis): Rename LockToOrigin to LockToPrincipal. See
  // https://crbug.com/846155.
  virtual void LockToOrigin(const GURL& lock_url) = 0;

  // Posts |task|, if this SlaverProcessHost is ready or when it becomes ready
  // (see SlaverProcessHost::IsReady method).  The |task| might not run at all
  // (e.g. if |slaver_process_host| is destroyed before becoming ready).  This
  // function can only be called on the master's UI thread (and the |task| will
  // be posted back on the UI thread).
  void PostTaskWhenProcessIsReady(base::OnceClosure task);

  // Static management functions -----------------------------------------------

  // Possibly start an unbound, spare SlaverProcessHost. A subsequent creation
  // of a SlaverProcessHost with a matching master_context may use this
  // preinitialized SlaverProcessHost, improving performance.
  //
  // It is safe to call this multiple times or when it is not certain that the
  // spare slaverer will be used, although calling this too eagerly may reduce
  // performance as unnecessary SlaverProcessHosts are created. The spare
  // slaverer will only be used if it using the default StoragePartition of a
  // matching MasterContext.
  //
  // The spare SlaverProcessHost is meant to be created in a situation where a
  // navigation is imminent and it is unlikely an existing SlaverProcessHost
  // will be used, for example in a cross-site navigation when a Service Worker
  // will need to be started.  Note that if ContentMasterClient opts into
  // strict site isolation (via ShouldEnableStrictSiteIsolation), then the
  // //samples layer will maintain a warm spare process host at all times
  // (without a need for separate calls to WarmupSpareSlaverProcessHost).
  static void WarmupSpareSlaverProcessHost(MasterContext* master_context);

  // Flag to run the slaverer in process.  This is primarily
  // for debugging purposes.  When running "in process", the
  // master maintains a single SlaverProcessHost which communicates
  // to a SlaverProcess which is instantiated in the same process
  // with the Master.  All IPC between the Master and the
  // Slaverer is the same, it's just not crossing a process boundary.
  static bool run_slaverer_in_process();

  // This also calls out to ContentMasterClient::GetApplicationLocale and
  // modifies the current process' command line.
  static void SetRunSlavererInProcess(bool value);

  // Allows iteration over all the SlaverProcessHosts in the master. Note
  // that each host may not be active, and therefore may have nullptr channels.
  static iterator AllHostsIterator();

  // Returns the SlaverProcessHost given its ID.  Returns nullptr if the ID does
  // not correspond to a live SlaverProcessHost.
  static SlaverProcessHost* FromID(int slaver_process_id);

  // Returns the SlaverProcessHost given its slaverer's service Identity.
  // Returns nullptr if the Identity does not correspond to a live
  // SlaverProcessHost.
  static SlaverProcessHost* FromSlavererIdentity(
      const service_manager::Identity& identity);

  // Returns whether the process-per-site model is in use (globally or just for
  // the current site), in which case we should ensure there is only one
  // SlaverProcessHost per site for the entire master context.
  static bool ShouldUseProcessPerSite(samples::MasterContext* master_context,
                                      const GURL& url);

  // Returns true if the caller should attempt to use an existing
  // SlaverProcessHost rather than creating a new one.
  static bool ShouldTryToUseExistingProcessHost(
      samples::MasterContext* master_context, const GURL& site_url);

  // Overrides the default heuristic for limiting the max slaverer process
  // count.  This is useful for unit testing process limit behaviors.  It is
  // also used to allow a command line parameter to configure the max number of
  // slaverer processes and should only be called once during startup.
  // A value of zero means to use the default heuristic.
  static void SetMaxSlavererProcessCount(size_t count);

  // Returns the current maximum number of slaverer process hosts kept by the
  // samples module.
  static size_t GetMaxSlavererProcessCount();

  // TODO(siggi): Remove once https://crbug.com/806661 is resolved.
  using AnalyzeHungSlavererFunction = void (*)(const base::Process& slaverer);
  static void SetHungSlavererAnalysisFunction(
      AnalyzeHungSlavererFunction analyze_hung_slaverer);

  // Counts current SlaverProcessHost(s), ignoring the spare process.
  static int GetCurrentSlaverProcessCountForTesting();
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_SLAVER_PROCESS_HOST_H_
