// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A handful of resource-like constants related to the Content application.

#ifndef SAMPLES_PUBLIC_COMMON_SAMPLES_CONSTANTS_H_
#define SAMPLES_PUBLIC_COMMON_SAMPLES_CONSTANTS_H_

#include <stddef.h>         // For size_t

#include "base/files/file_path.h"
#include "samples/common/export.h"

namespace samples {

// The name of the directory under MasterContext::GetPath where the AppCache is
// put.
SAMPLES_EXPORT extern const base::FilePath::CharType kAppCacheDirname[];

SAMPLES_EXPORT extern const size_t kMaxSlavererProcessCount;


// The maximum number of characters in the URL that we're willing to accept
// in the browser process. It is set low enough to avoid damage to the browser
// but high enough that a web site can abuse location.hash for a little storage.
// We have different values for "max accepted" and "max displayed" because
// a data: URI may be legitimately massive, but the full URI would kill all
// known operating systems if you dropped it into a UI control.
SAMPLES_EXPORT extern const size_t kMaxURLChars;
SAMPLES_EXPORT extern const size_t kMaxURLDisplayChars;

extern const char kStatsFilename[];
extern const int kStatsMaxThreads;
extern const int kStatsMaxCounters;

}  // namespace samples

#endif  // SAMPLES_PUBLIC_COMMON_SAMPLES_CONSTANTS_H_
