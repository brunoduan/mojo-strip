// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SLAVERER_SLAVER_THREAD_IMPL_H_
#define SAMPLES_SLAVERER_SLAVER_THREAD_IMPL_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/cancelable_callback.h"
#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/user_metrics_action.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "samples/common/slaver_message_filter.mojom.h"
#include "samples/common/slaverer.mojom.h"
#include "samples/common/slaverer_host.mojom.h"
#include "samples/common/slaver_message_filter.mojom.h"
#include "samples/public/slaverer/slaver_thread.h"
#include "samples/child/child_thread_impl.h"
#include "samples/common/export.h"
#include "ipc/ipc_sync_channel.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/thread_safe_interface_ptr.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

namespace base {
class SingleThreadTaskRunner;
class Thread;
}

namespace IPC {
class MessageFilter;
}

namespace samples {

class SlaverThreadObserver;
class SlavererBlinkPlatformImpl;

#if defined(OS_ANDROID)
class StreamTextureFactory;
#endif

#if defined(COMPILER_MSVC)
// See explanation for other SlaverViewHostImpl which is the same issue.
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

// The SlaverThreadImpl class represents a background thread where SlaverView
// instances live.  The SlaverThread supports an API that is used by its
// consumer to talk indirectly to the SlaverViews and supporting objects.
// Likewise, it provides an API for the SlaverViews to talk back to the main
// process (i.e., their corresponding WebContentsImpl).
//
// Most of the communication occurs in the form of IPC messages.  They are
// routed to the SlaverThread according to the routing IDs of the messages.
// The routing IDs correspond to SlaverView instances.
class SAMPLES_EXPORT SlaverThreadImpl
    : public SlaverThread,
      public ChildThreadImpl,
      public mojom::Slaverer {
 public:
  static SlaverThreadImpl* current();
  static mojom::SlaverMessageFilter* current_slaver_message_filter();
  static SlavererBlinkPlatformImpl* current_blink_platform_impl();

  static void SetSlaverMessageFilterForTesting(
      mojom::SlaverMessageFilter* slaver_message_filter);
  static void SetSlavererBlinkPlatformImplForTesting(
      SlavererBlinkPlatformImpl* blink_platform_impl);

  // Returns the task runner for the main thread where the SlaverThread lives.
  static scoped_refptr<base::SingleThreadTaskRunner>
  DeprecatedGetMainTaskRunner();

  SlaverThreadImpl(
      base::RepeatingClosure quit_closure);
  SlaverThreadImpl(
      const InProcessChildThreadParams& params);
  ~SlaverThreadImpl() override;
  void Shutdown() override;
  bool ShouldBeDestroyed() override;

  mojom::SlavererHost* GetSlavererHost();

  // When initializing WebKit, ensure that any schemes needed for the samples
  // module are registered properly.  Static to allow sharing with tests.
  static void RegisterSchemes();

  // SlaverThread implementation:
  bool Send(IPC::Message* msg) override;
  IPC::SyncChannel* GetChannel() override;
  std::string GetLocale() override;
  IPC::SyncMessageFilter* GetSyncMessageFilter() override;
  void AddRoute(int32_t routing_id, IPC::Listener* listener) override;
  void RemoveRoute(int32_t routing_id) override;
  int GenerateRoutingID() override;
  void AddFilter(IPC::MessageFilter* filter) override;
  void RemoveFilter(IPC::MessageFilter* filter) override;
  void AddObserver(SlaverThreadObserver* observer) override;
  void RemoveObserver(SlaverThreadObserver* observer) override;
  base::WaitableEvent* GetShutdownEvent() override;
  int32_t GetClientId() override;
  void SetSlavererProcessType(
      blink::scheduler::SlavererProcessType type) override;

  // IPC::Listener implementation via ChildThreadImpl:
  void OnAssociatedInterfaceRequest(
      const std::string& name,
      mojo::ScopedInterfaceEndpointHandle handle) override;

  // ChildThread implementation via ChildThreadImpl:
  scoped_refptr<base::SingleThreadTaskRunner> GetIOTaskRunner() override;

  bool IsThreadedAnimationEnabled();

  blink::AssociatedInterfaceRegistry* GetAssociatedInterfaceRegistry();

  mojom::SlaverMessageFilter* slaver_message_filter();

 private:
  void OnProcessFinalRelease() override;
  // IPC::Listener
  void OnChannelError() override;

  // ChildThread
  bool OnControlMessageReceived(const IPC::Message& msg) override;

  bool IsMainThread();

  void Init();
  void InitializeCompositorThread();
  void InitializeWebKit(service_manager::BinderRegistry* registry);

  // mojom::Slaverer:
  void CreateEmbedderSlavererService(
      service_manager::mojom::ServiceRequest service_request) override;
  void SetProcessBackgrounded(bool backgrounded) override;
  void ProcessPurgeAndSuspend() override;

  base::ObserverList<SlaverThreadObserver>::Unchecked observers_;


  mojom::SlaverMessageFilterAssociatedPtr slaver_message_filter_;
  mojom::SlavererHostAssociatedPtr slaverer_host_;

  blink::AssociatedInterfaceRegistry associated_interfaces_;

  mojo::AssociatedBinding<mojom::Slaverer> slaverer_binding_;

  int32_t client_id_;

  base::WeakPtrFactory<SlaverThreadImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SlaverThreadImpl);
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)
#endif

}  // namespace samples

#endif  // SAMPLES_SLAVERER_SLAVER_THREAD_IMPL_H_
