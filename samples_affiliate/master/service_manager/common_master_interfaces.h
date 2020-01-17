// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_SERVICE_MANAGER_COMMON_MASTER_INTERFACES_H_
#define SAMPLES_MASTER_SERVICE_MANAGER_COMMON_MASTER_INTERFACES_H_

namespace samples {

class ServiceManagerConnection;

// Registers interface binders for master-side interfaces that are common to
// all child process types.
void RegisterCommonMasterInterfaces(ServiceManagerConnection* connection);

}  // namespace samples

#endif  // SAMPLES_MASTER_SERVICE_MANAGER_COMMON_MASTER_INTERFACES_H_
