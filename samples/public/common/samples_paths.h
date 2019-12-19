// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_COMMON_SAMPLES_PATHS_H_
#define SAMPLES_PUBLIC_COMMON_SAMPLES_PATHS_H_

#include "samples/common/export.h"

// This file declares path keys for the samples module.  These can be used with
// the PathService to access various special directories and files.

namespace samples {

enum {
  PATH_START = 4000,

  // Path and filename to the executable to use for child processes.
  CHILD_PROCESS_EXE = PATH_START,

  // Valid only in development environment
  DIR_TEST_DATA,

  // Directory where the Media libraries reside.
  DIR_MEDIA_LIBS,

  PATH_END
};

// Call once to register the provider for the path keys defined above.
SAMPLES_EXPORT void RegisterPathProvider();

}  // namespace samples

#endif  // SAMPLES_PUBLIC_COMMON_SAMPLES_PATHS_H_
