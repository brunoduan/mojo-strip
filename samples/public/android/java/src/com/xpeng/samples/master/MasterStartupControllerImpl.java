// Copyright 2013 The Chromium Authors. All rights reserved.                                                                                                                                                
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
package com.xpeng.samples.master;

import android.content.Context;
import android.os.Handler;
import android.os.StrictMode;
import android.support.annotation.IntDef;

import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.library_loader.LibraryLoader;
import org.chromium.base.library_loader.LibraryProcessType;
import org.chromium.base.library_loader.LoaderErrors;
import org.chromium.base.library_loader.ProcessInitException;
import com.xpeng.samples.app.SamplesMain;
import com.xpeng.samples_public.master.MasterStartupController;
import com.xpeng.samples_public.master.MasterStartupController.StartupCallback;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

/**
 * This is a singleton, and stores a reference to the application context.
 */
@JNINamespace("samples")
public class MasterStartupControllerImpl implements MasterStartupController {
    private static final String TAG = "cr.MasterStartup";

    // Helper constants for {@link #executeEnqueuedCallbacks(int, boolean)}.
    @VisibleForTesting
    static final int STARTUP_SUCCESS = -1;
    @VisibleForTesting
    static final int STARTUP_FAILURE = 1;

    @IntDef({MASTER_START_TYPE_FULL_MASTER, MASTER_START_TYPE_SERVICE_MANAGER_ONLY})
    @Retention(RetentionPolicy.SOURCE)
    public @interface MasterStartType {}
    private static final int MASTER_START_TYPE_FULL_MASTER = 0;
    private static final int MASTER_START_TYPE_SERVICE_MANAGER_ONLY = 1;

    private static MasterStartupControllerImpl sInstance;


    @VisibleForTesting
    @CalledByNative
    static void masterStartupComplete(int result) {
        if (sInstance != null) {
            sInstance.executeEnqueuedCallbacks(result);
        }
    }

    @CalledByNative
    static void serviceManagerStartupComplete() {
        if (sInstance != null) {
            sInstance.serviceManagerStarted();
        }
    }

    private final List<StartupCallback> mAsyncStartupCallbacks;

    // A list of callbacks that should be called when the ServiceManager is started. These callbacks
    // will be called once all the ongoing requests to start ServiceManager or full master process
    // are completed.
    private final List<StartupCallback> mServiceManagerCallbacks;

    // Whether the async startup of the master process has started.
    private boolean mHasStartedInitializingMasterProcess;

    private boolean mHasCalledSamplesStart;

    // Whether the async startup of the master process is complete.
    private boolean mFullMasterStartupDone;

    private boolean mStartupSuccess;

    private int mLibraryProcessType;

    // Master start up type. If the type is |MASTER_START_TYPE_SERVICE_MANAGER_ONLY|, start up
    // will be paused after ServiceManager is launched. Additional request to launch the full
    // master process is needed to fully complete the startup process. Callbacks will executed
    // once the master is fully started, or when the ServiceManager is started and there is no
    // outstanding requests to start the full master.
    @MasterStartType
    private int mCurrentMasterStartType = MASTER_START_TYPE_FULL_MASTER;

    // If the app is only started with the ServiceManager, whether it needs to launch full master
    // funcionalities now.
    private boolean mLaunchFullMasterAfterServiceManagerStart;

    // Whether ServiceManager is started.
    private boolean mServiceManagerStarted;

    MasterStartupControllerImpl() {
        mAsyncStartupCallbacks = new ArrayList<>();
        mServiceManagerCallbacks = new ArrayList<>();
        mLibraryProcessType = LibraryProcessType.PROCESS_BROWSER;
    }

