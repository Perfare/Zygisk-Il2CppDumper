#ifndef ARCH_X86_REWRITER_X86_H_
#define ARCH_X86_REWRITER_X86_H_

#include <dbi/x86/distorm/distorm.h>
#include "assembler/x86/assembler_x86.h"
#include "base/primitive_types.h"
#include "dbi/backup_code.h"
#include "dbi/instruction_rewriter.h"
#include "dbi/instruction_set.h"
#include "base/macros.h"

namespace whale {
namespace x86 {

class X86InstructionRewriter : public InstructionReWriter<u1> {
 public:
    X86InstructionRewriter(whale::x86::X86Assembler *masm, BackupCode *code,
                             u4 origin_pc, u4 tail_pc)
            : masm_(masm), code_(code), cfg_pc_(origin_pc), tail_pc_(tail_pc) {}

    ~X86InstructionRewriter() = default;

    const InstructionSet GetISA() override {
        return InstructionSet::kX86;
    }

    void Rewrite() override;

    u1 *GetStartAddress() override {
        return masm_->GetBuffer()->contents();
    }

    size_t GetCodeSize() override {
        return masm_->GetBuffer()->Size();
    }

    void EmitCode(u1 *start, size_t size) {
        for (int i = 0; i < size; ++i) {
            AssemblerBuffer::EnsureCapacity ensured(masm_->GetBuffer());
            masm_->GetBuffer()->Emit<u1>(start[i]);
        }
    }

 private:
    const u4 cfg_pc_;
    const u4 tail_pc_;
    whale::x86::X86Assembler *masm_;
    BackupCode *code_;

    void Rewrite_Call(u1 *current, u4 pc, _DInst insn);

    bool IsGetPCThunkToRegister(u4 address, Register *reg);

    void Rewrite_Jmp(u1 *current, u4 pc, _DInst insn);

    void Rewrite_JRCXZ(u1 *current, u4 pc, _DInst insn);
};


}  // namespace arm64
}  // namespace whale


#endif  // ARCH_X86_REWRITER_X86_H_


