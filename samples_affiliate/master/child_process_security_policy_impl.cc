// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/master/child_process_security_policy_impl.h"

#include <algorithm>
#include <utility>

#include "base/command_line.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "samples/master/bad_message.h"
#include "samples/master/isolated_origin_util.h"
#include "samples/public/master/master_context.h"
#include "samples/public/master/master_thread.h"
#include "samples/public/master/child_process_data.h"
#include "samples/public/master/samples_master_client.h"
#include "samples/public/master/slaver_process_host.h"
#include "samples/public/master/site_isolation_policy.h"
#include "samples/public/common/url_constants.h"
#include "url/gurl.h"

namespace samples {

namespace {

// Used internally only. These bit positions have no relationship to any
// underlying OS and can be changed to accommodate finer-grained permissions.
enum ChildProcessSecurityPermissions {
  READ_FILE_PERMISSION             = 1 << 0,
  WRITE_FILE_PERMISSION            = 1 << 1,
  CREATE_NEW_FILE_PERMISSION       = 1 << 2,
  CREATE_OVERWRITE_FILE_PERMISSION = 1 << 3,
  DELETE_FILE_PERMISSION           = 1 << 4,

  // Used by Media Galleries API
  COPY_INTO_FILE_PERMISSION        = 1 << 5,
};

// Used internally only. Bitmasks that are actually used by the Grant* and Can*
// methods. These contain one or more ChildProcessSecurityPermissions.
enum ChildProcessSecurityGrants {
  READ_FILE_GRANT              = READ_FILE_PERMISSION,
  WRITE_FILE_GRANT             = WRITE_FILE_PERMISSION,

  CREATE_NEW_FILE_GRANT        = CREATE_NEW_FILE_PERMISSION |
                                 COPY_INTO_FILE_PERMISSION,

  CREATE_READ_WRITE_FILE_GRANT = CREATE_NEW_FILE_PERMISSION |
                                 CREATE_OVERWRITE_FILE_PERMISSION |
                                 READ_FILE_PERMISSION |
                                 WRITE_FILE_PERMISSION |
                                 COPY_INTO_FILE_PERMISSION |
                                 DELETE_FILE_PERMISSION,

  COPY_INTO_FILE_GRANT         = COPY_INTO_FILE_PERMISSION,
  DELETE_FILE_GRANT            = DELETE_FILE_PERMISSION,
};

// https://crbug.com/646278 Valid blob URLs should contain canonically
// serialized origins.
bool IsMalformedBlobUrl(const GURL& url) {
  if (!url.SchemeIsBlob())
    return false;

  // If the part after blob: survives a roundtrip through url::Origin, then
  // it's a normal blob URL.
  std::string canonical_origin = url::Origin::Create(url).Serialize();
  canonical_origin.append(1, '/');
  if (base::StartsWith(url.GetContent(), canonical_origin,
                       base::CompareCase::INSENSITIVE_ASCII))
    return false;

  // This is a malformed blob URL.
  return true;
}

}  // namespace

// The SecurityState class is used to maintain per-child process security state
// information.
class ChildProcessSecurityPolicyImpl::SecurityState {
 public:
  SecurityState()
    : enabled_bindings_(0),
      can_read_raw_cookies_(false),
      can_send_midi_sysex_(false) { }

  ~SecurityState() {}

  // Grant permission to request and commit URLs with the specified origin.
  void GrantCommitOrigin(const url::Origin& origin) {
    if (origin.opaque())
      return;
    origin_map_[origin] = CommitRequestPolicy::kCommitAndRequest;
  }

  void GrantRequestOrigin(const url::Origin& origin) {
    if (origin.opaque())
      return;
    // Anything already in |origin_map_| must have at least request permission
    // already. In that case, the emplace() below will be a no-op.
    origin_map_.emplace(origin, CommitRequestPolicy::kRequestOnly);
  }

  void GrantCommitScheme(const std::string& scheme) {
    scheme_map_[scheme] = CommitRequestPolicy::kCommitAndRequest;
  }

