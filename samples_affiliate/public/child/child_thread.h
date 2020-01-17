// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_CHILD_CHILD_THREAD_H_
#define SAMPLES_PUBLIC_CHILD_CHILD_THREAD_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "samples/common/export.h"
#include "ipc/ipc_sender.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace service_manager {
class Connector;
}

namespace samples {

class ServiceManagerConnection;

// An abstract base class that contains logic shared between most child
// processes of the embedder.
class SAMPLES_EXPORT ChildThread : public IPC::Sender {
 public:
  // Returns the one child thread for this process.  Note that this can only be
  // accessed when running on the child thread itself.
  static ChildThread* Get();

  ~ChildThread() override {}

  // Returns the ServiceManagerConnection for the thread (from which a
  // service_manager::Connector can be obtained).
  virtual ServiceManagerConnection* GetServiceManagerConnection() = 0;

  // Returns a connector that can be used to bind interfaces exposed by other
  // services.
  virtual service_manager::Connector* GetConnector() = 0;

  virtual scoped_refptr<base::SingleThreadTaskRunner> GetIOTaskRunner() = 0;

  // Tells the child process that a field trial was activated.
  virtual void SetFieldTrialGroup(const std::string& trial_name,
                                  const std::string& group_name) = 0;

};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_CHILD_CHILD_THREAD_H_
