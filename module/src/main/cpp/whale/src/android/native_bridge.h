#ifndef WHALE_ANDROID_NATIVE_BRIDGE_H_
#define WHALE_ANDROID_NATIVE_BRIDGE_H_

#include <cstdint>
#include <jni.h>
#include <asm/siginfo.h>

struct NativeBridgeRuntimeCallbacks;
struct NativeBridgeRuntimeValues;

// Function pointer type for sigaction. This is mostly the signature of a signal handler, except
// for the return type. The runtime needs to know whether the signal was handled or should be given
// to the chain.
typedef bool (*NativeBridgeSignalHandlerFn)(int, siginfo_t *, void *);

// Runtime interfaces to native bridge.
struct NativeBridgeRuntimeCallbacks {
    // Get shorty of a Java method. The shorty is supposed to be persistent in memory.
    //
    // Parameters:
    //   env [IN] pointer to JNIenv.
    //   mid [IN] Java methodID.
    // Returns:
    //   short descriptor for method.
    const char *(*getMethodShorty)(JNIEnv *env, jmethodID mid);

    // Get number of native methods for specified class.
    //
    // Parameters:
    //   env [IN] pointer to JNIenv.
    //   clazz [IN] Java class object.
    // Returns:
    //   number of native methods.
    uint32_t (*getNativeMethodCount)(JNIEnv *env, jclass clazz);

    // Get at most 'method_count' native methods for specified class 'clazz'. Results are outputed
    // via 'methods' [OUT]. The signature pointer in JNINativeMethod is reused as the method shorty.
    //
    // Parameters:
    //   env [IN] pointer to JNIenv.
    //   clazz [IN] Java class object.
    //   methods [OUT] array of method with the name, shorty, and fnPtr.
    //   method_count [IN] max number of elements in methods.
    // Returns:
    //   number of method it actually wrote to methods.
    uint32_t (*getNativeMethods)(JNIEnv *env, jclass clazz, JNINativeMethod *methods,
                                 uint32_t method_count);
};

// Native bridge interfaces to runtime.
struct NativeBridgeCallbacks {
    // Version number of the interface.
    uint32_t version;

    // Initialize native bridge. Native bridge's internal implementation must ensure MT safety and
    // that the native bridge is initialized only once. Thus it is OK to call this interface for an
    // already initialized native bridge.
    //
    // Parameters:
    //   runtime_cbs [IN] the pointer to NativeBridgeRuntimeCallbacks.
    // Returns:
    //   true iff initialization was successful.
    bool (*initialize)(const NativeBridgeRuntimeCallbacks *runtime_cbs, const char *private_dir,
                       const char *instruction_set);

    // Load a shared library that is supported by the native bridge.
    //
    // Parameters:
    //   libpath [IN] path to the shared library
    //   flag [IN] the stardard RTLD_XXX defined in bionic dlfcn.h
    // Returns:
    //   The opaque handle of the shared library if sucessful, otherwise NULL
    void *(*loadLibrary)(const char *libpath, int flag);

    // Get a native bridge trampoline for specified native method. The trampoline has same
    // sigature as the native method.
    //
    // Parameters:
    //   handle [IN] the handle returned from loadLibrary
    //   shorty [IN] short descriptor of native method
    //   len [IN] length of shorty
    // Returns:
    //   address of trampoline if successful, otherwise NULL
    void *(*getTrampoline)(void *handle, const char *name, const char *shorty, uint32_t len);

    // Check whether native library is valid and is for an ABI that is supported by native bridge.
    //
    // Parameters:
    //   libpath [IN] path to the shared library
    // Returns:
    //   TRUE if library is supported by native bridge, FALSE otherwise
    bool (*isSupported)(const char *libpath);

    // Provide environment values required by the app running with native bridge according to the
    // instruction set.
    //
    // Parameters:
    //    instruction_set [IN] the instruction set of the app
    // Returns:
    //    NULL if not supported by native bridge.
    //    Otherwise, return all environment values to be set after fork.
    const struct NativeBridgeRuntimeValues *(*getAppEnv)(const char *instruction_set);

    // Added callbacks in version 2.

    // Check whether the bridge is compatible with the given version. A bridge may decide not to be
    // forwards- or backwards-compatible, and libnativebridge will then stop using it.
    //
    // Parameters:
    //     bridge_version [IN] the version of libnativebridge.
    // Returns:
    //     true iff the native bridge supports the given version of libnativebridge.
    bool (*isCompatibleWith)(uint32_t bridge_version);

    // A callback to retrieve a native bridge's signal handler for the specified signal. The runtime
    // will ensure that the signal handler is being called after the runtime's own handler, but before
    // all chained handlers. The native bridge should not try to install the handler by itself, as
    // that will potentially lead to cycles.
    //
    // Parameters:
    //     signal [IN] the signal for which the handler is asked for. Currently, only SIGSEGV is
    //                 supported by the runtime.
    // Returns:
    //     NULL if the native bridge doesn't use a handler or doesn't want it to be managed by the
    //     runtime.
    //     Otherwise, a pointer to the signal handler.
    NativeBridgeSignalHandlerFn (*getSignalHandler)(int signal);
};


#endif  // WHALE_ANDROID_NATIVE_BRIDGE_H_
