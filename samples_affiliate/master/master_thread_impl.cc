// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/master_thread_impl.h"

#include <string>
#include <utility>

#include "base/atomicops.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/no_destructor.h"
#include "base/sequence_checker.h"
#include "base/task/task_executor.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "samples/public/master/samples_master_client.h"

namespace samples {

namespace {

// State of a given MasterThread::ID in chronological order throughout the
// master process' lifetime.
enum MasterThreadState {
  // MasterThread::ID isn't associated with anything yet.
  UNINITIALIZED = 0,
  // MasterThread::ID is associated to a TaskRunner and is accepting tasks.
  RUNNING,
  // MasterThread::ID no longer accepts tasks (it's still associated to a
  // TaskRunner but that TaskRunner doesn't have to accept tasks).
  SHUTDOWN
};

struct MasterThreadGlobals {
  MasterThreadGlobals() {
    // A few unit tests which do not use a TestMasterThreadBundle still invoke
    // code that reaches into CurrentlyOn()/IsThreadInitialized(). This can
    // result in instantiating MasterThreadGlobals off the main thread.
    // |main_thread_checker_| being bound incorrectly would then result in a
    // flake in the next test that instantiates a TestMasterThreadBundle in the
    // same process. Detaching here postpones binding |main_thread_checker_| to
    // the first invocation of MasterThreadImpl::MasterThreadImpl() and works
    // around this issue.
    DETACH_FROM_THREAD(main_thread_checker_);
  }

  // MasterThreadGlobals must be initialized on main thread before it's used by
  // any other threads.
  THREAD_CHECKER(main_thread_checker_);

  // |task_runners[id]| is safe to access on |main_thread_checker_| as
  // well as on any thread once it's read-only after initialization
  // (i.e. while |states[id] >= RUNNING|).
  scoped_refptr<base::SingleThreadTaskRunner>
      task_runners[MasterThread::ID_COUNT];

