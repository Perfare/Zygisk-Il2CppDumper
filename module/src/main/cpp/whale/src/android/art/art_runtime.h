#ifndef WHALE_ANDROID_ART_INTERCEPTOR_H_
#define WHALE_ANDROID_ART_INTERCEPTOR_H_

#include <jni.h>
#include <map>
#include "platform/linux/elf_image.h"
#include "platform/linux/process_map.h"
#include "dbi/instruction_set.h"
#include "android/art/art_symbol_resolver.h"
#include "android/art/art_hook_param.h"
#include "android/native_bridge.h"
#include "android/jni_helper.h"
#include "base/macros.h"
#include "base/primitive_types.h"

#if defined(__LP64__)
static constexpr const char *kAndroidLibDir = "/system/lib64/";
static constexpr const char *kLibNativeBridgePath = "/system/lib64/libnativebridge.so";
static constexpr const char *kLibArtPath = "/system/lib64/libart.so";
static constexpr const char *kLibAocPath = "/system/lib64/libaoc.so";
static constexpr const char *kLibHoudiniArtPath = "/system/lib64/arm64/libart.so";
#else
static constexpr const char *kAndroidLibDir = "/system/lib/";
static constexpr const char *kLibArtPath = "/system/lib/libart.so";
static constexpr const char *kLibAocPath = "/system/lib/libaoc.so";
static constexpr const char *kLibHoudiniArtPath = "/system/lib/arm/libart.so";
#endif


namespace whale {
namespace art {

class ArtThread;

struct ArtMethodOffsets final {
    size_t method_size_;
    offset_t jni_code_offset_;
    offset_t quick_code_offset_;
    offset_t OPTION interpreter_code_offset_;
    offset_t access_flags_offset_;
    offset_t dex_code_item_offset_offset_;
    offset_t dex_method_index_offset_;
    offset_t method_index_offset_;
    offset_t OPTION hotness_count_offset_;
};

struct RuntimeObjects final {
    ptr_t OPTION runtime_;
    ptr_t OPTION heap_;
    ptr_t OPTION thread_list_;
    ptr_t OPTION class_linker_;
    ptr_t OPTION intern_table_;
};

struct ClassLinkerObjects {
    ptr_t quick_generic_jni_trampoline_;
};

class ArtRuntime final {
 public:
    friend class ArtMethod;

    static ArtRuntime *Get();

    ArtRuntime() {}

    bool OnLoad(JavaVM *vm, JNIEnv *env, jclass java_class);

    jlong HookMethod(JNIEnv *env, jclass decl_class, jobject hooked_java_method,
                     jobject addition_info);

    JNIEnv *GetJniEnv() {
        JNIEnv *env = nullptr;
        jint ret = vm_->AttachCurrentThread(&env, nullptr);
        DCHECK_EQ(JNI_OK, ret);
        return env;
    }

    ArtMethodOffsets *GetArtMethodOffsets() {
        return &method_offset_;
    }


    RuntimeObjects *GetRuntimeObjects() {
        return &runtime_objects_;
    }

    ClassLinkerObjects *GetClassLinkerObjects() {
        return &class_linker_objects_;
    }

    ResolvedSymbols *GetSymbols() {
        return art_symbol_resolver_.GetSymbols();
    }

    ArtThread *GetCurrentArtThread();

    void EnsureClassInitialized(JNIEnv *env, jclass cl);


    jobject
    InvokeHookedMethodBridge(JNIEnv *env, ArtHookParam *param, jobject receiver,
                             jobjectArray array);

    jobject
    InvokeOriginalMethod(jlong slot, jobject this_object, jobjectArray args);

    jlong GetMethodSlot(JNIEnv *env, jclass cl, jobject method_obj);

    ALWAYS_INLINE void VisitInterceptParams(std::function<void(ArtHookParam *)> visitor) {
        for (auto &entry : hooked_method_map_) {
            visitor(entry.second);
        }
    }

    void SetObjectClass(JNIEnv *env, jobject obj, jclass cl);

    void SetObjectClassUnsafe(JNIEnv *env, jobject obj, jclass cl);

    jobject CloneToSubclass(JNIEnv *env, jobject obj, jclass sub_class);

    void RemoveFinalFlag(JNIEnv *env, jclass java_class);

    bool EnforceDisableHiddenAPIPolicy();

    ptr_t CloneArtObject(ptr_t art_object);

    void FixBugN();

 private:
    JavaVM *vm_;
    jclass java_class_;
    jmethodID bridge_method_;
    s4 api_level_;
    void *art_elf_image_;
    NativeBridgeCallbacks OPTION *android_bridge_callbacks_;
    ArtSymbolResolver art_symbol_resolver_;
    RuntimeObjects runtime_objects_;
    ClassLinkerObjects class_linker_objects_;
    ArtMethodOffsets method_offset_;
    std::map<jmethodID, ArtHookParam *> hooked_method_map_;
    pthread_mutex_t mutex;

    bool EnforceDisableHiddenAPIPolicyImpl();

    DISALLOW_COPY_AND_ASSIGN(ArtRuntime);
};


}  // namespace art
}  // namespace whale

#endif  // WHALE_ANDROID_ART_INTERCEPTOR_H_
