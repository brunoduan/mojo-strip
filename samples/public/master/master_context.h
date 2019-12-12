#ifndef SAMPLES_PUBLIC_MASTER_MASTER_CONTEXT_H_
#define SAMPLES_PUBLIC_MASTER_MASTER_CONTEXT_H_

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <string>

#include "base/supports_user_data.h"
#include "samples/common/export.h"

namespace samples {

class SAMPLES_EXPORT MasterContext : public base::SupportsUserData {
 public:
  MasterContext();
  ~MasterContext() override;

 private:
  const std::string unique_id_;
};

}

#endif // SAMPLES_PUBLIC_MASTER_MASTER_CONTEXT_H_
