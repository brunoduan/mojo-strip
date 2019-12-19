// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_SCHEDULER_MASTER_TASK_EXECUTOR_H_
#define SAMPLES_MASTER_SCHEDULER_MASTER_TASK_EXECUTOR_H_

#include "base/gtest_prod_util.h"
#include "base/task/task_executor.h"
#include "build/build_config.h"
#include "samples/common/export.h"
#include "samples/public/master/master_task_traits.h"

namespace samples {

// This class's job is to map base::TaskTraits to actual task queues for the
// master process.
class SAMPLES_EXPORT MasterTaskExecutor : public base::TaskExecutor {
 public:
  // Creates and registers a MasterTaskExecutor that facilitates posting tasks
  // to a MasterThread via //base/task/post_task.h.
  static void Create();

  // Unregister and delete the TaskExecutor after a test.
  static void ResetForTesting();

  // base::TaskExecutor implementation.
  bool PostDelayedTaskWithTraits(const base::Location& from_here,
                                 const base::TaskTraits& traits,
                                 base::OnceClosure task,
                                 base::TimeDelta delay) override;

  scoped_refptr<base::TaskRunner> CreateTaskRunnerWithTraits(
      const base::TaskTraits& traits) override;

  scoped_refptr<base::SequencedTaskRunner> CreateSequencedTaskRunnerWithTraits(
      const base::TaskTraits& traits) override;

  scoped_refptr<base::SingleThreadTaskRunner>
  CreateSingleThreadTaskRunnerWithTraits(
      const base::TaskTraits& traits,
      base::SingleThreadTaskRunnerThreadMode thread_mode) override;

#if defined(OS_WIN)
  scoped_refptr<base::SingleThreadTaskRunner> CreateCOMSTATaskRunnerWithTraits(
      const base::TaskTraits& traits,
      base::SingleThreadTaskRunnerThreadMode thread_mode) override;
#endif  // defined(OS_WIN)

 private:
  // For GetProxyTaskRunnerForThread().
  FRIEND_TEST_ALL_PREFIXES(MasterTaskExecutorTest,
                           EnsureUIThreadTraitPointsToExpectedQueue);
  FRIEND_TEST_ALL_PREFIXES(MasterTaskExecutorTest,
                           EnsureIOThreadTraitPointsToExpectedQueue);

  MasterTaskExecutor();
  ~MasterTaskExecutor() override;

  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner(
      const base::TaskTraits& traits);

  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner(
      const MasterTaskTraitsExtension& extension);

  static scoped_refptr<base::SingleThreadTaskRunner>
  GetProxyTaskRunnerForThread(MasterThread::ID id);

  DISALLOW_COPY_AND_ASSIGN(MasterTaskExecutor);
};

}  // namespace samples

#endif  // SAMPLES_MASTER_SCHEDULER_MASTER_TASK_EXECUTOR_H_
