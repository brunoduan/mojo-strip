// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_ANDROID_JAVA_INTERFACES_H_
#define SAMPLES_PUBLIC_MASTER_ANDROID_JAVA_INTERFACES_H_

#include "samples/common/export.h"

namespace service_manager {
class InterfaceProvider;
}

namespace samples {

// Returns an InterfaceProvider for global Java-implemented interfaces.
// This provides access to interfaces implemented in Java in the master process
// to C++ code in the master process. This and the returned InterfaceProvider
// may only be used on the UI thread.
SAMPLES_EXPORT service_manager::InterfaceProvider* GetGlobalJavaInterfaces();

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_ANDROID_JAVA_INTERFACES_H_
