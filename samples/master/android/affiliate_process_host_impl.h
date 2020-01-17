#ifndef SAMPLES_MASTER_ANDROID_AFFILIATE_PROCESS_HOST_IMPL_H_
#define SAMPLES_MASTER_ANDROID_AFFILIATE_PROCESS_HOST_IMPL_H_

#include "samples/master/android/affiliate_process_host.h"

#include <jni.h>
#include <string>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/sequenced_task_runner.h"

#include "samples/common/export.h"

namespace samples {

class UtilityProcessHost;

class SAMPLES_EXPORT AffiliateProcessHostImpl
    : public AffiliateProcessHost {
 public:
  explicit AffiliateProcessHostImpl(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner);
  ~AffiliateProcessHostImpl() override;

  static UtilityProcessHost* GetProcessHost(const std::string& pkg_name);

  void CreateProcessHost(const std::string& pkg_name) override;

 private:
  class Context;

  scoped_refptr<Context> context_;

  DISALLOW_COPY_AND_ASSIGN(AffiliateProcessHostImpl);
};

}

#endif  // SAMPLES_MASTER_ANDROID_AFFILIATE_PROCESS_HOST_IMPL_H_
