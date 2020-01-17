// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/slaverer/slaver_thread_observer.h"

namespace samples {

bool SlaverThreadObserver::OnControlMessageReceived(
    const IPC::Message& message) {
  return false;
}

}  // namespace samples
