// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_MASTER_MAIN_H_
#define SAMPLES_MASTER_MASTER_MAIN_H_

#include <memory>

#include "samples/common/export.h"

namespace samples {

struct MainFunctionParams;

SAMPLES_EXPORT int MasterMain(const samples::MainFunctionParams& parameters);

}  // namespace samples

#endif  // SAMPLES_MASTER_MASTER_MAIN_H_
