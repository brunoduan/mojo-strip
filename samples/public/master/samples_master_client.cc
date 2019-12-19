// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/master/samples_master_client.h"

#include <utility>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "services/service_manager/sandbox/sandbox_type.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace samples {

void OverrideOnBindInterface(const service_manager::BindSourceInfo& remote_info,
                             const std::string& name,
                             mojo::ScopedMessagePipeHandle* handle) {
  GetSamplesClient()->master()->OverrideOnBindInterface(remote_info, name,
                                                         handle);
}

MasterMainParts* SamplesMasterClient::CreateMasterMainParts(
    const MainFunctionParams& parameters) {
  return nullptr;
}

void SamplesMasterClient::PostAfterStartupTask(
    const base::Location& from_here,
    const scoped_refptr<base::TaskRunner>& task_runner,
    base::OnceClosure task) {
  task_runner->PostTask(from_here, std::move(task));
}

bool SamplesMasterClient::IsMasterStartupComplete() {
  return true;
}

void SamplesMasterClient::SetMasterStartupIsCompleteForTesting() {}

bool SamplesMasterClient::ShouldUseProcessPerSite(
    MasterContext* master_context, const GURL& effective_url) {
  return false;
}

bool SamplesMasterClient::IsSuitableHost(SlaverProcessHost* process_host,
                                          const GURL& site_url) {
  return true;
}

bool SamplesMasterClient::MayReuseHost(SlaverProcessHost* process_host) {
  return true;
}

bool SamplesMasterClient::ShouldTryToUseExistingProcessHost(
      MasterContext* master_context, const GURL& url) {
  return false;
}

std::vector<url::Origin>
SamplesMasterClient::GetOriginsRequiringDedicatedProcess() {
  return std::vector<url::Origin>();
}

bool SamplesMasterClient::ShouldEnableStrictSiteIsolation() {
#if defined(OS_ANDROID)
  return false;
#else
  return true;
#endif
}

bool SamplesMasterClient::IsFileAccessAllowed(
    const base::FilePath& path,
    const base::FilePath& absolute_path,
    const base::FilePath& profile_path) {
  return true;
}

std::string SamplesMasterClient::GetApplicationLocale() {
  return "en-US";
}

base::FilePath SamplesMasterClient::GetLoggingFileName(
    const base::CommandLine& command_line) {
  return base::FilePath();
}

base::FilePath SamplesMasterClient::GetShaderDiskCacheDirectory() {
  return base::FilePath();
}

base::FilePath SamplesMasterClient::GetGrShaderDiskCacheDirectory() {
  return base::FilePath();
}

std::string SamplesMasterClient::GetServiceUserIdForMasterContext(
    MasterContext* master_context) {
  return base::GenerateGUID();
}

std::unique_ptr<base::Value> SamplesMasterClient::GetServiceManifestOverlay(
    base::StringPiece name) {
  return nullptr;
}

SamplesMasterClient::OutOfProcessServiceInfo::OutOfProcessServiceInfo() =
    default;

SamplesMasterClient::OutOfProcessServiceInfo::OutOfProcessServiceInfo(
    const ProcessNameCallback& process_name_callback)
    : process_name_callback(process_name_callback) {}

SamplesMasterClient::OutOfProcessServiceInfo::OutOfProcessServiceInfo(
    const ProcessNameCallback& process_name_callback,
    const std::string& process_group)
    : process_name_callback(process_name_callback),
      process_group(process_group) {
  DCHECK(!process_group.empty());
}

SamplesMasterClient::OutOfProcessServiceInfo::~OutOfProcessServiceInfo() =
    default;

bool SamplesMasterClient::ShouldTerminateOnServiceQuit(
    const service_manager::Identity& id) {
  return false;
}

std::vector<SamplesMasterClient::ServiceManifestInfo>
SamplesMasterClient::GetExtraServiceManifests() {
  return std::vector<SamplesMasterClient::ServiceManifestInfo>();
}

std::vector<service_manager::Identity>
SamplesMasterClient::GetStartupServices() {
  return std::vector<service_manager::Identity>();
}

std::unique_ptr<base::TaskScheduler::InitParams>
SamplesMasterClient::GetTaskSchedulerInitParams() {
  return nullptr;
}

bool SamplesMasterClient::ShouldCreateTaskScheduler() {
  return true;
}

}  // namespace samples