  void GrantRequestScheme(const std::string& scheme) {
    // Anything already in |scheme_map_| must have at least request permission
    // already. In that case, the emplace() below will be a no-op.
    scheme_map_.emplace(scheme, CommitRequestPolicy::kRequestOnly);
  }

  // Grant certain permissions to a file.
  void GrantPermissionsForFile(const base::FilePath& file, int permissions) {
    base::FilePath stripped = file.StripTrailingSeparators();
    file_permissions_[stripped] |= permissions;
    UMA_HISTOGRAM_COUNTS_1M(
        "ChildProcessSecurityPolicy.FilePermissionPathLength",
        stripped.value().size());
  }

  // Grant navigation to a file but not the file:// scheme in general.
  void GrantRequestOfSpecificFile(const base::FilePath &file) {
    request_file_set_.insert(file.StripTrailingSeparators());
  }

  // Revokes all permissions granted to a file.
  void RevokeAllPermissionsForFile(const base::FilePath& file) {
    base::FilePath stripped = file.StripTrailingSeparators();
    file_permissions_.erase(stripped);
    request_file_set_.erase(stripped);
  }

  // Grant certain permissions to a file.
  void GrantPermissionsForFileSystem(const std::string& filesystem_id,
                                     int permissions) {
    //if (!base::ContainsKey(filesystem_permissions_, filesystem_id))
    //  storage::IsolatedContext::GetInstance()->AddReference(filesystem_id);
    filesystem_permissions_[filesystem_id] |= permissions;
  }

  bool HasPermissionsForFileSystem(const std::string& filesystem_id,
                                   int permissions) {
    FileSystemMap::const_iterator it =
        filesystem_permissions_.find(filesystem_id);
    if (it == filesystem_permissions_.end())
      return false;
    return (it->second & permissions) == permissions;
  }

#if defined(OS_ANDROID)
  // Determine if the certain permissions have been granted to a samples URI.
  bool HasPermissionsForContentUri(const base::FilePath& file,
                                   int permissions) {
    DCHECK(!file.empty());
    DCHECK(file.IsContentUri());
    if (!permissions)
      return false;
    base::FilePath file_path = file.StripTrailingSeparators();
    FileMap::const_iterator it = file_permissions_.find(file_path);
    if (it != file_permissions_.end())
      return (it->second & permissions) == permissions;
    return false;
  }
#endif

  void GrantBindings(int bindings) {
    enabled_bindings_ |= bindings;
  }

  void GrantReadRawCookies() {
    can_read_raw_cookies_ = true;
  }

  void RevokeReadRawCookies() {
    can_read_raw_cookies_ = false;
  }

  void GrantPermissionForMidiSysEx() {
    can_send_midi_sysex_ = true;
  }

  // Determine whether permission has been granted to commit |url|.
  bool CanCommitURL(const GURL& url) {
    return true;
  }

  bool CanRequestURL(const GURL& url) {
    DCHECK(!url.SchemeIsBlob() && !url.SchemeIsFileSystem())
        << "inner_url extraction should be done already.";
    // Having permission to a scheme implies permission to all of its URLs.
    auto scheme_judgment = scheme_map_.find(url.scheme());
    if (scheme_judgment != scheme_map_.end())
      return true;

    if (CanRequestOrigin(url::Origin::Create(url)))
      return true;

    // Otherwise, delegate to CanCommitURL. Unmentioned schemes are disallowed.
    // TODO(dcheng): It would be nice to avoid constructing the origin twice.
    return CanCommitURL(url);
  }

  // Determine if the certain permissions have been granted to a file.
  bool HasPermissionsForFile(const base::FilePath& file, int permissions) {
#if defined(OS_ANDROID)
    if (file.IsContentUri())
      return HasPermissionsForContentUri(file, permissions);
#endif
    if (!permissions || file.empty() || !file.IsAbsolute())
      return false;
    base::FilePath current_path = file.StripTrailingSeparators();
    base::FilePath last_path;
    int skip = 0;
    while (current_path != last_path) {
      base::FilePath base_name = current_path.BaseName();
      if (base_name.value() == base::FilePath::kParentDirectory) {
        ++skip;
      } else if (skip > 0) {
        if (base_name.value() != base::FilePath::kCurrentDirectory)
          --skip;
      } else {
        FileMap::const_iterator it = file_permissions_.find(current_path);
        if (it != file_permissions_.end())
          return (it->second & permissions) == permissions;
      }
      last_path = current_path;
      current_path = current_path.DirName();
    }

    return false;
  }

