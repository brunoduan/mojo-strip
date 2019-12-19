// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_COMMON_URL_CONSTANTS_H_
#define SAMPLES_PUBLIC_COMMON_URL_CONSTANTS_H_

#include "base/logging.h"
#include "build/build_config.h"
#include "samples/common/export.h"
#include "url/url_constants.h"

// Contains constants for known URLs and portions thereof.

namespace samples {

// Canonical schemes you can use as input to GURL.SchemeIs().
// TODO(jam): some of these don't below in the samples layer, but are accessed
// from there.
SAMPLES_EXPORT extern const char kChromeDevToolsScheme[];
SAMPLES_EXPORT extern const char kChromeErrorScheme[];
SAMPLES_EXPORT extern const char kChromeUIScheme[];  // Used for WebUIs.
SAMPLES_EXPORT extern const char kGuestScheme[];
SAMPLES_EXPORT extern const char kViewSourceScheme[];
#if defined(OS_CHROMEOS)
SAMPLES_EXPORT extern const char kExternalFileScheme[];
#endif

// Hosts for about URLs.
SAMPLES_EXPORT extern const char kAboutSrcDocURL[];

SAMPLES_EXPORT extern const char kChromeUIAccessibilityHost[];
SAMPLES_EXPORT extern const char kChromeUIAppCacheInternalsHost[];
SAMPLES_EXPORT extern const char kChromeUIBlobInternalsHost[];
SAMPLES_EXPORT extern const char kChromeUIBrowserCrashHost[];
SAMPLES_EXPORT extern const char kChromeUIDinoHost[];
SAMPLES_EXPORT extern const char kChromeUIGpuHost[];
SAMPLES_EXPORT extern const char kChromeUIHistogramHost[];
SAMPLES_EXPORT extern const char kChromeUIIndexedDBInternalsHost[];
SAMPLES_EXPORT extern const char kChromeUIMediaInternalsHost[];
SAMPLES_EXPORT extern const char kChromeUIMemoryExhaustHost[];
SAMPLES_EXPORT extern const char kChromeUINetworkErrorHost[];
SAMPLES_EXPORT extern const char kChromeUINetworkErrorsListingHost[];
SAMPLES_EXPORT extern const char kChromeUIProcessInternalsHost[];
SAMPLES_EXPORT extern const char kChromeUIResourcesHost[];
SAMPLES_EXPORT extern const char kChromeUIServiceWorkerInternalsHost[];
SAMPLES_EXPORT extern const char kChromeUITracingHost[];
SAMPLES_EXPORT extern const char kChromeUIWebRTCInternalsHost[];

// Full about URLs (including schemes).
SAMPLES_EXPORT extern const char kChromeUIBadCastCrashURL[];
SAMPLES_EXPORT extern const char kChromeUICheckCrashURL[];
SAMPLES_EXPORT extern const char kChromeUIBrowserCrashURL[];
SAMPLES_EXPORT extern const char kChromeUIBrowserUIHang[];
SAMPLES_EXPORT extern const char kChromeUICrashURL[];
SAMPLES_EXPORT extern const char kChromeUIDelayedBrowserUIHang[];
SAMPLES_EXPORT extern const char kChromeUIDumpURL[];
SAMPLES_EXPORT extern const char kChromeUIGpuCleanURL[];
SAMPLES_EXPORT extern const char kChromeUIGpuCrashURL[];
SAMPLES_EXPORT extern const char kChromeUIGpuHangURL[];
SAMPLES_EXPORT extern const char kChromeUIHangURL[];
SAMPLES_EXPORT extern const char kChromeUIKillURL[];
SAMPLES_EXPORT extern const char kChromeUIMemoryExhaustURL[];
SAMPLES_EXPORT extern const char kChromeUINetworkErrorsListingURL[];
SAMPLES_EXPORT extern const char kChromeUINetworkErrorURL[];
SAMPLES_EXPORT extern const char kChromeUIPpapiFlashCrashURL[];
SAMPLES_EXPORT extern const char kChromeUIPpapiFlashHangURL[];
SAMPLES_EXPORT extern const char kChromeUIProcessInternalsURL[];
#if defined(OS_ANDROID)
SAMPLES_EXPORT extern const char kChromeUIGpuJavaCrashURL[];
#endif
#if defined(OS_WIN)
SAMPLES_EXPORT extern const char kChromeUIBrowserHeapCorruptionURL[];
SAMPLES_EXPORT extern const char kChromeUIHeapCorruptionCrashURL[];
#endif
#if defined(ADDRESS_SANITIZER)
SAMPLES_EXPORT extern const char kChromeUICrashHeapOverflowURL[];
SAMPLES_EXPORT extern const char kChromeUICrashHeapUnderflowURL[];
SAMPLES_EXPORT extern const char kChromeUICrashUseAfterFreeURL[];
#if defined(OS_WIN)
SAMPLES_EXPORT extern const char kChromeUICrashCorruptHeapBlockURL[];
SAMPLES_EXPORT extern const char kChromeUICrashCorruptHeapURL[];
#endif  // OS_WIN
#endif  // ADDRESS_SANITIZER

#if DCHECK_IS_ON()
SAMPLES_EXPORT extern const char kChromeUICrashDcheckURL[];
#endif

// Special URL used to start a navigation to an error page.
SAMPLES_EXPORT extern const char kUnreachableWebDataURL[];

// Full about URLs (including schemes).
SAMPLES_EXPORT extern const char kChromeUINetworkViewCacheURL[];
SAMPLES_EXPORT extern const char kChromeUIResourcesURL[];
SAMPLES_EXPORT extern const char kChromeUIShorthangURL[];

}  // namespace samples

#endif  // SAMPLES_PUBLIC_COMMON_URL_CONSTANTS_H_
