// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_COMMON_IN_PROCESS_CHILD_THREAD_PARAMS_H_
#define SAMPLES_COMMON_IN_PROCESS_CHILD_THREAD_PARAMS_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "samples/common/export.h"
#include "mojo/public/cpp/system/invitation.h"

namespace samples {

// Tells ChildThreadImpl to run in in-process mode. There are a couple of
// parameters to run in the mode: An emulated io task runner used by
// ChnanelMojo, an IPC channel name to open.
class SAMPLES_EXPORT InProcessChildThreadParams {
 public:
  InProcessChildThreadParams(
      scoped_refptr<base::SingleThreadTaskRunner> io_runner,
      mojo::OutgoingInvitation* mojo_invitation,
      const std::string& service_request_token);
  InProcessChildThreadParams(const InProcessChildThreadParams& other);
  ~InProcessChildThreadParams();

  scoped_refptr<base::SingleThreadTaskRunner> io_runner() const {
    return io_runner_;
  }

  mojo::OutgoingInvitation* mojo_invitation() const { return mojo_invitation_; }

  const std::string& service_request_token() const {
    return service_request_token_;
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> io_runner_;
  mojo::OutgoingInvitation* const mojo_invitation_;
  std::string service_request_token_;
};

}  // namespace samples

#endif  // SAMPLES_COMMON_IN_PROCESS_CHILD_THREAD_PARAMS_H_