  bool CanAccessDataForOrigin(const GURL& site_url) {
    if (origin_lock_.is_empty())
      return true;
    return origin_lock_ == site_url;
  }

  void LockToOrigin(const GURL& gurl) {
    origin_lock_ = gurl;
  }

  const GURL& origin_lock() { return origin_lock_; }

  ChildProcessSecurityPolicyImpl::CheckOriginLockResult CheckOriginLock(
      const GURL& gurl) {
    if (origin_lock_.is_empty())
      return ChildProcessSecurityPolicyImpl::CheckOriginLockResult::NO_LOCK;

    if (origin_lock_ == gurl) {
      return ChildProcessSecurityPolicyImpl::CheckOriginLockResult::
          HAS_EQUAL_LOCK;
    }

    return ChildProcessSecurityPolicyImpl::CheckOriginLockResult::
        HAS_WRONG_LOCK;
  }

  bool can_read_raw_cookies() const {
    return can_read_raw_cookies_;
  }

  bool can_send_midi_sysex() const {
    return can_send_midi_sysex_;
  }

 private:
  enum class CommitRequestPolicy {
    kRequestOnly,
    kCommitAndRequest,
  };

  bool CanCommitOrigin(const url::Origin& origin) {
    auto it = origin_map_.find(origin);
    if (it == origin_map_.end())
      return false;
    return it->second == CommitRequestPolicy::kCommitAndRequest;
  }

  bool CanRequestOrigin(const url::Origin& origin) {
    // Anything already in |origin_map_| must have at least request permissions
    // already.
    return origin_map_.find(origin) != origin_map_.end();
  }

  typedef std::map<std::string, CommitRequestPolicy> SchemeMap;
  typedef std::map<url::Origin, CommitRequestPolicy> OriginMap;

  typedef int FilePermissionFlags;  // bit-set of base::File::Flags
  typedef std::map<base::FilePath, FilePermissionFlags> FileMap;
  typedef std::map<std::string, FilePermissionFlags> FileSystemMap;
  typedef std::set<base::FilePath> FileSet;

  // Maps URL schemes to commit/request policies the child process has been
  // granted. There is no provision for revoking.
  SchemeMap scheme_map_;

  // The map of URL origins to commit/request policies the child process has
  // been granted. There is no provision for revoking.
  OriginMap origin_map_;

  // The set of files the child process is permited to upload to the web.
  FileMap file_permissions_;

  // The set of files the child process is permitted to load.
  FileSet request_file_set_;

  int enabled_bindings_;

  bool can_read_raw_cookies_;

  bool can_send_midi_sysex_;

  GURL origin_lock_;

  // The set of isolated filesystems the child process is permitted to access.
  FileSystemMap filesystem_permissions_;

