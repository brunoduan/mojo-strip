// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_SLAVERER_SAMPLES_SLAVERER_CLIENT_H_
#define SAMPLES_PUBLIC_SLAVERER_SAMPLES_SLAVERER_CLIENT_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "base/task/task_scheduler/task_scheduler.h"
#include "build/build_config.h"
#include "samples/public/common/samples_client.h"
#include "services/service_manager/public/mojom/service.mojom.h"

namespace base {
class FilePath;
class SingleThreadTaskRunner;
}

namespace samples {

// Embedder API for participating in slaverer logic.
class SAMPLES_EXPORT SamplesSlavererClient {
 public:
  virtual ~SamplesSlavererClient() {}

  // Notifies us that the SlaverThread has been created.
  virtual void SlaverThreadStarted() {}

  // Called on the main-thread immediately after the io thread is
  // created.
  virtual void PostIOThreadCreated(
      base::SingleThreadTaskRunner* io_thread_task_runner);

  // Returns true if the slaverer process should schedule the idle handler when
  // all widgets are hidden.
  virtual bool RunIdleHandlerWhenWidgetsHidden();


  // Allows subclasses to enable some runtime features before Blink has
  // started.
  virtual void SetRuntimeFeaturesDefaultsBeforeBlinkInitialization() {}

  // Provides parameters for initializing the global task scheduler. Default
  // params are used if this returns nullptr.
  virtual std::unique_ptr<base::TaskScheduler::InitParams>
  GetTaskSchedulerInitParams();

  // Asks the embedder to bind |service_request| to its slaverer-side service
  // implementation.
  virtual void CreateSlavererService(
      service_manager::mojom::ServiceRequest service_request) {}
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_SLAVERER_SAMPLES_SLAVERER_CLIENT_H_
