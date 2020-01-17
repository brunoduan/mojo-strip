// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/master/master_message_filter.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/debug/dump_without_crashing.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/process/process_handle.h"
#include "base/task/post_task.h"
#include "base/task_runner.h"
#include "build/build_config.h"
#include "samples/master/child_process_launcher.h"
#include "samples/public/master/master_task_traits.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/result_codes.h"
#include "ipc/ipc_sync_message.h"
#include "ipc/message_filter.h"

using samples::MasterMessageFilter;

namespace samples {

class MasterMessageFilter::Internal : public IPC::MessageFilter {
 public:
  explicit Internal(MasterMessageFilter* filter) : filter_(filter) {}

 private:
  ~Internal() override {}

  // IPC::MessageFilter implementation:
  void OnFilterAdded(IPC::Channel* channel) override {
    filter_->sender_ = channel;
    filter_->OnFilterAdded(channel);
  }

  void OnFilterRemoved() override {
    for (auto& callback : filter_->filter_removed_callbacks_)
      std::move(callback).Run();
    filter_->filter_removed_callbacks_.clear();
    filter_->OnFilterRemoved();
  }

  void OnChannelClosing() override {
    filter_->sender_ = nullptr;
    filter_->OnChannelClosing();
  }

  void OnChannelError() override { filter_->OnChannelError(); }

  void OnChannelConnected(int32_t peer_pid) override {
    filter_->peer_process_ = base::Process::OpenWithExtraPrivileges(peer_pid);
    filter_->OnChannelConnected(peer_pid);
  }

  bool OnMessageReceived(const IPC::Message& message) override {
    MasterThread::ID thread = MasterThread::IO;
    filter_->OverrideThreadForMessage(message, &thread);

    if (thread == MasterThread::IO) {
      scoped_refptr<base::TaskRunner> runner =
          filter_->OverrideTaskRunnerForMessage(message);
      if (runner.get()) {
        runner->PostTask(
            FROM_HERE,
            base::BindOnce(base::IgnoreResult(&Internal::DispatchMessage), this,
                           message));
        return true;
      }
      return DispatchMessage(message);
    }

    base::PostTaskWithTraits(
        FROM_HERE, {thread},
        base::BindOnce(base::IgnoreResult(&Internal::DispatchMessage), this,
                       message));
    return true;
  }

  bool GetSupportedMessageClasses(
      std::vector<uint32_t>* supported_message_classes) const override {
    supported_message_classes->assign(
        filter_->message_classes_to_filter().begin(),
        filter_->message_classes_to_filter().end());
    return true;
  }

  // Dispatches a message to the derived class.
  bool DispatchMessage(const IPC::Message& message) {
    bool rv = filter_->OnMessageReceived(message);
    DCHECK(MasterThread::CurrentlyOn(MasterThread::IO) || rv) <<
        "Must handle messages that were dispatched to another thread!";
    return rv;
  }

  scoped_refptr<MasterMessageFilter> filter_;

  DISALLOW_COPY_AND_ASSIGN(Internal);
};

MasterMessageFilter::MasterMessageFilter(uint32_t message_class_to_filter)
    : internal_(nullptr),
      sender_(nullptr),
      message_classes_to_filter_(1, message_class_to_filter) {}

MasterMessageFilter::MasterMessageFilter(
    const uint32_t* message_classes_to_filter,
    size_t num_message_classes_to_filter)
    : internal_(nullptr),
      sender_(nullptr),
      message_classes_to_filter_(
          message_classes_to_filter,
          message_classes_to_filter + num_message_classes_to_filter) {
  DCHECK(num_message_classes_to_filter);
}

void MasterMessageFilter::AddAssociatedInterface(
    const std::string& name,
    const IPC::ChannelProxy::GenericAssociatedInterfaceFactory& factory,
    base::OnceClosure filter_removed_callback) {
  associated_interfaces_.emplace_back(name, factory);
  filter_removed_callbacks_.emplace_back(std::move(filter_removed_callback));
}

base::ProcessHandle MasterMessageFilter::PeerHandle() {
  return peer_process_.Handle();
}

void MasterMessageFilter::OnDestruct() const {
  delete this;
}

bool MasterMessageFilter::Send(IPC::Message* message) {
  if (message->is_sync()) {
    // We don't support sending synchronous messages from the master.  If we
    // really needed it, we can make this class derive from SyncMessageFilter
    // but it seems better to not allow sending synchronous messages from the
    // master, since it might allow a corrupt/malicious renderer to hang us.
    NOTREACHED() << "Can't send sync message through MasterMessageFilter!";
    return false;
  }

  if (!MasterThread::CurrentlyOn(MasterThread::IO)) {
    base::PostTaskWithTraits(
        FROM_HERE, {MasterThread::IO},
        base::BindOnce(base::IgnoreResult(&MasterMessageFilter::Send), this,
                       message));
    return true;
  }

  if (sender_)
    return sender_->Send(message);

  delete message;
  return false;
}

base::TaskRunner* MasterMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  return nullptr;
}

void MasterMessageFilter::ShutdownForBadMessage() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kDisableKillAfterBadIPC))
    return;

  if (base::Process::Current().Handle() == peer_process_.Handle()) {
    // Just crash in single process. Matches RenderProcessHostImpl behavior.
    CHECK(false);
  }

  ChildProcessLauncher::TerminateProcess(
      peer_process_, samples::RESULT_CODE_KILLED_BAD_MESSAGE);

  // Report a crash, since none will be generated by the killed renderer.
  base::debug::DumpWithoutCrashing();

}

MasterMessageFilter::~MasterMessageFilter() {
}

IPC::MessageFilter* MasterMessageFilter::GetFilter() {
  // We create this on demand so that if a filter is used in a unit test but
  // never attached to a channel, we don't leak Internal and this;
  DCHECK(!internal_) << "Should only be called once.";
  internal_ = new Internal(this);
  return internal_;
}

void MasterMessageFilter::RegisterAssociatedInterfaces(
    IPC::ChannelProxy* proxy) {
  for (const auto& entry : associated_interfaces_)
    proxy->AddGenericAssociatedInterfaceForIOThread(entry.first, entry.second);
  associated_interfaces_.clear();
}

}  // namespace samples
