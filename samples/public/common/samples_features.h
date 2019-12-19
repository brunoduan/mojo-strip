// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines all the public base::FeatureList features for the samples
// module.

#ifndef SAMPLES_PUBLIC_COMMON_SAMPLES_FEATURES_H_
#define SAMPLES_PUBLIC_COMMON_SAMPLES_FEATURES_H_

#include "base/feature_list.h"
#include "build/build_config.h"
#include "samples/common/export.h"

namespace features {

// All features in alphabetical order. The features should be documented
// alongside the definition of their values in the .cc file.
SAMPLES_EXPORT extern const base::Feature kAllowActivationDelegationAttr;
SAMPLES_EXPORT extern const base::Feature
    kAllowContentInitiatedDataUrlNavigations;
SAMPLES_EXPORT extern const base::Feature
    kAllowSignedHTTPExchangeCertsWithoutExtension;
SAMPLES_EXPORT extern const base::Feature kAudioServiceAudioStreams;
SAMPLES_EXPORT extern const base::Feature kAudioServiceLaunchOnStartup;
SAMPLES_EXPORT extern const base::Feature kAudioServiceOutOfProcess;
SAMPLES_EXPORT extern const base::Feature kBackgroundFetch;
SAMPLES_EXPORT extern const base::Feature kBlinkHeapIncrementalMarking;
SAMPLES_EXPORT extern const base::Feature kBloatedRendererDetection;
SAMPLES_EXPORT extern const base::Feature kBlockCredentialedSubresources;
SAMPLES_EXPORT extern const base::Feature kBrotliEncoding;
SAMPLES_EXPORT extern const base::Feature kCacheInlineScriptCode;
SAMPLES_EXPORT extern const base::Feature kCanvas2DImageChromium;
SAMPLES_EXPORT extern const base::Feature kCompositeOpaqueFixedPosition;
SAMPLES_EXPORT extern const base::Feature kCompositeOpaqueScrollers;
SAMPLES_EXPORT extern const base::Feature kCompositorTouchAction;
SAMPLES_EXPORT extern const base::Feature kCSSFragmentIdentifiers;
SAMPLES_EXPORT extern const base::Feature kDataSaverHoldback;
SAMPLES_EXPORT extern const base::Feature kExperimentalProductivityFeatures;
SAMPLES_EXPORT extern const base::Feature kExpensiveBackgroundTimerThrottling;
SAMPLES_EXPORT extern const base::Feature kExtendedMouseButtons;
SAMPLES_EXPORT extern const base::Feature kFontCacheScaling;
SAMPLES_EXPORT extern const base::Feature kFontSrcLocalMatching;
SAMPLES_EXPORT extern const base::Feature
    kFramebustingNeedsSameOriginOrUserGesture;
SAMPLES_EXPORT extern const base::Feature kGamepadExtensions;
SAMPLES_EXPORT extern const base::Feature kGamepadVibration;
SAMPLES_EXPORT extern const base::Feature kGuestViewCrossProcessFrames;
SAMPLES_EXPORT extern const base::Feature kHeapCompaction;
SAMPLES_EXPORT extern const base::Feature kImageCaptureAPI;
SAMPLES_EXPORT extern const base::Feature kIsolateOrigins;
SAMPLES_EXPORT extern const char kIsolateOriginsFieldTrialParamName[];
SAMPLES_EXPORT extern const base::Feature kKeyboardLockAPI;
SAMPLES_EXPORT extern const base::Feature kLayeredAPI;
SAMPLES_EXPORT extern const base::Feature kLazyFrameLoading;
SAMPLES_EXPORT extern const base::Feature kLazyFrameVisibleLoadTimeMetrics;
SAMPLES_EXPORT extern const base::Feature kLazyImageLoading;
SAMPLES_EXPORT extern const base::Feature kLazyImageVisibleLoadTimeMetrics;
SAMPLES_EXPORT extern const base::Feature kLazyInitializeMediaControls;
SAMPLES_EXPORT extern const base::Feature kLowPriorityIframes;
SAMPLES_EXPORT extern const base::Feature kMediaDevicesSystemMonitorCache;
SAMPLES_EXPORT extern const base::Feature kMemoryCoordinator;
SAMPLES_EXPORT extern const base::Feature kMimeHandlerViewInCrossProcessFrame;
SAMPLES_EXPORT extern const base::Feature kMojoVideoCapture;
SAMPLES_EXPORT extern const base::Feature kMojoVideoCaptureSecondary;
SAMPLES_EXPORT extern const base::Feature kNetworkQualityEstimatorWebHoldback;
SAMPLES_EXPORT extern const base::Feature kNetworkServiceInProcess;
SAMPLES_EXPORT extern const base::Feature kNotificationContentImage;
SAMPLES_EXPORT extern const base::Feature kOriginPolicy;
SAMPLES_EXPORT extern const base::Feature kOriginTrials;
SAMPLES_EXPORT extern const base::Feature kPageLifecycle;
SAMPLES_EXPORT extern const base::Feature kPassiveDocumentEventListeners;
SAMPLES_EXPORT extern const base::Feature kPassiveDocumentWheelEventListeners;
SAMPLES_EXPORT extern const base::Feature kPassiveEventListenersDueToFling;
SAMPLES_EXPORT extern const base::Feature kPdfIsolation;
SAMPLES_EXPORT extern const base::Feature kPerNavigationMojoInterface;
SAMPLES_EXPORT extern const base::Feature kPepper3DImageChromium;
SAMPLES_EXPORT extern const base::Feature kPurgeAndSuspend;
SAMPLES_EXPORT extern const base::Feature kPWAFullCodeCache;
SAMPLES_EXPORT extern const base::Feature kRasterInducingScroll;
SAMPLES_EXPORT extern const base::Feature kRenderingPipelineThrottling;
SAMPLES_EXPORT extern const base::Feature kRequireCSSExtensionForFile;
SAMPLES_EXPORT extern const base::Feature kResamplingInputEvents;
SAMPLES_EXPORT extern const base::Feature kResourceLoadScheduler;
SAMPLES_EXPORT extern const base::Feature
    kRunVideoCaptureServiceInBrowserProcess;
SAMPLES_EXPORT extern const base::Feature kScrollAnchorSerialization;
SAMPLES_EXPORT extern const base::Feature
    kSendBeaconThrowForBlobWithNonSimpleType;
SAMPLES_EXPORT extern const base::Feature kSecMetadata;
SAMPLES_EXPORT extern const base::Feature kServiceWorkerLongRunningMessage;
SAMPLES_EXPORT extern const base::Feature kServiceWorkerPaymentApps;
SAMPLES_EXPORT extern const base::Feature kServiceWorkerScriptFullCodeCache;
SAMPLES_EXPORT extern const base::Feature kSharedArrayBuffer;
SAMPLES_EXPORT extern const base::Feature kSignedHTTPExchange;
SAMPLES_EXPORT extern const base::Feature kSignedHTTPExchangeAcceptHeader;
SAMPLES_EXPORT extern const char
    kSignedHTTPExchangeAcceptHeaderFieldTrialParamName[];
SAMPLES_EXPORT extern const base::Feature kSignedHTTPExchangeOriginTrial;
SAMPLES_EXPORT extern const base::Feature kSpareSlavererForSitePerProcess;
SAMPLES_EXPORT extern const base::Feature kTimerThrottlingForHiddenFrames;
SAMPLES_EXPORT extern const base::Feature kTouchpadAsyncPinchEvents;
SAMPLES_EXPORT extern const base::Feature kTouchpadOverscrollHistoryNavigation;
SAMPLES_EXPORT extern const base::Feature kUserActivationV2;
SAMPLES_EXPORT extern const base::Feature kV8ContextSnapshot;
SAMPLES_EXPORT extern const base::Feature kV8LowMemoryModeForSubframes;
SAMPLES_EXPORT extern const base::Feature kV8Orinoco;
SAMPLES_EXPORT extern const base::Feature kV8VmFuture;
SAMPLES_EXPORT extern const base::Feature kWebAssembly;
SAMPLES_EXPORT extern const base::Feature kWebAssemblyBaseline;
SAMPLES_EXPORT extern const base::Feature kWebAssemblyThreads;
SAMPLES_EXPORT extern const base::Feature kWebAssemblyTrapHandler;
SAMPLES_EXPORT extern const base::Feature kWebAuth;
SAMPLES_EXPORT extern const base::Feature kWebAuthBle;
SAMPLES_EXPORT extern const base::Feature kWebAuthCable;
SAMPLES_EXPORT extern const base::Feature kWebAuthCableWin;
SAMPLES_EXPORT extern const base::Feature kWebAuthGetTransports;
SAMPLES_EXPORT extern const base::Feature kWebContentsOcclusion;
SAMPLES_EXPORT extern const base::Feature kWebGLImageChromium;
SAMPLES_EXPORT extern const base::Feature kWebPayments;
SAMPLES_EXPORT extern const base::Feature kWebRtcEcdsaDefault;
SAMPLES_EXPORT extern const base::Feature kWebRtcHWH264Encoding;
SAMPLES_EXPORT extern const base::Feature kWebRtcHWVP8Encoding;
SAMPLES_EXPORT extern const base::Feature kWebRtcMultiplexCodec;
SAMPLES_EXPORT extern const base::Feature kWebRtcScreenshareSwEncoding;
SAMPLES_EXPORT extern const base::Feature kWebRtcUseEchoCanceller3;
SAMPLES_EXPORT extern const base::Feature kWebRtcHybridAgc;
SAMPLES_EXPORT extern const base::Feature kWebRtcUseGpuMemoryBufferVideoFrames;
SAMPLES_EXPORT extern const base::Feature kWebRtcHideLocalIpsWithMdns;
SAMPLES_EXPORT extern const base::Feature kWebUsb;
SAMPLES_EXPORT extern const base::Feature kWebVrVsyncAlign;
SAMPLES_EXPORT extern const base::Feature kWebXr;
SAMPLES_EXPORT extern const base::Feature kWebXrGamepadSupport;
SAMPLES_EXPORT extern const base::Feature kWebXrHitTest;
SAMPLES_EXPORT extern const base::Feature kWebXrOrientationSensorDevice;
SAMPLES_EXPORT extern const base::Feature kWipeCorruptV2IDBDatabases;
SAMPLES_EXPORT extern const base::Feature kWorkStealingInScriptRunner;
SAMPLES_EXPORT extern const base::Feature kScheduledScriptStreaming;

#if defined(OS_ANDROID)
SAMPLES_EXPORT extern const base::Feature kAndroidAutofillAccessibility;
SAMPLES_EXPORT extern const base::Feature
    kBackgroundMediaRendererHasModerateBinding;
SAMPLES_EXPORT extern const base::Feature kDisplayCutoutAPI;
SAMPLES_EXPORT extern const base::Feature kHideIncorrectlySizedFullscreenFrames;
SAMPLES_EXPORT extern const base::Feature kWebNfc;
SAMPLES_EXPORT extern const base::Feature kWebXrRenderPath;
SAMPLES_EXPORT extern const char kWebXrRenderPathParamName[];
SAMPLES_EXPORT extern const char kWebXrRenderPathParamValueClientWait[];
SAMPLES_EXPORT extern const char kWebXrRenderPathParamValueGpuFence[];
SAMPLES_EXPORT extern const char kWebXrRenderPathParamValueSharedBuffer[];
#endif  // defined(OS_ANDROID)

#if !defined(OS_ANDROID)
SAMPLES_EXPORT extern const base::Feature kWebUIPolymer2;
#endif  // !defined(OS_ANDROID)

#if defined(OS_MACOSX)
SAMPLES_EXPORT extern const base::Feature kDeviceMonitorMac;
SAMPLES_EXPORT extern const base::Feature kIOSurfaceCapturer;
SAMPLES_EXPORT extern const base::Feature kMacV2Sandbox;
#endif  // defined(OS_MACOSX)

// DON'T ADD RANDOM STUFF HERE. Put it in the main section above in
// alphabetical order, or in one of the ifdefs (also in order in each section).

SAMPLES_EXPORT bool IsVideoCaptureServiceEnabledForOutOfProcess();
SAMPLES_EXPORT bool IsVideoCaptureServiceEnabledForBrowserProcess();

}  // namespace features

#endif  // SAMPLES_PUBLIC_COMMON_SAMPLES_FEATURES_H_
