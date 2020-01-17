// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/master/master_child_process_observer.h"

#include "samples/master/master_child_process_host_impl.h"

namespace samples {

// static
void MasterChildProcessObserver::Add(MasterChildProcessObserver* observer) {
  MasterChildProcessHostImpl::AddObserver(observer);
}

// static
void MasterChildProcessObserver::Remove(
    MasterChildProcessObserver* observer) {
  MasterChildProcessHostImpl::RemoveObserver(observer);
}

}  // namespace samples
