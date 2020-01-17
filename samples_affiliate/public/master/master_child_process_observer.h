// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_MASTER_CHILD_PROCESS_OBSERVER_H_
#define SAMPLES_PUBLIC_MASTER_MASTER_CHILD_PROCESS_OBSERVER_H_

#include "samples/common/export.h"

namespace samples {

struct ChildProcessData;
struct ChildProcessTerminationInfo;

// An observer API implemented by classes which are interested in browser child
// process events. Note that render processes cannot be observed through this
// interface; use RenderProcessHostObserver instead.
class SAMPLES_EXPORT MasterChildProcessObserver {
 public:
  // Called when a child process host has connected to a child process.
  // Note that |data.handle| may be invalid, if the child process connects to
  // the pipe before the process launcher's reply arrives.
  virtual void MasterChildProcessHostConnected(const ChildProcessData& data) {}

  // Called when a child process has successfully launched and has connected to
  // it child process host. The |data.handle| is guaranteed to be valid.
  virtual void MasterChildProcessLaunchedAndConnected(
      const ChildProcessData& data) {}

  // Called after a ChildProcessHost is disconnected from the child process.
  virtual void MasterChildProcessHostDisconnected(
      const ChildProcessData& data) {}

  // Called when a child process disappears unexpectedly as a result of a crash.
  virtual void MasterChildProcessCrashed(
      const ChildProcessData& data,
      const ChildProcessTerminationInfo& info) {}

  // Called when a child process disappears unexpectedly as a result of being
  // killed.
  virtual void MasterChildProcessKilled(
      const ChildProcessData& data,
      const ChildProcessTerminationInfo& info) {}

  // Note for Android. There is no way to reliably distinguish between Crash
  // and Kill. Arbitrarily choose all abnormal terminations on Android to call
  // MasterChildProcessKilled, which means MasterChildProcessCrashed will
  // never be called on Android.

 protected:
  // The observer can be destroyed on any thread.
  virtual ~MasterChildProcessObserver() {}

  static void Add(MasterChildProcessObserver* observer);
  static void Remove(MasterChildProcessObserver* observer);
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_MASTER_CHILD_PROCESS_OBSERVER_H_
