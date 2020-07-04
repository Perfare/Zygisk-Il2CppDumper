#ifndef WHALE_BASE_ENUMS_H_
#define WHALE_BASE_ENUMS_H_


#include <cstddef>
#include <ostream>

enum class PointerSize : size_t {
    k32 = 4,
    k64 = 8
};

inline std::ostream &operator<<(std::ostream &os, const PointerSize &rhs) {
    switch (rhs) {
        case PointerSize::k32:
            os << "k32";
            break;
        case PointerSize::k64:
            os << "k64";
            break;
        default:
            os << "PointerSize[" << static_cast<int>(rhs) << "]";
            break;
    }
    return os;
}

static constexpr PointerSize kRuntimePointerSize = sizeof(void *) == 8U
                                                   ? PointerSize::k64
                                                   : PointerSize::k32;

#endif // WHALE_BASE_ENUMS_H_
