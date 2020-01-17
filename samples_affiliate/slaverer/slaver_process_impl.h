// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SLAVERER_SLAVER_PROCESS_IMPL_H_
#define SAMPLES_SLAVERER_SLAVER_PROCESS_IMPL_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/task/task_scheduler/task_scheduler.h"
#include "samples/slaverer/slaver_process.h"

namespace samples {

// Implementation of the SlaverProcess interface for the regular master.
// See also MockSlaverProcess which implements the active "SlaverProcess" when
// running under certain unit tests.
class SlaverProcessImpl : public SlaverProcess {
 public:
  ~SlaverProcessImpl() override;

  // Creates and returns a SlaverProcessImpl instance.
  //
  // SlaverProcessImpl is created via a static method instead of a simple
  // constructor because non-trivial calls must be made to get the arguments
  // required by constructor of the base class.
  static std::unique_ptr<SlaverProcess> Create();

  // SlaverProcess implementation.
  void AddBindings(int bindings) override;
  int GetEnabledBindings() const override;

  // Do not use these functions.
  // The master process is the only one responsible for knowing when to
  // shutdown its slaverer processes. Reference counting to keep this process
  // alive is not used. To keep this process alive longer, see
  // mojo::KeepAliveHandle and samples::SlaverProcessHostImpl.
  void AddRefProcess() override;
  void ReleaseProcess() override;

 private:
  SlaverProcessImpl(std::unique_ptr<base::TaskScheduler::InitParams>
                        task_scheduler_init_params);

  // Bitwise-ORed set of extra bindings that have been enabled anywhere in this
  // process.  See BindingsPolicy for details.
  int enabled_bindings_;

  DISALLOW_COPY_AND_ASSIGN(SlaverProcessImpl);
};

}  // namespace samples

#endif  // SAMPLES_SLAVERER_SLAVER_PROCESS_IMPL_H_
