// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_MASTER_THREAD_IMPL_H_
#define SAMPLES_MASTER_MASTER_THREAD_IMPL_H_

#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "samples/common/export.h"
#include "samples/public/master/master_thread.h"

namespace samples {

class MasterMainLoop;
class MasterProcessSubThread;
class TestMasterThread;

// MasterThreadImpl is a scoped object which maps a SingleThreadTaskRunner to a
// MasterThread::ID. On ~MasterThreadImpl() that ID enters a SHUTDOWN state
// (in which MasterThread::IsThreadInitialized() returns false) but the mapping
// isn't undone to avoid shutdown races (the task runner is free to stop
// accepting tasks by then however).
//
// Very few users should use this directly. To mock MasterThreads, tests should
// use TestMasterThreadBundle instead.
class SAMPLES_EXPORT MasterThreadImpl : public MasterThread {
 public:
  ~MasterThreadImpl();

  // Returns the thread name for |identifier|.
  static const char* GetThreadName(MasterThread::ID identifier);

  // Resets globals for |identifier|. Used in tests to clear global state that
  // would otherwise leak to the next test. Globals are not otherwise fully
  // cleaned up in ~MasterThreadImpl() as there are subtle differences between
  // UNINITIALIZED and SHUTDOWN state (e.g. globals.task_runners are kept around
  // on shutdown). Must be called after ~MasterThreadImpl() for the given
  // |identifier|.
  static void ResetGlobalsForTesting(MasterThread::ID identifier);

  // Exposed for MasterTaskExecutor. Other code should use
  // base::CreateSingleThreadTaskRunnerWithTraits({MasterThread::UI/IO}).
  using MasterThread::GetTaskRunnerForThread;

 private:
  // Restrict instantiation to MasterProcessSubThread as it performs important
  // initialization that shouldn't be bypassed (except by MasterMainLoop for
  // the main thread).
  friend class MasterProcessSubThread;
  friend class MasterMainLoop;
  // TestMasterThread is also allowed to construct this when instantiating fake
  // threads.
  friend class TestMasterThread;

  // Binds |identifier| to |task_runner| for the master_thread.h API. This
  // needs to happen on the main thread before //samples and embedders are
  // kicked off and enabled to invoke the MasterThread API from other threads.
  MasterThreadImpl(MasterThread::ID identifier,
                    scoped_refptr<base::SingleThreadTaskRunner> task_runner);

  // The identifier of this thread.  Only one thread can exist with a given
  // identifier at a given time.
  ID identifier_;
};

}  // namespace samples

#endif  // SAMPLES_MASTER_MASTER_THREAD_IMPL_H_
