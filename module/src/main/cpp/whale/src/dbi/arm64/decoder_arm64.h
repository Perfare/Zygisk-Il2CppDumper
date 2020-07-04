#ifndef ARCH_ARM64_DECODER_H_
#define ARCH_ARM64_DECODER_H_

#include "base/primitive_types.h"

namespace whale {
namespace arm64 {

enum A64InsnType {
    kA64_CBZ_CBNZ,
    kA64_B_COND,
    kA64_TBZ_TBNZ,
    kA64_B_BL,
    kA64_LDR_LIT,
    kA64_ADR_ADRP,
    kA64_UNHANDLED
};

A64InsnType DecodeA64(u4 insn);

}  // namespace arm64
}  // namespace whale

#endif  // ARCH_ARM64_DECODER_H_

