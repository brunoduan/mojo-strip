#ifndef SAMPLES_PUBLIC_APP_SAMPLES_MAIN_H_
#define SAMPLES_PUBLIC_APP_SAMPLES_MAIN_H_

#include <stddef.h>

#include "base/callback_forward.h"
#include "build/build_config.h"
#include "samples/common/export.h"

namespace sandbox {
struct SandboxInterfaceInfo;
}

namespace samples {

//class BrowserMainParts;
class SamplesMainDelegate;

//using CreatedMainPartsClosure = base::Callback<void(BrowserMainParts*)>;

struct SamplesMainParams {
  explicit SamplesMainParams(SamplesMainDelegate* delegate)
      : delegate(delegate) {}

  SamplesMainDelegate* delegate;

  //CreatedMainPartsClosure* created_main_parts_closure = nullptr;

};

// This should only be called once before SamplesMainRunner actually running.
// The ownership of |delegate| is transferred.
SAMPLES_EXPORT void SetSamplesMainDelegate(SamplesMainDelegate* delegate);

}  // namespace samples

#endif  // SAMPLES_PUBLIC_APP_SAMPLES_MAIN_H_
