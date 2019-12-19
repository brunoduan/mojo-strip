// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/master_main.h"

#include <memory>

#include "samples/master/master_main_runner_impl.h"
#include "samples/common/samples_constants_internal.h"

namespace samples {

// Main routine for running as the Master process.
int MasterMain(const MainFunctionParams& parameters) {
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
