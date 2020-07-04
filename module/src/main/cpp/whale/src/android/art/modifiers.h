#ifndef WHALE_ANDROID_ART_MODIFIERS_H_
#define WHALE_ANDROID_ART_MODIFIERS_H_

#include "base/primitive_types.h"

namespace whale {
namespace art {

static constexpr u4 kAccPublic = 0x0001;  // class, field, method, ic
static constexpr u4 kAccPrivate = 0x0002;  // field, method, ic
static constexpr u4 kAccProtected = 0x0004;  // field, method, ic
static constexpr u4 kAccStatic = 0x0008;  // field, method, ic
static constexpr u4 kAccFinal = 0x0010;  // class, field, method, ic
static constexpr u4 kAccSynchronized = 0x0020;  // method (only allowed on natives)
static constexpr u4 kAccSuper = 0x0020;  // class (not used in dex)
static constexpr u4 kAccVolatile = 0x0040;  // field
static constexpr u4 kAccBridge = 0x0040;  // method (1.5)
static constexpr u4 kAccTransient = 0x0080;  // field
static constexpr u4 kAccVarargs = 0x0080;  // method (1.5)
static constexpr u4 kAccNative = 0x0100;  // method
static constexpr u4 kAccInterface = 0x0200;  // class, ic
static constexpr u4 kAccAbstract = 0x0400;  // class, method, ic
static constexpr u4 kAccStrict = 0x0800;  // method
static constexpr u4 kAccSynthetic = 0x1000;  // class, field, method, ic
static constexpr u4 kAccAnnotation = 0x2000;  // class, ic (1.5)
static constexpr u4 kAccEnum = 0x4000;  // class, field, ic (1.5)

static constexpr u4 kAccJavaFlagsMask = 0xffff;  // bits set from Java sources (low 16)

static constexpr u4 kAccConstructor = 0x00010000;  // method (dex only) <(cl)init>
static constexpr u4 kAccDeclaredSynchronized = 0x00020000;  // method (dex only)
static constexpr u4 kAccClassIsProxy = 0x00040000;  // class  (dex only)
// Set to indicate that the ArtMethod is obsolete and has a different DexCache + DexFile from its
// declaring class. This flag may only be applied to methods.
static constexpr u4 kAccObsoleteMethod = 0x00040000;  // method (runtime)

static constexpr uint32_t kAccFastNative = 0x00080000u;   // method (dex only)
static constexpr uint32_t kAccPreverified = kAccFastNative;  // class (runtime)
static constexpr uint32_t kAccSkipAccessChecks = kAccPreverified;
static constexpr uint32_t kAccCriticalNative_P = 0x00200000;  // method (runtime; native only)
// Android M only
static constexpr uint32_t kAccDontInline = 0x00400000u;  // method (dex only)
// Android N or later. Set by the verifier for a method we do not want the compiler to compile.
static constexpr uint32_t kAccCompileDontBother_N = 0x01000000u;  // method (runtime)
// Android O MR1 or later. Set by the verifier for a method we do not want the compiler to compile.
static constexpr uint32_t kAccCompileDontBother_O_MR1 = 0x02000000;  // method (runtime)
// Set by the JIT when clearing profiling infos to denote that a method was previously warm.
static constexpr uint32_t kAccPreviouslyWarm_O_MR1 = 0x00800000;  // method (runtime)

static constexpr uint32_t kAccDirectFlags = kAccStatic | kAccPrivate | kAccConstructor;

static constexpr uint32_t kAccPublicApi =             0x10000000;  // field, method
static constexpr uint32_t kAccHiddenapiBits =         0x30000000;  // field, method

}  // namespace art
}  // namespace whale

#endif  // WHALE_ANDROID_ART_MODIFIERS_H_
