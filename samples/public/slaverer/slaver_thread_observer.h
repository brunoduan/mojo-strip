// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_SLAVERER_SLAVER_THREAD_OBSERVER_H_
#define SAMPLES_PUBLIC_SLAVERER_SLAVER_THREAD_OBSERVER_H_

#include "base/macros.h"
#include "samples/common/export.h"

namespace blink {
class AssociatedInterfaceRegistry;
}

namespace IPC {
class Message;
}

namespace samples {

// Base class for objects that want to filter control IPC messages and get
// notified of events.
class SAMPLES_EXPORT SlaverThreadObserver {
 public:
  SlaverThreadObserver() {}
  virtual ~SlaverThreadObserver() {}

  // Allows handling incoming Mojo requests.
  virtual void RegisterMojoInterfaces(
      blink::AssociatedInterfaceRegistry* associated_interfaces) {}
  virtual void UnregisterMojoInterfaces(
      blink::AssociatedInterfaceRegistry* associated_interfaces) {}

  // Allows filtering of control messages.
  virtual bool OnControlMessageReceived(const IPC::Message& message);

  // Called when the renderer cache of the plugin list has changed.
  virtual void PluginListChanged() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SlaverThreadObserver);
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_SLAVERER_SLAVER_THREAD_OBSERVER_H_
