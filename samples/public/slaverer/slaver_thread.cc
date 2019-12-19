// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/slaverer/slaver_thread.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace samples {

// Keep the global SlaverThread in a TLS slot so it is impossible to access
// incorrectly from the wrong thread.
static base::LazyInstance<
    base::ThreadLocalPointer<SlaverThread>>::DestructorAtExit lazy_tls =
    LAZY_INSTANCE_INITIALIZER;

SlaverThread* SlaverThread::Get() {
  return lazy_tls.Pointer()->Get();
}

SlaverThread::SlaverThread() {
  lazy_tls.Pointer()->Set(this);
}

SlaverThread::~SlaverThread() {
  lazy_tls.Pointer()->Set(nullptr);
}

}  // namespace samples
