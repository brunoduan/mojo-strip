// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_SHELL_MASTER_SHELL_SAMPLES_MASTER_CLIENT_H_
#define SAMPLES_SHELL_MASTER_SHELL_SAMPLES_MASTER_CLIENT_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "build/build_config.h"
#include "samples/public/master/samples_master_client.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace samples {

class ShellMasterContext;
class ShellMasterMainParts;

class ShellSamplesMasterClient : public SamplesMasterClient {
 public:
  // Gets the current instance.
  static ShellSamplesMasterClient* Get();

  ShellSamplesMasterClient();
  ~ShellSamplesMasterClient() override;

  // SamplesMasterClient overrides.
  MasterMainParts* CreateMasterMainParts(
      const MainFunctionParams& parameters) override;
  void RegisterInProcessServices(StaticServiceMap* services,
                                 ServiceManagerConnection* connection) override;
  void RegisterOutOfProcessServices(OutOfProcessServiceMap* services) override;
  bool ShouldTerminateOnServiceQuit(
      const service_manager::Identity& id) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  std::unique_ptr<base::Value> GetServiceManifestOverlay(
      base::StringPiece name) override;
  void AdjustUtilityServiceProcessCommandLine(
      const service_manager::Identity& identity,
      base::CommandLine* command_line) override;

#if defined(OS_LINUX) || defined(OS_ANDROID)
  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      samples::PosixFileDescriptorInfo* mappings) override;
#endif  // defined(OS_LINUX) || defined(OS_ANDROID)


  ShellMasterContext* master_context();
  ShellMasterContext* off_the_record_master_context();
  ShellMasterMainParts* shell_master_main_parts() {
    return shell_master_main_parts_;
  }

  void set_should_terminate_on_service_quit_callback(
      base::Callback<bool(const service_manager::Identity&)> callback) {
    should_terminate_on_service_quit_callback_ = std::move(callback);
  }

 protected:
  void set_master_main_parts(ShellMasterMainParts* parts) {
    shell_master_main_parts_ = parts;
  }

 private:
  base::Callback<bool(const service_manager::Identity&)>
      should_terminate_on_service_quit_callback_;

  ShellMasterMainParts* shell_master_main_parts_;
};

}  // namespace samples

#endif  // SAMPLES_SHELL_MASTER_SHELL_SAMPLES_MASTER_CLIENT_H_
