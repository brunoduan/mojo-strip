// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_MASTER_CHILD_PROCESS_HOST_ITERATOR_H_
#define SAMPLES_PUBLIC_MASTER_MASTER_CHILD_PROCESS_HOST_ITERATOR_H_

#include <list>

#include "samples/common/export.h"

namespace IPC {
class Message;
}

namespace samples {
class MasterChildProcessHostDelegate;
class MasterChildProcessHostImpl;
struct ChildProcessData;
class ChildProcessHost;

// This class allows iteration through either all child processes, or ones of a
// specific type, depending on which constructor is used.  Note that this should
// be done from the IO thread and that the iterator should not be kept around as
// it may be invalidated on subsequent event processing in the event loop.
class SAMPLES_EXPORT MasterChildProcessHostIterator {
 public:
  MasterChildProcessHostIterator();
  explicit MasterChildProcessHostIterator(int type);
  ~MasterChildProcessHostIterator();

  // These methods work on the current iterator object. Only call them if
  // Done() returns false.
  bool operator++();
  bool Done();
  const ChildProcessData& GetData();
  bool Send(IPC::Message* message);
  MasterChildProcessHostDelegate* GetDelegate();
  ChildProcessHost* GetHost();

 private:
  bool all_;
  int process_type_;
  std::list<MasterChildProcessHostImpl*>::iterator iterator_;
};

// Helper class so that subclasses of MasterChildProcessHostDelegate can be
// iterated with no casting needing. Note that because of the components build,
// this class can only be used by BCPHD implementations that live in samples,
// otherwise link errors will result.
template <class T>
class SAMPLES_EXPORT MasterChildProcessHostTypeIterator
    : public MasterChildProcessHostIterator {
 public:
  explicit MasterChildProcessHostTypeIterator(int process_type)
      : MasterChildProcessHostIterator(process_type) {}
  T* operator->() {
    return static_cast<T*>(GetDelegate());
  }
  T* operator*() {
    return static_cast<T*>(GetDelegate());
  }
};

};  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_MASTER_CHILD_PROCESS_HOST_ITERATOR_H_
