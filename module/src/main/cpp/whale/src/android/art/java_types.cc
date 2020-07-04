#include "android/art/java_types.h"

namespace whale {
namespace art {

#define EXPORT_LANG_ClASS(c) jclass Types::java_lang_##c;  jmethodID Types::java_lang_##c##_init; jmethodID Types::java_value_##c;

EXPORT_LANG_ClASS(Integer);
EXPORT_LANG_ClASS(Long);
EXPORT_LANG_ClASS(Float);
EXPORT_LANG_ClASS(Double);
EXPORT_LANG_ClASS(Byte);
EXPORT_LANG_ClASS(Short);
EXPORT_LANG_ClASS(Boolean);
EXPORT_LANG_ClASS(Character);

#undef EXPORT_LANG_ClASS

void Types::Load(JNIEnv *env) {
    jclass clazz;
    env->PushLocalFrame(16);

#define LOAD_CLASS(c, s)        clazz = env->FindClass(s); c = reinterpret_cast<jclass>(env->NewWeakGlobalRef(clazz))
#define LOAD_LANG_CLASS(c, s)   LOAD_CLASS(java_lang_##c, "java/lang/" #c); java_lang_##c##_init = env->GetMethodID(java_lang_##c, "<init>", s)

    LOAD_LANG_CLASS(Integer, "(I)V");
    LOAD_LANG_CLASS(Long, "(J)V");
    LOAD_LANG_CLASS(Float, "(F)V");
    LOAD_LANG_CLASS(Double, "(D)V");
    LOAD_LANG_CLASS(Byte, "(B)V");
    LOAD_LANG_CLASS(Short, "(S)V");
    LOAD_LANG_CLASS(Boolean, "(Z)V");
    LOAD_LANG_CLASS(Character, "(C)V");

#undef LOAD_CLASS
#undef LOAD_LANG_CLASS

#define LOAD_METHOD(k, c, r, s) java_value_##c = env->GetMethodID(k, r "Value", s)
#define LOAD_NUMBER(c, r, s)    LOAD_METHOD(java_lang_Number, c, r, s)

    jclass java_lang_Number = env->FindClass("java/lang/Number");

    LOAD_NUMBER(Integer, "int", "()I");
    LOAD_NUMBER(Long, "long", "()J");
    LOAD_NUMBER(Float, "float", "()F");
    LOAD_NUMBER(Double, "double", "()D");
    LOAD_NUMBER(Byte, "byte", "()B");
    LOAD_NUMBER(Short, "short", "()S");

    LOAD_METHOD(java_lang_Boolean, Boolean, "boolean", "()Z");
    LOAD_METHOD(java_lang_Character, Character, "char", "()C");

    env->PopLocalFrame(nullptr);
#undef LOAD_METHOD
#undef LOAD_NUMBER
}


#define LANG_BOX(c, t) jobject Types::To##c(JNIEnv *env, t v) {  \
        return env->NewObject(Types::java_lang_##c, Types::java_lang_##c##_init, v);  \
}
#define LANG_UNBOX_V(k, c, t) t Types::From##c(JNIEnv *env, jobject j) {  \
        return env->Call##k##Method(j, Types::java_value_##c);  \
}
#define LANG_UNBOX(c, t) LANG_UNBOX_V(c, c, t)

LANG_BOX(Integer, jint);
LANG_BOX(Long, jlong);
LANG_BOX(Float, jfloat);
LANG_BOX(Double, jdouble);
LANG_BOX(Byte, jbyte);
LANG_BOX(Short, jshort);
LANG_BOX(Boolean, jboolean);
LANG_BOX(Character, jchar);

jobject Types::ToObject(JNIEnv *env, jobject obj) {
    return obj;
}

LANG_UNBOX_V(Int, Integer, jint);
LANG_UNBOX(Long, jlong);
LANG_UNBOX(Float, jfloat);
LANG_UNBOX(Double, jdouble);
LANG_UNBOX(Byte, jbyte);
LANG_UNBOX(Short, jshort);
LANG_UNBOX(Boolean, jboolean);
LANG_UNBOX_V(Char, Character, jchar);

jobject Types::FromObject(JNIEnv *env, jobject obj) {
    return obj;
}


#undef LANG_BOX
#undef LANG_UNBOX_V
#undef LANG_UNBOX

}  // namespace art
}  // namespace whale
