#include "samples/shell/android/shell_launcher.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/lazy_instance.h"

#include "jni/ShellLauncher_jni.h"

using base::android::JavaParamRef;

namespace {

struct GlobalState {
  GlobalState() {}
  base::android::ScopedJavaGlobalRef<jobject> j_shell_launcher;
};

base::LazyInstance<GlobalState>::DestructorAtExit g_global_state =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace samples {

namespace shell {

void Destroy() {
  JNIEnv* env = base::android::AttachCurrentThread();
  shell::Java_ShellLauncher_destroy(env, g_global_state.Get().j_shell_launcher);
}

static void JNI_ShellLauncher_Init(JNIEnv* env,
                                  const JavaParamRef<jclass>& clazz,
                                  const JavaParamRef<jobject>& obj) {
  g_global_state.Get().j_shell_launcher.Reset(obj);
}

void JNI_ShellLauncher_LaunchShell(JNIEnv* env,
                                  const JavaParamRef<jclass>& clazz) {
}

} // namespace shell

} // namespace samples
