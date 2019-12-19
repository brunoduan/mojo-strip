// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_SLAVER_PROCESS_HOST_OBSERVER_H_
#define SAMPLES_PUBLIC_MASTER_SLAVER_PROCESS_HOST_OBSERVER_H_

#include "base/process/kill.h"
#include "base/process/process_handle.h"
#include "samples/common/export.h"

namespace samples {

class SlaverProcessHost;
struct ChildProcessTerminationInfo;

// An observer API implemented by classes which are interested
// in SlaverProcessHost lifecycle events.
class SAMPLES_EXPORT SlaverProcessHostObserver {
 public:
  // This method is invoked when the process was launched and the channel was
  // connected. This is the earliest time it is safe to call Shutdown on the
  // SlaverProcessHost.
  virtual void SlaverProcessReady(SlaverProcessHost* host) {}

  // This method is invoked when the process of the observed SlaverProcessHost
  // exits (either normally or with a crash). To determine if the process closed
  // normally or crashed, examine the |status| parameter.
  //
  // A new slaver process may be spawned for this SlaverProcessHost, but there
  // are no guarantees (e.g. if shutdown is occurring, the HostDestroyed
  // callback will happen soon and that will be it, but if the slaverer crashed
  // and the user clicks 'reload', a new slaver process will be spawned).
  //
  // This will cause a call to WebContentsObserver::SlaverProcessGone() for the
  // active slaverer process for the top-level frame; for code that needs to be
  // a WebContentsObserver anyway, consider whether that API might be a better
  // choice.
  virtual void SlaverProcessExited(SlaverProcessHost* host,
                                   const ChildProcessTerminationInfo& info) {}

  // This method is invoked when the observed SlaverProcessHost itself is
  // destroyed. This is guaranteed to be the last call made to the observer, so
  // if the observer is tied to the observed SlaverProcessHost, it is safe to
  // delete it.
  virtual void SlaverProcessHostDestroyed(SlaverProcessHost* host) {}

 protected:
  virtual ~SlaverProcessHostObserver() {}
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_SLAVER_PROCESS_HOST_OBSERVER_H_
