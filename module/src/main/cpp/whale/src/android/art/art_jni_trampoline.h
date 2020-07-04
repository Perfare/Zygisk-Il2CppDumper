#ifndef WHALE_ANDROID_ART_JNI_TRAMPOLINE_H_
#define WHALE_ANDROID_ART_JNI_TRAMPOLINE_H_

#include "android/art/art_hook_param.h"

namespace whale {
namespace art {


void BuildJniClosure(ArtHookParam *param);


}  // namespace art
}  // namespace whale

#endif  // WHALE_ANDROID_ART_JNI_TRAMPOLINE_H_
