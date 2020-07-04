#ifndef WHALE_ANDROID_ART_JAVA_TYPES_H_
#define WHALE_ANDROID_ART_JAVA_TYPES_H_

#include <jni.h>
#include "android/art/art_runtime.h"


namespace whale {
namespace art {

struct Types {
#define LANG_ClASS(c) static jclass java_lang_##c; static jmethodID java_lang_##c##_init; static jmethodID java_value_##c;

    LANG_ClASS(Integer);
    LANG_ClASS(Long);
    LANG_ClASS(Float);
    LANG_ClASS(Double);
    LANG_ClASS(Byte);
    LANG_ClASS(Short);
    LANG_ClASS(Boolean);
    LANG_ClASS(Character);

#undef LANG_ClASS
    static void Load(JNIEnv *env);


#define LANG_BOX_DEF(c, t) static jobject To##c(JNIEnv *env, t v);

#define LANG_UNBOX_V_DEF(k, c, t) static t From##c(JNIEnv *env, jobject j);

#define LANG_UNBOX_DEF(c, t) LANG_UNBOX_V_DEF(c, c, t)

    LANG_BOX_DEF(Object, jobject);
    LANG_BOX_DEF(Integer, jint);
    LANG_BOX_DEF(Long, jlong);
    LANG_BOX_DEF(Float, jfloat);
    LANG_BOX_DEF(Double, jdouble);
    LANG_BOX_DEF(Byte, jbyte);
    LANG_BOX_DEF(Short, jshort);
    LANG_BOX_DEF(Boolean, jboolean);
    LANG_BOX_DEF(Character, jchar);

    LANG_UNBOX_V_DEF(Int, Integer, jint);
    LANG_UNBOX_DEF(Object, jobject);
    LANG_UNBOX_DEF(Long, jlong);
    LANG_UNBOX_DEF(Float, jfloat);
    LANG_UNBOX_DEF(Double, jdouble);
    LANG_UNBOX_DEF(Byte, jbyte);
    LANG_UNBOX_DEF(Short, jshort);
    LANG_UNBOX_DEF(Boolean, jboolean);
    LANG_UNBOX_V_DEF(Char, Character, jchar);

#undef LANG_BOX_DEF
#undef LANG_UNBOX_V_DEF
#undef LANG_UNBOX_DEF
};

}  // namespace art
}  // namespace whale

#endif  // WHALE_ANDROID_ART_JAVA_TYPES_H_
