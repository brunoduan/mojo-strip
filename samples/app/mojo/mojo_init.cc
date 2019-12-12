// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/app/mojo/mojo_init.h"

#include <memory>

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "ipc/ipc_channel.h"
#include "mojo/core/embedder/configuration.h"
#include "mojo/core/embedder/embedder.h"

namespace samples {

namespace {

class MojoInitializer {
 public:
  MojoInitializer() {
    mojo::core::Configuration config;
    config.max_message_num_bytes = IPC::Channel::kMaximumMessageSize;
    mojo::core::Init(config);
  }
};

base::LazyInstance<MojoInitializer>::Leaky mojo_initializer;

}  //  namespace

void InitializeMojo() {
  mojo_initializer.Get();
}

}  // namespace samples
