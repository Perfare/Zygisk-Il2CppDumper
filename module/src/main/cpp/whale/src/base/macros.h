#ifndef WHALE_BASE_MACROS_H_
#define WHALE_BASE_MACROS_H_

#define DISALLOW_ALLOCATION() \
  public: \
    NO_RETURN ALWAYS_INLINE void operator delete(void*, size_t) { UNREACHABLE(); } \
    ALWAYS_INLINE void* operator new(size_t, void* ptr) noexcept { return ptr; } \
    ALWAYS_INLINE void operator delete(void*, void*) noexcept { } \
  private: \
    void* operator new(size_t) = delete  // NOLINT

#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
private:                     \
TypeName();                                    \
DISALLOW_COPY_AND_ASSIGN(TypeName)

#define ALIGNED(x) __attribute__ ((__aligned__(x)))
#define PACKED(x) __attribute__ ((__aligned__(x), __packed__))

#define OFFSETOF_MEMBER(t, f) offsetof(t, f)

// Stringify the argument.
#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)

#ifndef NDEBUG
#define ALWAYS_INLINE
#else
#define ALWAYS_INLINE  __attribute__ ((always_inline))
#endif

// Define that a position within code is unreachable, for example:
//   int foo () { LOG(FATAL) << "Don't call me"; UNREACHABLE(); }
// without the UNREACHABLE a return statement would be necessary.
#define UNREACHABLE  __builtin_unreachable

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define NO_RETURN [[ noreturn ]]

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);   \
  void operator=(const TypeName&)

#define OPTION __unused

#define OPEN_API __attribute__((visibility("default")))
#define C_API extern "C"

#endif  // WHALE_BASE_MACROS_H_
