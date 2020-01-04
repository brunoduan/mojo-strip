// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SHELL_COMMON_SHELL_SAMPLES_CLIENT_H_
#define SAMPLES_SHELL_COMMON_SHELL_SAMPLES_CLIENT_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "samples/public/common/samples_client.h"

namespace samples {

std::string GetShellUserAgent();

class ShellSamplesClient : public SamplesClient {
 public:
  ShellSamplesClient();
  ~ShellSamplesClient() override;

  base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  base::RefCountedMemory* GetDataResourceBytes(
      int resource_id) const override;
};

}  // namespace samples

#endif  // SAMPLES_SHELL_COMMON_SHELL_SAMPLES_CLIENT_H_
