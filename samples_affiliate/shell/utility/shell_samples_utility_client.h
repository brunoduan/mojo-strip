// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SHELL_UTILITY_SHELL_SAMPLES_UTILITY_CLIENT_H_
#define SAMPLES_SHELL_UTILITY_SHELL_SAMPLES_UTILITY_CLIENT_H_

#include "base/macros.h"
#include "samples/public/utility/samples_utility_client.h"

namespace samples {

class ShellSamplesUtilityClient : public SamplesUtilityClient {
 public:
  explicit ShellSamplesUtilityClient(bool is_browsertest = false);
  ~ShellSamplesUtilityClient() override;

  // SamplesUtilityClient:
  void UtilityThreadStarted() override;
  void RegisterServices(StaticServiceMap* services) override;

 private:

  DISALLOW_COPY_AND_ASSIGN(ShellSamplesUtilityClient);
};

}  // namespace samples

#endif  // SAMPLES_SHELL_UTILITY_SHELL_SAMPLES_UTILITY_CLIENT_H_
