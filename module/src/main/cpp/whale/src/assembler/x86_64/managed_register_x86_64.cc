#include "assembler/x86_64/managed_register_x86_64.h"

namespace whale {
namespace x86_64 {

// Define register pairs.
// This list must be kept in sync with the RegisterPair enum.
#define REGISTER_PAIR_LIST(P) \
  P(RAX, RDX)                 \
  P(RAX, RCX)                 \
  P(RAX, RBX)                 \
  P(RAX, RDI)                 \
  P(RDX, RCX)                 \
  P(RDX, RBX)                 \
  P(RDX, RDI)                 \
  P(RCX, RBX)                 \
  P(RCX, RDI)                 \
  P(RBX, RDI)


struct RegisterPairDescriptor {
    RegisterPair reg;  // Used to verify that the enum is in sync.
    Register low;
    Register high;
};


static const RegisterPairDescriptor kRegisterPairs[] = {
#define REGISTER_PAIR_ENUMERATION(low, high) { low##_##high, low, high },
        REGISTER_PAIR_LIST(REGISTER_PAIR_ENUMERATION)
#undef REGISTER_PAIR_ENUMERATION
};


bool X86_64ManagedRegister::Overlaps(const X86_64ManagedRegister &other) const {
    if (IsNoRegister() || other.IsNoRegister()) return false;
    if (Equals(other)) return true;
    if (IsRegisterPair()) {
        Register low = AsRegisterPairLow().AsRegister();
        Register high = AsRegisterPairHigh().AsRegister();
        return X86_64ManagedRegister::FromCpuRegister(low).Overlaps(other) ||
               X86_64ManagedRegister::FromCpuRegister(high).Overlaps(other);
    }
    if (other.IsRegisterPair()) {
        return other.Overlaps(*this);
    }
    return false;
}


int X86_64ManagedRegister::AllocIdLow() const {
    const int r = RegId() - (kNumberOfCpuRegIds + kNumberOfXmmRegIds +
                             kNumberOfX87RegIds);
    return kRegisterPairs[r].low;
}


int X86_64ManagedRegister::AllocIdHigh() const {
    const int r = RegId() - (kNumberOfCpuRegIds + kNumberOfXmmRegIds +
                             kNumberOfX87RegIds);
    return kRegisterPairs[r].high;
}


}  // namespace x86_64
}  // namespace whale
