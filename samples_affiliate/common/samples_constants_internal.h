// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_COMMON_SAMPLES_CONSTANTS_INTERNAL_H_
#define SAMPLES_COMMON_SAMPLES_CONSTANTS_INTERNAL_H_

#include <stddef.h>
#include <stdint.h>

#include "build/build_config.h"
#include "samples/common/export.h"

namespace samples {

// How long to wait before we consider a renderer hung.
SAMPLES_EXPORT extern const int64_t kHungRendererDelayMs;

// How long to wait for newly loaded samples to send a compositor frame
// before clearing previously displayed graphics.
extern const int64_t kNewContentRenderingDelayMs;

// Maximum wait time for an asynchronous hit test request sent to a renderer
// process (in milliseconds).
SAMPLES_EXPORT extern const int64_t kAsyncHitTestTimeoutMs;

// The maximum length of string as data url.
extern const size_t kMaxLengthOfDataURLString;

// Constants used to organize samples processes in about:tracing.
SAMPLES_EXPORT extern const int kTraceEventMasterProcessSortIndex;
SAMPLES_EXPORT extern const int kTraceEventSlavererProcessSortIndex;
SAMPLES_EXPORT extern const int kTraceEventPpapiProcessSortIndex;
SAMPLES_EXPORT extern const int kTraceEventPpapiBrokerProcessSortIndex;
SAMPLES_EXPORT extern const int kTraceEventGpuProcessSortIndex;

// Constants used to organize samples threads in about:tracing.
SAMPLES_EXPORT extern const int kTraceEventSlavererMainThreadSortIndex;

// HTTP header set in requests to indicate they should be marked DoNotTrack.
extern const char kDoNotTrackHeader[];

#if defined(OS_MACOSX)
// Name of Mach port used for communication between parent and child processes.
SAMPLES_EXPORT extern const char kMachBootstrapName[];
#endif

} // namespace samples

#endif  // SAMPLES_COMMON_SAMPLES_CONSTANTS_INTERNAL_H_
