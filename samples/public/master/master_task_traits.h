// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SAMPLES_PUBLIC_MASTER_MASTER_TASK_TRAITS_H_
#define SAMPLES_PUBLIC_MASTER_MASTER_TASK_TRAITS_H_

#include "base/task/task_traits.h"
#include "base/task/task_traits_extension.h"
#include "samples/common/export.h"
#include "samples/public/master/master_thread.h"

namespace samples {

// Tasks with this trait will not be executed inside a nested RunLoop.
//
// Note: This should rarely be required. Drivers of nested loops should instead
// make sure to be reentrant when allowing nested application tasks (also rare).
//
// TODO(https://crbug.com/876272): Investigate removing this trait -- and any
// logic for deferred tasks in MessageLoop.
struct NonNestable {};

// TaskTraits for running tasks on the master threads.
//
// These traits enable the use of the //base/task/post_task.h APIs to post tasks
// to a MasterThread.
//
// To post a task to the UI thread (analogous for IO thread):
//     base::PostTaskWithTraits(FROM_HERE, {MasterThread::UI}, task);
//
// To obtain a TaskRunner for the UI thread (analogous for the IO thread):
//     base::CreateSingleThreadTaskRunnerWithTraits({MasterThread::UI});
//
// Tasks posted to the same MasterThread with the same traits will be executed
// in the order they were posted, regardless of the TaskRunners they were
// posted via.
//
// See //base/task/post_task.h for more detailed documentation.
//
// Posting to a MasterThread must only be done after it was initialized (ref.
// MasterMainLoop::CreateThreads() phase).
class SAMPLES_EXPORT MasterTaskTraitsExtension {
  using MasterThreadIDFilter =
      base::trait_helpers::RequiredEnumTraitFilter<MasterThread::ID>;
  using NonNestableFilter =
      base::trait_helpers::BooleanTraitFilter<NonNestable>;

 public:
  static constexpr uint8_t kExtensionId =
      base::TaskTraitsExtensionStorage::kFirstEmbedderExtensionId;

  struct ValidTrait : public base::TaskTraits::ValidTrait {
    using base::TaskTraits::ValidTrait::ValidTrait;

    ValidTrait(MasterThread::ID);
    ValidTrait(NonNestable);
  };

  template <
      class... ArgTypes,
      class CheckArgumentsAreValid = std::enable_if_t<
          base::trait_helpers::AreValidTraits<ValidTrait, ArgTypes...>::value>>
  constexpr MasterTaskTraitsExtension(ArgTypes... args)
      : master_thread_(
            base::trait_helpers::GetTraitFromArgList<MasterThreadIDFilter>(
                args...)),
        nestable_(!base::trait_helpers::GetTraitFromArgList<NonNestableFilter>(
            args...)) {}

  constexpr base::TaskTraitsExtensionStorage Serialize() const {
    static_assert(8 == sizeof(MasterTaskTraitsExtension),
                  "Update Serialize() and Parse() when changing "
                  "MasterTaskTraitsExtension");
    return {kExtensionId,
            {static_cast<uint8_t>(master_thread_),
             static_cast<uint8_t>(nestable_)}};
  }

  static const MasterTaskTraitsExtension Parse(
      const base::TaskTraitsExtensionStorage& extension) {
    return MasterTaskTraitsExtension(
        static_cast<MasterThread::ID>(extension.data[0]),
        static_cast<bool>(extension.data[1]));
  }

  constexpr MasterThread::ID master_thread() const { return master_thread_; }

  // Returns true if tasks with these traits may run in a nested RunLoop.
  constexpr bool nestable() const { return nestable_; }

 private:
  MasterTaskTraitsExtension(MasterThread::ID master_thread, bool nestable)
      : master_thread_(master_thread), nestable_(nestable) {}

  MasterThread::ID master_thread_;
  bool nestable_;
};

template <class... ArgTypes,
          class = std::enable_if_t<base::trait_helpers::AreValidTraits<
              MasterTaskTraitsExtension::ValidTrait,
              ArgTypes...>::value>>
constexpr base::TaskTraitsExtensionStorage MakeTaskTraitsExtension(
    ArgTypes&&... args) {
  return MasterTaskTraitsExtension(std::forward<ArgTypes>(args)...)
      .Serialize();
}

}  // namespace samples

#endif  // SAMPLES_PUBLIC_MASTER_MASTER_TASK_TRAITS_H_
