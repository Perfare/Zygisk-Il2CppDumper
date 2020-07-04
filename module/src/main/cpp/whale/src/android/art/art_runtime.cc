#include <dlfcn.h>
#include "whale.h"
#include "android/android_build.h"
#include "android/art/art_runtime.h"
#include "android/art/modifiers.h"
#include "android/art/native_on_load.h"
#include "android/art/art_method.h"
#include "android/art/art_symbol_resolver.h"
#include "android/art/scoped_thread_state_change.h"
#include "android/art/art_jni_trampoline.h"
#include "android/art/well_known_classes.h"
#include "android/art/java_types.h"
#include "platform/memory.h"
#include "base/logging.h"
#include "base/singleton.h"
#include "base/cxx_helper.h"

namespace whale {
namespace art {

ArtRuntime *ArtRuntime::Get() {
    static ArtRuntime instance;
    return &instance;
}

void PreLoadRequiredStuff(JNIEnv *env) {
    Types::Load(env);
    WellKnownClasses::Load(env);
    ScopedNoGCDaemons::Load(env);
}


bool ArtRuntime::OnLoad(JavaVM *vm, JNIEnv *env, jclass java_class) {
#define CHECK_FIELD(field, value)  \
    if ((field) == (value)) {  \
        LOG(ERROR) << "Failed to find " #field ".";  \
        return false;  \
    }
    if ((kRuntimeISA == InstructionSet::kArm
         || kRuntimeISA == InstructionSet::kArm64)
        && IsFileInMemory("libhoudini.so")) {
        LOG(INFO) << '[' << getpid() << ']' << " Unable to launch on houdini environment.";
        return false;
    }
    vm_ = vm;
    java_class_ = reinterpret_cast<jclass>(env->NewGlobalRef(java_class));
    bridge_method_ = env->GetStaticMethodID(
            java_class,
            "handleHookedMethod",
            "(Ljava/lang/reflect/Member;JLjava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;"
    );
    if (JNIExceptionCheck(env)) {
        return false;
    }
    api_level_ = GetAndroidApiLevel();
    PreLoadRequiredStuff(env);
    const char *art_path = kLibArtPath;
    art_elf_image_ = WDynamicLibOpen(art_path);
    if (art_elf_image_ == nullptr) {
        LOG(ERROR) << "Unable to read data from libart.so.";
        return false;
    }
    if (!art_symbol_resolver_.Resolve(art_elf_image_, api_level_)) {
        // The log will all output from ArtSymbolResolver.
        return false;
    }
    offset_t jni_code_offset = INT32_MAX;
    offset_t access_flags_offset = INT32_MAX;

    size_t entrypoint_filed_size = (api_level_ <= ANDROID_LOLLIPOP) ? 8
                                                                    : kPointerSize;
    u4 expected_access_flags = kAccPrivate | kAccStatic | kAccNative;
    jmethodID reserved0 = env->GetStaticMethodID(java_class, kMethodReserved0, "()V");
    jmethodID reserved1 = env->GetStaticMethodID(java_class, kMethodReserved1, "()V");

    for (offset_t offset = 0; offset != sizeof(u4) * 24; offset += sizeof(u4)) {
        if (MemberOf<u4>(reserved0, offset) == expected_access_flags) {
            access_flags_offset = offset;
            break;
        }
    }
    void *native_function = reinterpret_cast<void *>(WhaleRuntime_reserved0);

    for (offset_t offset = 0; offset != sizeof(u4) * 24; offset += sizeof(u4)) {
        if (MemberOf<ptr_t>(reserved0, offset) == native_function) {
            jni_code_offset = offset;
            break;
        }
    }
    CHECK_FIELD(access_flags_offset, INT32_MAX)
    CHECK_FIELD(jni_code_offset, INT32_MAX)

    method_offset_.method_size_ = DistanceOf(reserved0, reserved1);
    method_offset_.jni_code_offset_ = jni_code_offset;
    method_offset_.quick_code_offset_ = jni_code_offset + entrypoint_filed_size;
    method_offset_.access_flags_offset_ = access_flags_offset;
    method_offset_.dex_code_item_offset_offset_ = access_flags_offset + sizeof(u4);
    method_offset_.dex_method_index_offset_ = access_flags_offset + sizeof(u4) * 2;
    method_offset_.method_index_offset_ = access_flags_offset + sizeof(u4) * 3;
    if (api_level_ < ANDROID_N
        && GetSymbols()->artInterpreterToCompiledCodeBridge != nullptr) {
        method_offset_.interpreter_code_offset_ = jni_code_offset - entrypoint_filed_size;
    }
    if (api_level_ >= ANDROID_N) {
        method_offset_.hotness_count_offset_ = method_offset_.method_index_offset_ + sizeof(u2);
    }
    ptr_t quick_generic_jni_trampoline = WDynamicLibSymbol(
            art_elf_image_,
            "art_quick_generic_jni_trampoline"
    );
    env->CallStaticVoidMethod(java_class, reserved0);

    /**
     * Fallback to do a relative memory search for quick_generic_jni_trampoline,
     * This case is almost impossible to enter
     * because its symbols are found almost always on all devices.
     * This algorithm has been verified on 5.0 ~ 9.0.
     * And we're pretty sure that its structure has not changed in the OEM Rom.
     */
    if (quick_generic_jni_trampoline == nullptr) {
        ptr_t heap = nullptr;
        ptr_t thread_list = nullptr;
        ptr_t class_linker = nullptr;
        ptr_t intern_table = nullptr;

        ptr_t runtime = MemberOf<ptr_t>(vm, kPointerSize);
        CHECK_FIELD(runtime, nullptr)
        runtime_objects_.runtime_ = runtime;

        offset_t start = (kPointerSize == 4) ? 200 : 384;
        offset_t end = start + (100 * kPointerSize);
        for (offset_t offset = start; offset != end; offset += kPointerSize) {
            if (MemberOf<ptr_t>(runtime, offset) == vm) {
                size_t class_linker_offset = offset - (kPointerSize * 3) - (2 * kPointerSize);
                if (api_level_ >= ANDROID_O_MR1) {
                    class_linker_offset -= kPointerSize;
                }
                offset_t intern_table_offset = class_linker_offset - kPointerSize;
                offset_t thread_list_Offset = intern_table_offset - kPointerSize;
                offset_t heap_offset = thread_list_Offset - (4 * kPointerSize);
                if (api_level_ >= ANDROID_M) {
                    heap_offset -= 3 * kPointerSize;
                }
                if (api_level_ >= ANDROID_N) {
                    heap_offset -= kPointerSize;
                }
                heap = MemberOf<ptr_t>(runtime, heap_offset);
                thread_list = MemberOf<ptr_t>(runtime, thread_list_Offset);
                class_linker = MemberOf<ptr_t>(runtime, class_linker_offset);
                intern_table = MemberOf<ptr_t>(runtime, intern_table_offset);
                break;
            }
        }
        CHECK_FIELD(heap, nullptr)
        CHECK_FIELD(thread_list, nullptr)
        CHECK_FIELD(class_linker, nullptr)
        CHECK_FIELD(intern_table, nullptr)

        runtime_objects_.heap_ = heap;
        runtime_objects_.thread_list_ = thread_list;
        runtime_objects_.class_linker_ = class_linker;
        runtime_objects_.intern_table_ = intern_table;

        start = kPointerSize * 25;
        end = start + (100 * kPointerSize);
        for (offset_t offset = start; offset != end; offset += kPointerSize) {
            if (MemberOf<ptr_t>(class_linker, offset) == intern_table) {
                offset_t target_offset =
                        offset + ((api_level_ >= ANDROID_M) ? 3 : 5) * kPointerSize;
                quick_generic_jni_trampoline = MemberOf<ptr_t>(class_linker, target_offset);
                break;
            }
        }
    }
    CHECK_FIELD(quick_generic_jni_trampoline, nullptr)
    class_linker_objects_.quick_generic_jni_trampoline_ = quick_generic_jni_trampoline;

    pthread_mutex_init(&mutex, nullptr);
    EnforceDisableHiddenAPIPolicy();
    if (api_level_ >= ANDROID_N) {
        FixBugN();
    }
    return true;

#undef CHECK_OFFSET
}


jlong
ArtRuntime::HookMethod(JNIEnv *env, jclass decl_class, jobject hooked_java_method,
                       jobject addition_info) {
    ScopedSuspendAll suspend_all;

    jmethodID hooked_jni_method = env->FromReflectedMethod(hooked_java_method);
    ArtMethod hooked_method(hooked_jni_method);
    auto *param = new ArtHookParam();

    param->class_Loader_ = env->NewGlobalRef(
            env->CallObjectMethod(
                    decl_class,
                    WellKnownClasses::java_lang_Class_getClassLoader
            )
    );
    param->shorty_ = hooked_method.GetShorty(env, hooked_java_method);
    param->is_static_ = hooked_method.HasAccessFlags(kAccStatic);

    param->origin_compiled_code_ = hooked_method.GetEntryPointFromQuickCompiledCode();
    param->origin_code_item_off = hooked_method.GetDexCodeItemOffset();
    param->origin_jni_code_ = hooked_method.GetEntryPointFromJni();
    param->origin_access_flags = hooked_method.GetAccessFlags();
    jobject origin_java_method = hooked_method.Clone(env, param->origin_access_flags);

    ResolvedSymbols *symbols = GetSymbols();
    if (symbols->ProfileSaver_ForceProcessProfiles) {
        symbols->ProfileSaver_ForceProcessProfiles();
    }
    // After android P, hotness_count_ maybe an imt_index_ for abstract method
    if ((api_level_ > ANDROID_P && !hooked_method.HasAccessFlags(kAccAbstract))
        || api_level_ >= ANDROID_N) {
        hooked_method.SetHotnessCount(0);
    }
    // Clear the dex_code_item_offset_.
    // It needs to be 0 since hooked methods have no CodeItems but the
    // method they copy might.
    hooked_method.SetDexCodeItemOffset(0);
    u4 access_flags = hooked_method.GetAccessFlags();
    if (api_level_ < ANDROID_O_MR1) {
        access_flags |= kAccCompileDontBother_N;
    } else {
        access_flags |= kAccCompileDontBother_O_MR1;
        access_flags |= kAccPreviouslyWarm_O_MR1;
    }
    access_flags |= kAccNative;
    access_flags |= kAccFastNative;
    if (api_level_ >= ANDROID_P) {
        access_flags &= ~kAccCriticalNative_P;
    }
    hooked_method.SetAccessFlags(access_flags);
    hooked_method.SetEntryPointFromQuickCompiledCode(
            class_linker_objects_.quick_generic_jni_trampoline_
    );
    if (api_level_ < ANDROID_N
        && symbols->artInterpreterToCompiledCodeBridge != nullptr) {
        hooked_method.SetEntryPointFromInterpreterCode(symbols->artInterpreterToCompiledCodeBridge);
    }
    param->origin_native_method_ = env->FromReflectedMethod(origin_java_method);
    param->hooked_native_method_ = hooked_jni_method;
    param->addition_info_ = env->NewGlobalRef(addition_info);
    param->hooked_method_ = env->NewGlobalRef(hooked_java_method);
    param->origin_method_ = env->NewGlobalRef(origin_java_method);

    BuildJniClosure(param);

    hooked_method.SetEntryPointFromJni(param->jni_closure_->GetCode());
    param->decl_class_ = hooked_method.GetDeclaringClass();
    hooked_method_map_.insert(std::make_pair(hooked_jni_method, param));
    return reinterpret_cast<jlong>(param);
}

jobject
ArtRuntime::InvokeOriginalMethod(jlong slot, jobject this_object, jobjectArray args) {
    JNIEnv *env = GetJniEnv();
    auto *param = reinterpret_cast<ArtHookParam *>(slot);
    if (slot <= 0) {
        env->ThrowNew(
                WellKnownClasses::java_lang_IllegalArgumentException,
                "Failed to resolve slot."
        );
        return nullptr;
    }
    ArtMethod hooked_method(param->hooked_native_method_);
    ptr_t decl_class = hooked_method.GetDeclaringClass();
    if (param->decl_class_ != decl_class) {
        pthread_mutex_lock(&mutex);
        if (param->decl_class_ != decl_class) {
            ScopedSuspendAll suspend_all;
            LOG(INFO)
                    << "Notice: MovingGC cause the GcRoot References changed.";
            jobject origin_java_method = hooked_method.Clone(env, param->origin_access_flags);
            jmethodID origin_jni_method = env->FromReflectedMethod(origin_java_method);
            ArtMethod origin_method(origin_jni_method);
            origin_method.SetEntryPointFromQuickCompiledCode(param->origin_compiled_code_);
            origin_method.SetEntryPointFromJni(param->origin_jni_code_);
            origin_method.SetDexCodeItemOffset(param->origin_code_item_off);
            param->origin_native_method_ = origin_jni_method;
            env->DeleteGlobalRef(param->origin_method_);
            param->origin_method_ = env->NewGlobalRef(origin_java_method);
            param->decl_class_ = decl_class;
        }
        pthread_mutex_unlock(&mutex);
    }

    jobject ret = env->CallNonvirtualObjectMethod(
            param->origin_method_,
            WellKnownClasses::java_lang_reflect_Method,
            WellKnownClasses::java_lang_reflect_Method_invoke,
            this_object,
            args
    );
    return ret;
}

#if defined(__aarch64__)
# define __get_tls() ({ void** __val; __asm__("mrs %0, tpidr_el0" : "=r"(__val)); __val; })
#elif defined(__arm__)
# define __get_tls() ({ void** __val; __asm__("mrc p15, 0, %0, c13, c0, 3" : "=r"(__val)); __val; })
#elif defined(__i386__)
# define __get_tls() ({ void** __val; __asm__("movl %%gs:0, %0" : "=r"(__val)); __val; })
#elif defined(__x86_64__)
# define __get_tls() ({ void** __val; __asm__("mov %%fs:0, %0" : "=r"(__val)); __val; })
#else
#error unsupported architecture
#endif

ArtThread *ArtRuntime::GetCurrentArtThread() {
    if (WellKnownClasses::java_lang_Thread_nativePeer) {
        JNIEnv *env = GetJniEnv();
        jobject current = env->CallStaticObjectMethod(
                WellKnownClasses::java_lang_Thread,
                WellKnownClasses::java_lang_Thread_currentThread
        );
        return reinterpret_cast<ArtThread *>(
                env->GetLongField(current, WellKnownClasses::java_lang_Thread_nativePeer)
        );
    }
    return reinterpret_cast<ArtThread *>(__get_tls()[7/*TLS_SLOT_ART_THREAD_SELF*/]);
}

jobject
ArtRuntime::InvokeHookedMethodBridge(JNIEnv *env, ArtHookParam *param, jobject receiver,
                                     jobjectArray array) {
    return env->CallStaticObjectMethod(java_class_, bridge_method_,
                                       param->hooked_method_, reinterpret_cast<jlong>(param),
                                       param->addition_info_, receiver, array);
}

jlong ArtRuntime::GetMethodSlot(JNIEnv *env, jclass cl, jobject method_obj) {
    if (method_obj == nullptr) {
        env->ThrowNew(
                WellKnownClasses::java_lang_IllegalArgumentException,
                "Method param == null"
        );
        return 0;
    }
    jmethodID jni_method = env->FromReflectedMethod(method_obj);
    auto entry = hooked_method_map_.find(jni_method);
    if (entry == hooked_method_map_.end()) {
        env->ThrowNew(
                WellKnownClasses::java_lang_IllegalArgumentException,
                "Failed to find slot."
        );
        return 0;
    }
    return reinterpret_cast<jlong>(entry->second);
}

void ArtRuntime::EnsureClassInitialized(JNIEnv *env, jclass cl) {
    // This invocation will ensure the target class has been initialized also.
    ScopedLocalRef<jobject> unused(env, env->AllocObject(cl));
    JNIExceptionClear(env);
}

void ArtRuntime::SetObjectClass(JNIEnv *env, jobject obj, jclass cl) {
    SetObjectClassUnsafe(env, obj, cl);
}

void ArtRuntime::SetObjectClassUnsafe(JNIEnv *env, jobject obj, jclass cl) {
    jfieldID java_lang_Class_shadow$_klass_ = env->GetFieldID(
            WellKnownClasses::java_lang_Object,
            "shadow$_klass_",
            "Ljava/lang/Class;"
    );
    env->SetObjectField(obj, java_lang_Class_shadow$_klass_, cl);
}

jobject ArtRuntime::CloneToSubclass(JNIEnv *env, jobject obj, jclass sub_class) {
    ResolvedSymbols *symbols = GetSymbols();
    ArtThread *thread = GetCurrentArtThread();
    ptr_t art_object = symbols->Thread_DecodeJObject(thread, obj);
    ptr_t art_clone_object = CloneArtObject(art_object);
    jobject clone = symbols->JniEnvExt_NewLocalRef(env, art_clone_object);
    SetObjectClassUnsafe(env, clone, sub_class);
    return clone;
}

void ArtRuntime::RemoveFinalFlag(JNIEnv *env, jclass java_class) {
    jfieldID java_lang_Class_accessFlags = env->GetFieldID(
            WellKnownClasses::java_lang_Class,
            "accessFlags",
            "I"
    );
    jint access_flags = env->GetIntField(java_class, java_lang_Class_accessFlags);
    env->SetIntField(java_class, java_lang_Class_accessFlags, access_flags & ~kAccFinal);
}

bool ArtRuntime::EnforceDisableHiddenAPIPolicy() {
    if (GetAndroidApiLevel() < ANDROID_O_MR1) {
        return true;
    }
    static Singleton<bool> enforced([&](bool *result) {
        *result = EnforceDisableHiddenAPIPolicyImpl();
    });
    return enforced.Get();
}

bool OnInvokeHiddenAPI() {
    return false;
}

/**
 * NOTICE:
 * After Android Q(10.0), GetMemberActionImpl has been renamed to ShouldDenyAccessToMemberImpl,
 * But we don't know the symbols until it's published.
 */
ALWAYS_INLINE bool ArtRuntime::EnforceDisableHiddenAPIPolicyImpl() {
    JNIEnv *env = GetJniEnv();
    jfieldID java_lang_Class_shadow$_klass_ = env->GetFieldID(
            WellKnownClasses::java_lang_Object,
            "shadow$_klass_",
            "Ljava/lang/Class;"
    );
    JNIExceptionClear(env);
    if (java_lang_Class_shadow$_klass_ != nullptr) {
        return true;
    }
    void *symbol = nullptr;

    // Android P : Preview 1 ~ 4 version
    symbol = WDynamicLibSymbol(
            art_elf_image_,
            "_ZN3art9hiddenapi25ShouldBlockAccessToMemberINS_8ArtFieldEEEbPT_PNS_6ThreadENSt3__18functionIFbS6_EEENS0_12AccessMethodE"
    );
    if (symbol) {
        WInlineHookFunction(symbol, reinterpret_cast<void *>(OnInvokeHiddenAPI), nullptr);
    }
    symbol = WDynamicLibSymbol(
            art_elf_image_,
            "_ZN3art9hiddenapi25ShouldBlockAccessToMemberINS_9ArtMethodEEEbPT_PNS_6ThreadENSt3__18functionIFbS6_EEENS0_12AccessMethodE"
    );

    if (symbol) {
        WInlineHookFunction(symbol, reinterpret_cast<void *>(OnInvokeHiddenAPI), nullptr);
        return true;
    }
    // Android P : Release version
    symbol = WDynamicLibSymbol(
            art_elf_image_,
            "_ZN3art9hiddenapi6detail19GetMemberActionImplINS_8ArtFieldEEENS0_6ActionEPT_NS_20HiddenApiAccessFlags7ApiListES4_NS0_12AccessMethodE"
    );
    if (symbol) {
        WInlineHookFunction(symbol, reinterpret_cast<void *>(OnInvokeHiddenAPI), nullptr);
    }
    symbol = WDynamicLibSymbol(
            art_elf_image_,
            "_ZN3art9hiddenapi6detail19GetMemberActionImplINS_9ArtMethodEEENS0_6ActionEPT_NS_20HiddenApiAccessFlags7ApiListES4_NS0_12AccessMethodE"
    );
    if (symbol) {
        WInlineHookFunction(symbol, reinterpret_cast<void *>(OnInvokeHiddenAPI), nullptr);
    }
    return symbol != nullptr;
}

ptr_t ArtRuntime::CloneArtObject(ptr_t art_object) {
    ResolvedSymbols *symbols = GetSymbols();
    if (symbols->Object_Clone) {
        return symbols->Object_Clone(art_object, GetCurrentArtThread());
    }
    if (symbols->Object_CloneWithClass) {
        return symbols->Object_CloneWithClass(art_object, GetCurrentArtThread(), nullptr);
    }
    return symbols->Object_CloneWithSize(art_object, GetCurrentArtThread(), 0);
}

int (*old_ToDexPc)(void *thiz, void *a2, unsigned int a3, int a4);
int new_ToDexPc(void *thiz, void *a2, unsigned int a3, int a4) {
    return old_ToDexPc(thiz, a2, a3, 0);
}

bool is_hooked = false;
void ArtRuntime::FixBugN() {
    if (is_hooked)
        return;
    void *symbol = nullptr;
    symbol = WDynamicLibSymbol(
            art_elf_image_,
            "_ZNK3art20OatQuickMethodHeader7ToDexPcEPNS_9ArtMethodEjb"
    );
    if (symbol) {
        WInlineHookFunction(symbol, reinterpret_cast<void *>(new_ToDexPc), reinterpret_cast<void **>(&old_ToDexPc));
    }
    is_hooked = true;
}

}  // namespace art
}  // namespace whale
