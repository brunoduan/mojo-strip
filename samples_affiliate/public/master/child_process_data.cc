// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/master/child_process_data.h"

namespace samples {

ChildProcessData::ChildProcessData(int process_type)
    : process_type(process_type), id(0), handle_(base::kNullProcessHandle) {}

ChildProcessData::ChildProcessData(ChildProcessData&& rhs)
    : process_type(rhs.process_type),
      package_name(rhs.package_name),
      name(rhs.name),
      metrics_name(rhs.metrics_name),
      id(rhs.id) {
  handle_ = rhs.handle_;
  rhs.handle_ = base::kNullProcessHandle;
}

ChildProcessData::~ChildProcessData() {}

ChildProcessData ChildProcessData::Duplicate() const {
  ChildProcessData result(process_type);
  result.name = name;
  result.package_name = package_name;
  result.metrics_name = metrics_name;
  result.id = id;
  result.SetHandle(handle_);

  return result;
}

}  // namespace samples
