#ifndef WHALE_ARCH_INSTRUCTION_SET_H_
#define WHALE_ARCH_INSTRUCTION_SET_H_

#include "base/macros.h"
#include <iostream>

namespace whale {

enum class InstructionSet {
    kNone,
    kArm,
    kArm64,
    kThumb2,
    kX86,
    kX86_64,
    kMips,
    kMips64,
    kLast = kMips64
};

std::ostream &operator<<(std::ostream &os, const InstructionSet &rhs);

#if defined(__arm__)
static constexpr InstructionSet kRuntimeISA = InstructionSet::kArm;
#elif defined(__aarch64__) || defined(__arm64__)
static constexpr InstructionSet kRuntimeISA = InstructionSet::kArm64;
#elif defined(__mips__) && !defined(__LP64__)
static constexpr InstructionSet kRuntimeISA = InstructionSet::kMips;
#elif defined(__mips__) && defined(__LP64__)
static constexpr InstructionSet kRuntimeISA = InstructionSet::kMips64;
#elif defined(__i386__)
static constexpr InstructionSet kRuntimeISA = InstructionSet::kX86;
#elif defined(__x86_64__)
static constexpr InstructionSet kRuntimeISA = InstructionSet::kX86_64;
#else
static constexpr InstructionSet kRuntimeISA = InstructionSet::kNone;
#endif

static constexpr size_t kPointerSize = sizeof(void *);

static constexpr size_t kArmAlignment = 8;

static constexpr size_t kArm64Alignment = 16;

static constexpr size_t kMipsAlignment = 8;

static constexpr size_t kX86Alignment = 16;

static constexpr size_t kArmInstructionAlignment = 4;
static constexpr size_t kThumb2InstructionAlignment = 2;
static constexpr size_t kArm64InstructionAlignment = 4;
static constexpr size_t kX86InstructionAlignment = 1;
static constexpr size_t kX86_64InstructionAlignment = 1;
static constexpr size_t kMipsInstructionAlignment = 4;
static constexpr size_t kMips64InstructionAlignment = 4;

const char *GetInstructionSetString(InstructionSet isa);

// Note: Returns kNone when the string cannot be parsed to a known value.
InstructionSet GetInstructionSetFromString(const char *instruction_set);

// Fatal logging out of line to keep the header clean of logging.h.
NO_RETURN void InstructionSetAbort(InstructionSet isa);


constexpr bool Is64BitInstructionSet(InstructionSet isa) {
    switch (isa) {
        case InstructionSet::kArm:
        case InstructionSet::kThumb2:
        case InstructionSet::kX86:
        case InstructionSet::kMips:
            return false;

        case InstructionSet::kArm64:
        case InstructionSet::kX86_64:
        case InstructionSet::kMips64:
            return true;

        case InstructionSet::kNone:
            break;
    }
    InstructionSetAbort(isa);
}

constexpr size_t GetInstructionSetInstructionAlignment(InstructionSet isa) {
    switch (isa) {
        case InstructionSet::kArm:
            // Fall-through.
        case InstructionSet::kThumb2:
            return kThumb2InstructionAlignment;
        case InstructionSet::kArm64:
            return kArm64InstructionAlignment;
        case InstructionSet::kX86:
            return kX86InstructionAlignment;
        case InstructionSet::kX86_64:
            return kX86_64InstructionAlignment;
        case InstructionSet::kMips:
            return kMipsInstructionAlignment;
        case InstructionSet::kMips64:
            return kMips64InstructionAlignment;

        case InstructionSet::kNone:
            break;
    }
    InstructionSetAbort(isa);
}

}  // namespace whale

#endif  // WHALE_ARCH_INSTRUCTION_SET_H_
