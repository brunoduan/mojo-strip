// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/slaverer/samples_slaverer_client.h"


namespace samples {

void SamplesSlavererClient::PostIOThreadCreated(
    base::SingleThreadTaskRunner* io_thread_task_runner) {}

bool SamplesSlavererClient::RunIdleHandlerWhenWidgetsHidden() {
  return true;
}

std::unique_ptr<base::TaskScheduler::InitParams>
SamplesSlavererClient::GetTaskSchedulerInitParams() {
  return nullptr;
}

}  // namespace samples
