// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/samples/service.h"

#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/samples/service_delegate.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace samples {

Service::Service(ServiceDelegate* delegate) : delegate_(delegate) {
}

Service::~Service() {
  delegate_->WillDestroyServiceInstance(this);
}

void Service::ForceQuit() {
  // Force-disconnect from the Service Mangager. Under normal circumstances
  // (i.e. in non-test code), the call below destroys |this|.
  context()->QuitNow();
}

void Service::OnBindInterface(const service_manager::BindSourceInfo& source,
                              const std::string& interface_name,
                              mojo::ScopedMessagePipeHandle pipe) {
  binders_.BindInterface(interface_name, std::move(pipe));
}

}  // namespace samples
