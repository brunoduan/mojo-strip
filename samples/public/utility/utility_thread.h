// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_UTILITY_UTILITY_THREAD_H_
#define SAMPLES_PUBLIC_UTILITY_UTILITY_THREAD_H_

#include "build/build_config.h"
#include "samples/public/child/child_thread.h"

namespace service_manager {
class Connector;
}

namespace samples {

class SAMPLES_EXPORT UtilityThread : virtual public ChildThread {
 public:
  // Returns the one utility thread for this process.  Note that this can only
  // be accessed when running on the utility thread itself.
  static UtilityThread* Get();

  UtilityThread();
  ~UtilityThread() override;

  // Releases the process.
  virtual void ReleaseProcess() = 0;

  // Initializes blink if it hasn't already been initialized.
  virtual void EnsureBlinkInitialized() = 0;

};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_UTILITY_UTILITY_THREAD_H_
