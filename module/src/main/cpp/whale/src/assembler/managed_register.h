#ifndef MYAPPLICATION_MANAGED_REGISTER_H
#define MYAPPLICATION_MANAGED_REGISTER_H

#include "assembler/value_object.h"

namespace whale {

class ManagedRegister : public ValueObject {
 public:
    // ManagedRegister is a value class. There exists no method to change the
    // internal state. We therefore allow a copy constructor and an
    // assignment-operator.
    constexpr ManagedRegister(const ManagedRegister &other) = default;

    ManagedRegister &operator=(const ManagedRegister &other) = default;

    // It is valid to invoke Equals on and with a NoRegister.
    constexpr bool Equals(const ManagedRegister &other) const {
        return id_ == other.id_;
    }

    constexpr bool IsNoRegister() const {
        return id_ == kNoRegister;
    }

    static constexpr ManagedRegister NoRegister() {
        return ManagedRegister();
    }

    constexpr int RegId() const { return id_; }

    explicit constexpr ManagedRegister(int reg_id) : id_(reg_id) {}

 protected:
    static const int kNoRegister = -1;

    constexpr ManagedRegister() : id_(kNoRegister) {}

    int id_;
};

}  // namespace whale

#endif //MYAPPLICATION_MANAGED_REGISTER_H
