// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_MASTER_THREAD_DELEGATE_H_
#define SAMPLES_PUBLIC_MASTER_MASTER_THREAD_DELEGATE_H_

#include "samples/common/export.h"

namespace samples {

// A Delegate for samples embedders to perform extra initialization/cleanup on
// MasterThread::IO.
class MasterThreadDelegate {
 public:
  virtual ~MasterThreadDelegate() = default;

  // Called prior to completing initialization of MasterThread::IO.
  virtual void Init() = 0;

  // Called during teardown of MasterThread::IO.
  virtual void CleanUp() = 0;
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_MASTER_THREAD_DELEGATE_H_
