// Copyright 2013 The Chromium Authors. All rights reserved.                                                                                                                                                
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_ANDROID_MASTER_STARTUP_CONTROLLER_H_
#define SAMPLES_MASTER_ANDROID_MASTER_STARTUP_CONTROLLER_H_

namespace samples {

void MasterStartupComplete(int result);
bool ShouldStartGpuProcessOnMasterStartup();
void ServiceManagerStartupComplete();

}  // namespace samples 

#endif  // SAMPLES_MASTER_ANDROID_MASTER_STARTUP_CONTROLLER_H_
