// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/samples_service_delegate_impl.h"

#include "base/macros.h"
#include "services/samples/service.h"

namespace samples {

SamplesServiceDelegateImpl::SamplesServiceDelegateImpl(
    MasterContext* master_context) {}

SamplesServiceDelegateImpl::~SamplesServiceDelegateImpl() {
  // This delegate is destroyed immediately before |master_context_| is
  // destroyed. We force-kill any Samples Service instances which depend on
  // |this|, since they will no longer be functional anyway.
  std::set<samples::Service*> instances;
  std::swap(instances, service_instances_);
  for (samples::Service* service : instances) {
    // Eventually destroys |service|. Ensures that no more calls into |this|
    // will occur.
    service->ForceQuit();
  }
}

void SamplesServiceDelegateImpl::AddService(samples::Service* service) {
  service_instances_.insert(service);
}

void SamplesServiceDelegateImpl::WillDestroyServiceInstance(
    samples::Service* service) {
  service_instances_.erase(service);
}

}  // namespace samples
