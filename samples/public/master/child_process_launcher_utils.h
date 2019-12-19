// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_CHILD_PROCESS_LAUNCHER_UTILS_H_
#define SAMPLES_PUBLIC_MASTER_CHILD_PROCESS_LAUNCHER_UTILS_H_

#include "samples/common/export.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace samples {

// The caller must take a reference to the returned TaskRunner pointer if it
// wants to use the pointer directly.
SAMPLES_EXPORT base::SingleThreadTaskRunner* GetProcessLauncherTaskRunner();

SAMPLES_EXPORT bool CurrentlyOnProcessLauncherTaskRunner();

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_CHILD_PROCESS_LAUNCHER_UTILES_H_
