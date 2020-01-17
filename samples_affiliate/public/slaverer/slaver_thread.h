// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_SLAVERER_SLAVER_THREAD_H_
#define SAMPLES_PUBLIC_SLAVERER_SLAVER_THREAD_H_

#include <stddef.h>
#include <stdint.h>

#include "base/callback.h"
#include "base/memory/shared_memory.h"
#include "base/metrics/user_metrics_action.h"
#include "base/single_thread_task_runner.h"
#include "samples/common/export.h"
#include "samples/public/child/child_thread.h"
#include "ipc/ipc_channel_proxy.h"

class GURL;

namespace base {
class WaitableEvent;
}

namespace blink {
namespace scheduler {
enum class SlavererProcessType;
}
}  // namespace blink

namespace IPC {
class MessageFilter;
class SyncChannel;
class SyncMessageFilter;
}

namespace samples {

class SlaverThreadObserver;
class ResourceDispatcherDelegate;

class SAMPLES_EXPORT SlaverThread : virtual public ChildThread {
 public:
  // Returns the one render thread for this process.  Note that this can only
  // be accessed when running on the render thread itself.
  static SlaverThread* Get();

  SlaverThread();
  ~SlaverThread() override;

  virtual IPC::SyncChannel* GetChannel() = 0;
  virtual std::string GetLocale() = 0;
  virtual IPC::SyncMessageFilter* GetSyncMessageFilter() = 0;

  // Called to add or remove a listener for a particular message routing ID.
  // These methods normally get delegated to a MessageRouter.
  virtual void AddRoute(int32_t routing_id, IPC::Listener* listener) = 0;
  virtual void RemoveRoute(int32_t routing_id) = 0;
  virtual int GenerateRoutingID() = 0;

  // These map to IPC::ChannelProxy methods.
  virtual void AddFilter(IPC::MessageFilter* filter) = 0;
  virtual void RemoveFilter(IPC::MessageFilter* filter) = 0;

  // Add/remove observers for the process.
  virtual void AddObserver(SlaverThreadObserver* observer) = 0;
  virtual void RemoveObserver(SlaverThreadObserver* observer) = 0;

  // Gets the shutdown event for the process.
  virtual base::WaitableEvent* GetShutdownEvent() = 0;

  // Retrieve the process ID of the browser process.
  virtual int32_t GetClientId() = 0;

  // Set the renderer process type.
  virtual void SetSlavererProcessType(
      blink::scheduler::SlavererProcessType type) = 0;
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_SLAVERER_SLAVER_THREAD_H_
