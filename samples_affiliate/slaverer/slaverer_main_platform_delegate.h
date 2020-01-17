// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SLAVERER_SLAVERER_MAIN_PLATFORM_DELEGATE_H_
#define SAMPLES_SLAVERER_SLAVERER_MAIN_PLATFORM_DELEGATE_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#include "base/macros.h"
#include "samples/common/export.h"
#include "samples/public/common/main_function_params.h"

namespace samples {

class SAMPLES_EXPORT SlavererMainPlatformDelegate {
 public:
  explicit SlavererMainPlatformDelegate(
      const MainFunctionParams& parameters);
  ~SlavererMainPlatformDelegate();

  // Called first thing and last thing in the process' lifecycle, i.e. before
  // the sandbox is enabled.
  void PlatformInitialize();
  void PlatformUninitialize();

  // Initiate Lockdown, returns true on success.
  bool EnableSandbox();

 private:
#if defined(OS_WIN)
  const MainFunctionParams& parameters_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SlavererMainPlatformDelegate);
};

}  // namespace samples

#endif  // SAMPLES_SLAVERER_SLAVERER_MAIN_PLATFORM_DELEGATE_H_
