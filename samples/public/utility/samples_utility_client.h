// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_UTILITY_SAMPLES_UTILITY_CLIENT_H_
#define SAMPLES_PUBLIC_UTILITY_SAMPLES_UTILITY_CLIENT_H_

#include <map>
#include <memory>

#include "base/callback_forward.h"
#include "samples/public/common/samples_client.h"
#include "services/service_manager/embedder/embedded_service_info.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace samples {

// Embedder API for participating in renderer logic.
class SAMPLES_EXPORT SamplesUtilityClient {
 public:
  using StaticServiceMap =
      std::map<std::string, service_manager::EmbeddedServiceInfo>;

  virtual ~SamplesUtilityClient() {}

  // Notifies us that the UtilityThread has been created.
  virtual void UtilityThreadStarted() {}

  // Allows the embedder to filter messages.
  virtual bool OnMessageReceived(const IPC::Message& message);

  virtual void RegisterServices(StaticServiceMap* services) {}

};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_UTILITY_SAMPLES_UTILITY_CLIENT_H_
