#ifndef WHALE_ASSEMBLER_MANAGED_REGISTER_X86_H_
#define WHALE_ASSEMBLER_MANAGED_REGISTER_X86_H_

#include <ostream>
#include "assembler/x86/registers_x86.h"
#include "assembler/x86/constants_x86.h"
#include "assembler/managed_register.h"
#include "base/logging.h"

namespace whale {
namespace x86 {

enum RegisterPair {
    EAX_EDX = 0,
    EAX_ECX = 1,
    EAX_EBX = 2,
    EAX_EDI = 3,
    EDX_ECX = 4,
    EDX_EBX = 5,
    EDX_EDI = 6,
    ECX_EBX = 7,
    ECX_EDI = 8,
    EBX_EDI = 9,
    ECX_EDX = 10,
    kNumberOfRegisterPairs = 11,
    kNoRegisterPair = -1,
};

const int kNumberOfCpuRegIds = kNumberOfCpuRegisters;
const int kNumberOfCpuAllocIds = kNumberOfCpuRegisters;

const int kNumberOfXmmRegIds = kNumberOfXmmRegisters;
const int kNumberOfXmmAllocIds = kNumberOfXmmRegisters;

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
class X86ManagedRegister : public ManagedRegister {
 public:
    constexpr ByteRegister AsByteRegister() const {
        return static_cast<ByteRegister>(id_);
    }

    constexpr Register AsCpuRegister() const {
        return static_cast<Register>(id_);
    }

    constexpr XmmRegister AsXmmRegister() const {
        return static_cast<XmmRegister>(id_ - kNumberOfCpuRegIds);
    }

    constexpr X87Register AsX87Register() const {
        return static_cast<X87Register>(id_ -
                                        (kNumberOfCpuRegIds + kNumberOfXmmRegIds));
    }

    constexpr Register AsRegisterPairLow() const {
        // Appropriate mapping of register ids allows to use AllocIdLow().
        return FromRegId(AllocIdLow()).AsCpuRegister();
    }

    constexpr Register AsRegisterPairHigh() const {
        // Appropriate mapping of register ids allows to use AllocIdHigh().
        return FromRegId(AllocIdHigh()).AsCpuRegister();
    }

    constexpr RegisterPair AsRegisterPair() const {
        return static_cast<RegisterPair>(id_ -
                                         (kNumberOfCpuRegIds + kNumberOfXmmRegIds +
                                          kNumberOfX87RegIds));
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
    bool Overlaps(const X86ManagedRegister &other) const;

    static constexpr X86ManagedRegister FromCpuRegister(Register r) {
        return FromRegId(r);
    }

    static constexpr X86ManagedRegister FromXmmRegister(XmmRegister r) {
        return FromRegId(r + kNumberOfCpuRegIds);
    }

    static constexpr X86ManagedRegister FromX87Register(X87Register r) {
        return FromRegId(r + kNumberOfCpuRegIds + kNumberOfXmmRegIds);
    }

    static constexpr X86ManagedRegister FromRegisterPair(RegisterPair r) {
        return FromRegId(r + (kNumberOfCpuRegIds + kNumberOfXmmRegIds +
                              kNumberOfX87RegIds));
    }

 private:
    constexpr bool IsValidManagedRegister() const {
        return (0 <= id_) && (id_ < kNumberOfRegIds);
    }

    constexpr int RegId() const {
        CHECK(!IsNoRegister());
        return id_;
    }

    int AllocId() const {
        return id_;
    }

    int AllocIdLow() const;

    int AllocIdHigh() const;

    friend class ManagedRegister;

    explicit constexpr X86ManagedRegister(int reg_id) : ManagedRegister(reg_id) {}

    static constexpr X86ManagedRegister FromRegId(int reg_id) {
        X86ManagedRegister reg(reg_id);
        return reg;
    }
};

}  // namespace x86
}  // namespace whale

#endif  // WHALE_ASSEMBLER_MANAGED_REGISTER_X86_H_