  DISALLOW_COPY_AND_ASSIGN(SecurityState);
};

ChildProcessSecurityPolicyImpl::ChildProcessSecurityPolicyImpl() {
  // We know about these schemes and believe them to be safe.
  RegisterWebSafeScheme(url::kHttpScheme);
  RegisterWebSafeScheme(url::kHttpsScheme);
  RegisterWebSafeScheme(url::kFtpScheme);
  RegisterWebSafeScheme(url::kDataScheme);
  RegisterWebSafeScheme("feed");

  // TODO(nick): https://crbug.com/651534 blob: and filesystem: schemes embed
  // other origins, so we should not treat them as web safe. Remove callers of
  // IsWebSafeScheme(), and then eliminate the next two lines.
  RegisterWebSafeScheme(url::kBlobScheme);
  RegisterWebSafeScheme(url::kFileSystemScheme);

  // We know about the following pseudo schemes and treat them specially.
  RegisterPseudoScheme(url::kAboutScheme);
  RegisterPseudoScheme(url::kJavaScriptScheme);
  RegisterPseudoScheme(kViewSourceScheme);
}

ChildProcessSecurityPolicyImpl::~ChildProcessSecurityPolicyImpl() {
}

// static
ChildProcessSecurityPolicy* ChildProcessSecurityPolicy::GetInstance() {
  return ChildProcessSecurityPolicyImpl::GetInstance();
}

ChildProcessSecurityPolicyImpl* ChildProcessSecurityPolicyImpl::GetInstance() {
  return base::Singleton<ChildProcessSecurityPolicyImpl>::get();
}

void ChildProcessSecurityPolicyImpl::Add(int child_id) {
  base::AutoLock lock(lock_);
  AddChild(child_id);
}

void ChildProcessSecurityPolicyImpl::AddWorker(int child_id,
                                               int main_slaver_process_id) {
  base::AutoLock lock(lock_);
  AddChild(child_id);
  worker_map_[child_id] = main_slaver_process_id;
}

void ChildProcessSecurityPolicyImpl::Remove(int child_id) {
  base::AutoLock lock(lock_);
  security_state_.erase(child_id);
  worker_map_.erase(child_id);
}

void ChildProcessSecurityPolicyImpl::RegisterWebSafeScheme(
    const std::string& scheme) {
  base::AutoLock lock(lock_);
  DCHECK_EQ(0U, schemes_okay_to_request_in_any_process_.count(scheme))
      << "Add schemes at most once.";
  DCHECK_EQ(0U, pseudo_schemes_.count(scheme))
      << "Web-safe implies not pseudo.";

  schemes_okay_to_request_in_any_process_.insert(scheme);
  schemes_okay_to_commit_in_any_process_.insert(scheme);
}

void ChildProcessSecurityPolicyImpl::RegisterWebSafeIsolatedScheme(
    const std::string& scheme,
    bool always_allow_in_origin_headers) {
  base::AutoLock lock(lock_);
  DCHECK_EQ(0U, schemes_okay_to_request_in_any_process_.count(scheme))
      << "Add schemes at most once.";
  DCHECK_EQ(0U, pseudo_schemes_.count(scheme))
      << "Web-safe implies not pseudo.";

  schemes_okay_to_request_in_any_process_.insert(scheme);
  if (always_allow_in_origin_headers)
    schemes_okay_to_appear_as_origin_headers_.insert(scheme);
}

bool ChildProcessSecurityPolicyImpl::IsWebSafeScheme(
    const std::string& scheme) {
  base::AutoLock lock(lock_);

  return base::ContainsKey(schemes_okay_to_request_in_any_process_, scheme);
}

void ChildProcessSecurityPolicyImpl::RegisterPseudoScheme(
    const std::string& scheme) {
  base::AutoLock lock(lock_);
  DCHECK_EQ(0U, pseudo_schemes_.count(scheme)) << "Add schemes at most once.";
  DCHECK_EQ(0U, schemes_okay_to_request_in_any_process_.count(scheme))
      << "Pseudo implies not web-safe.";
  DCHECK_EQ(0U, schemes_okay_to_commit_in_any_process_.count(scheme))
      << "Pseudo implies not web-safe.";

  pseudo_schemes_.insert(scheme);
}

bool ChildProcessSecurityPolicyImpl::IsPseudoScheme(
    const std::string& scheme) {
  base::AutoLock lock(lock_);

  return base::ContainsKey(pseudo_schemes_, scheme);
}

void ChildProcessSecurityPolicyImpl::GrantCommitURL(int child_id,
                                                    const GURL& url) {
  // Can't grant the capability to commit invalid URLs.
  if (!url.is_valid())
    return;

  // Can't grant the capability to commit pseudo schemes.
  if (IsPseudoScheme(url.scheme()))
    return;

  url::Origin origin = url::Origin::Create(url);

  // Blob and filesystem URLs require special treatment; grant access to the
  // inner origin they embed instead.
  // TODO(dcheng): Can this logic be simplified to just derive an origin up
  // front and use that? That probably requires fixing GURL canonicalization of
  // blob URLs though. For now, be consistent with how CanRequestURL and
  // CanCommitURL normalize.
  if (url.SchemeIsBlob() || url.SchemeIsFileSystem()) {
    if (IsMalformedBlobUrl(url))
      return;

    GrantCommitURL(child_id, GURL(origin.Serialize()));
  }

  // TODO(dcheng): In the future, URLs with opaque origins would ideally carry
  // around an origin with them, so we wouldn't need to grant commit access to
  // the entire scheme.
  if (!origin.opaque())
    GrantCommitOrigin(child_id, origin);

  // The scheme has already been whitelisted for every child process, so no need
  // to do anything else.
  if (IsWebSafeScheme(url.scheme()))
    return;

  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return;

  if (origin.opaque()) {
    // If it's impossible to grant commit rights to just the origin (among other
    // things, URLs with non-standard schemes will be treated as opaque
    // origins), then grant access to commit all URLs of that scheme.
    state->second->GrantCommitScheme(url.scheme());
  } else {
    // When the child process has been commanded to request this scheme, grant
    // it the capability to request all URLs of that scheme.
    state->second->GrantRequestScheme(url.scheme());
  }
}

void ChildProcessSecurityPolicyImpl::GrantRequestSpecificFileURL(
    int child_id,
    const GURL& url) {
}

void ChildProcessSecurityPolicyImpl::GrantReadFile(int child_id,
                                                   const base::FilePath& file) {
  GrantPermissionsForFile(child_id, file, READ_FILE_GRANT);
}

void ChildProcessSecurityPolicyImpl::GrantCreateReadWriteFile(
    int child_id, const base::FilePath& file) {
  GrantPermissionsForFile(child_id, file, CREATE_READ_WRITE_FILE_GRANT);
}

void ChildProcessSecurityPolicyImpl::GrantCopyInto(int child_id,
                                                   const base::FilePath& dir) {
  GrantPermissionsForFile(child_id, dir, COPY_INTO_FILE_GRANT);
}

void ChildProcessSecurityPolicyImpl::GrantDeleteFrom(
    int child_id, const base::FilePath& dir) {
  GrantPermissionsForFile(child_id, dir, DELETE_FILE_GRANT);
}

void ChildProcessSecurityPolicyImpl::GrantPermissionsForFile(
    int child_id, const base::FilePath& file, int permissions) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return;

