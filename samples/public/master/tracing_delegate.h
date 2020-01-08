// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_TRACING_DELEGATE_H_
#define SAMPLES_PUBLIC_MASTER_TRACING_DELEGATE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "samples/common/export.h"

namespace base {
class DictionaryValue;
}

namespace samples {
class BackgroundTracingConfig;

typedef base::Callback<bool(const std::string& metadata_name)>
    MetadataFilterPredicate;

// This can be implemented by the embedder to provide functionality for the
// about://tracing WebUI.
class SAMPLES_EXPORT TracingDelegate {
 public:
  virtual ~TracingDelegate() {}

  // This can be used to veto a particular background tracing scenario.
  virtual bool IsAllowedToBeginBackgroundScenario(
      const BackgroundTracingConfig& config,
      bool requires_anonymized_data);

  virtual bool IsAllowedToEndBackgroundScenario(
      const samples::BackgroundTracingConfig& config,
      bool requires_anonymized_data);

  virtual bool IsProfileLoaded();

  // Used to add any additional metadata to traces.
  virtual std::unique_ptr<base::DictionaryValue> GenerateMetadataDict();

  virtual MetadataFilterPredicate GetMetadataFilterPredicate();
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_TRACING_DELEGATE_H_
