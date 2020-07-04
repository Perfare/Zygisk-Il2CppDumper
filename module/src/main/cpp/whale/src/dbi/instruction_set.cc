#include <base/macros.h>
#include <base/logging.h>
#include "dbi/instruction_set.h"

namespace whale {

std::ostream &operator<<(std::ostream &os, const InstructionSet &rhs) {
    return os << GetInstructionSetString(rhs);
}

void InstructionSetAbort(InstructionSet isa) {
    switch (isa) {
        case InstructionSet::kArm:
        case InstructionSet::kThumb2:
        case InstructionSet::kArm64:
        case InstructionSet::kX86:
        case InstructionSet::kX86_64:
        case InstructionSet::kMips:
        case InstructionSet::kMips64:
        case InstructionSet::kNone:
            LOG(FATAL) << "Unsupported instruction set " << isa;
            UNREACHABLE();
    }
    LOG(FATAL) << "Unknown ISA " << isa;
    UNREACHABLE();
}

const char *GetInstructionSetString(InstructionSet isa) {
    switch (isa) {
        case InstructionSet::kArm:
        case InstructionSet::kThumb2:
            return "arm";
        case InstructionSet::kArm64:
            return "arm64";
        case InstructionSet::kX86:
            return "x86";
        case InstructionSet::kX86_64:
            return "x86_64";
        case InstructionSet::kMips:
            return "mips";
        case InstructionSet::kMips64:
            return "mips64";
        case InstructionSet::kNone:
            return "none";
    }
    LOG(FATAL) << "Unknown ISA " << isa;
    UNREACHABLE();
}


size_t GetInstructionSetAlignment(InstructionSet isa) {
    switch (isa) {
        case InstructionSet::kArm:
            // Fall-through.
        case InstructionSet::kThumb2:
            return kArmAlignment;
        case InstructionSet::kArm64:
            return kArm64Alignment;
        case InstructionSet::kX86:
            // Fall-through.
        case InstructionSet::kX86_64:
            return kX86Alignment;
        case InstructionSet::kMips:
            // Fall-through.
        case InstructionSet::kMips64:
            return kMipsAlignment;
        case InstructionSet::kNone:
            LOG(FATAL) << "ISA kNone does not have alignment.";
            UNREACHABLE();
    }
    LOG(FATAL) << "Unknown ISA " << isa;
    UNREACHABLE();
}


}  // namespace whale

