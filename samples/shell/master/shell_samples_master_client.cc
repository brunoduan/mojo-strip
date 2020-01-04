// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/shell/master/shell_samples_master_client.h"

#include <stddef.h>
#include <utility>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/pattern.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "samples/public/master/slaver_process_host.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/service_names.mojom.h"
#include "samples/public/common/url_constants.h"
#include "samples/public/test/test_service.h"
#include "samples/shell/grit/shell_resources.h"
#include "samples/shell/master/shell_master_context.h"
#include "samples/shell/master/shell_master_main_parts.h"
#include "services/test/echo/public/mojom/echo.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

#if defined(OS_ANDROID)
#include "base/android/apk_assets.h"
#include "base/android/path_utils.h"
#include "samples/shell/android/shell_descriptors.h"
#endif

#include "ui/base/resource/resource_bundle.h"

namespace samples {

namespace {

ShellSamplesMasterClient* g_master_client;

}  // namespace

ShellSamplesMasterClient* ShellSamplesMasterClient::Get() {
  return g_master_client;
}

ShellSamplesMasterClient::ShellSamplesMasterClient()
    : shell_master_main_parts_(nullptr) {
  DCHECK(!g_master_client);
  g_master_client = this;
}

ShellSamplesMasterClient::~ShellSamplesMasterClient() {
  g_master_client = nullptr;
}

MasterMainParts* ShellSamplesMasterClient::CreateMasterMainParts(
    const MainFunctionParams& parameters) {
  shell_master_main_parts_ = new ShellMasterMainParts(parameters);
  return shell_master_main_parts_;
}

void ShellSamplesMasterClient::RegisterInProcessServices(
    StaticServiceMap* services,
    samples::ServiceManagerConnection* connection) {
}

void ShellSamplesMasterClient::RegisterOutOfProcessServices(
    OutOfProcessServiceMap* services) {
  (*services)[kTestServiceUrl] =
      base::BindRepeating(&base::ASCIIToUTF16, "Test Service");
  (*services)[echo::mojom::kServiceName] =
      base::BindRepeating(&base::ASCIIToUTF16, "Echo Service");
}

bool ShellSamplesMasterClient::ShouldTerminateOnServiceQuit(
    const service_manager::Identity& id) {
  if (should_terminate_on_service_quit_callback_)
    return should_terminate_on_service_quit_callback_.Run(id);
  return false;
}

void ShellSamplesMasterClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
}

std::unique_ptr<base::Value>
ShellSamplesMasterClient::GetServiceManifestOverlay(base::StringPiece name) {
  int id = -1;
  if (name == samples::mojom::kMasterServiceName)
    id = IDR_SAMPLES_SHELL_MASTER_MANIFEST_OVERLAY;
  else if (name == samples::mojom::kPackagedServicesServiceName)
    id = IDR_SAMPLES_SHELL_PACKAGED_SERVICES_MANIFEST_OVERLAY;
  else if (name == samples::mojom::kSlavererServiceName)
    id = IDR_SAMPLES_SHELL_SLAVERER_MANIFEST_OVERLAY;
  else if (name == samples::mojom::kUtilityServiceName)
    id = IDR_SAMPLES_SHELL_UTILITY_MANIFEST_OVERLAY;
  if (id == -1)
    return nullptr;

  base::StringPiece manifest_sampless =
      ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
          id, ui::ScaleFactor::SCALE_FACTOR_NONE);
  return base::JSONReader::Read(manifest_sampless);
}

void ShellSamplesMasterClient::AdjustUtilityServiceProcessCommandLine(
    const service_manager::Identity& identity,
    base::CommandLine* command_line) {
#if defined(OS_CHROMEOS)
  if (identity.name() == test_ws::mojom::kServiceName)
    command_line->AppendSwitch(switches::kMessageLoopTypeUi);
#endif
}

#if defined(OS_LINUX) || defined(OS_ANDROID)
void ShellSamplesMasterClient::GetAdditionalMappedFilesForChildProcess(
    const base::CommandLine& command_line,
    int child_process_id,
    samples::PosixFileDescriptorInfo* mappings) {
#if defined(OS_ANDROID)
  mappings->ShareWithRegion(
      kShellPakDescriptor,
      base::GlobalDescriptors::GetInstance()->Get(kShellPakDescriptor),
      base::GlobalDescriptors::GetInstance()->GetRegion(kShellPakDescriptor));

#else
  int crash_signal_fd = GetCrashSignalFD(command_line);
  if (crash_signal_fd >= 0) {
    mappings->Share(service_manager::kCrashDumpSignal, crash_signal_fd);
  }
#endif  // !defined(OS_ANDROID)
}
#endif  // defined(OS_LINUX) || defined(OS_ANDROID)

ShellMasterContext* ShellSamplesMasterClient::master_context() {
  return shell_master_main_parts_->master_context();
}

ShellMasterContext*
    ShellSamplesMasterClient::off_the_record_master_context() {
  return shell_master_main_parts_->off_the_record_master_context();
}

}  // namespace samples
