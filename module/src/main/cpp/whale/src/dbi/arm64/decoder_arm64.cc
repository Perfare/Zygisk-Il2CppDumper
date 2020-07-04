#include "dbi/arm64/decoder_arm64.h"
#include "base/macros.h"

#define CASE(mask, val, type)  \
if ((((insn) & (mask)) == val))  {  \
    return type;  \
}

namespace whale {
namespace arm64 {

A64InsnType DecodeA64(u4 insn) {
    CASE(0x7e000000, 0x34000000, kA64_CBZ_CBNZ);
    CASE(0xff000010, 0x54000000, kA64_B_COND);
    CASE(0x7e000000, 0x36000000, kA64_TBZ_TBNZ);
    CASE(0x7c000000, 0x14000000, kA64_B_BL);
    CASE(0x3b000000, 0x18000000, kA64_LDR_LIT);
    CASE(0x1f000000, 0x10000000, kA64_ADR_ADRP);
    return kA64_UNHANDLED;
}

}  // namespace arm64
}  // namespace whale
