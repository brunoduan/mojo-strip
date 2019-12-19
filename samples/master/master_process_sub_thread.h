// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_MASTER_PROCESS_SUB_THREAD_H_
#define SAMPLES_MASTER_MASTER_PROCESS_SUB_THREAD_H_

#include <memory>

#include "base/macros.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "build/build_config.h"
#include "samples/common/export.h"
#include "samples/public/master/master_thread.h"

#if defined(OS_WIN)
namespace base {
namespace win {
class ScopedCOMInitializer;
}
}
#endif

namespace samples {
class NotificationService;
}

namespace samples {

// ----------------------------------------------------------------------------
// A MasterProcessSubThread is a physical thread backing a MasterThread.
//
// Applications must initialize the COM library before they can call
// COM library functions other than CoGetMalloc and memory allocation
// functions, so this class initializes COM for those users.
class SAMPLES_EXPORT MasterProcessSubThread : public base::Thread {
 public:
  // Constructs a MasterProcessSubThread for |identifier|.
  explicit MasterProcessSubThread(MasterThread::ID identifier);
  ~MasterProcessSubThread() override;

  // Registers this thread to represent |identifier_| in the master_thread.h
  // API. This thread must already be running when this is called. This can only
  // be called once per MasterProcessSubThread instance.
  void RegisterAsMasterThread();

  // Ideally there wouldn't be a special blanket allowance to block the
  // MasterThreads in tests but TestMasterThreadImpl previously bypassed
  // MasterProcessSubThread and hence wasn't subject to ThreadRestrictions...
  // Flipping that around in favor of explicit scoped allowances would be
  // preferable but a non-trivial amount of work. Can only be called before
  // starting this MasterProcessSubThread.
  void AllowBlockingForTesting();

  // Creates and starts the IO thread. It should not be promoted to
  // MasterThread::IO until MasterMainLoop::CreateThreads().
  static std::unique_ptr<MasterProcessSubThread> CreateIOThread();

 protected:
  void Init() override;
  void Run(base::RunLoop* run_loop) override;
  void CleanUp() override;

 private:
  // Second Init() phase that must happen on this thread but can only happen
  // after it's promoted to a MasterThread in |RegisterAsMasterThread()|.
  void CompleteInitializationOnMasterThread();

  // These methods merely forwards to Thread::Run() but are useful to identify
  // which MasterThread this represents in stack traces.
  void UIThreadRun(base::RunLoop* run_loop);
  void IOThreadRun(base::RunLoop* run_loop);

  // This method encapsulates cleanup that needs to happen on the IO thread.
  void IOThreadCleanUp();

  const MasterThread::ID identifier_;

  // MasterThreads are not allowed to do file I/O nor wait on synchronization
  // primivives except when explicitly allowed in tests.
  bool is_blocking_allowed_for_testing_ = false;

  // The MasterThread registration for this |identifier_|, initialized in
  // RegisterAsMasterThread().
  std::unique_ptr<MasterThreadImpl> master_thread_;

  // Each specialized thread has its own notification service.
  std::unique_ptr<NotificationService> notification_service_;

  THREAD_CHECKER(master_thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(MasterProcessSubThread);
};

}  // namespace samples

#endif  // SAMPLES_MASTER_MASTER_PROCESS_SUB_THREAD_H_
