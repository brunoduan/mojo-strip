// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SEQUENCE_MANAGER_TEST_TEST_TASK_QUEUE_H_
#define BASE_TASK_SEQUENCE_MANAGER_TEST_TEST_TASK_QUEUE_H_

#include "base/memory/weak_ptr.h"
#include "base/task/sequence_manager/task_queue.h"

namespace base {
namespace sequence_manager {

class TestTaskQueue : public TaskQueue {
 public:
  explicit TestTaskQueue(std::unique_ptr<internal::TaskQueueImpl> impl,
                         const TaskQueue::Spec& spec);

  // TODO(kraynov): Remove PostTask methods after TaskQueue refactoring.

  bool PostTask(const Location& location, OnceClosure callback) {
    return PostDelayedTask(location, std::move(callback), TimeDelta());
  }

  bool PostDelayedTask(const Location& location,
                       OnceClosure callback,
                       TimeDelta delay) {
    return task_runner()->PostDelayedTask(location, std::move(callback), delay);
  }

  bool PostNonNestableTask(const Location& location, OnceClosure callback) {
    return PostNonNestableDelayedTask(location, std::move(callback),
                                      TimeDelta());
  }

  bool PostNonNestableDelayedTask(const Location& location,
                                  OnceClosure callback,
                                  TimeDelta delay) {
    return task_runner()->PostNonNestableDelayedTask(
        location, std::move(callback), delay);
  }

  bool RunsTasksInCurrentSequence() const {
    return task_runner()->RunsTasksInCurrentSequence();
  }

  using TaskQueue::GetTaskQueueImpl;

  WeakPtr<TestTaskQueue> GetWeakPtr();

 private:
  ~TestTaskQueue() override;  // Ref-counted.

  // Used to ensure that task queue is deleted in tests.
  WeakPtrFactory<TestTaskQueue> weak_factory_;
};

}  // namespace sequence_manager
}  // namespace base

#endif  // BASE_TASK_SEQUENCE_MANAGER_TEST_TEST_TASK_QUEUE_H_
