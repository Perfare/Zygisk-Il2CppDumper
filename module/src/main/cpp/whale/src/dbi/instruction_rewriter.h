#ifndef ARCH_INSTRUCTION_REWRITER_H_
#define ARCH_INSTRUCTION_REWRITER_H_

#include "assembler/vixl/code-buffer-vixl.h"
#include "dbi/instruction_set.h"

namespace whale {

using CodeBuffer = vixl::CodeBuffer;

template<typename InsnType>
class InstructionReWriter {

    virtual const InstructionSet GetISA() {
        return InstructionSet::kNone;
    }

    virtual InsnType *GetStartAddress() = 0;

    virtual size_t GetCodeSize() = 0;

    virtual void Rewrite() = 0;

};

}  // namespace whale

#endif  // ARCH_INSTRUCTION_REWRITER_H_


