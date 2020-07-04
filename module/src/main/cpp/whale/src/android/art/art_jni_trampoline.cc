#include <cstdarg>
#include <jni.h>
#include "android/art/art_jni_trampoline.h"
#include "android/art/java_types.h"
#include "android/art/well_known_classes.h"
#include "android/art/art_runtime.h"
#include "platform/memory.h"
#include "base/macros.h"
#include "ffi_cxx.h"

namespace whale {
namespace art {

static void UnBoxValue(JNIEnv *env, jvalue *jv, jobject obj, char type) {
    if (obj == nullptr) {
        jv->l = obj;
        return;
    }
    switch (type) {
        case 'I':
            jv->i = Types::FromInteger(env, obj);
            break;
        case 'Z':
            jv->z = Types::FromBoolean(env, obj);
            break;
        case 'J':
            jv->j = Types::FromLong(env, obj);
            break;
        case 'F':
            jv->f = Types::FromFloat(env, obj);
            break;
        case 'B':
            jv->b = Types::FromByte(env, obj);
            break;
        case 'D':
            jv->d = Types::FromDouble(env, obj);
            break;
        case 'S':
            jv->s = Types::FromShort(env, obj);
            break;
        case 'C':
            jv->c = Types::FromCharacter(env, obj);
            break;
        default:
            jv->l = obj;
    }
}

template<typename RetType>
static RetType
InvokeJavaBridge(JNIEnv *env, ArtHookParam *param, jobject this_object,
                 jobjectArray arguments) {
    jobject ret = ArtRuntime::Get()->InvokeHookedMethodBridge(
            env,
            param,
            this_object,
            arguments
    );
    jvalue val;
    UnBoxValue(env, &val, ret, param->shorty_[0]);
    return ForceCast<RetType>(val);
}

static void InvokeVoidJavaBridge(JNIEnv *env, ArtHookParam *param, jobject this_object,
                                 jobjectArray arguments) {
    ArtRuntime::Get()->InvokeHookedMethodBridge(
            env,
            param,
            this_object,
            arguments
    );
}

class QuickArgumentBuilder {
 public:
    QuickArgumentBuilder(JNIEnv *env, size_t len) : env_(env), index_(0) {
        array_ = env->NewObjectArray(
                static_cast<jsize>(len),
                WellKnownClasses::java_lang_Object,
                nullptr
        );
    }

#define APPEND_DEF(name, type)  \
    void Append##name(type value) {  \
        env_->SetObjectArrayElement(array_, index_++,  \
                                    Types::To##name(env_, value));  \
    }  \


    APPEND_DEF(Integer, jint)

    APPEND_DEF(Boolean, jboolean)

    APPEND_DEF(Byte, jbyte)

    APPEND_DEF(Character, jchar)

    APPEND_DEF(Short, jshort)

    APPEND_DEF(Float, jfloat)

    APPEND_DEF(Double, jdouble)

    APPEND_DEF(Long, jlong)

    APPEND_DEF(Object, jobject)


#undef APPEND_DEF

    jobjectArray GetArray() {
        return array_;
    }

 private:
    JNIEnv *env_;
    jobjectArray array_;
    int index_;
};

FFIType FFIGetJniParameter(char shorty) {
    switch (shorty) {
        case 'Z':
            return FFIType::kFFITypeU1;
        case 'B':
            return FFIType::kFFITypeS1;
        case 'C':
            return FFIType::kFFITypeU2;
        case 'S':
            return FFIType::kFFITypeS2;
        case 'I':
            return FFIType::kFFITypeS4;
        case 'J':
            return FFIType::kFFITypeS8;
        case 'F':
            return FFIType::kFFITypeFloat;
        case 'D':
            return FFIType::kFFITypeDouble;
        case 'L':
            return FFIType::kFFITypePointer;
        case 'V':
            return FFIType::kFFITypeVoid;
        default:
            LOG(FATAL) << "unhandled shorty type: " << shorty;
            UNREACHABLE();
    }
}

void FFIJniDispatcher(FFIClosure *closure, void *resp, void **args, void *userdata) {
#define FFI_ARG(name, type)  \
    builder.Append##name(*reinterpret_cast<type *>(args[i]));

    ArtHookParam *param = reinterpret_cast<ArtHookParam *>(userdata);
    const char *argument = param->shorty_ + 1;
    unsigned int argument_len = (unsigned int) strlen(argument);
    JNIEnv *env = *reinterpret_cast<JNIEnv **>(args[0]);
    jobject this_object = nullptr;
    if (!param->is_static_) {
        this_object = *reinterpret_cast<jobject *>(args[1]);
    }
    // skip first two arguments
    args += 2;
    QuickArgumentBuilder builder(env, argument_len);

    for (int i = 0; i < argument_len; ++i) {
        switch (argument[i]) {
            case 'Z':
                FFI_ARG(Boolean, jboolean);
                break;
            case 'B':
                FFI_ARG(Byte, jbyte);
                break;
            case 'C':
                FFI_ARG(Character, jchar);
                break;
            case 'S':
                FFI_ARG(Short, jshort);
                break;
            case 'I':
                FFI_ARG(Integer, jint);
                break;
            case 'J':
                FFI_ARG(Long, jlong);
                break;
            case 'F':
                FFI_ARG(Float, jfloat);
                break;
            case 'D':
                FFI_ARG(Double, jdouble);
                break;
            case 'L':
                FFI_ARG(Object, jobject);
                break;
            default:
                LOG(FATAL) << "unhandled shorty type: " << argument[i];
                UNREACHABLE();
        }
    }
#define INVOKE(type)  \
    *reinterpret_cast<type *>(resp) = InvokeJavaBridge<type>(env, param, this_object,  \
                                                                    builder.GetArray());

    switch (param->shorty_[0]) {
        case 'Z':
            INVOKE(jboolean);
            break;
        case 'B':
            INVOKE(jbyte);
            break;
        case 'C':
            INVOKE(jchar);
            break;
        case 'S':
            INVOKE(jshort);
            break;
        case 'I':
            INVOKE(jint);
            break;
        case 'J':
            INVOKE(jlong);
            break;
        case 'F':
            INVOKE(jfloat);
            break;
        case 'D':
            INVOKE(jdouble);
            break;
        case 'L':
            INVOKE(jobject);
            break;
        case 'V':
            InvokeVoidJavaBridge(env, param, this_object, builder.GetArray());
            break;
        default:
            LOG(FATAL) << "unhandled shorty type: " << param->shorty_[0];
            UNREACHABLE();
    }
#undef INVOKE
#undef FFI_ARG
}


void BuildJniClosure(ArtHookParam *param) {
    const char *argument = param->shorty_ + 1;
    unsigned int java_argument_len = (unsigned int) strlen(argument);
    unsigned int jni_argument_len = java_argument_len + 2;
    FFICallInterface *cif = new FFICallInterface(
            FFIGetJniParameter(param->shorty_[0])
    );
    cif->Parameter(FFIType::kFFITypePointer);  // JNIEnv *
    cif->Parameter(FFIType::kFFITypePointer);  // jclass or jobject
    for (int i = 2; i < jni_argument_len; ++i) {
        cif->Parameter(FFIGetJniParameter(argument[i - 2]));
    }
    cif->FinalizeCif();
    param->jni_closure_ = cif->CreateClosure(param, FFIJniDispatcher);
}

}  // namespace art
}  // namespace whale
