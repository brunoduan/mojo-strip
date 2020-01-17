// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_APP_SAMPLES_MAIN_RUNNER_H_
#define SAMPLES_PUBLIC_APP_SAMPLES_MAIN_RUNNER_H_

#include "build/build_config.h"
#include "samples/common/export.h"

namespace samples {
struct SamplesMainParams;

// This class is responsible for samples initialization, running and shutdown.
class SAMPLES_EXPORT SamplesMainRunner {
 public:
  virtual ~SamplesMainRunner() {}

  // Create a new SamplesMainRunner object.
  static SamplesMainRunner* Create();

  // Initialize all necessary samples state.
  virtual int Initialize(const SamplesMainParams& params) = 0;

  // Perform the default run logic.
  virtual int Run(bool start_service_manager_only) = 0;

  // Shut down the samples state.
  virtual void Shutdown() = 0;
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_APP_SAMPLES_MAIN_RUNNER_H_
