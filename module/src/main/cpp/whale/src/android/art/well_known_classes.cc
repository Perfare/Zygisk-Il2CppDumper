#include "android/art/well_known_classes.h"
#include "android/jni_helper.h"
#include "base/logging.h"
#include "base/stringprintf.h"

namespace whale {
namespace art {

jclass WellKnownClasses::java_lang_Object;
jclass WellKnownClasses::java_lang_reflect_Method;
jclass WellKnownClasses::java_lang_Class;
jclass WellKnownClasses::java_lang_ClassLoader;
jclass WellKnownClasses::java_lang_reflect_AccessibleObject;
jclass WellKnownClasses::java_lang_Thread;
jclass WellKnownClasses::java_lang_IllegalArgumentException;

jmethodID WellKnownClasses::java_lang_reflect_Method_invoke;
jmethodID WellKnownClasses::java_lang_Class_getClassLoader;
jmethodID WellKnownClasses::java_lang_reflect_AccessibleObject_setAccessible;
jmethodID WellKnownClasses::java_lang_Thread_currentThread;

jfieldID WellKnownClasses::java_lang_Thread_nativePeer;

static jclass CacheClass(JNIEnv *env, const char *jni_class_name) {
    ScopedLocalRef<jclass> c(env, env->FindClass(jni_class_name));
    if (c.get() == nullptr) {
        LOG(FATAL) << "Couldn't find class: " << jni_class_name;
    }
    return reinterpret_cast<jclass>(env->NewGlobalRef(c.get()));
}

static jmethodID CacheMethod(JNIEnv *env, jclass c, bool is_static,
                             const char *name, const char *signature) {
    jmethodID mid = is_static ? env->GetStaticMethodID(c, name, signature) :
                    env->GetMethodID(c, name, signature);
    if (mid == nullptr) {
        LOG(FATAL) << "Couldn't find method \"" << name << "\" with signature \"" << signature;
        if (env->ExceptionCheck()) {
            env->ExceptionOccurred();
        }
    }
    return mid;
}

static jfieldID CacheField(JNIEnv *env, jclass c, bool is_static,
                           const char *name, const char *signature, bool weak = false) {
    jfieldID fid = is_static ? env->GetStaticFieldID(c, name, signature) :
                   env->GetFieldID(c, name, signature);
    if (fid == nullptr) {
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        if (weak) {
            LOG(FATAL) << "Couldn't find field \"" << name << "\" with signature \"" << signature;
        }
    }
    return fid;
}

static jmethodID CacheMethod(JNIEnv *env, const char *klass, bool is_static,
                             const char *name, const char *signature) {
    ScopedLocalRef<jclass> java_class(env, env->FindClass(klass));
    return CacheMethod(env, java_class.get(), is_static, name, signature);
}

static jmethodID CachePrimitiveBoxingMethod(JNIEnv *env, char prim_name, const char *boxed_name) {
    ScopedLocalRef<jclass> boxed_class(env, env->FindClass(boxed_name));
    return CacheMethod(env, boxed_class.get(), true, "valueOf",
                       StringPrintf("(%c)L%s;", prim_name, boxed_name).c_str());
}


void WellKnownClasses::Load(JNIEnv *env) {
    java_lang_Object = CacheClass(env, "java/lang/Object");
    java_lang_reflect_Method = CacheClass(env, "java/lang/reflect/Method");
    java_lang_Class = CacheClass(env, "java/lang/Class");
    java_lang_ClassLoader = CacheClass(env, "java/lang/ClassLoader");
    java_lang_reflect_AccessibleObject = CacheClass(env, "java/lang/reflect/AccessibleObject");
    java_lang_Thread = CacheClass(env, "java/lang/Thread");
    java_lang_IllegalArgumentException = CacheClass(env, "java/lang/IllegalArgumentException");

    java_lang_Thread_currentThread = CacheMethod(env, java_lang_Thread, true, "currentThread",
                                                 "()Ljava/lang/Thread;");
    java_lang_reflect_Method_invoke = CacheMethod(env, java_lang_reflect_Method, false, "invoke",
                                                  "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
    java_lang_Class_getClassLoader = CacheMethod(env, java_lang_Class,
                                                             false,
                                                             "getClassLoader",
                                                             "()Ljava/lang/ClassLoader;");
    java_lang_reflect_AccessibleObject_setAccessible = CacheMethod(env,
                                                                   java_lang_reflect_AccessibleObject,
                                                                   false,
                                                                   "setAccessible",
                                                                   "(Z)V");

    java_lang_Thread_nativePeer = CacheField(env, java_lang_Thread, false, "nativePeer", "J", true);
}


}  // namespace art
}  // namespace whale
