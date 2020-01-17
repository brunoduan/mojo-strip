// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/master_main.h"

#include <memory>

#include "base/trace_event/trace_event.h"
#include "samples/master/master_main_runner_impl.h"
#include "samples/common/samples_constants_internal.h"

namespace {

// Generates a pair of MasterMain async events. We don't use the TRACE_EVENT0
// macro because the tracing infrastructure doesn't expect synchronous events
// around the main loop of a thread.
class ScopedMasterMainEvent {
 public:
  ScopedMasterMainEvent() {
    TRACE_EVENT_ASYNC_BEGIN0("startup", "MasterMain", 0);
  }
  ~ScopedMasterMainEvent() {
    TRACE_EVENT_ASYNC_END0("startup", "MasterMain", 0);
  }
};

}  // namespace

namespace samples {

// Main routine for running as the Master process.
int MasterMain(const MainFunctionParams& parameters) {
  ScopedMasterMainEvent scoped_master_main_event;

  base::trace_event::TraceLog::GetInstance()->set_process_name("Master");
  base::trace_event::TraceLog::GetInstance()->SetProcessSortIndex(
      kTraceEventMasterProcessSortIndex);

  std::unique_ptr<MasterMainRunnerImpl> main_runner(
      MasterMainRunnerImpl::Create());

  int exit_code = main_runner->Initialize(parameters);
  if (exit_code >= 0)
    return exit_code;

  exit_code = main_runner->Run();

  main_runner->Shutdown();

  return exit_code;
}

}  // namespace samples
