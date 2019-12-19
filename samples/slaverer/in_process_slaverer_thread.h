// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SLAVERER_IN_PROCESS_SLAVERER_THREAD_H_
#define SAMPLES_SLAVERER_IN_PROCESS_SLAVERER_THREAD_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/threading/thread.h"
#include "samples/common/export.h"
#include "samples/common/in_process_child_thread_params.h"

namespace samples {
class SlaverProcess;

// This class creates the IO thread for the slaverer when running in
// single-process mode.  It's not used in multi-process mode.
class InProcessSlavererThread : public base::Thread {
 public:
  explicit InProcessSlavererThread(const InProcessChildThreadParams& params);
  ~InProcessSlavererThread() override;

 protected:
  void Init() override;
  void CleanUp() override;

 private:
  InProcessChildThreadParams params_;
  std::unique_ptr<SlaverProcess> slaver_process_;

  DISALLOW_COPY_AND_ASSIGN(InProcessSlavererThread);
};

SAMPLES_EXPORT base::Thread* CreateInProcessSlavererThread(
    const InProcessChildThreadParams& params);

}  // namespace samples

#endif  // SAMPLES_SLAVERER_IN_PROCESS_SLAVERER_THREAD_H_
