#ifndef SAMPLES_MASTER_ANDROID_AFFILIATE_PROCESS_HOST_H_
#define SAMPLES_MASTER_ANDROID_AFFILIATE_PROCESS_HOST_H_

#include <jni.h>

#include <string>

#include "base/sequenced_task_runner.h"

#include "samples/common/export.h"

namespace samples {

class SAMPLES_EXPORT AffiliateProcessHost {
 public:
  virtual ~AffiliateProcessHost();

  static void SetForProcess(std::unique_ptr<AffiliateProcessHost> helper);
  static AffiliateProcessHost* GetForProcess();
  static std::unique_ptr<AffiliateProcessHost> Create(
      scoped_refptr<base::SequencedTaskRunner> io_task_runner);

  virtual void CreateProcessHost(const std::string& pkg_name) = 0;

};

}

#endif  // SAMPLES_MASTER_ANDROID_AFFILIATE_PROCESS_HOST_H_
