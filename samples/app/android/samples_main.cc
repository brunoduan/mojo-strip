#include <memory>

#include "base/lazy_instance.h"

#include "samples/public/app/samples_main.h"
#include "jni/SamplesMain_jni.h"

using base::LazyInstance;
using base::android::JavaParamRef;

namespace {

//LazyInstance<std::unique_ptr<service_manager::MainDelegate>>::DestructorAtExit
//    g_service_manager_main_delegate = LAZY_INSTANCE_INITIALIZER;
// 
//LazyInstance<std::unique_ptr<ContentMainDelegate>>::DestructorAtExit
//    g_content_main_delegate = LAZY_INSTANCE_INITIALIZER;
//
//}

} // namespace

namespace samples {

static jint JNI_SamplesMain_Start(JNIEnv* env,
                                  const JavaParamRef<jclass>& clazz,
                                  jboolean start_service_manager_only) {
  return 0;
}

} // namesapce samples
