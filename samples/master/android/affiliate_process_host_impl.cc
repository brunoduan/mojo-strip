#include "samples/master/android/affiliate_process_host_impl.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/sequenced_task_runner.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/task_traits.h"

#include "jni/AffiliateProcessHostImpl_jni.h"

#include "samples/public/master/master_thread.h"
#include "samples/public/master/master_task_traits.h"
#include "samples/master/utility_process_host.h"
#include "samples/master/utility_process_host_client.h"
#include "services/service_manager/sandbox/sandbox_type.h"

using base::android::JavaParamRef;

namespace samples {
namespace {

base::LazyInstance<std::unique_ptr<AffiliateProcessHost>>::Leaky
    g_affiliate_process_host = LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<std::map<std::string, samples::UtilityProcessHost*>>::DestructorAtExit
    g_name_process_hosts = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// A ref-counted object which owns the IO thread state of a AffiliateProcessHostImpl.
class AffiliateProcessHostImpl::Context
  : public base::RefCountedThreadSafe<Context> {
 public:
  Context(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner)
      : io_task_runner_(io_task_runner) {
  }

  void CreateProcessHost(const std::string& pkg_name) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&AffiliateProcessHostImpl::Context::CreateProcessHostOnIoThread,
                       this, pkg_name));
  }

 private:
  friend class base::RefCountedThreadSafe<Context>;

  ~Context() {}

  void CreateProcessHostOnIoThread(const std::string& pkg_name) {
    UtilityProcessHost* process_host = AffiliateProcessHostImpl::GetProcessHost(pkg_name);
    if (process_host != nullptr) {
      return;
    }
  
    service_manager::SandboxType sandbox_type =
        service_manager::SandboxType::SANDBOX_TYPE_UTILITY;
    process_host = new UtilityProcessHost(nullptr, nullptr);
    process_host->SetName(base::ASCIIToUTF16(pkg_name));
    process_host->SetPackageName(pkg_name);
    process_host->SetSandboxType(sandbox_type);
    process_host->Start();
    g_name_process_hosts.Get()[pkg_name] = process_host;
  }

  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(Context);
};

AffiliateProcessHost::~AffiliateProcessHost() {}

AffiliateProcessHostImpl::AffiliateProcessHostImpl(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner) {
  context_ = new Context(io_task_runner);
}

AffiliateProcessHostImpl::~AffiliateProcessHostImpl() {
}

// static
std::unique_ptr<AffiliateProcessHost> AffiliateProcessHost::Create(
    scoped_refptr<base::SequencedTaskRunner> io_task_runner) {
  return std::make_unique<AffiliateProcessHostImpl>(io_task_runner);
}

static void JNI_AffiliateProcessHostImpl_CreateAffiliateProcess(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& jpkg_name) {
  std::string pkg_name =
    base::android::ConvertJavaStringToUTF8(env, jpkg_name);
  DLOG(INFO) << "pkg_name " << pkg_name;

  if (AffiliateProcessHost::GetForProcess() == nullptr) {
  } else {
    AffiliateProcessHost::GetForProcess()->CreateProcessHost(pkg_name);
  }
}

void AffiliateProcessHostImpl::CreateProcessHost(const std::string& pkg_name) {
  context_->CreateProcessHost(pkg_name);
}

// static
UtilityProcessHost* AffiliateProcessHostImpl::GetProcessHost(const std::string& pkg_name) {
  auto it = g_name_process_hosts.Get().find(pkg_name);
  return it != g_name_process_hosts.Get().end() ? it->second : nullptr;
}

// static
void AffiliateProcessHost::SetForProcess(std::unique_ptr<AffiliateProcessHost> helper) {
  DCHECK(!g_affiliate_process_host.Get());
  g_affiliate_process_host.Get() = std::move(helper);
}

// static
AffiliateProcessHost* AffiliateProcessHost::GetForProcess() {
  return g_affiliate_process_host.Get().get();
}

} // namespace samples
