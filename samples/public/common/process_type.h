// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_COMMON_PROCESS_TYPE_H_
#define SAMPLES_PUBLIC_COMMON_PROCESS_TYPE_H_

#include <string>

#include "samples/common/export.h"

namespace samples {

// Defines the different process types.
// NOTE: Do not remove or reorder the elements in this enum, and only add new
// items at the end, right before PROCESS_TYPE_MAX. We depend on these specific
// values in histograms.
enum ProcessType {
  PROCESS_TYPE_UNKNOWN = 1,
  PROCESS_TYPE_MASTER,
  PROCESS_TYPE_SLAVERER,
  PROCESS_TYPE_PLUGIN_DEPRECATED,
  PROCESS_TYPE_WORKER_DEPRECATED,
  PROCESS_TYPE_UTILITY,
  PROCESS_TYPE_ZYGOTE,
  PROCESS_TYPE_SANDBOX_HELPER,
  PROCESS_TYPE_GPU,
  PROCESS_TYPE_PPAPI_PLUGIN,
  PROCESS_TYPE_PPAPI_BROKER,
  // Custom process types used by the embedder should start from here.
  PROCESS_TYPE_SAMPLES_END,
  // If any embedder has more than 10 custom process types, update this.
  // We can switch to getting it from SamplesClient, but that seems like
  // overkill at this time.
  PROCESS_TYPE_MAX = PROCESS_TYPE_SAMPLES_END + 10,
};

// Returns an English name of the process type, should only be used for non
// user-visible strings or debugging pages.
SAMPLES_EXPORT std::string GetProcessTypeNameInEnglish(int type);

}  // namespace samples

#endif  // SAMPLES_PUBLIC_COMMON_PROCESS_TYPE_H_
