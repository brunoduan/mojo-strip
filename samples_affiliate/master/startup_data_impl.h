// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_STARTUP_DATA_IMPL_H_
#define SAMPLES_MASTER_STARTUP_DATA_IMPL_H_

#include <memory>

#include "samples/master/master_process_sub_thread.h"
#include "samples/public/master/startup_data.h"

namespace samples {

// The master implementation of StartupData.
struct StartupDataImpl : public StartupData {
  StartupDataImpl();
  ~StartupDataImpl() override;

  // TODO(hanxi): add ServiceManagerContext* here.
  std::unique_ptr<MasterProcessSubThread> thread;
};

}  // namespace samples

#endif  // SAMPLES_MASTER_STARTUP_DATA_IMPL_H_
