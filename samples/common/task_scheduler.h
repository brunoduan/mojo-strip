// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_COMMON_TASK_SCHEDULER_
#define SAMPLES_COMMON_TASK_SCHEDULER_

#include "samples/common/export.h"

namespace samples {

// Returns the minimum number of threads that the TaskScheduler foreground pool
// must have in a process that runs a renderer.
int GetMinThreadsInSlavererTaskSchedulerForegroundPool();

}  // namespace samples

#endif  // SAMPLES_COMMON_TASK_SCHEDULER_
