// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SHELL_SLAVERER_SHELL_SAMPLES_SLAVERER_CLIENT_H_
#define SAMPLES_SHELL_SLAVERER_SHELL_SAMPLES_SLAVERER_CLIENT_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "build/build_config.h"
#include "samples/public/slaverer/samples_slaverer_client.h"

namespace samples {

class ShellSamplesSlavererClient : public SamplesSlavererClient {
 public:
  ShellSamplesSlavererClient();
  ~ShellSamplesSlavererClient() override;

  // SamplesSlavererClient implementation.
  void SlaverThreadStarted() override;

};

}  // namespace samples

#endif  // SAMPLES_SHELL_SLAVERER_SHELL_SAMPLES_SLAVERER_CLIENT_H_
