#ifndef WHALE_ANDROID_ART_INTERCEPT_PARAM_H_
#define WHALE_ANDROID_ART_INTERCEPT_PARAM_H_

#include <jni.h>
#include "base/primitive_types.h"
#include "ffi_cxx.h"

namespace whale {
namespace art {

struct ArtHookParam final {
    bool is_static_;
    const char *shorty_;
    jobject addition_info_;
    ptr_t origin_compiled_code_;
    ptr_t origin_jni_code_;
    u4 origin_access_flags;
    u4 origin_code_item_off;
    jobject origin_method_;
    jobject hooked_method_;
    volatile ptr_t decl_class_;
    jobject class_Loader_;
    jmethodID hooked_native_method_;
    jmethodID origin_native_method_;
    FFIClosure *jni_closure_;
};

}  // namespace art
}  // namespace whale

#endif  // WHALE_ANDROID_ART_INTERCEPT_PARAM_H_
