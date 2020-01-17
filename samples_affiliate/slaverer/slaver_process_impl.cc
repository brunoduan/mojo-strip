// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/slaverer/slaver_process_impl.h"

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#include <objidl.h>
#include <mlang.h>
#endif

#include <stddef.h>

#include <algorithm>
#include <utility>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/debug/crash_logging.h"
#include "base/debug/stack_trace.h"
#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "base/sys_info.h"
#include "base/task/task_scheduler/initialization_util.h"
#include "base/time/time.h"
#include "samples/common/task_scheduler.h"
#include "samples/public/common/samples_client.h"
#include "samples/public/common/samples_features.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/slaverer/samples_slaverer_client.h"
#include "services/service_manager/embedder/switches.h"

namespace {

std::unique_ptr<base::TaskScheduler::InitParams>
GetDefaultTaskSchedulerInitParams() {

  constexpr int kMaxNumThreadsInBackgroundPool = 1;
  constexpr int kMaxNumThreadsInBackgroundBlockingPool = 1;
  constexpr int kMaxNumThreadsInForegroundPoolLowerBound = 2;
  constexpr int kMaxNumThreadsInForegroundBlockingPool = 1;
  constexpr auto kSuggestedReclaimTime = base::TimeDelta::FromSeconds(30);

  return std::make_unique<base::TaskScheduler::InitParams>(
      base::SchedulerWorkerPoolParams(kMaxNumThreadsInBackgroundPool,
                                      kSuggestedReclaimTime),
      base::SchedulerWorkerPoolParams(kMaxNumThreadsInBackgroundBlockingPool,
                                      kSuggestedReclaimTime),
      base::SchedulerWorkerPoolParams(
          std::max(
              kMaxNumThreadsInForegroundPoolLowerBound,
              samples::GetMinThreadsInSlavererTaskSchedulerForegroundPool()),
          kSuggestedReclaimTime),
      base::SchedulerWorkerPoolParams(kMaxNumThreadsInForegroundBlockingPool,
                                      kSuggestedReclaimTime));
}

}  // namespace

namespace samples {

SlaverProcessImpl::SlaverProcessImpl(
    std::unique_ptr<base::TaskScheduler::InitParams> task_scheduler_init_params)
    : SlaverProcess("Slaverer", std::move(task_scheduler_init_params)),
      enabled_bindings_(0) {
}

SlaverProcessImpl::~SlaverProcessImpl() {
  GetShutDownEvent()->Signal();
}

std::unique_ptr<SlaverProcess> SlaverProcessImpl::Create() {
  auto task_scheduler_init_params =
      samples::GetSamplesClient()->slaverer()->GetTaskSchedulerInitParams();
  if (!task_scheduler_init_params)
    task_scheduler_init_params = GetDefaultTaskSchedulerInitParams();

  return base::WrapUnique(
      new SlaverProcessImpl(std::move(task_scheduler_init_params)));
}

void SlaverProcessImpl::AddBindings(int bindings) {
  enabled_bindings_ |= bindings;
}

int SlaverProcessImpl::GetEnabledBindings() const {
  return enabled_bindings_;
}

void SlaverProcessImpl::AddRefProcess() {
  NOTREACHED();
}

void SlaverProcessImpl::ReleaseProcess() {
  NOTREACHED();
}

}  // namespace samples
