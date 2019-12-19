// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_NOTIFICATION_OBSERVER_H_
#define SAMPLES_PUBLIC_MASTER_NOTIFICATION_OBSERVER_H_

#include "samples/common/export.h"

namespace samples {

class NotificationDetails;
class NotificationSource;

// This is the base class for notification observers. When a matching
// notification is posted to the notification service, Observe is called.
class SAMPLES_EXPORT NotificationObserver {
 public:
  virtual void Observe(int type,
                       const NotificationSource& source,
                       const NotificationDetails& details) = 0;

 protected:
  virtual ~NotificationObserver() {}
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_NOTIFICATION_OBSERVER_H_
