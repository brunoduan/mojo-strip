// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_BROWSER_BROWSER_MAIN_LOOP_H_
#define SAMPLES_BROWSER_BROWSER_MAIN_LOOP_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/task/task_scheduler/task_scheduler.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "samples/master/master_process_sub_thread.h"
#include "samples/public/master/master_main_runner.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace base {
class CommandLine;
class FilePath;
class HighResolutionTimerManager;
class MessageLoop;
class SingleThreadTaskRunner;
}  // namespace base

namespace mojo {
namespace core {
class ScopedIPCSupport;
}  // namespace core
}  // namespace mojo

namespace samples {
class MasterMainParts;
class MasterThreadImpl;
class LoaderDelegateImpl;
class ServiceManagerContext;
class SpeechRecognitionManagerImpl;
class StartupTaskRunner;
struct MainFunctionParams;

// Implements the main master loop stages called from MasterMainRunner.
// See comments in master_main_parts.h for additional info.
class SAMPLES_EXPORT MasterMainLoop {
 public:
  // Returns the current instance. This is used to get access to the getters
  // that return objects which are owned by this class.
  static MasterMainLoop* GetInstance();

  // The TaskScheduler instance must exist but not to be started when building
  // MasterMainLoop.
  explicit MasterMainLoop(
      const MainFunctionParams& parameters,
      std::unique_ptr<base::TaskScheduler::ScopedExecutionFence> fence);
  virtual ~MasterMainLoop();

  void Init();

  // Return value is exit status. Anything other than RESULT_CODE_NORMAL_EXIT
  // is considered an error.
  int EarlyInitialization();

  // Initializes the toolkit. Returns whether the toolkit initialization was
  // successful or not.
  bool InitializeToolkit();

  void PreMainMessageLoopStart();
  void MainMessageLoopStart();
  void PostMainMessageLoopStart();
  void PreShutdown();

  // Create and start running the tasks we need to complete startup. Note that
  // this can be called more than once (currently only on Android) if we get a
  // request for synchronous startup while the tasks created by asynchronous
  // startup are still running.
  void CreateStartupTasks();

  // Perform the default message loop run logic.
  void RunMainMessageLoopParts();

  // Performs the shutdown sequence, starting with PostMainMessageLoopRun
  // through stopping threads to PostDestroyThreads.
  void ShutdownThreadsAndCleanUp();

  int GetResultCode() const { return result_code_; }

  // Returns the task runner for tasks that that are critical to producing a new
  // CompositorFrame on resize. On Mac this will be the task runner provided by
  // WindowResizeHelperMac, on other platforms it will just be the thread task
  // runner.
  scoped_refptr<base::SingleThreadTaskRunner> GetResizeTaskRunner();

#if defined(OS_ANDROID)
  void SynchronouslyFlushStartupTasks();
#endif  // OS_ANDROID

  MasterMainParts* parts() { return parts_.get(); }

 private:
  FRIEND_TEST_ALL_PREFIXES(MasterMainLoopTest, CreateThreadsInSingleProcess);

  void InitializeMainThread();

  // Called just before creating the threads
  int PreCreateThreads();

  // Create all secondary threads.
  int CreateThreads();

  // Called just after creating the threads.
  int PostCreateThreads();

  // Called right after the master threads have been started.
  int MasterThreadsStarted();

  int PreMainMessageLoopRun();

  void MainMessageLoopRun();

  void InitializeMojo();

  void InitializeAudio();

  void InitializeMemoryManagementComponent();

  // Quick reference for initialization order:
  // Constructor
  // Init()
  // EarlyInitialization()
  // InitializeToolkit()
  // PreMainMessageLoopStart()
  // MainMessageLoopStart()
  //   InitializeMainThread()
  // PostMainMessageLoopStart()
  // CreateStartupTasks()
  //   PreCreateThreads()
  //   CreateThreads()
  //   PostCreateThreads()
  //   MasterThreadsStarted()
  //     InitializeMojo()
  //   PreMainMessageLoopRun()

  // Members initialized on construction ---------------------------------------
  const MainFunctionParams& parameters_;
  const base::CommandLine& parsed_command_line_;
  int result_code_;
  bool created_threads_;  // True if the non-UI threads were created.
  // //samples must be initialized single-threaded until
  // MasterMainLoop::CreateThreads() as things initialized before it require an
  // initialize-once happens-before relationship with all eventual samples tasks
  // running on other threads. This ScopedExecutionFence ensures that no tasks
  // posted to TaskScheduler gets to run before CreateThreads(); satisfying this
  // requirement even though the TaskScheduler is created and started before
  // samples is entered.
  std::unique_ptr<base::TaskScheduler::ScopedExecutionFence>
      scoped_execution_fence_;

  // Members initialized in |MainMessageLoopStart()| ---------------------------
  std::unique_ptr<base::MessageLoop> main_message_loop_;

  // Members initialized in |PostMainMessageLoopStart()| -----------------------
  std::unique_ptr<MasterProcessSubThread> io_thread_;
  std::unique_ptr<base::HighResolutionTimerManager> hi_res_timer_manager_;

  // Members initialized in |Init()| -------------------------------------------
  // Destroy |parts_| before |main_message_loop_| (required) and before other
  // classes constructed in samples (but after |main_thread_|).
  std::unique_ptr<MasterMainParts> parts_;

  // Members initialized in |InitializeMainThread()| ---------------------------
  // This must get destroyed before other threads that are created in |parts_|.
  std::unique_ptr<MasterThreadImpl> main_thread_;

  // Members initialized in |CreateStartupTasks()| -----------------------------
  std::unique_ptr<StartupTaskRunner> startup_task_runner_;

  // Members initialized in |MasterThreadsStarted()| --------------------------
  std::unique_ptr<ServiceManagerContext> service_manager_context_;
  std::unique_ptr<mojo::core::ScopedIPCSupport> mojo_ipc_support_;

  DISALLOW_COPY_AND_ASSIGN(MasterMainLoop);
};

}  // namespace samples

#endif  // SAMPLES_BROWSER_BROWSER_MAIN_LOOP_H_
