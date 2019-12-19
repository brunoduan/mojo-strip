// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/file/file_service.h"

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/task/post_task.h"
#include "components/services/filesystem/lock_table.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/file/file_system.h"
#include "services/file/user_id_map.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace file {

class FileService::FileSystemObjects
    : public base::SupportsWeakPtr<FileSystemObjects> {
 public:
  // Created on the main thread.
  FileSystemObjects(base::FilePath user_dir) : user_dir_(user_dir) {}

  // Destroyed on the |file_service_runner_|.
  ~FileSystemObjects() {}

  // Called on the |file_service_runner_|.
  void OnFileSystemRequest(const service_manager::Identity& remote_identity,
                           mojom::FileSystemRequest request) {
    if (!lock_table_)
      lock_table_ = new filesystem::LockTable;
    mojo::MakeStrongBinding(
        std::make_unique<FileSystem>(user_dir_, lock_table_),
        std::move(request));
  }

 private:
  scoped_refptr<filesystem::LockTable> lock_table_;
  base::FilePath user_dir_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemObjects);
};

std::unique_ptr<service_manager::Service> CreateFileService() {
  return std::make_unique<FileService>();
}

FileService::FileService()
    : file_service_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN})) {
  registry_.AddInterface<mojom::FileSystem>(
      base::Bind(&FileService::BindFileSystemRequest, base::Unretained(this)));
}

FileService::~FileService() {
  file_service_runner_->DeleteSoon(FROM_HERE, file_system_objects_.release());
}

void FileService::OnStart() {
  file_system_objects_.reset(new FileService::FileSystemObjects(
      GetUserDirForUserId(context()->identity().user_id())));
}

void FileService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe),
                          source_info);
}

void FileService::BindFileSystemRequest(
    mojom::FileSystemRequest request,
    const service_manager::BindSourceInfo& source_info) {
  file_service_runner_->PostTask(
      FROM_HERE,
      base::Bind(&FileService::FileSystemObjects::OnFileSystemRequest,
                 file_system_objects_->AsWeakPtr(), source_info.identity,
                 base::Passed(&request)));
}

}  // namespace user_service
