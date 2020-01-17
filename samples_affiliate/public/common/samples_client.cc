// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/common/samples_client.h"

#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_piece.h"
#include "base/values.h"
#include "build/build_config.h"

namespace samples {

static SamplesClient* g_client;

class InternalTestInitializer {
 public:
  static SamplesMasterClient* SetMaster(SamplesMasterClient* b) {
    SamplesMasterClient* rv = g_client->master_;
    g_client->master_ = b;
    return rv;
  }

  static SamplesSlavererClient* SetSlave(SamplesSlavererClient* r) {
    SamplesSlavererClient* rv = g_client->slaverer_;
    g_client->slaverer_ = r;
    return rv;
  }

  static SamplesUtilityClient* SetUtility(SamplesUtilityClient* u) {
    SamplesUtilityClient* rv = g_client->utility_;
    g_client->utility_ = u;
    return rv;
  }
};

void SetSamplesClient(SamplesClient* client) {
  g_client = client;
}

SamplesClient* GetSamplesClient() {
  return g_client;
}

SamplesMasterClient* SetMasterClientForTesting(SamplesMasterClient* b) {
  return InternalTestInitializer::SetMaster(b);
}

SamplesSlavererClient* SetSlaveClientForTesting(SamplesSlavererClient* r) {
  return InternalTestInitializer::SetSlave(r);
}

SamplesUtilityClient* SetUtilityClientForTesting(SamplesUtilityClient* u) {
  return InternalTestInitializer::SetUtility(u);
}

SamplesClient::Schemes::Schemes() = default;
SamplesClient::Schemes::~Schemes() = default;

SamplesClient::SamplesClient()
    : master_(nullptr), slaverer_(nullptr), utility_(nullptr) {}

SamplesClient::~SamplesClient() {
}

bool SamplesClient::CanSendWhileSwappedOut(const IPC::Message* message) {
  return false;
}

std::string SamplesClient::GetProduct() const {
  return std::string();
}

base::string16 SamplesClient::GetLocalizedString(int message_id) const {
  return base::string16();
}

std::string SamplesClient::GetProcessTypeNameInEnglish(int type) {
  NOTIMPLEMENTED();
  return std::string();
}

base::DictionaryValue SamplesClient::GetNetLogConstants() const {
  return base::DictionaryValue();
}

bool SamplesClient::AllowScriptExtensionForServiceWorker(
    const GURL& script_url) {
  return false;
}

#if defined(OS_ANDROID)
bool SamplesClient::UsingSynchronousCompositing() {
  return false;
}

#endif  // OS_ANDROID

base::StringPiece SamplesClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) const {
  return base::StringPiece();
}

base::RefCountedMemory* SamplesClient::GetDataResourceBytes(
    int resource_id) const {
  return nullptr;
}

void SamplesClient::OnServiceManagerConnected(
    ServiceManagerConnection* connection) {}

}  // namespace samples
