// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_APP_SHELL_MAIN_DELEGATE_H_
#define CONTENT_SHELL_APP_SHELL_MAIN_DELEGATE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "samples/public/app/samples_main_delegate.h"

namespace samples {
class SamplesClient;
class ShellSamplesMasterClient;
class ShellSamplesSlavererClient;
class ShellSamplesUtilityClient;

#if defined(OS_ANDROID)
class MasterMainRunner;
#endif

class ShellMainDelegate : public SamplesMainDelegate {
 public:
  explicit ShellMainDelegate(bool is_mastertest = false);
  ~ShellMainDelegate() override;

  // SamplesMainDelegate implementation:
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  int RunProcess(const std::string& process_type,
                 const MainFunctionParams& main_function_params) override;
#if defined(OS_LINUX)
  void ZygoteForked() override;
#endif
  void PreCreateMainMessageLoop() override;
  SamplesMasterClient* CreateSamplesMasterClient() override;
  SamplesSlavererClient* CreateSamplesSlavererClient() override;
  SamplesUtilityClient* CreateSamplesUtilityClient() override;

  static void InitializeResourceBundle();

 private:
  bool is_mastertest_;
  std::unique_ptr<ShellSamplesMasterClient> master_client_;
  std::unique_ptr<ShellSamplesSlavererClient> slaverer_client_;
  std::unique_ptr<ShellSamplesUtilityClient> utility_client_;
  std::unique_ptr<SamplesClient> samples_client_;

#if defined(OS_ANDROID)
  std::unique_ptr<MasterMainRunner> master_runner_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ShellMainDelegate);
};

}  // namespace samples

#endif  // CONTENT_SHELL_APP_SHELL_MAIN_DELEGATE_H_
