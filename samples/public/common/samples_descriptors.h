// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_COMMON_SAMPLES_DESCRIPTORS_H_
#define SAMPLES_PUBLIC_COMMON_SAMPLES_DESCRIPTORS_H_

#include "build/build_config.h"

#include "services/service_manager/embedder/descriptors.h"

// This is a list of global descriptor keys to be used with the
// base::GlobalDescriptors object (see base/posix/global_descriptors.h)
enum {
#if defined(OS_ANDROID)
  kAndroidPropertyDescriptor = service_manager::kFirstEmbedderDescriptor,
  kAndroidICUDataDescriptor,
#endif

  // Reserves 100 to 199 for dynamically generated IDs.
  kSamplesDynamicDescriptorStart = 100,
  kSamplesDynamicDescriptorMax = 199,

  // The first key that embedders can use to register descriptors (see
  // base/posix/global_descriptors.h).
  kSamplesIPCDescriptorMax
};

#endif  // SAMPLES_PUBLIC_COMMON_SAMPLES_DESCRIPTORS_H_
