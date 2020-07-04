#ifndef WHALE_ANDROID_ART_SYMBOL_RESOLVER_H_
#define WHALE_ANDROID_ART_SYMBOL_RESOLVER_H_

#include <jni.h>
#include "platform/linux/elf_image.h"
#include "base/primitive_types.h"

namespace whale {
namespace art {

struct ResolvedSymbols {
    const char *(*Art_GetMethodShorty)(JNIEnv *, jmethodID);

    void (*Dbg_SuspendVM)();

    void (*Dbg_ResumeVM)();

    void *art_quick_to_interpreter_bridge;
    void *artInterpreterToCompiledCodeBridge;

    void (*ProfileSaver_ForceProcessProfiles)();

    void (*ArtMethod_CopyFrom)(jmethodID this_ptr, jmethodID from, size_t num_bytes);

    ptr_t (*Thread_DecodeJObject)(ptr_t thread, jobject obj);

    ptr_t (*Object_Clone)(ptr_t object_this, ptr_t thread);

    ptr_t (*Object_CloneWithClass)(ptr_t object_this, ptr_t thread, ptr_t cls);

    ptr_t (*Object_CloneWithSize)(ptr_t object_this, ptr_t thread, size_t num_bytes);

    jobject (*JniEnvExt_NewLocalRef)(JNIEnv *jnienv_ext_this, ptr_t art_object);

};

class ArtSymbolResolver {
 public:
    ArtSymbolResolver() = default;

    bool Resolve(void *elf_image, s4 api_level);

    ResolvedSymbols *GetSymbols() {
        return &symbols_;
    };

 private:
    ResolvedSymbols symbols_;
};

}  // namespace art
}  // namespace whale

#endif  // WHALE_ANDROID_ART_SYMBOL_RESOLVER_H_
