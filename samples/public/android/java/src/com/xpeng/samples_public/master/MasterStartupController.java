// Copyright 2013 The Chromium Authors. All rights reserved.                                                                                                                                                
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.xpeng.samples_public.master;

import com.xpeng.samples.master.MasterStartupControllerImpl;

import org.chromium.base.library_loader.ProcessInitException;


/**
 * This class controls how C++ master main loop is started and ensures it happens only once.
 *
 * It supports kicking off the startup sequence in an asynchronous way. Startup can be called as
 * many times as needed (for instance, multiple activities for the same application), but the
 * master process will still only be initialized once.
 *
 * All communication with this class must happen on the main thread.
 */
public interface MasterStartupController {
  public interface StartupCallback {
      void onSuccess();
      void onFailure();
  }

  /**
   * Get MasterStartupController instance, create a new one if no existing.
   *
   * @return MasterStartupController instance.
   */
  public static MasterStartupController get(int libraryProcessType) {
      return MasterStartupControllerImpl.get(libraryProcessType);
  }

  /**
   * Start the master process asynchronously. This will set up a queue of UI thread tasks to
   * initialize the browser process.
   * <p/>
   * Note that this can only be called on the UI thread.
   *
   * @param startServiceManagerOnly Whether browser startup will be paused after ServiceManager
   *                                is started.
   * @param callback the callback to be called when browser startup is complete.
   */
  void startMasterProcessesAsync(boolean startServiceManagerOnly,
          final StartupCallback callback) throws ProcessInitException;

  /**
   * Start the master process synchronously. If the master is already being started
   * asynchronously then complete startup synchronously
   *
   * <p/>
   * Note that this can only be called on the UI thread.
   *
   * @param singleProcess true iff the master should run single-process.
   * @throws ProcessInitException
   */
  void startMasterProcessesSync(boolean singleProcess) throws ProcessInitException;

  boolean isStartupSuccessfullyCompleted();

  void addStartupCompletedObserver(StartupCallback callback);
}
