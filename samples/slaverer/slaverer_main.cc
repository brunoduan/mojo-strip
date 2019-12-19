// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <utility>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug/debugger.h"
#include "base/debug/leak_annotations.h"
#include "base/i18n/rtl.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram_macros.h"
#include "base/pending_task.h"
#include "base/run_loop.h"
#include "base/sampling_heap_profiler/sampling_heap_profiler.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "base/threading/platform_thread.h"
#include "base/timer/hi_res_timer_manager.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "samples/common/samples_constants_internal.h"
#include "samples/common/samples_switches_internal.h"
#include "samples/common/service_manager/service_manager_connection_impl.h"
#include "samples/public/common/samples_switches.h"
#include "samples/public/common/main_function_params.h"
#include "samples/public/slaverer/samples_slaverer_client.h"
#include "samples/slaverer/slaver_process_impl.h"
#include "samples/slaverer/slaver_thread_impl.h"
#include "samples/slaverer/slaverer_main_platform_delegate.h"
#include "services/service_manager/sandbox/switches.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"

#if defined(OS_ANDROID)
#include "base/android/library_loader/library_loader_hooks.h"
#endif  // OS_ANDROID

#if defined(OS_MACOSX)
#include <Carbon/Carbon.h>
#include <signal.h>
#include <unistd.h>

#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/message_loop/message_pump_mac.h"
#include "third_party/blink/public/web/web_view.h"
#endif  // OS_MACOSX

namespace samples {
namespace {
// This function provides some ways to test crash and assertion handling
// behavior of the slaverer.
static void HandleSlavererErrorTestParameters(
    const base::CommandLine& command_line) {
  if (command_line.HasSwitch(switches::kWaitForDebugger))
    base::debug::WaitForDebugger(60, true);

  if (command_line.HasSwitch(switches::kSlavererStartupDialog))
    WaitForDebugger("Slaverer");
}

}  // namespace

// mainline routine for running as the Slaverer process
int SlavererMain(const MainFunctionParams& parameters) {
  // Don't use the TRACE_EVENT0 macro because the tracing infrastructure doesn't
  // expect synchronous events around the main loop of a thread.
  TRACE_EVENT_ASYNC_BEGIN0("startup", "SlavererMain", 0);

  base::trace_event::TraceLog::GetInstance()->set_process_name("Slaverer");
  base::trace_event::TraceLog::GetInstance()->SetProcessSortIndex(
      kTraceEventSlavererProcessSortIndex);

  const base::CommandLine& command_line = parameters.command_line;

  base::SamplingHeapProfiler::Init();
  if (command_line.HasSwitch(switches::kSamplingHeapProfiler)) {
    base::SamplingHeapProfiler* profiler = base::SamplingHeapProfiler::Get();
    unsigned sampling_interval = 0;
    bool parsed = base::StringToUint(
        command_line.GetSwitchValueASCII(switches::kSamplingHeapProfiler),
        &sampling_interval);
    if (parsed && sampling_interval > 0)
      profiler->SetSamplingInterval(sampling_interval * 1024);
    profiler->Start();
  }

#if defined(OS_MACOSX)
  base::mac::ScopedNSAutoreleasePool* pool = parameters.autorelease_pool;
#endif  // OS_MACOSX

#if defined(OS_CHROMEOS)
  // As Zygote process starts up earlier than master process gets its own
  // locale (at login time for Chrome OS), we have to set the ICU default
  // locale for slaverer process here.
  // ICU locale will be used for fallback font selection etc.
  if (command_line.HasSwitch(switches::kLang)) {
    const std::string locale =
        command_line.GetSwitchValueASCII(switches::kLang);
    base::i18n::SetICUDefaultLocale(locale);
  }
#endif

  // This function allows pausing execution using the --slaverer-startup-dialog
  // flag allowing us to attach a debugger.
  // Do not move this function down since that would mean we can't easily debug
  // whatever occurs before it.
  HandleSlavererErrorTestParameters(command_line);

  SlavererMainPlatformDelegate platform(parameters);
#if defined(OS_MACOSX)
  // As long as scrollbars on Mac are painted with Cocoa, the message pump
  // needs to be backed by a Foundation-level loop to process NSTimers. See
  // http://crbug.com/306348#c24 for details.
  std::unique_ptr<base::MessagePump> pump(new base::MessagePumpNSRunLoop());
  std::unique_ptr<base::MessageLoop> main_message_loop(
      new base::MessageLoop(std::move(pump)));
#else
  // The main message loop of the slaverer services doesn't have IO or UI tasks.
  std::unique_ptr<base::MessageLoop> main_message_loop(new base::MessageLoop());
#endif

  base::PlatformThread::SetName("CrSlavererMain");

  base::Optional<base::Time> initial_virtual_time;
  if (command_line.HasSwitch(switches::kInitialVirtualTime)) {
    double initial_time;
    if (base::StringToDouble(
            command_line.GetSwitchValueASCII(switches::kInitialVirtualTime),
            &initial_time)) {
      initial_virtual_time = base::Time::FromDoubleT(initial_time);
    }
  }

  // PlatformInitialize uses FieldTrials, so this must happen later.
  platform.PlatformInitialize();

  {
    bool should_run_loop = true;
    bool need_sandbox =
        !command_line.HasSwitch(service_manager::switches::kNoSandbox);

#if !defined(OS_WIN) && !defined(OS_MACOSX)
    // Sandbox is enabled before SlaverProcess initialization on all platforms,
    // except Windows and Mac.
    // TODO(markus): Check if it is OK to remove ifdefs for Windows and Mac.
    if (need_sandbox) {
      should_run_loop = platform.EnableSandbox();
      need_sandbox = false;
    }
#endif

    std::unique_ptr<SlaverProcess> slaver_process = SlaverProcessImpl::Create();
    // It's not a memory leak since SlaverThread has the same lifetime
    // as a slaverer process.
    base::RunLoop run_loop;
    new SlaverThreadImpl(run_loop.QuitClosure());

    if (need_sandbox)
      should_run_loop = platform.EnableSandbox();

    base::HighResolutionTimerManager hi_res_timer_manager;

    if (should_run_loop) {
#if defined(OS_MACOSX)
      if (pool)
        pool->Recycle();
#endif
      TRACE_EVENT_ASYNC_BEGIN0("toplevel", "SlavererMain.START_MSG_LOOP", 0);
      run_loop.Run();
      TRACE_EVENT_ASYNC_END0("toplevel", "SlavererMain.START_MSG_LOOP", 0);
    }

#if defined(LEAK_SANITIZER)
    // Run leak detection before SlaverProcessImpl goes out of scope. This helps
    // ignore shutdown-only leaks.
    __lsan_do_leak_check();
#endif
  }
  platform.PlatformUninitialize();
  TRACE_EVENT_ASYNC_END0("startup", "SlavererMain", 0);
  return 0;
}

}  // namespace samples
