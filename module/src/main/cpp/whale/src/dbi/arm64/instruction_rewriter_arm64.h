#ifndef ARCH_REWRITER_ARM64_H_
#define ARCH_REWRITER_ARM64_H_

#include "assembler/vixl/aarch64/macro-assembler-aarch64.h"
#include "dbi/backup_code.h"
#include "dbi/instruction_rewriter.h"
#include "dbi/instruction_set.h"
#include "base/primitive_types.h"
#include "base/macros.h"

namespace whale {
namespace arm64 {

class Arm64InstructionRewriter : public InstructionReWriter<u4> {
 public:
    Arm64InstructionRewriter(vixl::aarch64::MacroAssembler *masm, BackupCode *code,
                             u8 origin_pc, u8 tail_pc)
            : masm_(masm), code_(code), cfg_pc_(origin_pc), tail_pc_(tail_pc) {}

    ~Arm64InstructionRewriter() = default;

    const InstructionSet GetISA() override {
        return InstructionSet::kArm64;
    }

    void Rewrite() override;

    ALWAYS_INLINE u4 *GetStartAddress() override {
        return masm_->GetBuffer()->GetStartAddress<u4 *>();
    }

    ALWAYS_INLINE size_t GetCodeSize() override {
        return masm_->GetBuffer()->GetSizeInBytes();
    }

 private:
    const u8 cfg_pc_;
    const u8 tail_pc_;
    vixl::aarch64::MacroAssembler *masm_;
    BackupCode *code_;

    ALWAYS_INLINE void EmitCode(u4 insn) {
        masm_->GetBuffer()->Emit32(insn);
    }

    void RewriteADR_ADRP(u8 pc, u4 insn);

    void RewriteB_BL(u8 pc, u4 insn);

    void RewriteCBZ_CBNZ(u8 pc, u4 insn);

    void RewriteB_COND(u8 pc, u4 insn);

    void RewriteTBZ_TBNZ(u8 pc, u4 insn);

    void RewriteLDR_LIT(u8 pc, u4 insn);
};


}  // namespace arm64
}  // namespace whale


#endif  // ARCH_REWRITER_ARM64_H_


