#ifndef WHALE_ANDROID_ART__SCOPED_THREAD_STATE_CHANGE_H_
#define WHALE_ANDROID_ART__SCOPED_THREAD_STATE_CHANGE_H_

#include <jni.h>

namespace whale {
namespace art {

class ScopedNoGCDaemons {
    static jclass java_lang_Daemons;
    static jmethodID java_lang_Daemons_start;
    static jmethodID java_lang_Daemons_stop;

 public:
    static void Load(JNIEnv *env);

    ScopedNoGCDaemons(JNIEnv *env);

    ~ScopedNoGCDaemons();

 private:
    JNIEnv *env_;
};

class ScopedSuspendAll {
 public:
    ScopedSuspendAll();

    ~ScopedSuspendAll();
};


}  // namespace art
}  // namespace whale

#endif  // WHALE_ANDROID_ART__SCOPED_THREAD_STATE_CHANGE_H_

