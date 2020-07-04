#ifndef WHALE_BASE_CXX_HELPER_H_
#define WHALE_BASE_CXX_HELPER_H_

#include <cstdlib>
#include "base/primitive_types.h"

template<typename U, typename T>
U ForceCast(T *x) {
    return (U) (uintptr_t) x;
}

template<typename U, typename T>
U ForceCast(T &x) {
    return *(U *) &x;
}

template<typename T>
struct Identity {
    using type = T;
};

template<typename R>
static inline R OffsetOf(uintptr_t ptr, size_t offset) {
    return reinterpret_cast<R>(ptr + offset);
}

template<typename R>
static inline R OffsetOf(intptr_t ptr, size_t offset) {
    return reinterpret_cast<R>(ptr + offset);
}

template<typename R>
static inline R OffsetOf(ptr_t ptr, size_t offset) {
    return (R) (reinterpret_cast<intptr_t>(ptr) + offset);
}

template<typename T>
static inline T MemberOf(ptr_t ptr, size_t offset) {
    return *OffsetOf<T *>(ptr, offset);
}

static inline size_t DistanceOf(ptr_t a, ptr_t b) {
    return static_cast<size_t>(
            abs(reinterpret_cast<intptr_t>(b) - reinterpret_cast<intptr_t>(a))
    );
}

template<typename T>
static inline void AssignOffset(ptr_t ptr, size_t offset, T member) {
    *OffsetOf<T *>(ptr, offset) = member;
}

#endif  // WHALE_BASE_CXX_HELPER_H_
