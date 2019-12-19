// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_COMMON_SAMPLES_SWITCHES_INTERNAL_H_
#define SAMPLES_COMMON_SAMPLES_SWITCHES_INTERNAL_H_

#include <string>

#include "build/build_config.h"
#include "samples/common/export.h"

namespace base {
class CommandLine;
}

namespace samples {

void WaitForDebugger(const std::string& label);

// Returns all comma-separated values from all instances of a switch, in the
// order they appear.  For example: given command line "--foo=aa,bb --foo=cc",
// the feature list for switch "foo" will be ["aa", "bb", "cc"].
SAMPLES_EXPORT std::vector<std::string> FeaturesFromSwitch(
    const base::CommandLine& command_line,
    const char* switch_name);

} // namespace samples

#endif  // SAMPLES_COMMON_SAMPLES_SWITCHES_INTERNAL_H_
