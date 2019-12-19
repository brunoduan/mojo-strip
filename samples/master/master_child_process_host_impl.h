// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_MASTER_CHILD_PROCESS_HOST_IMPL_H_
#define SAMPLES_MASTER_MASTER_CHILD_PROCESS_HOST_IMPL_H_

#include <stdint.h>

#include <list>
#include <memory>

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/process/process.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event_watcher.h"
#include "build/build_config.h"
#include "samples/master/child_process_launcher.h"
#include "samples/public/master/master_child_process_host.h"
#include "samples/public/master/child_process_data.h"
#include "samples/public/common/child_process_host_delegate.h"
#include "mojo/public/cpp/system/invitation.h"

namespace base {
class CommandLine;
}

namespace samples {

class MasterChildProcessHostIterator;
class MasterChildProcessObserver;
class MasterMessageFilter;
class ChildConnection;

// Plugins/workers and other child processes that live on the IO thread use this
// class. RenderProcessHostImpl is the main exception that doesn't use this
/// class because it lives on the UI thread.
class SAMPLES_EXPORT MasterChildProcessHostImpl
    : public MasterChildProcessHost,
      public ChildProcessHostDelegate,
      public ChildProcessLauncher::Client {
 public:
  MasterChildProcessHostImpl(samples::ProcessType process_type,
                              MasterChildProcessHostDelegate* delegate,
                              const std::string& service_name);
  ~MasterChildProcessHostImpl() override;

  // Terminates all child processes and deletes each MasterChildProcessHost
  // instance.
  static void TerminateAll();

  // Copies kEnableFeatures and kDisableFeatures to the command line. Generates
  // them from the FeatureList override state, to take into account overrides
  // from FieldTrials.
  static void CopyFeatureAndFieldTrialFlags(base::CommandLine* cmd_line);

  // MasterChildProcessHost implementation:
  bool Send(IPC::Message* message) override;
  void Launch(std::unique_ptr<SandboxedProcessLauncherDelegate> delegate,
              std::unique_ptr<base::CommandLine> cmd_line,
              bool terminate_on_shutdown) override;
  const ChildProcessData& GetData() const override;
  ChildProcessHost* GetHost() const override;
  ChildProcessTerminationInfo GetTerminationInfo(bool known_dead) override;
  void SetName(const base::string16& name) override;
  void SetMetricsName(const std::string& metrics_name) override;
  void SetHandle(base::ProcessHandle handle) override;
  service_manager::mojom::ServiceRequest TakeInProcessServiceRequest() override;

  // ChildProcessHostDelegate implementation:
  void OnChannelInitialized(IPC::Channel* channel) override;
  void OnChildDisconnected() override;
  const base::Process& GetProcess() const override;
  void BindInterface(const std::string& interface_name,
                     mojo::ScopedMessagePipeHandle interface_pipe) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnChannelConnected(int32_t peer_pid) override;
  void OnChannelError() override;
  void OnBadMessageReceived(const IPC::Message& message) override;

  // Terminates the process and logs a stack trace after a bad message was
  // received from the child process.
  void TerminateOnBadMessageReceived(const std::string& error);

  // Removes this host from the host list. Calls ChildProcessHost::ForceShutdown
  void ForceShutdown();

  // Adds an IPC message filter.
  void AddFilter(MasterMessageFilter* filter);

  MasterChildProcessHostDelegate* delegate() const { return delegate_; }

  ChildConnection* child_connection() const {
    return child_connection_.get();
  }

  mojo::OutgoingInvitation* GetInProcessMojoInvitation() {
    return &mojo_invitation_;
  }

  IPC::Channel* child_channel() const { return channel_; }

  typedef std::list<MasterChildProcessHostImpl*> MasterChildProcessList;
 private:
  friend class MasterChildProcessHostIterator;
  friend class MasterChildProcessObserver;

  static MasterChildProcessList* GetIterator();

  static void AddObserver(MasterChildProcessObserver* observer);
  static void RemoveObserver(MasterChildProcessObserver* observer);

  // ChildProcessLauncher::Client implementation.
  void OnProcessLaunched() override;
  void OnProcessLaunchFailed(int error_code) override;

  // Returns true if the process has successfully launched. Must only be called
  // on the IO thread.
  bool IsProcessLaunched() const;

  static void OnMojoError(
      base::WeakPtr<MasterChildProcessHostImpl> process,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      const std::string& error);

  ChildProcessData data_;
  std::string metrics_name_;
  MasterChildProcessHostDelegate* delegate_;
  std::unique_ptr<ChildProcessHost> child_process_host_;

  mojo::OutgoingInvitation mojo_invitation_;
  std::unique_ptr<ChildConnection> child_connection_;

  std::unique_ptr<ChildProcessLauncher> child_process_;

  // The memory allocator, if any, in which the process will write its metrics.
  std::unique_ptr<base::SharedPersistentMemoryAllocator> metrics_allocator_;

  IPC::Channel* channel_ = nullptr;
  bool is_channel_connected_;
  bool notify_child_disconnected_;

  base::WeakPtrFactory<MasterChildProcessHostImpl> weak_factory_;
};

}  // namespace samples

#endif  // SAMPLES_MASTER_MASTER_CHILD_PROCESS_HOST_IMPL_H_
