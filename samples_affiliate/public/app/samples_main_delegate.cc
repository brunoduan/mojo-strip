// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/app/samples_main_delegate.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "samples/public/slaverer/samples_slaverer_client.h"
#include "samples/public/utility/samples_utility_client.h"

#if !defined(CHROME_MULTIPLE_DLL_CHILD)
#include "samples/public/master/samples_master_client.h"
#endif

namespace samples {

bool SamplesMainDelegate::BasicStartupComplete(int* exit_code) {
  return false;
}

int SamplesMainDelegate::RunProcess(
    const std::string& process_type,
    const samples::MainFunctionParams& main_function_params) {
  return -1;
}

int SamplesMainDelegate::TerminateForFatalInitializationError() {
  CHECK(false);
  return 0;
}

bool SamplesMainDelegate::ShouldEnableProfilerRecording() {
  return false;
}

service_manager::ProcessType SamplesMainDelegate::OverrideProcessType() {
  return service_manager::ProcessType::kDefault;
}

void SamplesMainDelegate::AdjustServiceProcessCommandLine(
    const service_manager::Identity& identity,
    base::CommandLine* command_line) {}

void SamplesMainDelegate::OnServiceManagerInitialized(
    const base::Closure& quit_closure,
    service_manager::BackgroundServiceManager* service_manager) {}

bool SamplesMainDelegate::ShouldCreateFeatureList() {
  return true;
}

SamplesMasterClient* SamplesMainDelegate::CreateSamplesMasterClient() {
#if defined(CHROME_MULTIPLE_DLL_CHILD)
  return NULL;
#else
  return new SamplesMasterClient();
#endif
}

SamplesSlavererClient* SamplesMainDelegate::CreateSamplesSlavererClient() {
#if defined(CHROME_MULTIPLE_DLL_BROWSER)
  return NULL;
#else
  return new SamplesSlavererClient();
#endif
}

SamplesUtilityClient* SamplesMainDelegate::CreateSamplesUtilityClient() {
#if defined(CHROME_MULTIPLE_DLL_BROWSER)
  return NULL;
#else
  return new SamplesUtilityClient();
#endif
}

}  // namespace samples
