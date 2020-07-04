#include "android/art/art_method.h"
#include "well_known_classes.h"

namespace whale {
namespace art {

jobject ArtMethod::Clone(JNIEnv *env, u4 access_flags) {
    int32_t api_level = GetAndroidApiLevel();
    jmethodID jni_clone_method = nullptr;
    if (api_level < ANDROID_M) {
        jni_clone_method =
                reinterpret_cast<jmethodID>(ArtRuntime::Get()->CloneArtObject(jni_method_));
    } else {
        jni_clone_method = reinterpret_cast<jmethodID>(malloc(offset_->method_size_));
        if (symbols_->ArtMethod_CopyFrom) {
            symbols_->ArtMethod_CopyFrom(jni_clone_method, jni_method_, sizeof(ptr_t));
        } else {
            memcpy(jni_clone_method, jni_method_, offset_->method_size_);
        }
    }

    ArtMethod clone_method = ArtMethod(jni_clone_method);
    bool is_direct_method = (access_flags & kAccDirectFlags) != 0;
    bool is_native_method = (access_flags & kAccNative) != 0;
    if (!is_direct_method) {
        access_flags &= ~(kAccPublic | kAccProtected);
        access_flags |= kAccPrivate;
    }
    access_flags &= ~kAccSynchronized;
    if (api_level < ANDROID_O_MR1) {
        access_flags |= kAccCompileDontBother_N;
    } else {
        access_flags |= kAccCompileDontBother_O_MR1;
        access_flags |= kAccPreviouslyWarm_O_MR1;
    }
    if (!is_native_method) {
        access_flags |= kAccSkipAccessChecks;
    }
    if (api_level >= ANDROID_N) {
        clone_method.SetHotnessCount(0);
        if (!is_native_method) {
            ptr_t profiling_info = GetEntryPointFromJni();
            if (profiling_info != nullptr) {
                offset_t end = sizeof(u4) * 4;
                for (offset_t offset = 0; offset != end; offset += sizeof(u4)) {
                    if (MemberOf<ptr_t>(profiling_info, offset) == jni_method_) {
                        AssignOffset<ptr_t>(profiling_info, offset, jni_clone_method);
                    }
                }
            }
        }
    }
    if (!is_native_method && symbols_->art_quick_to_interpreter_bridge) {
        clone_method.SetEntryPointFromQuickCompiledCode(
                symbols_->art_quick_to_interpreter_bridge);
    }

    clone_method.SetAccessFlags(access_flags);

    bool is_constructor = (access_flags & kAccConstructor) != 0;
    bool is_static = (access_flags & kAccStatic) != 0;
    if (is_constructor) {
        clone_method.RemoveAccessFlags(kAccConstructor);
    }
    jobject java_method = env->ToReflectedMethod(WellKnownClasses::java_lang_Object,
                                                 jni_clone_method,
                                                 static_cast<jboolean>(is_static));
    env->CallVoidMethod(java_method,
                        WellKnownClasses::java_lang_reflect_AccessibleObject_setAccessible,
                        true);
    if (is_constructor) {
        clone_method.AddAccessFlags(kAccConstructor);
    }
    return java_method;
}


}  // namespace art
}  // namespace whale
