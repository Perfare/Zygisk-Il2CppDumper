#include "dbi/arm/decoder_arm.h"

#define CASE(mask, val, type)  \
if ((((insn) & (mask)) == val))  {  \
    return type;  \
}

namespace whale {
namespace arm {

ArmInsnType DecodeArm(u4 insn) {
    CASE(0xe5f0000, 0x41f0000, kARM_LDR);
    CASE(0xfef0010, 0x8f0000, kARM_ADD);
    CASE(0xdef0000, 0x1a00000, kARM_MOV);
    CASE(0xf000000, 0xa000000, kARM_B);
    CASE(0xf000000, 0xb000000, kARM_BL);
    return kARM_UNHANDLED;
}

}  // namespace arm
}  // namespace whale
