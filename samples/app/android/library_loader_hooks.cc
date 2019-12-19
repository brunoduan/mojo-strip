// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/app/android/library_loader_hooks.h"

#include "base/android/library_loader/library_loader_hooks.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "samples/common/samples_constants_internal.h"

namespace samples {

bool LibraryLoaded(JNIEnv* env, jclass clazz) {
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
  logging::InitLogging(settings);
  // To view log output with IDs and timestamps use "adb logcat -v threadtime".
  logging::SetLogItems(false,    // Process ID
                       false,    // Thread ID
                       false,    // Timestamp
                       false);   // Tick count
  VLOG(0) << "Chromium logging enabled: level = " << logging::GetMinLogLevel()
          << ", default verbosity = " << logging::GetVlogVerbosity();

  return true;
}

}  // namespace samples
