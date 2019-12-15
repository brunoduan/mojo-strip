// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines all the "samples" command-line switches.

#ifndef SAMPLES_PUBLIC_COMMON_SAMPLES_SWITCHES_H_
#define SAMPLES_PUBLIC_COMMON_SAMPLES_SWITCHES_H_

#include "build/build_config.h"
#include "samples/common/export.h"

namespace switches {

// All switches in alphabetical order. The switches should be documented
// alongside the definition of their values in the .cc file.
SAMPLES_EXPORT extern const char kAcceleratedCanvas2dMSAASampleCount[];
SAMPLES_EXPORT extern const char kAllowFileAccessFromFiles[];
SAMPLES_EXPORT extern const char kAllowInsecureLocalhost[];
SAMPLES_EXPORT extern const char kAllowLoopbackInPeerConnection[];
SAMPLES_EXPORT extern const char kAndroidFontsPath[];
SAMPLES_EXPORT extern const char kBlinkSettings[];
SAMPLES_EXPORT extern const char kBrowserCrashTest[];
SAMPLES_EXPORT extern const char kBrowserStartupDialog[];
SAMPLES_EXPORT extern const char kBrowserSubprocessPath[];
SAMPLES_EXPORT extern const char kBrowserTest[];
SAMPLES_EXPORT extern const char kDefaultTileWidth[];
SAMPLES_EXPORT extern const char kDefaultTileHeight[];
SAMPLES_EXPORT extern const char kDisable2dCanvasAntialiasing[];
SAMPLES_EXPORT extern const char kDisable2dCanvasImageChromium[];
SAMPLES_EXPORT extern const char kDisable3DAPIs[];
SAMPLES_EXPORT extern const char kDisableAccelerated2dCanvas[];
SAMPLES_EXPORT extern const char kDisableAcceleratedJpegDecoding[];
SAMPLES_EXPORT extern const char kDisableAcceleratedVideoDecode[];
SAMPLES_EXPORT extern const char kDisableAcceleratedVideoEncode[];
SAMPLES_EXPORT extern const char kDisableAudioSupportForDesktopShare[];
extern const char kDisableBackingStoreLimit[];
SAMPLES_EXPORT extern const char
    kDisableBackgroundingOccludedWindowsForTesting[];
SAMPLES_EXPORT extern const char kDisableBackgroundTimerThrottling[];
SAMPLES_EXPORT extern const char kDisableBlinkFeatures[];
SAMPLES_EXPORT extern const char kDisableCompositorUkmForTests[];
SAMPLES_EXPORT extern const char kDisableDatabases[];
SAMPLES_EXPORT extern const char kDisableDisplayList2dCanvas[];
extern const char kDisableDomainBlockingFor3DAPIs[];
SAMPLES_EXPORT extern const char kDisableWebGL[];
SAMPLES_EXPORT extern const char kDisableWebGL2[];
SAMPLES_EXPORT extern const char kDisableFileSystem[];
SAMPLES_EXPORT extern const char kDisableFlash3d[];
SAMPLES_EXPORT extern const char kDisableFlashStage3d[];
SAMPLES_EXPORT extern const char kDisableGestureRequirementForPresentation[];
SAMPLES_EXPORT extern const char kDisableGpu[];
SAMPLES_EXPORT extern const char kDisableGpuCompositing[];
SAMPLES_EXPORT extern const char kDisableGpuEarlyInit[];
SAMPLES_EXPORT extern const char kDisableGpuMemoryBufferCompositorResources[];
SAMPLES_EXPORT extern const char kDisableGpuMemoryBufferVideoFrames[];
extern const char kDisableGpuProcessCrashLimit[];
SAMPLES_EXPORT extern const char kDisableGpuWatchdog[];
SAMPLES_EXPORT extern const char kDisableImageAnimationResync[];
SAMPLES_EXPORT extern const char kDisableJavaScriptHarmonyShipping[];
SAMPLES_EXPORT extern const char kDisableLowLatencyDxva[];
SAMPLES_EXPORT extern const char kDisableLowResTiling[];
SAMPLES_EXPORT extern const char kDisableHangMonitor[];
extern const char kDisableHistogramCustomizer[];
SAMPLES_EXPORT extern const char kDisableLCDText[];
SAMPLES_EXPORT extern const char kDisablePreferCompositingToLCDText[];
SAMPLES_EXPORT extern const char kDisableKillAfterBadIPC[];
SAMPLES_EXPORT extern const char kDisableLocalStorage[];
SAMPLES_EXPORT extern const char kDisableLogging[];
SAMPLES_EXPORT extern const char kDisableNewContentRenderingTimeout[];
SAMPLES_EXPORT extern const char kDisableNotifications[];
SAMPLES_EXPORT extern const char kDisableOriginTrialControlledBlinkFeatures[];
SAMPLES_EXPORT extern const char kDisablePartialRaster[];
SAMPLES_EXPORT extern const char kEnablePartialRaster[];
extern const char kDisablePepper3d[];
SAMPLES_EXPORT extern const char kDisablePepper3DImageChromium[];
SAMPLES_EXPORT extern const char kDisablePermissionsAPI[];
SAMPLES_EXPORT extern const char kDisablePinch[];
SAMPLES_EXPORT extern const char kDisablePresentationAPI[];
SAMPLES_EXPORT extern const char kDisablePushStateThrottle[];
SAMPLES_EXPORT extern const char kDisableRGBA4444Textures[];
SAMPLES_EXPORT extern const char kDisableReadingFromCanvas[];
extern const char kDisableRemoteFonts[];
SAMPLES_EXPORT extern const char kDisableRemotePlaybackAPI[];
extern const char kDisableRendererAccessibility[];
SAMPLES_EXPORT extern const char kDisableRendererBackgrounding[];
SAMPLES_EXPORT extern const char kDisableResizeLock[];
SAMPLES_EXPORT extern const char kDisableResourceScheduler[];
SAMPLES_EXPORT extern const char kDisableSharedWorkers[];
SAMPLES_EXPORT extern const char kDisableSkiaRuntimeOpts[];
SAMPLES_EXPORT extern const char kDisableSmoothScrolling[];
SAMPLES_EXPORT extern const char kDisableSoftwareRasterizer[];
SAMPLES_EXPORT extern const char kDisableSpeechAPI[];
SAMPLES_EXPORT extern const char kDisableThreadedCompositing[];
SAMPLES_EXPORT extern const char kDisableThreadedScrolling[];
extern const char kDisableV8IdleTasks[];
SAMPLES_EXPORT extern const char kDisableWebGLImageChromium[];
SAMPLES_EXPORT extern const char kDisableWebSecurity[];
extern const char kDisableXSSAuditor[];
SAMPLES_EXPORT extern const char kDisableZeroCopy[];
SAMPLES_EXPORT extern const char kDisableZeroCopyDxgiVideo[];
SAMPLES_EXPORT extern const char kDomAutomationController[];
extern const char kDisable2dCanvasClipAntialiasing[];
SAMPLES_EXPORT extern const char kDumpBlinkRuntimeCallStats[];
SAMPLES_EXPORT extern const char kEnableAccessibilityObjectModel[];
SAMPLES_EXPORT extern const char kEnableAggressiveDOMStorageFlushing[];
SAMPLES_EXPORT extern const char kEnableAutomation[];
SAMPLES_EXPORT extern const char kEnablePreferCompositingToLCDText[];
SAMPLES_EXPORT extern const char kEnableBlinkFeatures[];
SAMPLES_EXPORT extern const char kEnableBlinkGenPropertyTrees[];
SAMPLES_EXPORT extern const char kEnableDisplayList2dCanvas[];
SAMPLES_EXPORT extern const char kEnableExperimentalWebPlatformFeatures[];
SAMPLES_EXPORT extern const char kEnableGpuMemoryBufferCompositorResources[];
SAMPLES_EXPORT extern const char kEnableGpuMemoryBufferVideoFrames[];
SAMPLES_EXPORT extern const char kGpuRasterizationMSAASampleCount[];
SAMPLES_EXPORT extern const char kEnableLowResTiling[];
SAMPLES_EXPORT extern const char kEnableLCDText[];
SAMPLES_EXPORT extern const char kEnableLogging[];
SAMPLES_EXPORT extern const char kEnableNetworkInformationDownlinkMax[];
SAMPLES_EXPORT extern const char kDisableNv12DxgiVideo[];
SAMPLES_EXPORT extern const char kEnablePinch[];
SAMPLES_EXPORT extern const char kEnablePluginPlaceholderTesting[];
SAMPLES_EXPORT extern const char kEnablePreciseMemoryInfo[];
SAMPLES_EXPORT extern const char kEnablePrintBrowser[];
SAMPLES_EXPORT extern const char kEnableRGBA4444Textures[];
SAMPLES_EXPORT extern const char kEnableServiceBinaryLauncher[];
extern const char kEnableSkiaBenchmarking[];
SAMPLES_EXPORT extern const char kEnableSlimmingPaintV2[];
SAMPLES_EXPORT extern const char kEnableSmoothScrolling[];
SAMPLES_EXPORT extern const char kEnableSpatialNavigation[];
SAMPLES_EXPORT extern const char kEnableStrictMixedContentChecking[];
SAMPLES_EXPORT extern const char kEnableStrictPowerfulFeatureRestrictions[];
SAMPLES_EXPORT extern const char kEnableThreadedCompositing[];
SAMPLES_EXPORT extern const char kEnableTracing[];
SAMPLES_EXPORT extern const char kEnableTracingOutput[];
SAMPLES_EXPORT extern const char kEnableUserMediaScreenCapturing[];
SAMPLES_EXPORT extern const char kEnableUseZoomForDSF[];
SAMPLES_EXPORT extern const char kEnableViewport[];
SAMPLES_EXPORT extern const char kEnableVtune[];
SAMPLES_EXPORT extern const char kEnableVulkan[];
SAMPLES_EXPORT extern const char kEnableWebAuthTestingAPI[];
SAMPLES_EXPORT extern const char kEnableWebGL2ComputeContext[];
SAMPLES_EXPORT extern const char kEnableWebGLDraftExtensions[];
SAMPLES_EXPORT extern const char kEnableWebGLImageChromium[];
SAMPLES_EXPORT extern const char kEnableWebVR[];
SAMPLES_EXPORT extern const char kEnableZeroCopy[];
SAMPLES_EXPORT extern const char kExplicitlyAllowedPorts[];
SAMPLES_EXPORT extern const char kFieldTrialHandle[];
SAMPLES_EXPORT extern const char kFileUrlPathAlias[];
SAMPLES_EXPORT extern const char kForceDisplayList2dCanvas[];
SAMPLES_EXPORT extern const char kForceGpuRasterization[];
SAMPLES_EXPORT extern const char kDisableOopRasterization[];
SAMPLES_EXPORT extern const char kEnableOopRasterization[];
SAMPLES_EXPORT extern const char kEnableOopRasterizationDDL[];
SAMPLES_EXPORT extern const char kForceOverlayFullscreenVideo[];
SAMPLES_EXPORT extern const char kForcePresentationReceiverForTesting[];
SAMPLES_EXPORT extern const char kForceRendererAccessibility[];
SAMPLES_EXPORT extern const char kGenerateAccessibilityTestExpectations[];
extern const char kGpuLauncher[];
SAMPLES_EXPORT extern const char kGpuProcess[];
SAMPLES_EXPORT extern const char kGpuSandboxStartEarly[];
SAMPLES_EXPORT extern const char kGpuStartupDialog[];
SAMPLES_EXPORT extern const char kSamplingHeapProfiler[];
SAMPLES_EXPORT extern const char kHistoryEntryRequiresUserGesture[];
SAMPLES_EXPORT extern const char kInitialVirtualTime[];
SAMPLES_EXPORT extern const char kInProcessGPU[];
SAMPLES_EXPORT extern const char kIPCConnectionTimeout[];
SAMPLES_EXPORT extern const char kIsolateOrigins[];
SAMPLES_EXPORT extern const char kJavaScriptFlags[];
SAMPLES_EXPORT extern const char kJavaScriptHarmony[];
SAMPLES_EXPORT extern const char kLogGpuControlListDecisions[];
SAMPLES_EXPORT extern const char kLoggingLevel[];
SAMPLES_EXPORT extern const char kLogFile[];
SAMPLES_EXPORT extern const char kMainFrameResizesAreOrientationChanges[];
extern const char kMaxDecodedImageSizeMb[];
extern const char kMaxUntiledLayerHeight[];
extern const char kMaxUntiledLayerWidth[];
SAMPLES_EXPORT extern const char kMessageLoopTypeUi[];
SAMPLES_EXPORT extern const char kMHTMLGeneratorOption[];
SAMPLES_EXPORT extern const char kMHTMLSkipNostoreMain[];
SAMPLES_EXPORT extern const char kMHTMLSkipNostoreAll[];
SAMPLES_EXPORT extern const char kMojoLocalStorage[];
SAMPLES_EXPORT extern const char kNetworkQuietTimeout[];
SAMPLES_EXPORT extern const char kNoZygote[];
extern const char kNoV8UntrustedCodeMitigations[];
SAMPLES_EXPORT extern const char kEnableAppContainer[];
SAMPLES_EXPORT extern const char kDisableAppContainer[];
SAMPLES_EXPORT extern const char kNumRasterThreads[];
SAMPLES_EXPORT extern const char kOverridePluginPowerSaverForTesting[];
SAMPLES_EXPORT extern const char kOverscrollHistoryNavigation[];
SAMPLES_EXPORT extern const char kOverscrollStartThreshold[];
SAMPLES_EXPORT extern const char kPassiveListenersDefault[];
SAMPLES_EXPORT extern const char kPpapiBrokerProcess[];
SAMPLES_EXPORT extern const char kPpapiFlashArgs[];
SAMPLES_EXPORT extern const char kPpapiInProcess[];
extern const char kPpapiPluginLauncher[];
SAMPLES_EXPORT extern const char kPpapiPluginProcess[];
extern const char kPpapiStartupDialog[];
SAMPLES_EXPORT extern const char kProcessPerSite[];
SAMPLES_EXPORT extern const char kProcessPerTab[];
SAMPLES_EXPORT extern const char kProcessType[];
SAMPLES_EXPORT extern const char kProxyServer[];
SAMPLES_EXPORT extern const char kPullToRefresh[];
SAMPLES_EXPORT extern const char kReducedReferrerGranularity[];
SAMPLES_EXPORT extern const char kRegisterPepperPlugins[];
SAMPLES_EXPORT extern const char kRemoteDebuggingPipe[];
SAMPLES_EXPORT extern const char kRemoteDebuggingPort[];
SAMPLES_EXPORT extern const char kRendererClientId[];
extern const char kRendererCmdPrefix[];
SAMPLES_EXPORT extern const char kRendererProcess[];
SAMPLES_EXPORT extern const char kRendererProcessLimit[];
SAMPLES_EXPORT extern const char kRendererStartupDialog[];
extern const char kSandboxIPCProcess[];
SAMPLES_EXPORT extern const char kSavePreviousDocumentResources[];
extern const char kShowPaintRects[];
SAMPLES_EXPORT extern const char kSingleProcess[];
SAMPLES_EXPORT extern const char kSitePerProcess[];
SAMPLES_EXPORT extern const char kDisableSiteIsolationTrials[];
SAMPLES_EXPORT extern const char kStartFullscreen[];
SAMPLES_EXPORT extern const char kStatsCollectionController[];
extern const char kSkiaFontCacheLimitMb[];
extern const char kSkiaResourceCacheLimitMb[];
SAMPLES_EXPORT extern const char kTestType[];
SAMPLES_EXPORT extern const char kTouchEventFeatureDetection[];
SAMPLES_EXPORT extern const char kTouchEventFeatureDetectionAuto[];
SAMPLES_EXPORT extern const char kTouchEventFeatureDetectionEnabled[];
SAMPLES_EXPORT extern const char kTouchEventFeatureDetectionDisabled[];
SAMPLES_EXPORT extern const char kTouchTextSelectionStrategy[];
SAMPLES_EXPORT extern const char kUseFakeUIForMediaStream[];
SAMPLES_EXPORT extern const char kVideoImageTextureTarget[];
SAMPLES_EXPORT extern const char kUseMobileUserAgent[];
SAMPLES_EXPORT extern const char kUseMockCertVerifierForTesting[];
extern const char kUtilityCmdPrefix[];
SAMPLES_EXPORT extern const char kUtilityProcess[];
SAMPLES_EXPORT extern const char kUtilityStartupDialog[];
SAMPLES_EXPORT extern const char kV8CacheOptions[];
SAMPLES_EXPORT extern const char kValidateInputEventStream[];
SAMPLES_EXPORT extern const char kWaitForDebuggerChildren[];

SAMPLES_EXPORT extern const char kDisableWebRtcEncryption[];
SAMPLES_EXPORT extern const char kDisableWebRtcHWDecoding[];
SAMPLES_EXPORT extern const char kDisableWebRtcHWEncoding[];
SAMPLES_EXPORT extern const char kEnableWebRtcSrtpAesGcm[];
SAMPLES_EXPORT extern const char kEnableWebRtcSrtpEncryptedHeaders[];
SAMPLES_EXPORT extern const char kEnableWebRtcStunOrigin[];
SAMPLES_EXPORT extern const char kEnforceWebRtcIPPermissionCheck[];
SAMPLES_EXPORT extern const char kForceWebRtcIPHandlingPolicy[];
extern const char kWebRtcMaxCaptureFramerate[];
extern const char kWebRtcMaxCpuConsumptionPercentage[];
SAMPLES_EXPORT extern const char kWebRtcStunProbeTrialParameter[];
SAMPLES_EXPORT extern const char kWebRtcLocalEventLogging[];

#if defined(OS_ANDROID)
SAMPLES_EXPORT extern const char kDisableMediaSessionAPI[];
SAMPLES_EXPORT extern const char kDisableOverscrollEdgeEffect[];
SAMPLES_EXPORT extern const char kDisablePullToRefreshEffect[];
SAMPLES_EXPORT extern const char kDisableScreenOrientationLock[];
SAMPLES_EXPORT extern const char kDisableTimeoutsForProfiling[];
SAMPLES_EXPORT extern const char kEnableAdaptiveSelectionHandleOrientation[];
SAMPLES_EXPORT extern const char kEnableLongpressDragSelection[];
extern const char kNetworkCountryIso[];
SAMPLES_EXPORT extern const char kRemoteDebuggingSocketName[];
SAMPLES_EXPORT extern const char kRendererWaitForJavaDebugger[];
SAMPLES_EXPORT extern const char kEnableOSKOverscroll[];
#endif

#if defined(OS_CHROMEOS)
SAMPLES_EXPORT extern const char kDisablePanelFitting[];
#endif

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
SAMPLES_EXPORT extern const char kEnableSpeechDispatcher[];
#endif

#if defined(OS_WIN)
SAMPLES_EXPORT extern const char kPrefetchArgumentRenderer[];
SAMPLES_EXPORT extern const char kPrefetchArgumentGpu[];
SAMPLES_EXPORT extern const char kPrefetchArgumentPpapi[];
SAMPLES_EXPORT extern const char kPrefetchArgumentPpapiBroker[];
SAMPLES_EXPORT extern const char kPrefetchArgumentOther[];
// This switch contains the device scale factor passed to certain processes
// like renderers, etc.
SAMPLES_EXPORT extern const char kDeviceScaleFactor[];
SAMPLES_EXPORT extern const char kDisableLegacyIntermediateWindow[];
SAMPLES_EXPORT extern const char kEnableAcceleratedVpxDecode[];
SAMPLES_EXPORT extern const char kEnableWin7WebRtcHWH264Decoding[];
// Switch to pass the font cache shared memory handle to the renderer.
SAMPLES_EXPORT extern const char kFontCacheSharedHandle[];
SAMPLES_EXPORT extern const char kMemoryPressureThresholdsMb[];
SAMPLES_EXPORT extern const char kPpapiAntialiasedTextEnabled[];
SAMPLES_EXPORT extern const char kPpapiSubpixelRenderingSetting[];
SAMPLES_EXPORT extern const char kTraceExportEventsToETW[];
#endif

#if defined(ENABLE_IPC_FUZZER)
extern const char kIpcDumpDirectory[];
extern const char kIpcFuzzerTestcase[];
#endif

// DON'T ADD RANDOM STUFF HERE. Put it in the main section above in
// alphabetical order, or in one of the ifdefs (also in order in each section).

}  // namespace switches

#endif  // SAMPLES_PUBLIC_COMMON_SAMPLES_SWITCHES_H_
