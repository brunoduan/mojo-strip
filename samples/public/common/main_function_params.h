// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Wrapper to the parameter list for the "main" entry points (master, slave),
// to shield the call sites from the differences between platforms
// (e.g., POSIX doesn't need to pass any sandbox information).

#ifndef SAMPLES_PUBLIC_COMMON_MAIN_FUNCTION_PARAMS_H_
#define SAMPLES_PUBLIC_COMMON_MAIN_FUNCTION_PARAMS_H_

#include "base/callback_forward.h"
#include "base/command_line.h"
#include "build/build_config.h"

namespace samples {

class MasterMainParts;
struct StartupData;

using CreatedMainPartsClosure = base::Callback<void(MasterMainParts*)>;

struct MainFunctionParams {
  explicit MainFunctionParams(const base::CommandLine& cl) : command_line(cl) {}

  const base::CommandLine& command_line;

  // TODO(sky): fix ownership of these tasks. MainFunctionParams should really
  // be passed as an r-value, at which point these can be unique_ptrs. For the
  // time ownership is passed with MainFunctionParams (meaning these are deleted
  // in samples or client code).

  // Used by InProcessMasterTest. If non-null MasterMain schedules this
  // task to run on the MessageLoop and MasterInit is not invoked.
  base::Closure* ui_task = nullptr;

  // Used by InProcessMasterTest. If non-null this is Run() after
  // MasterMainParts has been created and before PreEarlyInitialization().
  CreatedMainPartsClosure* created_main_parts_closure = nullptr;

  // Used by //samples, when the embedder yields control back to it, to extract
  // startup data passed from ContentMainRunner.
  StartupData* startup_data = nullptr;
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_COMMON_MAIN_FUNCTION_PARAMS_H_
