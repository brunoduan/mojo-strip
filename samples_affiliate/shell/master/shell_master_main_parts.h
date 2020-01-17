// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SHELL_MASTER_SHELL_MASTER_MAIN_PARTS_H_
#define SAMPLES_SHELL_MASTER_SHELL_MASTER_MAIN_PARTS_H_

#include <memory>

#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "build/build_config.h"
#include "samples/public/master/master_main_parts.h"
#include "samples/public/common/main_function_params.h"
#include "samples/shell/master/shell_master_context.h"

namespace samples {

class ShellMasterMainParts : public MasterMainParts {
 public:
  explicit ShellMasterMainParts(const MainFunctionParams& parameters);
  ~ShellMasterMainParts() override;

  // MasterMainParts overrides.
  int PreEarlyInitialization() override;
  int PreCreateThreads() override;
  void PreMainMessageLoopStart() override;
  void PostMainMessageLoopStart() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PreDefaultMainMessageLoopRun(base::OnceClosure quit_closure) override;
  void PostMainMessageLoopRun() override;
  void PostDestroyThreads() override;

  ShellMasterContext* master_context() { return master_context_.get(); }
  ShellMasterContext* off_the_record_master_context() {
    return off_the_record_master_context_.get();
  }

 protected:
  virtual void InitializeMasterContexts();
  virtual void InitializeMessageLoopContext();

  void set_master_context(ShellMasterContext* context) {
    master_context_.reset(context);
  }
  void set_off_the_record_master_context(ShellMasterContext* context) {
    off_the_record_master_context_.reset(context);
  }

 private:

  std::unique_ptr<ShellMasterContext> master_context_;
  std::unique_ptr<ShellMasterContext> off_the_record_master_context_;

  // For running samples_mastertests.
  const MainFunctionParams parameters_;
  bool run_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(ShellMasterMainParts);
};

}  // namespace samples

#endif  // SAMPLES_SHELL_MASTER_SHELL_MASTER_MAIN_PARTS_H_
