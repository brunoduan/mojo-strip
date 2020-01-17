// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_MASTER_CHILD_PROCESS_HOST_DELEGATE_H_
#define SAMPLES_PUBLIC_MASTER_MASTER_CHILD_PROCESS_HOST_DELEGATE_H_

#include "samples/common/export.h"
#include "ipc/ipc_listener.h"

namespace samples {

// Interface that all users of MasterChildProcessHost need to provide.
class SAMPLES_EXPORT MasterChildProcessHostDelegate : public IPC::Listener {
 public:
  ~MasterChildProcessHostDelegate() override {}

  // Called when the process has been started.
  virtual void OnProcessLaunched() {}

  // Called if the process failed to launch.  In this case the process never
  // started so the code here is a platform specific error code.
  virtual void OnProcessLaunchFailed(int error_code) {}

  // Called if the process crashed. |exit_code| is the status returned when the
  // process crashed (for posix, as returned from waitpid(), for Windows, as
  // returned from GetExitCodeProcess()).
  virtual void OnProcessCrashed(int exit_code) {}
};

};  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_MASTER_CHILD_PROCESS_HOST_DELEGATE_H_
