#ifndef WHALE_ASSEMBLER_X86_REGISTERS_X86_H_
#define WHALE_ASSEMBLER_X86_REGISTERS_X86_H_

#include <iosfwd>

namespace whale {
namespace x86 {

enum Register {
    EAX = 0,
    ECX = 1,
    EDX = 2,
    EBX = 3,
    ESP = 4,
    EBP = 5,
    ESI = 6,
    EDI = 7,
    kNumberOfCpuRegisters = 8,
    kFirstByteUnsafeRegister = 4,
    kNoRegister = -1  // Signals an illegal register.
};

enum XmmRegister {
    XMM0 = 0,
    XMM1 = 1,
    XMM2 = 2,
    XMM3 = 3,
    XMM4 = 4,
    XMM5 = 5,
    XMM6 = 6,
    XMM7 = 7,
    kNumberOfXmmRegisters = 8,
    kNoXmmRegister = -1  // Signals an illegal register.
};

}  // namespace x86
}  // namespace whale

#endif  // WHALE_ASSEMBLER_X86_REGISTERS_X86_H_
