#ifndef ARCH_REWRITER_ARM_H_
#define ARCH_REWRITER_ARM_H_

#include "assembler/vixl/aarch32/macro-assembler-aarch32.h"
#include "dbi/arm/decoder_thumb.h"
#include "dbi/backup_code.h"
#include "dbi/instruction_rewriter.h"
#include "dbi/instruction_set.h"
#include "base/macros.h"
#include "base/primitive_types.h"
#include "decoder_arm.h"

namespace whale {
namespace arm {

class InstructionIterator {
 public:
    explicit InstructionIterator(intptr_t insn, size_t size) : insns_(insn), index_(0),
                                                               size_(size) {}

    ~InstructionIterator() = default;

    template<typename T>
    T *GetCurrentRef() {
        return reinterpret_cast<T *>(insns_ + index_);
    }

    template<typename T>
    T GetCurrent() {
        return *GetCurrentRef<T>();
    }

    bool HasNext() {
        return index_ < size_;
    }

    void Step(u4 step) {
        DCHECK_LE(index_ + step, size_);
        index_ += step;
    }

    u4 GetIndex() {
        return index_;
    }

 private:
    intptr_t insns_;
    u4 index_;
    size_t size_;
};

class ArmInstructionRewriter : public InstructionReWriter<u4> {
 public:
    ArmInstructionRewriter(vixl::aarch32::MacroAssembler *masm, BackupCode *code,
                           u4 origin_pc, u4 tail_pc, bool is_thumb)
            : masm_(masm), code_(code), cfg_pc_(origin_pc), tail_pc_(tail_pc),
              is_thumb_(is_thumb) {}

    ~ArmInstructionRewriter() {
        delete code_;
    }

    const InstructionSet GetISA() override {
        return InstructionSet::kArm;
    }

    void Rewrite() override;

    u4 *GetStartAddress() override {
        return masm_->GetBuffer()->GetStartAddress<u4 *>();
    }

    size_t GetCodeSize() override {
        return masm_->GetBuffer()->GetSizeInBytes();
    }

 private:
    bool is_thumb_;
    const u4 cfg_pc_;
    const u4 tail_pc_;
    vixl::aarch32::MacroAssembler *masm_;
    BackupCode *code_;

    void EmitT16(const u2 insn) {
        masm_->GetBuffer()->Emit16(insn);
    }

    void EmitT32(const u4 insn) {
        EmitT16(static_cast<const u2>(insn >> 16));
        EmitT16(static_cast<const u2>(insn & 0xffff));
    }

    void EmitA32(const u4 insn) {
        masm_->GetBuffer()->Emit32(insn);
    }

    void RewriteThumb();

    void RewriteArm();

    void RewriteThumb_LDR_PC16(u4 pc, u2 insn);

    void RewriteThumb_DataProcessing16(ThumbInsnType type, u4 cfg_pc, u2 insn);

    void RewriteThumb_B_COND16(u4 cfg_pc, u2 insn);

    void RewriteThumb_B16(u4 pc, u2 insn);

    void RewriteThumb_ADD_FROM_PC16(u4 pc, u2 insn);

    void RewriteThumb_BX16(ThumbInsnType type, u4 pc, u2 insn);

    void RewriteThumb_CBZ16(ThumbInsnType type, u4 pc, u2 insn);

    void RewriteThumb_LDRL32(ThumbInsnType type, u4 pc, u4 insn);

    void RewriteThumb_B32(ThumbInsnType type, u4 pc, u4 insn);

    void RewriteArm_LDR(ArmInsnType type, u4 pc, u4 insn);

    void RewriteArm_Add(ArmInsnType type, u4 pc, u4 insn);

    void RewriteArm_B(ArmInsnType type, u4 pc, u4 insn);
};


}  // namespace arm
}  // namespace whale


#endif  // ARCH_REWRITER_ARM_H_


