// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SLAVERER_SLAVER_PROCESS_H_
#define SAMPLES_SLAVERER_SLAVER_PROCESS_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/task/task_scheduler/task_scheduler.h"
#include "samples/child/child_process.h"

namespace samples {

// A abstract interface representing the slaverer end of the master<->slaverer
// connection. The opposite end is the SlaverProcessHost. This is a singleton
// object for each slaverer.
//
// SlaverProcessImpl implements this interface for the regular master.
// MockSlaverProcess implements this interface for certain tests, especially
// ones derived from SlaverViewTest.
class SlaverProcess : public ChildProcess {
 public:
  SlaverProcess() = default;
  SlaverProcess(const std::string& task_scheduler_name,
                std::unique_ptr<base::TaskScheduler::InitParams>
                    task_scheduler_init_params);
  ~SlaverProcess() override {}

  // Keep track of the cumulative set of enabled bindings for this process,
  // across any view.
  virtual void AddBindings(int bindings) = 0;

  // The cumulative set of enabled bindings for this process.
  virtual int GetEnabledBindings() const = 0;

  // Returns a pointer to the SlaverProcess singleton instance. Assuming that
  // we're actually a slaverer or a slaverer test, this static cast will
  // be correct.
  static SlaverProcess* current() {
    return static_cast<SlaverProcess*>(ChildProcess::current());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SlaverProcess);
};

}  // namespace samples

#endif  // SAMPLES_SLAVERER_SLAVER_PROCESS_H_
