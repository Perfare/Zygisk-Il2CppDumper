#ifndef WHALE_BASE_OFFSETS_H_
#define WHALE_BASE_OFFSETS_H_

#include <cstddef>
#include <cstdint>
#include "base/enums.h"

namespace whale {


// Allow the meaning of offsets to be strongly typed.
class Offset {
 public:
    constexpr explicit Offset(size_t val) : val_(val) {}

    constexpr int32_t Int32Value() const {
        return static_cast<int32_t>(val_);
    }

    constexpr uint32_t Uint32Value() const {
        return static_cast<uint32_t>(val_);
    }

    constexpr size_t SizeValue() const {
        return val_;
    }

 protected:
    size_t val_;
};

// Offsets relative to the current frame.
class FrameOffset : public Offset {
 public:
    constexpr explicit FrameOffset(size_t val) : Offset(val) {}

    bool operator>(FrameOffset other) const { return val_ > other.val_; }

    bool operator<(FrameOffset other) const { return val_ < other.val_; }
};

// Offsets relative to the current running thread.
template<PointerSize pointer_size>
class ThreadOffset : public Offset {
 public:
    constexpr explicit ThreadOffset(size_t val) : Offset(val) {}
};

using ThreadOffset32 = ThreadOffset<PointerSize::k32>;
using ThreadOffset64 = ThreadOffset<PointerSize::k64>;

// Offsets relative to an object.
class MemberOffset : public Offset {
 public:
    constexpr explicit MemberOffset(size_t val) : Offset(val) {}
};

}

#endif  // WHALE_BASE_OFFSETS_H_
