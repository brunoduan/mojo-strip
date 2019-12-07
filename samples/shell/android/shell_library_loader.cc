#include "base/android/jni_android.h"
#include "base/android/jni_utils.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "base/bind.h"

// This is called by the VM when the shared library is first loaded.
JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  base::android::InitVM(vm);

  return JNI_VERSION_1_4;
}