  state->second->GrantPermissionsForFile(file, permissions);
}

void ChildProcessSecurityPolicyImpl::RevokeAllPermissionsForFile(
    int child_id, const base::FilePath& file) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return;

  state->second->RevokeAllPermissionsForFile(file);
}

void ChildProcessSecurityPolicyImpl::GrantReadFileSystem(
    int child_id, const std::string& filesystem_id) {
  GrantPermissionsForFileSystem(child_id, filesystem_id, READ_FILE_GRANT);
}

void ChildProcessSecurityPolicyImpl::GrantWriteFileSystem(
    int child_id, const std::string& filesystem_id) {
  GrantPermissionsForFileSystem(child_id, filesystem_id, WRITE_FILE_GRANT);
}

void ChildProcessSecurityPolicyImpl::GrantCreateFileForFileSystem(
    int child_id, const std::string& filesystem_id) {
  GrantPermissionsForFileSystem(child_id, filesystem_id, CREATE_NEW_FILE_GRANT);
}

void ChildProcessSecurityPolicyImpl::GrantCreateReadWriteFileSystem(
    int child_id, const std::string& filesystem_id) {
  GrantPermissionsForFileSystem(
      child_id, filesystem_id, CREATE_READ_WRITE_FILE_GRANT);
}

void ChildProcessSecurityPolicyImpl::GrantCopyIntoFileSystem(
    int child_id, const std::string& filesystem_id) {
  GrantPermissionsForFileSystem(child_id, filesystem_id, COPY_INTO_FILE_GRANT);
}

void ChildProcessSecurityPolicyImpl::GrantDeleteFromFileSystem(
    int child_id, const std::string& filesystem_id) {
  GrantPermissionsForFileSystem(child_id, filesystem_id, DELETE_FILE_GRANT);
}

void ChildProcessSecurityPolicyImpl::GrantSendMidiSysExMessage(int child_id) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return;

  state->second->GrantPermissionForMidiSysEx();
}

