// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/scheduler/master_task_executor.h"

#include "base/no_destructor.h"
#include "samples/master/master_thread_impl.h"

namespace samples {
namespace {

// |g_master_task_executor| is intentionally leaked on shutdown.
MasterTaskExecutor* g_master_task_executor = nullptr;

// An implementation of SingleThreadTaskRunner to be used in conjunction with
// MasterThread. MasterThreadTaskRunners are vended by
// base::Create*TaskRunnerWithTraits({MasterThread::UI/IO}).
//
// TODO(gab): Consider replacing this with direct calls to task runners obtained
// via |MasterThreadImpl::GetTaskRunnerForThread()| -- only works if none are
// requested before starting the threads.
class MasterThreadTaskRunner : public base::SingleThreadTaskRunner {
 public:
  explicit MasterThreadTaskRunner(MasterThread::ID identifier)
      : id_(identifier) {}

  // SingleThreadTaskRunner implementation.
  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override {
    return MasterThreadImpl::GetTaskRunnerForThread(id_)->PostDelayedTask(
        from_here, std::move(task), delay);
  }

  bool PostNonNestableDelayedTask(const base::Location& from_here,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override {
    return MasterThreadImpl::GetTaskRunnerForThread(id_)
        ->PostNonNestableDelayedTask(from_here, std::move(task), delay);
  }

  bool RunsTasksInCurrentSequence() const override {
    return MasterThread::CurrentlyOn(id_);
  }

 private:
  ~MasterThreadTaskRunner() override {}

  const MasterThread::ID id_;

  DISALLOW_COPY_AND_ASSIGN(MasterThreadTaskRunner);
};

}  // namespace

MasterTaskExecutor::MasterTaskExecutor() = default;
MasterTaskExecutor::~MasterTaskExecutor() = default;

// static
void MasterTaskExecutor::Create() {
  DCHECK(!g_master_task_executor);
  g_master_task_executor = new MasterTaskExecutor();
  base::RegisterTaskExecutor(MasterTaskTraitsExtension::kExtensionId,
                             g_master_task_executor);
}

// static
void MasterTaskExecutor::ResetForTesting() {
  if (g_master_task_executor) {
    base::UnregisterTaskExecutorForTesting(
        MasterTaskTraitsExtension::kExtensionId);
    delete g_master_task_executor;
    g_master_task_executor = nullptr;
  }
}

bool MasterTaskExecutor::PostDelayedTaskWithTraits(
    const base::Location& from_here,
    const base::TaskTraits& traits,
    base::OnceClosure task,
    base::TimeDelta delay) {
  DCHECK_EQ(MasterTaskTraitsExtension::kExtensionId, traits.extension_id());
  const MasterTaskTraitsExtension& extension =
      traits.GetExtension<MasterTaskTraitsExtension>();
  if (extension.nestable()) {
    return GetTaskRunner(extension)->PostDelayedTask(from_here, std::move(task),
                                                     delay);
  } else {
    return GetTaskRunner(extension)->PostNonNestableDelayedTask(
        from_here, std::move(task), delay);
  }
}

scoped_refptr<base::TaskRunner> MasterTaskExecutor::CreateTaskRunnerWithTraits(
    const base::TaskTraits& traits) {
  return GetTaskRunner(traits);
}

scoped_refptr<base::SequencedTaskRunner>
MasterTaskExecutor::CreateSequencedTaskRunnerWithTraits(
    const base::TaskTraits& traits) {
  return GetTaskRunner(traits);
}

scoped_refptr<base::SingleThreadTaskRunner>
MasterTaskExecutor::CreateSingleThreadTaskRunnerWithTraits(
    const base::TaskTraits& traits,
    base::SingleThreadTaskRunnerThreadMode thread_mode) {
  return GetTaskRunner(traits);
}

#if defined(OS_WIN)
scoped_refptr<base::SingleThreadTaskRunner>
MasterTaskExecutor::CreateCOMSTATaskRunnerWithTraits(
    const base::TaskTraits& traits,
    base::SingleThreadTaskRunnerThreadMode thread_mode) {
  return GetTaskRunner(traits);
}
#endif  // defined(OS_WIN)

scoped_refptr<base::SingleThreadTaskRunner> MasterTaskExecutor::GetTaskRunner(
    const base::TaskTraits& traits) {
  DCHECK_EQ(MasterTaskTraitsExtension::kExtensionId, traits.extension_id());
  const MasterTaskTraitsExtension& extension =
      traits.GetExtension<MasterTaskTraitsExtension>();
  return GetTaskRunner(extension);
}

scoped_refptr<base::SingleThreadTaskRunner> MasterTaskExecutor::GetTaskRunner(
    const MasterTaskTraitsExtension& extension) {
  MasterThread::ID thread_id = extension.master_thread();
  DCHECK_GE(thread_id, 0);
  DCHECK_LT(thread_id, MasterThread::ID::ID_COUNT);
  return GetProxyTaskRunnerForThread(thread_id);
}

// static
scoped_refptr<base::SingleThreadTaskRunner>
MasterTaskExecutor::GetProxyTaskRunnerForThread(MasterThread::ID id) {
  using TaskRunnerMap = std::array<scoped_refptr<base::SingleThreadTaskRunner>,
                                   MasterThread::ID_COUNT>;
  static const base::NoDestructor<TaskRunnerMap> task_runners([] {
    TaskRunnerMap task_runners;
    for (int i = 0; i < MasterThread::ID_COUNT; ++i)
      task_runners[i] = base::MakeRefCounted<MasterThreadTaskRunner>(
          static_cast<MasterThread::ID>(i));
    return task_runners;
  }());
  return (*task_runners)[id];
}

}  // namespace samples
