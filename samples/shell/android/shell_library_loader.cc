#include "base/android/jni_android.h"
#include "base/android/jni_utils.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "base/bind.h"

#include "samples/shell/android/samples_shell_jni_registration.h"

// This is called by the VM when the shared library is first loaded.
JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  base::android::InitVM(vm);
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!RegisterMainDexNatives(env)) {
    return -1;
  }

  bool is_browser_process =
      !base::android::IsSelectiveJniRegistrationEnabled(env);
  if (is_browser_process && !RegisterNonMainDexNatives(env)) {
    return -1;
  }

  return JNI_VERSION_1_4;
}
