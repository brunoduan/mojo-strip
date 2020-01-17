// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_STARTUP_DATA_H_
#define SAMPLES_PUBLIC_MASTER_STARTUP_DATA_H_

namespace samples {

// Data that //samples routes through its embedder which should be handed back
// to //samples when the embedder launches it.
struct StartupData {
  virtual ~StartupData() = default;
};

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_STARTUP_DATA_H_
