#include "whale.h"
#include "android/art/art_symbol_resolver.h"
#include "android/android_build.h"
#include "android/art/art_runtime.h"

#define SYMBOL static constexpr const char *

namespace whale {
namespace art {

// art::ArtMethod::CopyFrom(art::ArtMethod*, art::PointerSize)
SYMBOL kArtMethod_CopyFrom_O = "_ZN3art9ArtMethod8CopyFromEPS0_NS_11PointerSizeE";
#if defined(__LP64__)
// art::ArtMethod::CopyFrom(art::ArtMethod*, unsigned long)
SYMBOL kArtMethod_CopyFrom_N = "_ZN3art9ArtMethod8CopyFromEPS0_m";
// art::ArtMethod::CopyFrom(art::ArtMethod const*, unsigned long)
SYMBOL kArtMethod_CopyFrom_M = "_ZN3art9ArtMethod8CopyFromEPKS0_m";
#else
// art::ArtMethod::CopyFrom(art::ArtMethod*, unsigned int)
SYMBOL kArtMethod_CopyFrom_N = "_ZN3art9ArtMethod8CopyFromEPS0_j";
// art::ArtMethod::CopyFrom(art::ArtMethod const*, unsigned int)
SYMBOL kArtMethod_CopyFrom_M = "_ZN3art9ArtMethod8CopyFromEPKS0_j";
#endif

// art::GetMethodShorty(_JNIEnv*, _jmethodID*)
SYMBOL kArt_GetMethodShorty = "_ZN3artL15GetMethodShortyEP7_JNIEnvP10_jmethodID";
SYMBOL kArt_GetMethodShorty_Legacy = "_ZN3art15GetMethodShortyEP7_JNIEnvP10_jmethodID";

// art::Dbg::SuspendVM()
SYMBOL kDbg_SuspendVM = "_ZN3art3Dbg9SuspendVMEv";
// art::Dbg::ResumeVM()
SYMBOL kDbg_ResumeVM = "_ZN3art3Dbg8ResumeVMEv";

// art_quick_to_interpreter_bridge()
SYMBOL kArt_art_quick_to_interpreter_bridge = "art_quick_to_interpreter_bridge";

// artInterpreterToCompiledCodeBridge
SYMBOL kArt_artInterpreterToCompiledCodeBridge = "artInterpreterToCompiledCodeBridge";

// art::ProfileSaver::ForceProcessProfiles()
SYMBOL kArt_ProfileSaver_ForceProcessProfiles = "_ZN3art12ProfileSaver20ForceProcessProfilesEv";

#if defined(__LP64__)
// art::LinearAlloc::Alloc(art::Thread*, unsigned int)
SYMBOL kArt_LinearAlloc_Alloc = "_ZN3art11LinearAlloc5AllocEPNS_6ThreadEm";
#else
SYMBOL kArt_LinearAlloc_Alloc = "_ZN3art11LinearAlloc5AllocEPNS_6ThreadEj";
#endif
// art::Thread::DecodeJObject(_jobject*) const
SYMBOL kArt_DecodeJObject = "_ZNK3art6Thread13DecodeJObjectEP8_jobject";

// art::ClassLinker::EnsureInitialized(art::Thread*, art::Handle<art::mirror::Class>, bool, bool)
SYMBOL kArt_EnsureInitialized = "_ZN3art11ClassLinker17EnsureInitializedEPNS_6ThreadENS_6HandleINS_6mirror5ClassEEEbb";

// art::mirror::Object::Clone(art::Thread*)
SYMBOL kArt_Object_Clone = "_ZN3art6mirror6Object5CloneEPNS_6ThreadE";

// art::mirror::Object::Clone(art::Thread*, art::mirror::Class*)
SYMBOL kArt_Object_CloneWithClass = "_ZN3art6mirror6Object5CloneEPNS_6ThreadEPNS0_5ClassE";

#if defined(__LP64__)
// art::mirror::Object::Clone(art::Thread*, unsigned long)
SYMBOL kArt_Object_CloneWithSize = "_ZN3art6mirror6Object5CloneEPNS_6ThreadEm";
#else
// art::mirror::Object::Clone(art::Thread*, unsigned int)
SYMBOL kArt_Object_CloneWithSize = "_ZN3art6mirror6Object5CloneEPNS_6ThreadEj";
#endif

SYMBOL kArt_JniEnvExt_NewLocalRef = "_ZN3art9JNIEnvExt11NewLocalRefEPNS_6mirror6ObjectE";


bool ArtSymbolResolver::Resolve(void *elf_image, s4 api_level) {
#define FIND_SYMBOL(symbol, decl, ret)  \
        if ((decl = reinterpret_cast<typeof(decl)>(WDynamicLibSymbol(elf_image, symbol))) == nullptr) {  \
        if (ret) {  \
            LOG(ERROR) << "Failed to resolve symbol : " << #symbol;  \
            return false;  \
         } \
        }
    FIND_SYMBOL(kArt_GetMethodShorty, symbols_.Art_GetMethodShorty, false);
    if (symbols_.Art_GetMethodShorty == nullptr) {
        FIND_SYMBOL(kArt_GetMethodShorty_Legacy, symbols_.Art_GetMethodShorty, false);
    }
    if (api_level < ANDROID_N) {
        FIND_SYMBOL(kArt_artInterpreterToCompiledCodeBridge,
                    symbols_.artInterpreterToCompiledCodeBridge, false);
    }
    FIND_SYMBOL(kDbg_SuspendVM, symbols_.Dbg_SuspendVM, false);
    FIND_SYMBOL(kDbg_ResumeVM, symbols_.Dbg_ResumeVM, false);
    FIND_SYMBOL(kArt_art_quick_to_interpreter_bridge, symbols_.art_quick_to_interpreter_bridge,
                false);
    if (api_level > ANDROID_N) {
        FIND_SYMBOL(kArt_ProfileSaver_ForceProcessProfiles,
                    symbols_.ProfileSaver_ForceProcessProfiles,
                    false);
    }
    if (api_level > ANDROID_O) {
        FIND_SYMBOL(kArtMethod_CopyFrom_O, symbols_.ArtMethod_CopyFrom, false);
    } else if (api_level > ANDROID_N) {
        FIND_SYMBOL(kArtMethod_CopyFrom_N, symbols_.ArtMethod_CopyFrom, false);
    } else {
        FIND_SYMBOL(kArtMethod_CopyFrom_M, symbols_.ArtMethod_CopyFrom, false);
    }
    FIND_SYMBOL(kArt_Object_Clone, symbols_.Object_Clone, false);
    if (symbols_.Object_Clone == nullptr) {
        FIND_SYMBOL(kArt_Object_CloneWithSize, symbols_.Object_CloneWithSize, false);
    }
    if (symbols_.Object_Clone == nullptr) {
        FIND_SYMBOL(kArt_Object_CloneWithClass, symbols_.Object_CloneWithClass, true);
    }
    FIND_SYMBOL(kArt_DecodeJObject, symbols_.Thread_DecodeJObject, true);
    FIND_SYMBOL(kArt_JniEnvExt_NewLocalRef, symbols_.JniEnvExt_NewLocalRef, true);
    return true;
#undef FIND_SYMBOL
}

}  // namespace art
}  // namespace whale
