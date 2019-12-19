// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_COMMON_SAMPLES_CLIENT_H_
#define SAMPLES_PUBLIC_COMMON_SAMPLES_CLIENT_H_

#include <set>
#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "build/build_config.h"
#include "samples/common/export.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_util.h"

namespace base {
class RefCountedMemory;
class DictionaryValue;
}

namespace IPC {
class Message;
}

namespace samples {

class SamplesMasterClient;
class SamplesClient;
class SamplesSlavererClient;
class SamplesUtilityClient;
class ServiceManagerConnection;

// Setter and getter for the client.  The client should be set early, before any
// samples code is called.
SAMPLES_EXPORT void SetSamplesClient(SamplesClient* client);

#if defined(SAMPLES_IMPLEMENTATION)
// Samples's embedder API should only be used by samples.
SamplesClient* GetSamplesClient();
#endif

// Used for tests to override the relevant embedder interfaces. Each method
// returns the old value.
SAMPLES_EXPORT SamplesMasterClient* SetMasterClientForTesting(
    SamplesMasterClient* b);
SAMPLES_EXPORT SamplesSlavererClient* SetSlaveClientForTesting(
    SamplesSlavererClient* r);

// Interface that the embedder implements.
class SAMPLES_EXPORT SamplesClient {
 public:
  SamplesClient();
  virtual ~SamplesClient();

  SamplesMasterClient* master() { return master_; }
  SamplesSlavererClient* slaverer() { return slaverer_; }
  SamplesUtilityClient* utility() { return utility_; }


  // Sets the active URL (the URL of a frame that is navigating or processing an
  // IPC message), and the origin of the main frame (for diagnosing crashes).
  // Use GURL() or std::string() to clear the URL/origin.
  //
  // A string is used for the origin because the source of that value may be a
  // WebSecurityOrigin or a full URL (if called from the master process) and a
  // string is the lowest-common-denominator.
  virtual void SetActiveURL(const GURL& url, std::string top_origin) {}

  // Gives the embedder a chance to register its own schemes early in the
  // startup sequence.
  struct SAMPLES_EXPORT Schemes {
    Schemes();
    ~Schemes();
    std::vector<std::string> standard_schemes;
    std::vector<std::string> referrer_schemes;
    std::vector<std::string> savable_schemes;
    // Additional schemes that should be allowed to register service workers.
    // Only secure and trustworthy schemes should be added.
    std::vector<std::string> service_worker_schemes;
    // Registers a URL scheme to be treated as a local scheme (i.e., with the
    // same security rules as those applied to "file" URLs). This means that
    // normal pages cannot link to or access URLs of this scheme.
    std::vector<std::string> local_schemes;
    // Registers a URL scheme to be treated as a noAccess scheme. This means
    // that pages loaded with this URL scheme always have an opaque origin.
    std::vector<std::string> no_access_schemes;
    // Registers a non-HTTP URL scheme which can be sent CORS requests.
    std::vector<std::string> cors_enabled_schemes;
    // See https://www.w3.org/TR/powerful-features/#is-origin-trustworthy.
    std::vector<std::string> secure_schemes;
    // Registers a serialized origin or a hostname pattern that should be
    // considered trustworthy.
    std::vector<std::string> secure_origins;
    // Registers a URL scheme as strictly empty documents, allowing them to
    // commit synchronously.
    std::vector<std::string> empty_document_schemes;
  };

  virtual void AddAdditionalSchemes(Schemes* schemes) {}

  // Returns whether the given message should be sent in a swapped out slave.
  virtual bool CanSendWhileSwappedOut(const IPC::Message* message);

  // Returns a string describing the embedder product name and version,
  // of the form "productname/version", with no other slashes.
  // Used as part of the user agent string.
  virtual std::string GetProduct() const;

  // Returns a string resource given its id.
  virtual base::string16 GetLocalizedString(int message_id) const;

  // Called by samples::GetProcessTypeNameInEnglish for process types that it
  // doesn't know about because they're from the embedder.
  virtual std::string GetProcessTypeNameInEnglish(int type);

  // Called once during initialization of NetworkService to provide constants
  // to NetLog.  (Though it may be called multiples times if NetworkService
  // crashes and needs to be reinitialized).  The return value is merged with
  // |GetNetConstants()| and passed to FileNetLogObserver - see documentation
  // of |FileNetLogObserver::CreateBounded()| for more information.  The
  // convention is to put new constants under a subdict at the key "clientInfo".
  virtual base::DictionaryValue GetNetLogConstants() const;

  // Returns whether or not V8 script extensions should be allowed for a
  // service worker.
  virtual bool AllowScriptExtensionForServiceWorker(const GURL& script_url);

#if defined(OS_ANDROID)
  // Returns true for clients like Android WebView that uses synchronous
  // compositor. Note setting this to true will permit synchronous IPCs from
  // the master UI thread.
  virtual bool UsingSynchronousCompositing();

#endif  // OS_ANDROID

  virtual void OnServiceManagerConnected(ServiceManagerConnection* connection);

 private:
  friend class SamplesClientInitializer;  // To set these pointers.
  friend class InternalTestInitializer;

  // The embedder API for participating in master logic.
  SamplesMasterClient* master_;
  // The embedder API for participating in slave logic.
  SamplesSlavererClient* slaverer_;
  // The embedder API for participating in utility logic.
  SamplesUtilityClient* utility_;
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_COMMON_SAMPLES_CLIENT_H_
