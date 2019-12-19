// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_WEB_THREAD_SCHEDULER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_WEB_THREAD_SCHEDULER_H_

#include <memory>
#include "base/macros.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "third_party/blink/public/platform/scheduler/single_thread_idle_task_runner.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_scoped_virtual_time_pauser.h"

namespace blink {
class Thread;
}  // namespace blink

namespace blink {
namespace scheduler {

enum class RendererProcessType;

class BLINK_PLATFORM_EXPORT WebThreadScheduler {
 public:
  virtual ~WebThreadScheduler();

  // ==== Functions for any scheduler =========================================
  //
  // Functions below work on a scheduler instance on any thread.

  // Returns the idle task runner. Tasks posted to this runner may be reordered
  // relative to other task types and may be starved for an arbitrarily long
  // time if no idle time is available.
  virtual scoped_refptr<SingleThreadIdleTaskRunner> IdleTaskRunner() = 0;

  // Shuts down the scheduler by dropping any remaining pending work in the work
  // queues. After this call any work posted to the task runners will be
  // silently dropped.
  virtual void Shutdown() = 0;

  // ==== Functions for the main thread scheduler  ============================
  //
  // Virtual functions below should only be called against the scheduler on
  // the main thread. They have default implementation that only does
  // NOTREACHED(), and are overridden only by the main thread scheduler.

  // If |initial_virtual_time| is specified then the scheduler will be created
  // with virtual time enabled and paused, and base::Time will be overridden to
  // start at |initial_virtual_time|.
  static std::unique_ptr<WebThreadScheduler> CreateMainThreadScheduler(
      base::Optional<base::Time> initial_virtual_time = base::nullopt);

  // Returns the default task runner.
  virtual scoped_refptr<base::SingleThreadTaskRunner> DefaultTaskRunner();

  virtual scoped_refptr<base::SingleThreadTaskRunner> IPCTaskRunner();

  // Returns the cleanup task runner, which is for cleaning up.
  virtual scoped_refptr<base::SingleThreadTaskRunner> CleanupTaskRunner();

  // Creates a WebThread implementation for the renderer main thread.
  virtual std::unique_ptr<Thread> CreateMainThread();

  // Tells the scheduler about the change of renderer visibility status (e.g.
  // "all widgets are hidden" condition). Used mostly for metric purposes.
  // Must be called on the main thread.
  virtual void SetRendererHidden(bool hidden);

  // Tells the scheduler about the change of renderer background status, i.e.,
  // there are no critical, user facing activities (visual, audio, etc...)
  // driven by this process. A stricter condition than |SetRendererHidden()|,
  // the process is assumed to be foregrounded when the scheduler is
  // constructed. Must be called on the main thread.
  virtual void SetRendererBackgrounded(bool backgrounded);

  // Tells the scheduler about "keep-alive" state which can be due to:
  // service workers, shared workers, or fetch keep-alive.
  // If set to true, then the scheduler should not freeze the renderer.
  virtual void SetSchedulerKeepActive(bool keep_active);

  // RAII handle for pausing the renderer. Renderer is paused while
  // at least one pause handle exists.
  class BLINK_PLATFORM_EXPORT RendererPauseHandle {
   public:
    RendererPauseHandle() = default;
    virtual ~RendererPauseHandle() = default;

   private:
    DISALLOW_COPY_AND_ASSIGN(RendererPauseHandle);
  };

  // Tells the scheduler that the renderer process should be paused.
  // Pausing means that all javascript callbacks should not fire.
  // https://html.spec.whatwg.org/#pause
  //
  // Renderer will be resumed when the handle is destroyed.
  // Handle should be destroyed before the renderer.
  virtual std::unique_ptr<RendererPauseHandle> PauseRenderer()
      WARN_UNUSED_RESULT;

  // Returns true if the scheduler has reason to believe that high priority work
  // may soon arrive on the main thread, e.g., if gesture events were observed
  // recently.
  // Must be called from the main thread.
  virtual bool IsHighPriorityWorkAnticipated();

  // Sets the kind of renderer process. Should be called on the main thread
  // once.
  virtual void SetRendererProcessType(RendererProcessType type);

  // Returns a WebScopedVirtualTimePauser which can be used to vote for pausing
  // virtual time. Virtual time will be paused if any WebScopedVirtualTimePauser
  // votes to pause it, and only unpaused only if all
  // WebScopedVirtualTimePausers are either destroyed or vote to unpause.  Note
  // the WebScopedVirtualTimePauser returned by this method is initially
  // unpaused.
  virtual WebScopedVirtualTimePauser CreateWebScopedVirtualTimePauser(
      const char* name,
      WebScopedVirtualTimePauser::VirtualTaskDuration duration =
          WebScopedVirtualTimePauser::VirtualTaskDuration::kNonInstant);

 protected:
  WebThreadScheduler() = default;
  DISALLOW_COPY_AND_ASSIGN(WebThreadScheduler);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_WEB_THREAD_SCHEDULER_H_