void ChildProcessSecurityPolicyImpl::GrantCommitOrigin(
    int child_id,
    const url::Origin& origin) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return;

  state->second->GrantCommitOrigin(origin);
}

void ChildProcessSecurityPolicyImpl::GrantRequestOrigin(
    int child_id,
    const url::Origin& origin) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return;

  state->second->GrantRequestOrigin(origin);
}

void ChildProcessSecurityPolicyImpl::GrantRequestScheme(
    int child_id,
    const std::string& scheme) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return;

  state->second->GrantRequestScheme(scheme);
}

void ChildProcessSecurityPolicyImpl::GrantReadRawCookies(int child_id) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return;

  state->second->GrantReadRawCookies();
}

void ChildProcessSecurityPolicyImpl::RevokeReadRawCookies(int child_id) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return;

  state->second->RevokeReadRawCookies();
}

bool ChildProcessSecurityPolicyImpl::CanRequestURL(
    int child_id, const GURL& url) {
  return true;
}

bool ChildProcessSecurityPolicyImpl::CanRedirectToURL(const GURL& url) {
  if (!url.is_valid())
    return false;  // Can't redirect to invalid URLs.

  const std::string& scheme = url.scheme();

  // Can't redirect to error pages.
  if (scheme == kChromeErrorScheme)
    return false;

  if (IsPseudoScheme(scheme)) {
    // Redirects to a pseudo scheme (about, javascript, view-source, ...) are
    // not allowed. An exception is made for <about:blank> and its variations.
    return url.IsAboutBlank();
  }

  // Note about redirects and special URLs:
  // * data-url: Blocked by net::DataProtocolHandler::IsSafeRedirectTarget().
  // * filesystem-url: Blocked by
  // storage::FilesystemProtocolHandler::IsSafeRedirectTarget().
  // Depending on their inner origins and if the request is master-initiated or
  // slaverer-initiated, blob-urls might get blocked by CanCommitURL or in
  // DocumentLoader::RedirectReceived. If not blocked, a 'file not found'
  // response will be generated in net::BlobURLRequestJob::DidStart().

  return true;
}

bool ChildProcessSecurityPolicyImpl::CanCommitURL(int child_id,
                                                  const GURL& url,
                                                  bool check_origin_locks) {
  if (!url.is_valid())
    return false;  // Can't commit invalid URLs.

  const std::string& scheme = url.scheme();

  // Of all the pseudo schemes, only about:blank and about:srcdoc are allowed to
  // commit.
  if (IsPseudoScheme(scheme))
    return url == url::kAboutBlankURL || url == kAboutSrcDocURL;

  // Blob and filesystem URLs require special treatment; validate the inner
  // origin they embed.
  if (url.SchemeIsBlob() || url.SchemeIsFileSystem()) {
    if (IsMalformedBlobUrl(url))
      return false;

    url::Origin origin = url::Origin::Create(url);
    return origin.opaque() ||
           CanCommitURL(child_id, GURL(origin.Serialize()), check_origin_locks);
  }

  // With site isolation, a URL from a site may only be committed in a process
  // dedicated to that site.  This check will ensure that |url| can't commit if
  // the process is locked to a different site.  Note that this check is only
  // effective for processes that are locked to a site, but even with strict
  // site isolation, currently not all processes are locked (e.g., extensions
  // or <webview> tags - see ShouldLockToOrigin()).
  if (check_origin_locks && !CanAccessDataForOrigin(child_id, url))
    return false;

  {
    base::AutoLock lock(lock_);

    // Most schemes can commit in any process. Note that we check
    // schemes_okay_to_commit_in_any_process_ here, which is stricter than
    // IsWebSafeScheme().
    //
    // TODO(creis, nick): https://crbug.com/515309: The line below does not
    // enforce that http pages cannot commit in an extension process.
    if (base::ContainsKey(schemes_okay_to_commit_in_any_process_, scheme))
      return true;

    auto state = security_state_.find(child_id);
    if (state == security_state_.end())
      return false;

    // Otherwise, we consult the child process's security state to see if it is
    // allowed to commit the URL.
    return state->second->CanCommitURL(url);
  }
}

