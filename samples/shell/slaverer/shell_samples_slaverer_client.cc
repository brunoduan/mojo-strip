// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "samples/shell/slaverer/shell_samples_slaverer_client.h"

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "samples/public/child/child_thread.h"
#include "samples/public/common/service_manager_connection.h"
#include "samples/public/common/simple_connection_filter.h"
#include "samples/public/test/test_service.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/binder_registry.h"


namespace samples {

namespace {

// A test service which can be driven by browser tests for various reasons.
class TestSlavererServiceImpl : public mojom::TestService {
 public:
  explicit TestSlavererServiceImpl(mojom::TestServiceRequest request)
      : binding_(this, std::move(request)) {
    binding_.set_connection_error_handler(base::BindOnce(
        &TestSlavererServiceImpl::OnConnectionError, base::Unretained(this)));
  }

  ~TestSlavererServiceImpl() override {}

 private:
  void OnConnectionError() { delete this; }

  // mojom::TestService:
  void DoSomething(DoSomethingCallback callback) override {
    // Instead of responding normally, unbind the pipe, write some garbage,
    // and go away.
    const std::string kBadMessage = "This is definitely not a valid response!";
    mojo::ScopedMessagePipeHandle pipe = binding_.Unbind().PassMessagePipe();
    MojoResult rv = mojo::WriteMessageRaw(
        pipe.get(), kBadMessage.data(), kBadMessage.size(), nullptr, 0,
        MOJO_WRITE_MESSAGE_FLAG_NONE);
    DCHECK_EQ(rv, MOJO_RESULT_OK);

    // Deletes this.
    OnConnectionError();
  }

  void DoTerminateProcess(DoTerminateProcessCallback callback) override {
    NOTREACHED();
  }

  void DoCrashImmediately(DoCrashImmediatelyCallback callback) override {
    NOTREACHED();
  }

  void CreateFolder(CreateFolderCallback callback) override { NOTREACHED(); }

  void GetRequestorName(GetRequestorNameCallback callback) override {
    std::move(callback).Run("Not implemented.");
  }

  void CreateSharedBuffer(const std::string& message,
                          CreateSharedBufferCallback callback) override {
    NOTREACHED();
  }

  mojo::Binding<mojom::TestService> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestSlavererServiceImpl);
};

void CreateSlavererTestService(mojom::TestServiceRequest request) {
  // Owns itself.
  new TestSlavererServiceImpl(std::move(request));
}

}  // namespace

ShellSamplesSlavererClient::ShellSamplesSlavererClient() {}

ShellSamplesSlavererClient::~ShellSamplesSlavererClient() {
}

void ShellSamplesSlavererClient::SlaverThreadStarted() {
  auto registry = std::make_unique<service_manager::BinderRegistry>();
  registry->AddInterface<mojom::TestService>(
      base::Bind(&CreateSlavererTestService),
      base::ThreadTaskRunnerHandle::Get());
  //registry->AddInterface<mojom::PowerMonitorTest>(
  //    base::Bind(&PowerMonitorTestImpl::MakeStrongBinding,
  //               base::Passed(std::make_unique<PowerMonitorTestImpl>())),
  //    base::ThreadTaskRunnerHandle::Get());
  samples::ChildThread::Get()
      ->GetServiceManagerConnection()
      ->AddConnectionFilter(
          std::make_unique<SimpleConnectionFilter>(std::move(registry)));
}

}  // namespace samples
