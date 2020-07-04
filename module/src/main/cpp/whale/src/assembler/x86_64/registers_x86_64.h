#ifndef WHALE_ASSEMBLER_REGISTERS_X86_64_H_
#define WHALE_ASSEMBLER_REGISTERS_X86_64_H_

namespace whale {
namespace x86_64 {

enum Register {
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3,
    RSP = 4,
    RBP = 5,
    RSI = 6,
    RDI = 7,
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
    kLastCpuRegister = 15,
    kNumberOfCpuRegisters = 16,
    kNoRegister = -1  // Signals an illegal register.
};

enum FloatRegister {
    XMM0 = 0,
    XMM1 = 1,
    XMM2 = 2,
    XMM3 = 3,
    XMM4 = 4,
    XMM5 = 5,
    XMM6 = 6,
    XMM7 = 7,
    XMM8 = 8,
    XMM9 = 9,
    XMM10 = 10,
    XMM11 = 11,
    XMM12 = 12,
    XMM13 = 13,
    XMM14 = 14,
    XMM15 = 15,
    kNumberOfFloatRegisters = 16
};

}  // namespace x86_64
}  // namespace whale

#endif  // WHALE_ASSEMBLER_REGISTERS_X86_64_H_