bool ChildProcessSecurityPolicyImpl::CanCommitURL(int child_id,
                                                  const GURL& url) {
  return CanCommitURL(child_id, url, true /* check_origin_lock */);
}

bool ChildProcessSecurityPolicyImpl::CanSetAsOriginHeader(int child_id,
                                                          const GURL& url) {
  if (!url.is_valid())
    return false;  // Can't set invalid URLs as origin headers.

  // about:srcdoc cannot be used as an origin
  if (url == kAboutSrcDocURL)
    return false;

  // If this process can commit |url|, it can use |url| as an origin for
  // outbound requests.
  //
  // TODO(alexmos): This should eventually also check the origin lock, but
  // currently this is not done due to certain corner cases involving HTML
  // imports and layout tests that simulate requests from isolated worlds.  See
  // https://crbug.com/515309.
  if (CanCommitURL(child_id, url, false /* check_origin_lock */))
    return true;

  // Allow schemes which may come from scripts executing in isolated worlds;
  // XHRs issued by such scripts reflect the script origin rather than the
  // document origin.
  {
    base::AutoLock lock(lock_);
    if (base::ContainsKey(schemes_okay_to_appear_as_origin_headers_,
                          url.scheme())) {
      return true;
    }
  }
  return false;
}

bool ChildProcessSecurityPolicyImpl::CanReadFile(int child_id,
                                                 const base::FilePath& file) {
  return HasPermissionsForFile(child_id, file, READ_FILE_GRANT);
}

bool ChildProcessSecurityPolicyImpl::CanReadAllFiles(
    int child_id,
    const std::vector<base::FilePath>& files) {
  return std::all_of(files.begin(), files.end(),
                     [this, child_id](const base::FilePath& file) {
                       return CanReadFile(child_id, file);
                     });
}

bool ChildProcessSecurityPolicyImpl::CanCreateReadWriteFile(
    int child_id,
    const base::FilePath& file) {
  return HasPermissionsForFile(child_id, file, CREATE_READ_WRITE_FILE_GRANT);
}

bool ChildProcessSecurityPolicyImpl::CanReadFileSystem(
    int child_id, const std::string& filesystem_id) {
  return HasPermissionsForFileSystem(child_id, filesystem_id, READ_FILE_GRANT);
}

bool ChildProcessSecurityPolicyImpl::CanReadWriteFileSystem(
    int child_id, const std::string& filesystem_id) {
  return HasPermissionsForFileSystem(child_id, filesystem_id,
                                     READ_FILE_GRANT | WRITE_FILE_GRANT);
}

bool ChildProcessSecurityPolicyImpl::CanCopyIntoFileSystem(
    int child_id, const std::string& filesystem_id) {
  return HasPermissionsForFileSystem(child_id, filesystem_id,
                                     COPY_INTO_FILE_GRANT);
}

bool ChildProcessSecurityPolicyImpl::CanDeleteFromFileSystem(
    int child_id, const std::string& filesystem_id) {
  return HasPermissionsForFileSystem(child_id, filesystem_id,
                                     DELETE_FILE_GRANT);
}

bool ChildProcessSecurityPolicyImpl::HasPermissionsForFile(
    int child_id, const base::FilePath& file, int permissions) {
  base::AutoLock lock(lock_);
  bool result = ChildProcessHasPermissionsForFile(child_id, file, permissions);
  if (!result) {
    // If this is a worker thread that has no access to a given file,
    // let's check that its slaverer process has access to that file instead.
    auto iter = worker_map_.find(child_id);
    if (iter != worker_map_.end() && iter->second != 0) {
      result = ChildProcessHasPermissionsForFile(iter->second,
                                                 file,
                                                 permissions);
    }
  }
  return result;
}

bool ChildProcessSecurityPolicyImpl::CanReadRawCookies(int child_id) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return false;

  return state->second->can_read_raw_cookies();
}

void ChildProcessSecurityPolicyImpl::AddChild(int child_id) {
  if (security_state_.count(child_id) != 0) {
    NOTREACHED() << "Add child process at most once.";
    return;
  }

  security_state_[child_id] = std::make_unique<SecurityState>();
}

