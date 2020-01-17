// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/shell/app/shell_main_delegate.h"

#include <iostream>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/cpu.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/trace_event/trace_log.h"
#include "build/build_config.h"
#include "samples/common/samples_constants_internal.h"
#include "samples/public/master/master_main_runner.h"
#include "samples/shell/common/shell_samples_client.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/url_constants.h"
#include "samples/shell/master/shell_master_main.h"
#include "samples/shell/master/shell_samples_master_client.h"
//#include "samples/shell/common/shell_switches.h"
#include "samples/shell/slaverer/shell_samples_slaverer_client.h"
#include "samples/shell/utility/shell_samples_utility_client.h"
#include "ipc/ipc_buildflags.h"
#include "services/service_manager/embedder/switches.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"

//#if BUILDFLAG(IPC_MESSAGE_LOG_ENABLED)
//#define IPC_MESSAGE_MACROS_LOG_ENABLED
//#include "samples/public/common/samples_ipc_logging.h"
//#define IPC_LOG_TABLE_ADD_ENTRY(msg_id, logger) \
//    samples::RegisterIPCLogger(msg_id, logger)
//#include "samples/shell/common/shell_messages.h"
//#endif

#if defined(OS_ANDROID)
#include "base/android/apk_assets.h"
#include "base/posix/global_descriptors.h"
#include "samples/shell/android/shell_descriptors.h"
#endif

namespace {

void InitLogging(const base::CommandLine& command_line) {
  base::FilePath log_filename =
      command_line.GetSwitchValuePath(switches::kLogFile);
  if (log_filename.empty()) {
#if defined(OS_FUCHSIA)
    base::PathService::Get(base::DIR_TEMP, &log_filename);
#else
    base::PathService::Get(base::DIR_EXE, &log_filename);
#endif
    log_filename = log_filename.AppendASCII("samples_shell.log");
  }

  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_ALL;
  settings.log_file = log_filename.value().c_str();
  settings.delete_old = logging::DELETE_OLD_LOG_FILE;
  logging::InitLogging(settings);
  logging::SetLogItems(true /* Process ID */, true /* Thread ID */,
                       true /* Timestamp */, false /* Tick count */);
}

}  // namespace

namespace samples {

ShellMainDelegate::ShellMainDelegate(bool is_mastertest)
    : is_mastertest_(is_mastertest) {}

ShellMainDelegate::~ShellMainDelegate() {
}

bool ShellMainDelegate::BasicStartupComplete(int* exit_code) {
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  int dummy;
  if (!exit_code)
    exit_code = &dummy;

  InitLogging(command_line);

  samples_client_.reset(new ShellSamplesClient);
  SetSamplesClient(samples_client_.get());

  return false;
}

void ShellMainDelegate::PreSandboxStartup() {
#if defined(ARCH_CPU_ARM_FAMILY) && (defined(OS_ANDROID) || defined(OS_LINUX))
  // Create an instance of the CPU class to parse /proc/cpuinfo and cache
  // cpu_brand info.
  base::CPU cpu_info;
#endif

  InitializeResourceBundle();
}

int ShellMainDelegate::RunProcess(
    const std::string& process_type,
    const MainFunctionParams& main_function_params) {
  if (!process_type.empty())
    return -1;

#if !defined(OS_ANDROID)
  // Android stores the MasterMainRunner instance as a scoped member pointer
  // on the ShellMainDelegate class because of different object lifetime.
  std::unique_ptr<MasterMainRunner> master_runner_;
#endif

  base::trace_event::TraceLog::GetInstance()->set_process_name("Master");
  base::trace_event::TraceLog::GetInstance()->SetProcessSortIndex(
      kTraceEventMasterProcessSortIndex);

  master_runner_.reset(MasterMainRunner::Create());
  return ShellMasterMain(main_function_params, master_runner_);
}

void ShellMainDelegate::InitializeResourceBundle() {
#if defined(OS_ANDROID)
  // On Android, the renderer runs with a different UID and can never access
  // the file system. Use the file descriptor passed in at launch time.
  auto* global_descriptors = base::GlobalDescriptors::GetInstance();
  int pak_fd = global_descriptors->MaybeGet(kShellPakDescriptor);
  base::MemoryMappedFile::Region pak_region;
  if (pak_fd >= 0) {
    pak_region = global_descriptors->GetRegion(kShellPakDescriptor);
  } else {
    pak_fd =
        base::android::OpenApkAsset("assets/samples_shell.pak", &pak_region);
    // Loaded from disk for browsertests.
    if (pak_fd < 0) {
      base::FilePath pak_file;
      bool r = base::PathService::Get(base::DIR_ANDROID_APP_DATA, &pak_file);
      DCHECK(r);
      pak_file = pak_file.Append(FILE_PATH_LITERAL("paks"));
      pak_file = pak_file.Append(FILE_PATH_LITERAL("samples_shell.pak"));
      int flags = base::File::FLAG_OPEN | base::File::FLAG_READ;
      pak_fd = base::File(pak_file, flags).TakePlatformFile();
      pak_region = base::MemoryMappedFile::Region::kWholeFile;
    }
    global_descriptors->Set(kShellPakDescriptor, pak_fd, pak_region);
  }
  DCHECK_GE(pak_fd, 0);
  // This is clearly wrong. See crbug.com/330930
  ui::ResourceBundle::InitSharedInstanceWithPakFileRegion(base::File(pak_fd),
                                                          pak_region);
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromFileRegion(
      base::File(pak_fd), pak_region, ui::SCALE_FACTOR_100P);
#elif defined(OS_MACOSX)
  ui::ResourceBundle::InitSharedInstanceWithPakPath(GetResourcesPakFilePath());
#else
  base::FilePath pak_file;
  bool r = base::PathService::Get(base::DIR_ASSETS, &pak_file);
  DCHECK(r);
  pak_file = pak_file.Append(FILE_PATH_LITERAL("samples_shell.pak"));
  ui::ResourceBundle::InitSharedInstanceWithPakPath(pak_file);
#endif
}

void ShellMainDelegate::PreCreateMainMessageLoop() {
}

SamplesMasterClient* ShellMainDelegate::CreateSamplesMasterClient() {
  master_client_.reset(new ShellSamplesMasterClient);

  return master_client_.get();
}

SamplesSlavererClient* ShellMainDelegate::CreateSamplesSlavererClient() {
  slaverer_client_.reset(new ShellSamplesSlavererClient);

  return slaverer_client_.get();
}

SamplesUtilityClient* ShellMainDelegate::CreateSamplesUtilityClient() {
  utility_client_.reset(new ShellSamplesUtilityClient(is_mastertest_));
  return utility_client_.get();
}

}  // namespace samples
