// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/public/master/master_child_process_host_iterator.h"

#include "base/logging.h"
#include "samples/master/master_child_process_host_impl.h"
#include "samples/public/master/master_thread.h"

namespace samples {

MasterChildProcessHostIterator::MasterChildProcessHostIterator()
    : all_(true), process_type_(PROCESS_TYPE_UNKNOWN) {
  CHECK(MasterThread::CurrentlyOn(MasterThread::IO)) <<
      "MasterChildProcessHostIterator must be used on the IO thread.";
  iterator_ = MasterChildProcessHostImpl::GetIterator()->begin();
}

MasterChildProcessHostIterator::MasterChildProcessHostIterator(int type)
    : all_(false), process_type_(type) {
  CHECK(MasterThread::CurrentlyOn(MasterThread::IO)) <<
      "MasterChildProcessHostIterator must be used on the IO thread.";
  DCHECK_NE(PROCESS_TYPE_SLAVERER, type) <<
      "MasterChildProcessHostIterator doesn't work for renderer processes; "
      "try RenderProcessHost::AllHostsIterator() instead.";
  iterator_ = MasterChildProcessHostImpl::GetIterator()->begin();
  if (!Done() && (*iterator_)->GetData().process_type != process_type_)
    ++(*this);
}

MasterChildProcessHostIterator::~MasterChildProcessHostIterator() {
}

bool MasterChildProcessHostIterator::operator++() {
  CHECK(!Done());
  do {
    ++iterator_;
    if (Done())
      break;

    if (!all_ && (*iterator_)->GetData().process_type != process_type_)
      continue;

    return true;
  } while (true);

  return false;
}

bool MasterChildProcessHostIterator::Done() {
  return iterator_ == MasterChildProcessHostImpl::GetIterator()->end();
}

const ChildProcessData& MasterChildProcessHostIterator::GetData() {
  CHECK(!Done());
  return (*iterator_)->GetData();
}

bool MasterChildProcessHostIterator::Send(IPC::Message* message) {
  CHECK(!Done());
  return (*iterator_)->Send(message);
}

MasterChildProcessHostDelegate*
    MasterChildProcessHostIterator::GetDelegate() {
  return (*iterator_)->delegate();
}

ChildProcessHost* MasterChildProcessHostIterator::GetHost() {
  CHECK(!Done());
  return (*iterator_)->GetHost();
}

}  // namespace samples
