#ifndef ARCH_ARM_DECODER_THUMB_H_
#define ARCH_ARM_DECODER_THUMB_H_

#include "base/primitive_types.h"

namespace whale {
namespace arm {

enum ThumbInsnType {
    kTHUMB_ADD_FROM_PC16,
    kTHUMB_ADDH16,
    kTHUMB_B16,
    kTHUMB_B_COND16,
    kTHUMB_BLX16,
    kTHUMB_BX16,
    kTHUMB_CBNZ16,
    kTHUMB_CBZ16,
    kTHUMB_CMPH16,
    kTHUMB_LDR_PC_16,
    kTHUMB_MOVH16,
    kTHUMB_UNHANDLED16,

    kTHUMB_B32,
    kTHUMB_BL32,
    kTHUMB_BL_ARM32,
    kTHUMB_LDRBL32,
    kTHUMB_LDRHL32,
    kTHUMB_LDRL32,
    kTHUMB_LDRSBL32,
    kTHUMB_LDRSHL32,
    kTHUMB_UNHANDLED32
};

ThumbInsnType DecodeThumb16(u2 insn);

ThumbInsnType DecodeThumb32(u4 insn);

/**
 * Bit[15:11] in Thumb32 :
 * 0b11101
 * 0b11110
 * 0b11111
 */
static inline bool Is32BitThumbInstruction(u2 insn) {
    return ((insn & 0xF000) == 0xF000) || ((insn & 0xF800) == 0xE800);
}

}  // namespace arm
}  // namespace whale

#endif  // ARCH_ARM_DECODER_THUMB_H_
