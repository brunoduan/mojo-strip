// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_STARTUP_HELPER_H_
#define SAMPLES_MASTER_STARTUP_HELPER_H_

#include "base/metrics/field_trial.h"
#include "samples/common/export.h"

namespace samples {

// Setups fields trials and the FeatureList, and returns the unique pointer of
// the field trials.
std::unique_ptr<base::FieldTrialList> SAMPLES_EXPORT
SetUpFieldTrialsAndFeatureList();

// Starts the task scheduler.
void SAMPLES_EXPORT StartMasterTaskScheduler();

}  // namespace samples

#endif  // SAMPLES_MASTER_STARTUP_HELPER_H_
