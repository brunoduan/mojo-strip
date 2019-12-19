// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SHELL_MASTER_SHELL_MASTER_MAIN_H_
#define SAMPLES_SHELL_MASTER_SHELL_MASTER_MAIN_H_

#include <memory>

namespace samples {
class MasterMainRunner;
struct MainFunctionParams;
}

int ShellMasterMain(
    const samples::MainFunctionParams& parameters,
    const std::unique_ptr<samples::MasterMainRunner>& main_runner);

#endif  // SAMPLES_SHELL_MASTER_SHELL_MASTER_MAIN_H_
