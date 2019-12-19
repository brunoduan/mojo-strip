// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/android/samples_startup_flags.h"

#include "base/android/build_info.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/sys_info.h"
#include "samples/public/common/samples_switches.h"

namespace samples {

void SetSamplesCommandLineFlags(bool single_process) {
  // May be called multiple times, to cover all possible program entry points.
  static bool already_initialized = false;
  if (already_initialized)
    return;
  already_initialized = true;

  base::CommandLine* parsed_command_line =
      base::CommandLine::ForCurrentProcess();

  if (single_process) {
    // Need to ensure the command line flag is consistent as a lot of chrome
    // internal code checks this directly, but it wouldn't normally get set when
    // we are implementing an embedded WebView.
    parsed_command_line->AppendSwitch(switches::kSingleProcess);
  }
}

}  // namespace samples
