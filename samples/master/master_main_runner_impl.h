// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_MASTER_MAIN_RUNNER_IMPL_H_
#define SAMPLES_MASTER_MASTER_MAIN_RUNNER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/task/task_scheduler/task_scheduler.h"
#include "build/build_config.h"
#include "samples/public/master/master_main_runner.h"

namespace samples {

class MasterMainLoop;

class MasterMainRunnerImpl : public MasterMainRunner {
 public:
  static MasterMainRunnerImpl* Create();

  MasterMainRunnerImpl();
  ~MasterMainRunnerImpl() override;

  // MasterMainRunner:
  int Initialize(const MainFunctionParams& parameters) override;
#if defined(OS_ANDROID)
  void SynchronouslyFlushStartupTasks() override;
#endif
  int Run() override;
  void Shutdown() override;

 private:
  // True if we have started to initialize the runner.
  bool initialization_started_;

  // True if the runner has been shut down.
  bool is_shutdown_;

  // Prevents execution of TaskScheduler tasks from the moment samples is
  // entered. Handed off to |main_loop_| later so it can decide when to release
  // worker threads again.
  std::unique_ptr<base::TaskScheduler::ScopedExecutionFence>
      scoped_execution_fence_;

  std::unique_ptr<MasterMainLoop> main_loop_;

  DISALLOW_COPY_AND_ASSIGN(MasterMainRunnerImpl);
};

}  // namespace samples

#endif  // SAMPLES_MASTER_MASTER_MAIN_RUNNER_IMPL_H_
