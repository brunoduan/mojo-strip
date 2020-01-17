// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_SLAVER_PROCESS_HOST_FACTORY_H_
#define SAMPLES_PUBLIC_MASTER_SLAVER_PROCESS_HOST_FACTORY_H_

#include "samples/common/export.h"

namespace samples {
class MasterContext;
class SlaverProcessHost;

// Factory object for SlaverProcessHosts. Using this factory allows tests to
// swap out a different one to use a TestSlaverProcessHost.
class SlaverProcessHostFactory {
 public:
  virtual ~SlaverProcessHostFactory() {}
  virtual SlaverProcessHost* CreateSlaverProcessHost(
      MasterContext* browser_context) const = 0;
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_SLAVER_PROCESS_HOST_FACTORY_H_
