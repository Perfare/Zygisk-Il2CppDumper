#ifndef WHALE_ANDROID_ART_ART_METHOD_H_
#define WHALE_ANDROID_ART_ART_METHOD_H_

#include <jni.h>
#include <android/android_build.h>
#include "android/art/art_runtime.h"
#include "android/art/modifiers.h"
#include "base/cxx_helper.h"
#include "base/primitive_types.h"

namespace whale {
namespace art {

class ArtMethod final {
 public:
    explicit ArtMethod(jmethodID method) : jni_method_(method) {
        ArtRuntime *runtime = ArtRuntime::Get();
        offset_ = runtime->GetArtMethodOffsets();
        symbols_ = runtime->GetSymbols();
    }

    u4 GetAccessFlags() {
        return MemberOf<u4>(jni_method_, offset_->access_flags_offset_);
    }

    void SetAccessFlags(u4 access_flags) {
        AssignOffset<u4>(jni_method_, offset_->access_flags_offset_, access_flags);
    }

    void AddAccessFlags(u4 access_flags) {
        SetAccessFlags(GetAccessFlags() | access_flags);
    }

    void RemoveAccessFlags(u4 access_flags) {
        SetAccessFlags(GetAccessFlags() & ~access_flags);
    }

    bool HasAccessFlags(u4 access_flags) {
        return (access_flags & GetAccessFlags()) != 0;
    }

    u2 GetMethodIndex() {
        return MemberOf<u2>(jni_method_, offset_->method_index_offset_);
    }

    u4 GetDexMethodIndex() {
        return MemberOf<u4>(jni_method_, offset_->dex_method_index_offset_);
    }

    u4 GetDexCodeItemOffset() {
        return MemberOf<u4>(jni_method_, offset_->dex_code_item_offset_offset_);
    }

    u2 GetHotnessCount() {
        return MemberOf<u2>(jni_method_, offset_->hotness_count_offset_);
    }

    void SetMethodIndex(u2 index) {
        AssignOffset<u2>(jni_method_, offset_->method_index_offset_, index);
    }

    void SetDexMethodIndex(u4 index) {
        AssignOffset<u4>(jni_method_, offset_->dex_method_index_offset_, index);
    }

    void SetDexCodeItemOffset(u4 offset) {
        AssignOffset<u4>(jni_method_, offset_->dex_code_item_offset_offset_, offset);
    }

    void SetHotnessCount(u2 count) {
        AssignOffset<u2>(jni_method_, offset_->hotness_count_offset_, count);
    }

    ptr_t GetEntryPointFromQuickCompiledCode() {
        return MemberOf<ptr_t>(jni_method_, offset_->quick_code_offset_);
    }

    void SetEntryPointFromQuickCompiledCode(ptr_t entry_point) {
        AssignOffset<ptr_t>(jni_method_, offset_->quick_code_offset_, entry_point);
    }

    ptr_t GetEntryPointFromInterpreterCode() {
        return MemberOf<ptr_t>(jni_method_, offset_->interpreter_code_offset_);
    }

    void SetEntryPointFromInterpreterCode(ptr_t entry_point) {
        AssignOffset<ptr_t>(jni_method_, offset_->interpreter_code_offset_, entry_point);
    }

    ptr_t GetEntryPointFromJni() {
        return MemberOf<ptr_t>(jni_method_, offset_->jni_code_offset_);
    }

    void SetEntryPointFromJni(ptr_t entry_point) {
        AssignOffset<ptr_t>(jni_method_, offset_->jni_code_offset_, entry_point);
    }

    /*
     * Notice: This is a GcRoot reference.
     */
    ptr_t GetDeclaringClass() {
        return MemberOf<ptr_t>(jni_method_, 0);
    }

    void SetDeclaringClass(ptr_t declaring_class) {
        AssignOffset<ptr_t>(jni_method_, 0, declaring_class);
    }

    const char *GetShorty(JNIEnv *env, jobject java_method) {
        if (symbols_->Art_GetMethodShorty != nullptr) {
            return symbols_->Art_GetMethodShorty(env, jni_method_);
        } else {
            static jmethodID WhaleRuntime_getShorty = nullptr;
            jclass java_class = ArtRuntime::Get()->java_class_;
            if (WhaleRuntime_getShorty == nullptr) {
                WhaleRuntime_getShorty = env->GetStaticMethodID(
                        java_class,
                        "getShorty",
                        "(Ljava/lang/reflect/Member;)Ljava/lang/String;"
                );
            }
            jstring jshorty = static_cast<jstring>(env->CallStaticObjectMethod(
                    java_class,
                    WhaleRuntime_getShorty,
                    java_method
            ));
            const char *shorty = env->GetStringUTFChars(jshorty, nullptr);
            return shorty;
        }
    }

    jobject Clone(JNIEnv *env, u4 access_flags);

 private:
    jmethodID jni_method_;
    // Convenient for quick invocation
    ArtMethodOffsets *offset_;
    ResolvedSymbols *symbols_;
};


}  // namespace art
}  // namespace whale

#endif  // WHALE_ANDROID_ART_ART_METHOD_H_
