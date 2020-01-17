// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/common/samples_switches_internal.h"

#include <string>

#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "samples/public/common/samples_switches.h"

#if defined(OS_ANDROID)
#include "base/debug/debugger.h"
#include "base/feature_list.h"
#endif

#if defined(OS_POSIX) && !defined(OS_ANDROID)
#include <signal.h>
static void SigUSR1Handler(int signal) {}
#endif

namespace samples {

namespace {

std::string ToNativeString(const std::string& string) {
  return string;
}

std::string FromNativeString(const std::string& string) {
  return string;
}

}  // namespace

void WaitForDebugger(const std::string& label) {
#if defined(OS_POSIX)
#if defined(OS_ANDROID)
  LOG(ERROR) << label << " waiting for GDB.";
  // Wait 24 hours for a debugger to be attached to the current process.
  base::debug::WaitForDebugger(24 * 60 * 60, true);
#else
  // TODO(playmobil): In the long term, overriding this flag doesn't seem
  // right, either use our own flag or open a dialog we can use.
  // This is just to ease debugging in the interim.
  LOG(ERROR) << label << " (" << getpid()
             << ") paused waiting for debugger to attach. "
             << "Send SIGUSR1 to unpause.";
  // Install a signal handler so that pause can be woken.
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SigUSR1Handler;
  sigaction(SIGUSR1, &sa, nullptr);

  pause();
#endif  // defined(OS_ANDROID)
#endif  // defined(OS_POSIX)
}

std::vector<std::string> FeaturesFromSwitch(
    const base::CommandLine& command_line,
    const char* switch_name) {
  using NativeString = base::CommandLine::StringType;
  using NativeStringPiece = base::BasicStringPiece<NativeString>;

  std::vector<std::string> features;
  if (!command_line.HasSwitch(switch_name))
    return features;

  // Store prefix as native string to avoid conversions for every arg.
  // (No string copies for the args that don't match the prefix.)
  NativeString prefix =
      ToNativeString(base::StringPrintf("--%s=", switch_name));
  for (NativeStringPiece arg : command_line.argv()) {
    // Switch names are case insensitive on Windows, but base::CommandLine has
    // already made them lowercase when building argv().
    if (!StartsWith(arg, prefix, base::CompareCase::SENSITIVE))
      continue;
    arg.remove_prefix(prefix.size());
    if (!IsStringASCII(arg))
      continue;
    auto vals = SplitString(FromNativeString(arg.as_string()), ",",
                            base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    features.insert(features.end(), vals.begin(), vals.end());
  }
  return features;
}

} // namespace samples
