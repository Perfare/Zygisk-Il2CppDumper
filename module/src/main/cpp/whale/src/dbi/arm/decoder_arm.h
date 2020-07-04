#ifndef ARCH_ARM_DECODER_ARM_H_
#define ARCH_ARM_DECODER_ARM_H_

#include "base/primitive_types.h"

namespace whale {
namespace arm {

enum ArmInsnType {
    kARM_LDR,
    kARM_ADD,
    kARM_MOV,
    kARM_B,
    kARM_BL,
    kARM_VFP_VSTM_DP,
    kARM_VFP_VSTM_SP,
    kARM_VFP_VLDM_SP,
    kARM_VFP_VLDM_DP,
    kARM_VFP_VSTR_DP,
    kARM_VFP_VSTR_SP,
    kARM_VFP_VLDR_DP,
    kARM_VFP_VLDR_SP,

    kARM_UNHANDLED,
};

ArmInsnType DecodeArm(u4 insn);

}  // namespace arm
}  // namespace whale

#endif  // ARCH_ARM_DECODER_ARM_H_
