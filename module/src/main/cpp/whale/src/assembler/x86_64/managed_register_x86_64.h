#ifndef WHALE_ASSEMBLER_MANAGED_REGISTER_X86_64_H_
#define WHALE_ASSEMBLER_MANAGED_REGISTER_X86_64_H_

#include "base/logging.h"
#include "assembler/x86_64/registers_x86_64.h"
#include "assembler/x86_64/constants_x86_64.h"
#include "assembler/managed_register.h"

namespace whale {
namespace x86_64 {


// Values for register pairs.
// The registers in kReservedCpuRegistersArray in x86.cc are not used in pairs.
// The table kRegisterPairs in x86.cc must be kept in sync with this enum.
enum RegisterPair {
    RAX_RDX = 0,
    RAX_RCX = 1,
    RAX_RBX = 2,
    RAX_RDI = 3,
    RDX_RCX = 4,
    RDX_RBX = 5,
    RDX_RDI = 6,
    RCX_RBX = 7,
    RCX_RDI = 8,
    RBX_RDI = 9,
    kNumberOfRegisterPairs = 10,
    kNoRegisterPair = -1,
};

std::ostream &operator<<(std::ostream &os, const RegisterPair &reg);

const int kNumberOfCpuRegIds = kNumberOfCpuRegisters;
const int kNumberOfCpuAllocIds = kNumberOfCpuRegisters;

const int kNumberOfXmmRegIds = kNumberOfFloatRegisters;
const int kNumberOfXmmAllocIds = kNumberOfFloatRegisters;

const int kNumberOfX87RegIds = kNumberOfX87Registers;
const int kNumberOfX87AllocIds = kNumberOfX87Registers;

const int kNumberOfPairRegIds = kNumberOfRegisterPairs;

const int kNumberOfRegIds = kNumberOfCpuRegIds + kNumberOfXmmRegIds +
                            kNumberOfX87RegIds + kNumberOfPairRegIds;
const int kNumberOfAllocIds = kNumberOfCpuAllocIds + kNumberOfXmmAllocIds +
                              kNumberOfX87RegIds;

// Register ids map:
//   [0..R[  cpu registers (enum Register)
//   [R..X[  xmm registers (enum XmmRegister)
//   [X..S[  x87 registers (enum X87Register)
//   [S..P[  register pairs (enum RegisterPair)
// where
//   R = kNumberOfCpuRegIds
//   X = R + kNumberOfXmmRegIds
//   S = X + kNumberOfX87RegIds
//   P = X + kNumberOfRegisterPairs

// Allocation ids map:
//   [0..R[  cpu registers (enum Register)
//   [R..X[  xmm registers (enum XmmRegister)
//   [X..S[  x87 registers (enum X87Register)
// where
//   R = kNumberOfCpuRegIds
//   X = R + kNumberOfXmmRegIds
//   S = X + kNumberOfX87RegIds


// An instance of class 'ManagedRegister' represents a single cpu register (enum
// Register), an xmm register (enum XmmRegister), or a pair of cpu registers
// (enum RegisterPair).
// 'ManagedRegister::NoRegister()' provides an invalid register.
// There is a one-to-one mapping between ManagedRegister and register id.
class X86_64ManagedRegister : public ManagedRegister {
 public:
    constexpr CpuRegister AsCpuRegister() const {
        return CpuRegister(static_cast<Register>(id_));
    }

    constexpr XmmRegister AsXmmRegister() const {
        return XmmRegister(static_cast<FloatRegister>(id_ - kNumberOfCpuRegIds));
    }

    constexpr X87Register AsX87Register() const {
        return static_cast<X87Register>(id_ -
                                        (kNumberOfCpuRegIds + kNumberOfXmmRegIds));
    }

    constexpr CpuRegister AsRegisterPairLow() const {
        // Appropriate mapping of register ids allows to use AllocIdLow().
        return FromRegId(AllocIdLow()).AsCpuRegister();
    }

    constexpr CpuRegister AsRegisterPairHigh() const {
        // Appropriate mapping of register ids allows to use AllocIdHigh().
        return FromRegId(AllocIdHigh()).AsCpuRegister();
    }

    constexpr bool IsCpuRegister() const {
        return (0 <= id_) && (id_ < kNumberOfCpuRegIds);
    }

    constexpr bool IsXmmRegister() const {
        const int test = id_ - kNumberOfCpuRegIds;
        return (0 <= test) && (test < kNumberOfXmmRegIds);
    }

    constexpr bool IsX87Register() const {
        const int test = id_ - (kNumberOfCpuRegIds + kNumberOfXmmRegIds);
        return (0 <= test) && (test < kNumberOfX87RegIds);
    }

    constexpr bool IsRegisterPair() const {
        const int test = id_ -
                         (kNumberOfCpuRegIds + kNumberOfXmmRegIds + kNumberOfX87RegIds);
        return (0 <= test) && (test < kNumberOfPairRegIds);
    }

    // Returns true if the two managed-registers ('this' and 'other') overlap.
    // Either managed-register may be the NoRegister. If both are the NoRegister
    // then false is returned.
    bool Overlaps(const X86_64ManagedRegister &other) const;

    static constexpr X86_64ManagedRegister FromCpuRegister(Register r) {
        return FromRegId(r);
    }

    static constexpr X86_64ManagedRegister FromXmmRegister(FloatRegister r) {
        return FromRegId(r + kNumberOfCpuRegIds);
    }

    static constexpr X86_64ManagedRegister FromX87Register(X87Register r) {
        return FromRegId(r + kNumberOfCpuRegIds + kNumberOfXmmRegIds);
    }

    static constexpr X86_64ManagedRegister FromRegisterPair(RegisterPair r) {
        return FromRegId(r + (kNumberOfCpuRegIds + kNumberOfXmmRegIds +
                              kNumberOfX87RegIds));
    }

 private:
    constexpr bool IsValidManagedRegister() const {
        return (0 <= id_) && (id_ < kNumberOfRegIds);
    }

    constexpr int RegId() const {
        return id_;
    }

    int AllocId() const {
        return id_;
    }

    int AllocIdLow() const;

    int AllocIdHigh() const;

    friend class ManagedRegister;

    explicit constexpr X86_64ManagedRegister(int reg_id) : ManagedRegister(reg_id) {}

    static constexpr X86_64ManagedRegister FromRegId(int reg_id) {
        X86_64ManagedRegister reg(reg_id);
        return reg;
    }
};

}  // namespace x86_64
}  // namespace whale

#endif  // WHALE_ASSEMBLER_MANAGED_REGISTER_X86_64_H_