  // Tracks the runtime state of MasterThreadImpls. Atomic because a few
  // methods below read this value outside |main_thread_checker_| to
  // confirm it's >= RUNNING and doing so requires an atomic read as it could be
  // in the middle of transitioning to SHUTDOWN (which the check is fine with
  // but reading a non-atomic value as it's written to by another thread can
  // result in undefined behaviour on some platforms).
  // Only NoBarrier atomic operations should be used on |states| as it shouldn't
  // be used to establish happens-after relationships but rather checking the
  // runtime state of various threads (once again: it's only atomic to support
  // reading while transitioning from RUNNING=>SHUTDOWN).
  base::subtle::Atomic32 states[MasterThread::ID_COUNT] = {};
};

MasterThreadGlobals& GetMasterThreadGlobals() {
  static base::NoDestructor<MasterThreadGlobals> globals;
  return *globals;
}

}  // namespace

MasterThreadImpl::MasterThreadImpl(
    ID identifier,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : identifier_(identifier) {
  DCHECK_GE(identifier_, 0);
  DCHECK_LT(identifier_, ID_COUNT);
  DCHECK(task_runner);

  MasterThreadGlobals& globals = GetMasterThreadGlobals();

  DCHECK_CALLED_ON_VALID_THREAD(globals.main_thread_checker_);

  DCHECK_EQ(base::subtle::NoBarrier_Load(&globals.states[identifier_]),
            MasterThreadState::UNINITIALIZED);
  base::subtle::NoBarrier_Store(&globals.states[identifier_],
                                MasterThreadState::RUNNING);

  DCHECK(!globals.task_runners[identifier_]);
  globals.task_runners[identifier_] = std::move(task_runner);
}

MasterThreadImpl::~MasterThreadImpl() {
  MasterThreadGlobals& globals = GetMasterThreadGlobals();
  DCHECK_CALLED_ON_VALID_THREAD(globals.main_thread_checker_);

  DCHECK_EQ(base::subtle::NoBarrier_Load(&globals.states[identifier_]),
            MasterThreadState::RUNNING);
  base::subtle::NoBarrier_Store(&globals.states[identifier_],
                                MasterThreadState::SHUTDOWN);

  // The mapping is kept alive after shutdown to avoid requiring a lock only for
  // shutdown (the SingleThreadTaskRunner itself may stop accepting tasks at any
  // point -- usually soon before/after destroying the MasterThreadImpl).
  DCHECK(globals.task_runners[identifier_]);
}

// static
void MasterThreadImpl::ResetGlobalsForTesting(MasterThread::ID identifier) {
  MasterThreadGlobals& globals = GetMasterThreadGlobals();
  DCHECK_CALLED_ON_VALID_THREAD(globals.main_thread_checker_);

  DCHECK_EQ(base::subtle::NoBarrier_Load(&globals.states[identifier]),
            MasterThreadState::SHUTDOWN);
  base::subtle::NoBarrier_Store(&globals.states[identifier],
                                MasterThreadState::UNINITIALIZED);

  globals.task_runners[identifier] = nullptr;
}

// static
const char* MasterThreadImpl::GetThreadName(MasterThread::ID thread) {
  static const char* const kMasterThreadNames[MasterThread::ID_COUNT] = {
      "",                 // UI (name assembled in master_main_loop.cc).
      "Samples_IOThread",  // IO
  };

  if (MasterThread::UI < thread && thread < MasterThread::ID_COUNT)
    return kMasterThreadNames[thread];
  if (thread == MasterThread::UI)
    return "Samples_UIThread";
  return "Unknown Thread";
}

// static
void MasterThread::PostAfterStartupTask(
    const base::Location& from_here,
    const scoped_refptr<base::TaskRunner>& task_runner,
    base::OnceClosure task) {
  GetSamplesClient()->master()->PostAfterStartupTask(from_here, task_runner,
                                                      std::move(task));
}

// static
bool MasterThread::IsThreadInitialized(ID identifier) {
  DCHECK_GE(identifier, 0);
  DCHECK_LT(identifier, ID_COUNT);

  MasterThreadGlobals& globals = GetMasterThreadGlobals();
  return base::subtle::NoBarrier_Load(&globals.states[identifier]) ==
         MasterThreadState::RUNNING;
}

// static
bool MasterThread::CurrentlyOn(ID identifier) {
  DCHECK_GE(identifier, 0);
  DCHECK_LT(identifier, ID_COUNT);

  MasterThreadGlobals& globals = GetMasterThreadGlobals();

  // Thread-safe since |globals.task_runners| is read-only after being
  // initialized from main thread (which happens before //samples and embedders
  // are kicked off and enabled to call the MasterThread API from other
  // threads).
  return globals.task_runners[identifier] &&
         globals.task_runners[identifier]->RunsTasksInCurrentSequence();
}

// static
std::string MasterThread::GetDCheckCurrentlyOnErrorMessage(ID expected) {
  std::string actual_name = base::PlatformThread::GetName();
  if (actual_name.empty())
    actual_name = "Unknown Thread";

  std::string result = "Must be called on ";
  result += MasterThreadImpl::GetThreadName(expected);
  result += "; actually called on ";
  result += actual_name;
  result += ".";
  return result;
}

// static
bool MasterThread::GetCurrentThreadIdentifier(ID* identifier) {
  MasterThreadGlobals& globals = GetMasterThreadGlobals();

  // Thread-safe since |globals.task_runners| is read-only after being
  // initialized from main thread (which happens before //samples and embedders
  // are kicked off and enabled to call the MasterThread API from other
  // threads).
  for (int i = 0; i < ID_COUNT; ++i) {
    if (globals.task_runners[i] &&
        globals.task_runners[i]->RunsTasksInCurrentSequence()) {
      *identifier = static_cast<ID>(i);
      return true;
    }
  }

  return false;
}

// static
scoped_refptr<base::SingleThreadTaskRunner>
MasterThread::GetTaskRunnerForThread(ID identifier) {
  DCHECK_GE(identifier, 0);
  DCHECK_LT(identifier, ID_COUNT);

  MasterThreadGlobals& globals = GetMasterThreadGlobals();

  // Tasks should always be posted while the MasterThread is in a RUNNING or
  // SHUTDOWN state (will return false if SHUTDOWN).
  //
  // Posting tasks before MasterThreads are initialized is incorrect as it
  // would silently no-op. If you need to support posting early, gate it on
  // MasterThread::IsThreadInitialized(). If you hit this check in unittests,
  // you most likely posted a task outside the scope of a
  // TestMasterThreadBundle (which also completely resets the state after
  // shutdown in ~TestMasterThreadBundle(), ref. ResetGlobalsForTesting(),
  // making sure TestMasterThreadBundle is the first member of your test
  // fixture and thus outlives everything is usually the right solution).
  DCHECK_GE(base::subtle::NoBarrier_Load(&globals.states[identifier]),
            MasterThreadState::RUNNING);
  DCHECK(globals.task_runners[identifier]);

  return globals.task_runners[identifier];
}

}  // namespace samples