bool ChildProcessSecurityPolicyImpl::ChildProcessHasPermissionsForFile(
    int child_id, const base::FilePath& file, int permissions) {
  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return false;
  return state->second->HasPermissionsForFile(file, permissions);
}

bool ChildProcessSecurityPolicyImpl::CanAccessDataForOrigin(int child_id,
                                                            const GURL& url) {
  return true;
}

void ChildProcessSecurityPolicyImpl::LockToOrigin(int child_id,
                                                  const GURL& gurl) {
}

ChildProcessSecurityPolicyImpl::CheckOriginLockResult
ChildProcessSecurityPolicyImpl::CheckOriginLock(int child_id,
                                                const GURL& site_url) {
  base::AutoLock lock(lock_);
  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return ChildProcessSecurityPolicyImpl::CheckOriginLockResult::NO_LOCK;
  return state->second->CheckOriginLock(site_url);
}

GURL ChildProcessSecurityPolicyImpl::GetOriginLock(int child_id) {
  base::AutoLock lock(lock_);
  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return GURL();
  return state->second->origin_lock();
}

void ChildProcessSecurityPolicyImpl::GrantPermissionsForFileSystem(
    int child_id,
    const std::string& filesystem_id,
    int permission) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return;
  state->second->GrantPermissionsForFileSystem(filesystem_id, permission);
}

bool ChildProcessSecurityPolicyImpl::HasPermissionsForFileSystem(
    int child_id,
    const std::string& filesystem_id,
    int permission) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return false;
  return state->second->HasPermissionsForFileSystem(filesystem_id, permission);
}

bool ChildProcessSecurityPolicyImpl::CanSendMidiSysExMessage(int child_id) {
  base::AutoLock lock(lock_);

  auto state = security_state_.find(child_id);
  if (state == security_state_.end())
    return false;

  return state->second->can_send_midi_sysex();
}

void ChildProcessSecurityPolicyImpl::AddIsolatedOrigins(
    std::vector<url::Origin> origins_to_add) {
}

bool ChildProcessSecurityPolicyImpl::IsIsolatedOrigin(
    const url::Origin& origin) {
  url::Origin unused_result;
  return GetMatchingIsolatedOrigin(origin, &unused_result);
}

bool ChildProcessSecurityPolicyImpl::GetMatchingIsolatedOrigin(
    const url::Origin& origin,
    url::Origin* result) {
  return true;
}

bool ChildProcessSecurityPolicyImpl::GetMatchingIsolatedOrigin(
    const url::Origin& origin,
    const GURL& site_url,
    url::Origin* result) {
  *result = url::Origin();
  base::AutoLock lock(lock_);

  // Look up the list of origins corresponding to |origin|'s site.
  auto it = isolated_origins_.find(site_url);

  // Subtle corner case: if the site's host ends with a dot, do the lookup
  // without it.  A trailing dot shouldn't be able to bypass isolated origins:
  // if "https://foo.com" is an isolated origin, "https://foo.com." should
  // match it.
  if (it == isolated_origins_.end() && site_url.has_host() &&
      site_url.host_piece().back() == '.') {
    GURL::Replacements replacements;
    base::StringPiece host(site_url.host_piece());
    host.remove_suffix(1);
    replacements.SetHostStr(host);
    it = isolated_origins_.find(site_url.ReplaceComponents(replacements));
  }

  // If multiple isolated origins are registered with a common domain suffix,
  // return the most specific one.  For example, if foo.isolated.com and
  // isolated.com are both isolated origins, bar.foo.isolated.com should
  // return foo.isolated.com.
  bool found = false;
  if (it != isolated_origins_.end()) {
    for (const url::Origin& isolated_origin : it->second) {
      if (IsolatedOriginUtil::DoesOriginMatchIsolatedOrigin(origin,
                                                            isolated_origin)) {
        if (!found ||
            result->host().length() < isolated_origin.host().length()) {
          *result = isolated_origin;
          found = true;
        }
      }
    }
  }

  return found;
}

void ChildProcessSecurityPolicyImpl::RemoveIsolatedOriginForTesting(
    const url::Origin& origin) {
}

}  // namespace samples
