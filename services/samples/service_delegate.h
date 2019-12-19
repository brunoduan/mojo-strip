// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SAMPLES_SERVICE_DELEGATE_H_
#define SERVICES_SAMPLES_SERVICE_DELEGATE_H_

namespace samples {

class Service;

// This is a delegate interface which allows the Samples Service implementation
// to delegate out to private src/samples code without a circular dependency
// between them.
//
// This interface is strictly intended to host transitional APIs for aiding in
// incremental conversion of src/samples and its dependents into/onto the
// Samples Service. As such, APIs should only be added here with a plan for
// eventual removal.
class ServiceDelegate {
 public:
  virtual ~ServiceDelegate() {}

  // Called when an instance of Service (specifically one using this
  // delegate) is about to be destroyed.
  virtual void WillDestroyServiceInstance(Service* service) = 0;

};

};  // namespace samples

#endif  // SERVICES_SAMPLES_SAMPLES_SERVICE_DELEGATE_H_
