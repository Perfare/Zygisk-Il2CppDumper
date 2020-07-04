#ifndef ARM64_ARM_CONVENTION_H_
#define ARM64_ARM_CONVENTION_H_

#include <base/primitive_types.h>
#include "assembler/vixl/aarch32/operands-aarch32.h"

namespace whale {
namespace arm {


enum ArmRegister {
    R0 = 0,
    R1 = 1,
    R2 = 2,
    R3 = 3,
    R4 = 4,
    R5 = 5,
    R6 = 6,
    R7 = 7,
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
    MR = 8,
    TR = 9,
    FP = 11,
    IP = 12,
    SP = 13,
    LR = 14,
    PC = 15,
    kNumberOfCoreRegisters = 16,
    kNoRegister = -1,
};

const vixl::aarch32::Register &Reg(int reg) {
#define CASE(x) case R##x:  \
        return vixl::aarch32::r##x;

    switch (reg) {
        CASE(0)
        CASE(1)
        CASE(2)
        CASE(3)
        CASE(4)
        CASE(5)
        CASE(6)
        CASE(7)
        CASE(8)
        CASE(9)
        CASE(10)
        CASE(11)
        CASE(12)
        CASE(13)
        CASE(14)
        CASE(15)
        default:
            LOG(FATAL) << "Unexpected register : " << reg;
            UNREACHABLE();
    }
#undef CASE
}

}  // namespace arm
}  // namespace whale


#endif  // ARM64_ARM_CONVENTION_H_


