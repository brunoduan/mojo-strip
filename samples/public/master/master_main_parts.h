// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_MASTER_MAIN_PARTS_H_
#define SAMPLES_PUBLIC_MASTER_MASTER_MAIN_PARTS_H_

#include "base/callback.h"
#include "samples/common/export.h"

namespace samples {

class ServiceManagerConnection;

// This class contains different "stages" to be executed by |MasterMain()|,
// Each stage is represented by a single MasterMainParts method, called from
// the corresponding method in |MasterMainLoop| (e.g., EarlyInitialization())
// which does the following:
//  - calls a method (e.g., "PreEarlyInitialization()") which implements
//    platform / tookit specific code for that stage.
//  - calls various methods for things common to all platforms (for that stage).
//  - calls a method (e.g., "PostEarlyInitialization()") for platform-specific
//    code to be called after the common code.
//
// Stages:
//  - EarlyInitialization: things which should be done as soon as possible on
//    program start (such as setting up signal handlers) and things to be done
//    at some generic time before the start of the main message loop.
//  - MainMessageLoopStart: things beginning with the start of the main message
//    loop and ending with initialization of the main thread; platform-specific
//    things which should be done immediately before the start of the main
//    message loop should go in |PreMainMessageLoopStart()|.
//  - RunMainMessageLoopParts:  things to be done before and after invoking the
//    main message loop run method (e.g. MessageLoopCurrentForUI::Get()->Run()).
//
// How to add stuff (to existing parts):
//  - Figure out when your new code should be executed. What must happen
//    before/after your code is executed? Are there performance reasons for
//    running your code at a particular time? Document these things!
//  - Split out any platform-specific bits. Please avoid #ifdefs it at all
//    possible. You have two choices for platform-specific code: (1) Execute it
//    from one of the platform-specific |Pre/Post...()| methods; do this if the
//    code is unique to a platform type. Or (2) execute it from one of the
//    "parts" (e.g., |EarlyInitialization()|) and provide platform-specific
//    implementations of your code (in a virtual method); do this if you need to
//    provide different implementations across most/all platforms.
//  - Unless your new code is just one or two lines, put it into a separate
//    method with a well-defined purpose. (Likewise, if you're adding to an
//    existing chunk which makes it longer than one or two lines, please move
//    the code out into a separate method.)
//
class SAMPLES_EXPORT MasterMainParts {
 public:
  MasterMainParts() {}
  virtual ~MasterMainParts() {}

  // A return value other than RESULT_CODE_NORMAL_EXIT indicates error and is
  // used as the exit status.
  virtual int PreEarlyInitialization();

  virtual void PostEarlyInitialization() {}

  virtual void PreMainMessageLoopStart() {}

  virtual void PostMainMessageLoopStart() {}

  // Allows an embedder to do any extra toolkit initialization.
  virtual void ToolkitInitialized() {}

  // Called just before any child threads owned by the samples
  // framework are created.
  //
  // The main message loop has been started at this point (but has not
  // been run), and the toolkit has been initialized. Returns the error code
  // (or 0 if no error).
  virtual int PreCreateThreads();

  // This is called right after all child threads owned by the samples framework
  // are created.
  virtual void PostCreateThreads() {}

  virtual void ServiceManagerConnectionStarted(
      ServiceManagerConnection* connection) {}

  // This is called just before the main message loop is run.  The
  // various master threads have all been created at this point
  virtual void PreMainMessageLoopRun() {}

  // Returns true if the message loop was run, false otherwise.
  // If this returns false, the default implementation will be run.
  // May set |result_code|, which will be returned by |MasterMain()|.
  virtual bool MainMessageLoopRun(int* result_code);

  // Provides an embedder with a Closure which will quit the default main
  // message loop. This is call only if MainMessageLoopRun returns false.
  virtual void PreDefaultMainMessageLoopRun(base::OnceClosure quit_closure) {}

  // This happens after the main message loop has stopped, but before
  // threads are stopped.
  virtual void PostMainMessageLoopRun() {}

  // Called when shutdown is about to begin.
  virtual void PreShutdown() {}

  // Called as the very last part of shutdown, after threads have been
  // stopped and destroyed.
  virtual void PostDestroyThreads() {}
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_MASTER_MAIN_PARTS_H_
