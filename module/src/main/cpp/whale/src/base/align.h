#ifndef WHALE_ASSEMBLER_ASSEMBLER_UTILS_H_
#define WHALE_ASSEMBLER_ASSEMBLER_UTILS_H_


#include <cstddef>
#include <zconf.h>
#include <unistd.h>
#include "base/cxx_helper.h"
#include "base/logging.h"
#include "base/macros.h"


// Use implicit_cast as a safe version of static_cast or const_cast
// for upcasting in the type hierarchy (i.e. casting a pointer to Foo
// to a pointer to SuperclassOfFoo or casting a pointer to Foo to
// a const pointer to Foo).
// When you use implicit_cast, the compiler checks that the cast is safe.
// Such explicit implicit_casts are necessary in surprisingly many
// situations where C++ demands an exact type match instead of an
// argument type convertible to a target type.
//
// The From type can be inferred, so the preferred syntax for using
// implicit_cast is the same as for static_cast etc.:
//
//   implicit_cast<ToType>(expr)
//
// implicit_cast would have been part of the C++ standard library,
// but the proposal was submitted too late.  It will probably make
// its way into the language in the future.
template<typename To, typename From>
inline To implicit_cast(From const &f) {
    return f;
}

// When you upcast (that is, cast a pointer from type Foo to type
// SuperclassOfFoo), it's fine to use implicit_cast<>, since upcasts
// always succeed.  When you downcast (that is, cast a pointer from
// type Foo to type SubclassOfFoo), static_cast<> isn't safe, because
// how do you know the pointer is really of type SubclassOfFoo?  It
// could be a bare Foo, or of type DifferentSubclassOfFoo.  Thus,
// when you downcast, you should use this macro.  In debug mode, we
// use dynamic_cast<> to double-check the downcast is legal (we die
// if it's not).  In normal mode, we do the efficient static_cast<>
// instead.  Thus, it's important to test in debug mode to make sure
// the cast is legal!
//    This is the only place in the code we should use dynamic_cast<>.
// In particular, you SHOULDN'T be using dynamic_cast<> in order to
// do RTTI (eg code like this:
//    if (dynamic_cast<Subclass1>(foo)) HandleASubclass1Object(foo);
//    if (dynamic_cast<Subclass2>(foo)) HandleASubclass2Object(foo);
// You should design the code some other way not to need this.

template<typename To, typename From>
// use like this: down_cast<T*>(foo);
inline To down_cast(From *f) {                   // so we only accept pointers
    static_assert(std::is_base_of<From, typename std::remove_pointer<To>::type>::value,
                  "down_cast unsafe as To is not a subtype of From");

    return static_cast<To>(f);
}

template<typename To, typename From>
// use like this: down_cast<T&>(foo);
inline To down_cast(From &f) {           // so we only accept references
    static_assert(std::is_base_of<From, typename std::remove_reference<To>::type>::value,
                  "down_cast unsafe as To is not a subtype of From");

    return static_cast<To>(f);
}

template<class Dest, class Source>
inline Dest bit_cast(const Source &source) {
    // Compile time assertion: sizeof(Dest) == sizeof(Source)
    // A compile error here means your Dest and Source have different sizes.
    static_assert(sizeof(Dest) == sizeof(Source), "sizes should be equal");
    Dest dest;
    memcpy(&dest, &source, sizeof(dest));
    return dest;
}

template<typename T>
constexpr bool IsPowerOfTwo(T x) {
    static_assert(std::is_integral<T>::value, "T must be integral");
    return (x & (x - 1)) == 0;
}

template<typename T>
constexpr T RoundDown(T x, typename Identity<T>::type n) {
    DCHECK(IsPowerOfTwo(n));
    return (x & -n);
}

template<typename T>
constexpr T RoundUp(T x, typename std::remove_reference<T>::type n) {
    return RoundDown(x + n - 1, n);
}


template<typename T>
inline T *AlignDown(T *x, uintptr_t n) {
    return reinterpret_cast<T *>(RoundDown(reinterpret_cast<uintptr_t>(x), n));
}


template<typename T>
inline T *AlignUp(T *x, uintptr_t n) {
    return reinterpret_cast<T *>(RoundUp(reinterpret_cast<uintptr_t>(x), n));
}

template<int n, typename T>
constexpr bool IsAligned(T x) {
    static_assert((n & (n - 1)) == 0, "n is not a power of two");
    return (x & (n - 1)) == 0;
}

template<int n, typename T>
inline bool IsAligned(T *x) {
    return IsAligned<n>(reinterpret_cast<const uintptr_t>(x));
}

static inline size_t GetPageSize() {
    return static_cast<size_t>(sysconf(_SC_PAGE_SIZE));
}

inline intptr_t PageAlign(intptr_t x) {
    return RoundUp(x, GetPageSize());
}

template<typename T>
inline T PageAlign(T x) {
    return RoundUp(x, GetPageSize());
}

inline intptr_t PageStart(intptr_t x) {
    return ~(GetPageSize() - 1) & x;
}

template<typename T>
inline T *PageStart(T *x) {
    return reinterpret_cast<T *>(PageStart(reinterpret_cast<intptr_t>(x)));
}


#endif  // WHALE_ASSEMBLER_ASSEMBLER_UTILS_H_
