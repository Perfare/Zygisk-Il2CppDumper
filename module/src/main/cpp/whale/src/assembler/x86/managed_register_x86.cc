#include "assembler/x86/managed_register_x86.h"

namespace whale {
namespace x86 {

// Define register pairs.
// This list must be kept in sync with the RegisterPair enum.
#define REGISTER_PAIR_LIST(P) \
  P(EAX, EDX)                 \
  P(EAX, ECX)                 \
  P(EAX, EBX)                 \
  P(EAX, EDI)                 \
  P(EDX, ECX)                 \
  P(EDX, EBX)                 \
  P(EDX, EDI)                 \
  P(ECX, EBX)                 \
  P(ECX, EDI)                 \
  P(EBX, EDI)                 \
  P(ECX, EDX)


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


bool X86ManagedRegister::Overlaps(const X86ManagedRegister &other) const {
    if (IsNoRegister() || other.IsNoRegister()) return false;
    CHECK(IsValidManagedRegister());
    CHECK(other.IsValidManagedRegister());
    if (Equals(other)) return true;
    if (IsRegisterPair()) {
        Register low = AsRegisterPairLow();
        Register high = AsRegisterPairHigh();
        return X86ManagedRegister::FromCpuRegister(low).Overlaps(other) ||
               X86ManagedRegister::FromCpuRegister(high).Overlaps(other);
    }
    if (other.IsRegisterPair()) {
        return other.Overlaps(*this);
    }
    return false;
}


int X86ManagedRegister::AllocIdLow() const {
    const int r = RegId() - (kNumberOfCpuRegIds + kNumberOfXmmRegIds +
                             kNumberOfX87RegIds);
    return kRegisterPairs[r].low;
}


int X86ManagedRegister::AllocIdHigh() const {
    const int r = RegId() - (kNumberOfCpuRegIds + kNumberOfXmmRegIds +
                             kNumberOfX87RegIds);
    return kRegisterPairs[r].high;
}


}  // namespace x86
}  // namespace whale
