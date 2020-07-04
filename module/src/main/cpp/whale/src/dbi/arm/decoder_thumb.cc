#include "dbi/arm/decoder_thumb.h"

#define CASE(mask, val, type)  \
if ((((insn) & (mask)) == val))  {  \
    return type;  \
}

namespace whale {
namespace arm {


ThumbInsnType DecodeThumb16(u2 insn) {
    CASE(0xf800, 0xa000, kTHUMB_ADD_FROM_PC16);
    CASE(0xff00, 0x4400, kTHUMB_ADDH16);
    CASE(0xf800, 0xe000, kTHUMB_B16);
    CASE(0xf000, 0xd000, kTHUMB_B_COND16);
    CASE(0xff87, 0x4780, kTHUMB_BLX16);
    CASE(0xff87, 0x4700, kTHUMB_BX16);
    CASE(0xfd00, 0xb900, kTHUMB_CBNZ16);
    CASE(0xfd00, 0xb100, kTHUMB_CBZ16);
    CASE(0xff00, 0x4500, kTHUMB_CMPH16);
    CASE(0xf800, 0x4800, kTHUMB_LDR_PC_16);
    CASE(0xff00, 0x4600, kTHUMB_MOVH16);
    return kTHUMB_UNHANDLED16;
}

ThumbInsnType DecodeThumb32(u4 insn) {
    CASE(0xf800d000, 0xf0009000, kTHUMB_B32);
    CASE(0xf800d000, 0xf000d000, kTHUMB_BL32);
    CASE(0xf800d000, 0xf000c000, kTHUMB_BL_ARM32);
    CASE(0xff7f0000, 0xf81f0000, kTHUMB_LDRBL32);
    CASE(0xff7f0000, 0xf83f0000, kTHUMB_LDRHL32);
    CASE(0xff7f0000, 0xf85f0000, kTHUMB_LDRL32);
    CASE(0xfff00fc0, 0xf9100000, kTHUMB_LDRSBL32);
    CASE(0xff7f0000, 0xf93f0000, kTHUMB_LDRSHL32);
    return kTHUMB_UNHANDLED32;
}


}  // namespace arm
}  // namespace whale
