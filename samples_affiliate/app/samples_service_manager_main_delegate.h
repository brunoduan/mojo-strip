#ifndef SAMPLES_APP_SAMPLES_SERVICE_MANAGER_MAIN_DELEGATE_H_
#define SAMPLES_APP_SAMPLES_SERVICE_MANAGER_MAIN_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "samples/public/app/samples_main.h"
#include "services/service_manager/embedder/main_delegate.h"

namespace samples {

class SamplesMainRunnerImpl;

class SamplesServiceManagerMainDelegate : public service_manager::MainDelegate {
 public:
  explicit SamplesServiceManagerMainDelegate(const SamplesMainParams& params);
  ~SamplesServiceManagerMainDelegate() override;

  // service_manager::MainDelegate:
  int Initialize(const InitializeParams& params) override;
  bool IsEmbedderSubprocess() override;
  int RunEmbedderProcess() override;
  void ShutDownEmbedderProcess() override;
  service_manager::ProcessType OverrideProcessType() override;
  void OverrideMojoConfiguration(mojo::core::Configuration* config) override;
  std::unique_ptr<base::Value> CreateServiceCatalog() override;
  bool ShouldLaunchAsServiceProcess(
      const service_manager::Identity& identity) override;
  void AdjustServiceProcessCommandLine(
      const service_manager::Identity& identity,
      base::CommandLine* command_line) override;
  void OnServiceManagerInitialized(
      const base::Closure& quit_closure,
      service_manager::BackgroundServiceManager* service_manager) override;
  std::unique_ptr<service_manager::Service> CreateEmbeddedService(
      const std::string& service_name) override;

  // Sets the flag whether to start the Service Manager without starting the
  // full browser.
  void SetStartServiceManagerOnly(bool start_service_manager_only);

 private:
  SamplesMainParams samples_main_params_;
  std::unique_ptr<SamplesMainRunnerImpl> samples_main_runner_;

#if defined(OS_ANDROID)
  bool initialized_ = false;
#endif

  // Indicates whether to start the Service Manager without starting the full
  // browser.
  bool start_service_manager_only_ = false;

  DISALLOW_COPY_AND_ASSIGN(SamplesServiceManagerMainDelegate);
};

}  // namespace samples

#endif  // SAMPLES_APP_SAMPLES_SERVICE_MANAGER_MAIN_DELEGATE_H_
