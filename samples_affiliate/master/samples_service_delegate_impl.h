// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_MASTER_SAMPLES_SERVICE_DELEGATE_IMPL_H_
#define SAMPLES_MASTER_SAMPLES_SERVICE_DELEGATE_IMPL_H_

#include <memory>
#include <set>

#include "base/macros.h"
#include "services/samples/service_delegate.h"

namespace samples {

class MasterContext;

// Implementation of the main delegate interface for the Samples Service. This
// is used to support the Samples Service implementation with samples/master
// details, without the Samples Service having any build dependencies on
// src/samples. There is one instance of this delegate per MasterContext,
// shared by any SamplesService instance instantiated on behalf of that
// MasterContext.
class SamplesServiceDelegateImpl : public samples::ServiceDelegate {
 public:
  // Constructs a new SamplesServiceDelegateImpl for |master_context|.
  // |master_context| must outlive |this|.
  explicit SamplesServiceDelegateImpl(MasterContext* master_context);
  ~SamplesServiceDelegateImpl() override;

  // Registers |service| with this delegate. Must be called for any |service|
  // using |this| as its SamplesServiceDelegate. Automatically balanced by
  // |WillDestroyServiceInstance()|.
  void AddService(samples::Service* service);

 private:
  // samples::SamplesServiceDelegate:
  void WillDestroyServiceInstance(samples::Service* service) override;

  // Tracks SamplesService instances currently using this delegate. Necessary
  // because the lifetime of |this| is tied to the lifetime of
  // |master_context_|; on destruction of |this|, we need to force all of these
  // SamplesService instances to terminate, since they cannot operate without
  // their delegate.
  std::set<samples::Service*> service_instances_;

  DISALLOW_COPY_AND_ASSIGN(SamplesServiceDelegateImpl);
};

}  // namespace samples

#endif  // SAMPLES_MASTER_SAMPLES_SERVICE_DELEGATE_IMPL_H_
