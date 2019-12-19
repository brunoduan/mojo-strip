// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SAMPLES_SERVICE_H_
#define SERVICES_SAMPLES_SERVICE_H_

#include <map>

#include "base/macros.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace samples {

class ServiceDelegate;

// The core Service implementation of the Samples Service. This takes
// responsibility for owning top-level state for an instance of the service,
// binding incoming interface requests, etc.
//
// NOTE: This type is exposed to ServiceDelegate implementations outside
// of private Samples Service code. The public API surface of this class should
// therefore remain as minimal as possible.
class Service : public service_manager::Service {
 public:
  // |delegate| is not owned and must outlive |this|.
  explicit Service(ServiceDelegate* delegate);
  ~Service() override;

  ServiceDelegate* delegate() const { return delegate_; }

  // Forces this instance of the Service to be terminated. Useful if the
  // delegate implementation encounters a scenario in which it can no longer
  // operate correctly. May delete |this|.
  void ForceQuit();

 private:

  // service_manager::Service:
  void OnBindInterface(const service_manager::BindSourceInfo& source,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle pipe) override;

  ServiceDelegate* const delegate_;
  service_manager::BinderRegistry binders_;

  DISALLOW_COPY_AND_ASSIGN(Service);
};

};  // namespace samples

#endif  // SERVICES_SAMPLES_SERVICE_H_
