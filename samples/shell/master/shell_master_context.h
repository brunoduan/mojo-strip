#ifndef SAMPLES_SHELL_MASTER_SHELL_MASTER_CONTEXT_H_
#define SAMPLES_SHELL_MASTER_SHELL_MASTER_CONTEXT_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

#include "samples/public/master/master_context.h"

namespace samples {

class ShellMasterContext : public MasterContext {
 public:
  ShellMasterContext(bool delay_services_creation);
  ~ShellMasterContext() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ShellMasterContext);
};

}

#endif // SAMPLES_SHELL_MASTER_SHELL_MASTER_CONTEXT_H_