    public static MasterStartupController get() {
        assert ThreadUtils.runningOnUiThread() : "Tried to start the master on the wrong thread.";
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new MasterStartupControllerImpl();
        }
        return sInstance;
    }

    @Override
    public void startMasterProcessesAsync(boolean startServiceManagerOnly,
            final StartupCallback callback) throws ProcessInitException {
        assert ThreadUtils.runningOnUiThread() : "Tried to start the master on the wrong thread.";
        if (mFullMasterStartupDone || (startServiceManagerOnly && mServiceManagerStarted)) {
            postStartupCompleted(callback);
            return;
        }

        if (startServiceManagerOnly) {
            mServiceManagerCallbacks.add(callback);
        } else {
            mAsyncStartupCallbacks.add(callback);
        }
        // If the master process is launched with ServiceManager only, we need to relaunch the full
        // process in serviceManagerStarted() if such a request was received.
        mLaunchFullMasterAfterServiceManagerStart |=
                (mCurrentMasterStartType == MASTER_START_TYPE_SERVICE_MANAGER_ONLY)
                && !startServiceManagerOnly;
        if (!mHasStartedInitializingMasterProcess) {
            // This is the first time we have been asked to start the master process. We set the
            // flag that indicates that we have kicked off starting the master process.
            mHasStartedInitializingMasterProcess = true;

            prepareToStartMasterProcess(false, new Runnable() {
                @Override
                public void run() {
                    ThreadUtils.assertOnUiThread();
                    if (mHasCalledSamplesStart) return;
                    mCurrentMasterStartType = startServiceManagerOnly
                            ? MASTER_START_TYPE_SERVICE_MANAGER_ONLY
                            : MASTER_START_TYPE_FULL_MASTER;
                    if (samplesStart() > 0) {
                        enqueueCallbackExecution(STARTUP_FAILURE);
                    }
                }
            });
        } else if (mServiceManagerStarted && mLaunchFullMasterAfterServiceManagerStart) {
            // If we missed the serviceManagerStarted() call, launch the full master now if needed.
            // Otherwise, serviceManagerStarted() will handle the full master launch.
            mCurrentMasterStartType = MASTER_START_TYPE_FULL_MASTER;
            if (samplesStart() > 0) enqueueCallbackExecution(STARTUP_FAILURE);
        }
    }

    @Override
    public void startMasterProcessesSync(boolean singleProcess) throws ProcessInitException {
        // If already started skip to checking the result
        if (!mFullMasterStartupDone) {
            if (!mHasStartedInitializingMasterProcess) {
                prepareToStartMasterProcess(singleProcess, null);
            }

            boolean startedSuccessfully = true;
            if (!mHasCalledSamplesStart) {
                mCurrentMasterStartType = MASTER_START_TYPE_FULL_MASTER;
                if (samplesStart() > 0) {
                    // Failed. The callbacks may not have run, so run them.
                    enqueueCallbackExecution(STARTUP_FAILURE);
                    startedSuccessfully = false;
                }
            } else if (mCurrentMasterStartType == MASTER_START_TYPE_SERVICE_MANAGER_ONLY) {
                mCurrentMasterStartType = MASTER_START_TYPE_FULL_MASTER;
                if (samplesStart() > 0) {
                    enqueueCallbackExecution(STARTUP_FAILURE);
                    startedSuccessfully = false;
                }
            }
            if (startedSuccessfully) {
                flushStartupTasks();
            }
        }

        // Startup should now be complete
        assert mFullMasterStartupDone;
        if (!mStartupSuccess) {
            throw new ProcessInitException(LoaderErrors.LOADER_ERROR_NATIVE_STARTUP_FAILED);
        }
    }

    /**
     * Start the master process by calling SamplesMain.start().
     */
    int samplesStart() {
        boolean startServiceManagerOnly =
                mCurrentMasterStartType == MASTER_START_TYPE_SERVICE_MANAGER_ONLY;
        int result = samplesMainStart(startServiceManagerOnly);
        mHasCalledSamplesStart = true;
        // No need to launch the full master again if we are launching full master now.
        if (!startServiceManagerOnly) mLaunchFullMasterAfterServiceManagerStart = false;
        return result;
    }

    @VisibleForTesting
    int samplesMainStart(boolean startServiceManagerOnly) {
        return SamplesMain.start(startServiceManagerOnly);
    }

    @VisibleForTesting
    void flushStartupTasks() {
        nativeFlushStartupTasks();
    }

    @Override
    public boolean isStartupSuccessfullyCompleted() {
        ThreadUtils.assertOnUiThread();
        return mFullMasterStartupDone && mStartupSuccess;
    }

    @Override
    public void addStartupCompletedObserver(StartupCallback callback) {
        ThreadUtils.assertOnUiThread();
        if (mFullMasterStartupDone) {
            postStartupCompleted(callback);
        } else {
            mAsyncStartupCallbacks.add(callback);
        }
    }

    /**
     * Called when ServiceManager is launched.
     */
    private void serviceManagerStarted() {
        mServiceManagerStarted = true;
        if (mLaunchFullMasterAfterServiceManagerStart) {
            // If startFullMaster() fails, execute the callbacks right away. Otherwise,
            // callbacks will be deferred until master startup completes.
            mCurrentMasterStartType = MASTER_START_TYPE_FULL_MASTER;
            if (samplesStart() > 0) enqueueCallbackExecution(STARTUP_FAILURE);
        } else if (mCurrentMasterStartType == MASTER_START_TYPE_SERVICE_MANAGER_ONLY) {
            // If full master startup is not needed, execute all the callbacks now.
            executeEnqueuedCallbacks(STARTUP_SUCCESS);
        }
    }

    private void executeEnqueuedCallbacks(int startupResult) {
        assert ThreadUtils.runningOnUiThread() : "Callback from master startup from wrong thread.";
        // If only ServiceManager is launched, don't set mFullMasterStartupDone, wait for the full
        // master launch to set this variable.
        mFullMasterStartupDone = mCurrentMasterStartType == MASTER_START_TYPE_FULL_MASTER;
        mStartupSuccess = (startupResult <= 0);
        if (mFullMasterStartupDone) {
            for (StartupCallback asyncStartupCallback : mAsyncStartupCallbacks) {
                if (mStartupSuccess) {
                    asyncStartupCallback.onSuccess();
                } else {
                    asyncStartupCallback.onFailure();
                }
            }
            // We don't want to hold on to any objects after we do not need them anymore.
            mAsyncStartupCallbacks.clear();
        }
        // The ServiceManager should have been started, call the callbacks now.
        // TODO(qinmin): Handle mServiceManagerCallbacks in serviceManagerStarted() instead of
        // here once http://crbug.com/854231 is fixed.
        for (StartupCallback serviceMangerCallback : mServiceManagerCallbacks) {
            if (mStartupSuccess) {
                serviceMangerCallback.onSuccess();
            } else {
                serviceMangerCallback.onFailure();
            }
        }
        mServiceManagerCallbacks.clear();
    }

    // Queue the callbacks to run. Since running the callbacks clears the list it is safe to call
    // this more than once.
    private void enqueueCallbackExecution(final int startupFailure) {
        new Handler().post(new Runnable() {
            @Override
            public void run() {
                executeEnqueuedCallbacks(startupFailure);
            }
        });
    }

    private void postStartupCompleted(final StartupCallback callback) {
        new Handler().post(new Runnable() {
            @Override
            public void run() {
                if (mStartupSuccess) {
                    callback.onSuccess();
                } else {
                    callback.onFailure();
                }
            }
        });
    }

    @VisibleForTesting
    void prepareToStartMasterProcess(final boolean singleProcess,
            final Runnable completionCallback) throws ProcessInitException {
        Log.i(TAG, "Initializing chromium process, singleProcess=%b", singleProcess);

        // This strictmode exception is to cover the case where the master process is being started
        // asynchronously but not in the main master flow.  The main master flow will trigger
        // library loading earlier and this will be a no-op, but in the other cases this will need
        // to block on loading libraries.
        // This applies to tests and ManageSpaceActivity, which can be launched from Settings.
        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            // Normally Main.java will have already loaded the library asynchronously, we only need
            // to load it here if we arrived via another flow, e.g. bookmark access & sync setup.
            LibraryLoader.getInstance().ensureInitialized(mLibraryProcessType);
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }
    }

    private static native void nativeSetCommandLineFlags(boolean singleProcess);

    private static native void nativeFlushStartupTasks();
}
