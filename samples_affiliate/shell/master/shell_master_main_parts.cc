// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/shell/master/shell_master_main_parts.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop_current.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "samples/public/master/master_thread.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/main_function_params.h"
#include "samples/public/common/url_constants.h"
#include "samples/shell/android/shell_descriptors.h"
#include "samples/shell/master/shell_master_context.h"
#include "services/service_manager/embedder/result_codes.h"
#include "url/gurl.h"

namespace samples {

ShellMasterMainParts::ShellMasterMainParts(
    const MainFunctionParams& parameters)
    : parameters_(parameters),
      run_message_loop_(true) {
}

ShellMasterMainParts::~ShellMasterMainParts() {
}

#if !defined(OS_MACOSX)
void ShellMasterMainParts::PreMainMessageLoopStart() {
}
#endif

void ShellMasterMainParts::PostMainMessageLoopStart() {
}

int ShellMasterMainParts::PreEarlyInitialization() {
  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void ShellMasterMainParts::InitializeMasterContexts() {
  set_master_context(new ShellMasterContext(false));
  set_off_the_record_master_context(
      new ShellMasterContext(true));
}

void ShellMasterMainParts::InitializeMessageLoopContext() {
}

int ShellMasterMainParts::PreCreateThreads() {
  return 0;
}

void ShellMasterMainParts::PreMainMessageLoopRun() {
  InitializeMasterContexts();
  InitializeMessageLoopContext();

  if (parameters_.ui_task) {
    parameters_.ui_task->Run();
    delete parameters_.ui_task;
    run_message_loop_ = false;
  }
}

bool ShellMasterMainParts::MainMessageLoopRun(int* result_code)  {
  return !run_message_loop_;
}

void ShellMasterMainParts::PostMainMessageLoopRun() {
  master_context_.reset();
  off_the_record_master_context_.reset();
}

void ShellMasterMainParts::PreDefaultMainMessageLoopRun(
    base::OnceClosure quit_closure) {
  //Shell::SetMainMessageLoopQuitClosure(std::move(quit_closure));
}

void ShellMasterMainParts::PostDestroyThreads() {
}

}  // namespace
