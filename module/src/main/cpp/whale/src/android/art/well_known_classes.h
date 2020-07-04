#ifndef WHALE_ANDROID_ART_WELL_KNOWN_CLASSES_H_
#define WHALE_ANDROID_ART_WELL_KNOWN_CLASSES_H_

#include <jni.h>

namespace whale {
namespace art {

struct WellKnownClasses {
    static void Load(JNIEnv *env);

    static jclass java_lang_Object;
    static jclass java_lang_reflect_Method;
    static jclass java_lang_Class;
    static jclass java_lang_ClassLoader;
    static jclass java_lang_reflect_AccessibleObject;
    static jclass java_lang_Thread;
    static jclass java_lang_IllegalArgumentException;

    static jmethodID java_lang_reflect_Method_invoke;
    static jmethodID java_lang_Class_getClassLoader;
    static jmethodID java_lang_reflect_AccessibleObject_setAccessible;
    static jmethodID java_lang_Thread_currentThread;

    static jfieldID java_lang_Thread_nativePeer;
};

}  // namespace art
}  // namespace whale

#endif  // WHALE_ANDROID_ART_WELL_KNOWN_CLASSES_H_
