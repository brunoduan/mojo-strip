// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_MASTER_MAIN_RUNNER_H_
#define SAMPLES_PUBLIC_MASTER_MASTER_MAIN_RUNNER_H_

#include "build/build_config.h"
#include "samples/common/export.h"

namespace samples {

struct MainFunctionParams;

// This class is responsible for browser initialization, running and shutdown.
class SAMPLES_EXPORT MasterMainRunner {
 public:
  virtual ~MasterMainRunner() {}

  // Create a new MasterMainRunner object.
  static MasterMainRunner* Create();

  // Returns true if the MasterMainRunner has exited the main loop.
  static bool ExitedMainMessageLoop();

  // Initialize all necessary browser state. The |parameters| values will be
  // copied. Returning a non-negative value indicates that initialization
  // failed, and the returned value is used as the exit code for the process.
  virtual int Initialize(const samples::MainFunctionParams& parameters) = 0;

#if defined(OS_ANDROID)
  // Run all queued startup tasks. Only defined on Android because other
  // platforms run startup tasks immediately.
  virtual void SynchronouslyFlushStartupTasks() = 0;
#endif  // OS_ANDROID

  // Perform the default run logic.
  virtual int Run() = 0;

  // Shut down the browser state.
  virtual void Shutdown() = 0;
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_MASTER_MAIN_RUNNER_H_
