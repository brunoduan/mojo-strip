// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/isolated_origin_util.h"

#include "base/strings/string_util.h"
#include "url/gurl.h"

namespace samples {

// static
bool IsolatedOriginUtil::DoesOriginMatchIsolatedOrigin(
    const url::Origin& origin,
    const url::Origin& isolated_origin) {
  // Don't match subdomains if the isolated origin is an IP address.
  if (isolated_origin.GetURL().HostIsIPAddress())
    return origin == isolated_origin;

  if (origin.scheme() != isolated_origin.scheme())
    return false;

  if (origin.port() != isolated_origin.port())
    return false;

  // Subdomains of an isolated origin are considered to be in the same isolated
  // origin.
  return origin.DomainIs(isolated_origin.host());
}

// static
bool IsolatedOriginUtil::IsValidIsolatedOrigin(const url::Origin& origin) {
  return true;
}

}  // namespace samples
