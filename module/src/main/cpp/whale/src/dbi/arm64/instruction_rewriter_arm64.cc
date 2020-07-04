#include "dbi/arm64/instruction_rewriter_arm64.h"
#include "dbi/arm64/decoder_arm64.h"
#include "dbi/arm64/registers_arm64.h"
#include "base/logging.h"

#define __ masm_->

namespace whale {
namespace arm64 {
using namespace vixl::aarch64;  // NOLINT

static inline u8 SignExtend64(unsigned int bits, uint64_t value) {
    u8 C = (u8) ((-1) << (bits - (u8) 1)); // NOLINT
    return (value + C) ^ C;
}

void Arm64InstructionRewriter::Rewrite() {
    u8 current_pc = cfg_pc_;
    u4 *insns = code_->GetInstructions<u4>();
    for (int i = 0; i < code_->GetCount<u4>(); ++i) {
        u4 insn = insns[i];
        A64InsnType type = DecodeA64(insn);
        switch (type) {
            case A64InsnType::kA64_ADR_ADRP:
                RewriteADR_ADRP(current_pc, insn);
                break;
            case A64InsnType::kA64_B_BL:
                RewriteB_BL(current_pc, insn);
                break;
            case A64InsnType::kA64_B_COND:
                RewriteB_COND(current_pc, insn);
                break;
            case A64InsnType::kA64_CBZ_CBNZ:
                RewriteCBZ_CBNZ(current_pc, insn);
                break;
            case A64InsnType::kA64_TBZ_TBNZ:
                RewriteTBZ_TBNZ(current_pc, insn);
                break;
            case A64InsnType::kA64_LDR_LIT:
                RewriteLDR_LIT(current_pc, insn);
                break;
            default:
                EmitCode(insn);
                break;
        }
        current_pc += kArm64InstructionAlignment;
    }
    masm_->FinalizeCode();
}

ALWAYS_INLINE void Arm64InstructionRewriter::RewriteADR_ADRP(u8 pc, u4 insn) {
    u4 op = (insn >> 31) & 0x1;
    u4 immlo = (insn >> 29) & 0x3;
    u4 immhi = (insn >> 5) & 0x7ffff;
    auto rd = XReg((insn >> 0) & 0x1f);
    u8 imm = SignExtend64(12, (immhi << 2) | immlo);
    u8 pcrel_address;
    if (op == 0) {  // adr
        pcrel_address = (uint64_t) pc;
    } else {  // adrp
        imm = imm << 12;
        pcrel_address = (uint64_t) pc & ~0xFFF;
    }
    pcrel_address += imm;
    __ Mov(rd, pcrel_address);
}

ALWAYS_INLINE void Arm64InstructionRewriter::RewriteB_BL(u8 pc, u4 insn) {
    u4 op = (insn >> 31) & 0x1;
    u4 imm26 = (insn >> 0) & 0x3ffffff;
    u8 branch_offset = (SignExtend64(26, imm26) << 2);
    u8 target = pc + branch_offset;

    // Check if the jump target still in rewrite range of code,
    // in this cause, there is no need to rewrite it.
    if (target < tail_pc_) {
        EmitCode(insn);
    } else {
        if (op == 1) {  // bl
            __ Mov(lr, pc + kArm64InstructionAlignment);
        }
        __ Mov(xTarget, target);
        __ Br(xTarget);
    }
}

ALWAYS_INLINE void Arm64InstructionRewriter::RewriteCBZ_CBNZ(u8 pc, u4 insn) {
    u4 sf = (insn >> 31) & 0x1;
    u4 op = (insn >> 24) & 0x1;
    u4 imm19 = (insn >> 5) & 0x7ffff;
    u4 rt_value = (insn >> 0) & 0x1f;
    u8 branch_offset = SignExtend64(19, imm19) << 2;
    u8 pcrel_address = pc + branch_offset;

    if (pcrel_address < tail_pc_) {
        EmitCode(insn);
    } else {
        auto rt = sf ? XReg(rt_value) : WReg(rt_value);

        Label true_label, false_label;

        if (op == 1) {
            __ Cbnz(rt, &true_label);
        } else {
            __ Cbz(rt, &true_label);
        }
        __ B(&false_label);

        __ Bind(&true_label);
        __ Mov(xTarget, pcrel_address);
        __ Br(xTarget);

        __ Bind(&false_label);

        masm_->FinalizeCode();
    }
}

void Arm64InstructionRewriter::RewriteB_COND(u8 pc, u4 insn) {
    u4 imm19 = (insn >> 5) & 0x7ffff;
    u4 cond = (insn >> 0) & 0xf;
    u8 branch_offset = SignExtend64(19, imm19) << 2;
    u8 pcrel_address = pc + branch_offset;

    if (pcrel_address < tail_pc_) {
        EmitCode(insn);
    } else {
        Label true_label, false_label;

        __ B(Condition(cond), &true_label);
        __ B(&false_label);

        __ Bind(&true_label);
        __ Mov(xTarget, pcrel_address);
        __ Br(xTarget);

        __ Bind(&false_label);
    }
}

ALWAYS_INLINE void Arm64InstructionRewriter::RewriteTBZ_TBNZ(u8 pc, u4 insn) {
    u4 b5 = (insn >> 31) & 0x1;
    u4 op = (insn >> 24) & 0x1;
    u4 b40 = (insn >> 19) & 0x1f;
    u4 imm14 = (insn >> 5) & 0x3fff;
    u4 rt_value = (insn >> 0) & 0x1f;
    u8 branch_offset = SignExtend64(14, imm14) << 2;
    u4 bit = (b5 << 5) | b40;
    u8 pcrel_address = pc + branch_offset;

    if (pcrel_address < tail_pc_) {
        EmitCode(insn);
    } else {
        auto rt = b5 ? XReg(rt_value) : WReg(rt_value);

        Label true_label, false_label;

        if (op == 1) {
            __ Tbnz(rt, bit, &true_label);
        } else {
            __ Tbz(rt, bit, &true_label);
        }
        __ B(&false_label);

        __ Bind(&true_label);
        __ Mov(xTarget, pcrel_address);
        __ Br(xTarget);

        __ Bind(&false_label);
    }
}

/*
 *                         LOAD LITERAL
 * ----------------------------------------------------------------
 * opc  V  Instruction                Variant
 * ----------------------------------------------------------------
 *  00  0  LDR   (literal)            32-bit variant on page C6-527
 *  01  0  LDR   (literal)            64-bit variant on page C6-527
 *  10  0  LDRSW (literal)            -
 *  11  0  PRFM  (literal)            -
 *
 *  00  1  LDR   (literal, SIMD&FP)   32-bit variant on page C7-1027
 *  01  1  LDR   (literal, SIMD&FP)   64-bit variant on page C7-1027
 *  10  1  LDR   (literal, SIMD&FP)  128-bit variant on page C7-1027
 */
ALWAYS_INLINE void Arm64InstructionRewriter::RewriteLDR_LIT(u8 pc, u4 insn) {
    u4 opc = (insn >> 30) & 0x3;
    u4 v = (insn >> 26) & 0x1;
    u4 imm19 = (insn >> 5) & 0x7ffff;
    u4 rt = (insn >> 0) & 0x1f;
    uint64_t offset = SignExtend64(19, imm19) << 2;
    u8 pcrel_address = pc + offset;

    if (pcrel_address < tail_pc_) {
        EmitCode(insn);
        return;
    }
    if (v == 0) {
        auto wt = WReg(rt);
        auto xt = XReg(rt);
        switch (opc) {
            case 0:  // LDR literal 32-bit variant
                __ Mov(xt, pcrel_address);
                __ Ldr(wt, MemOperand(wt, 0, Offset));
                break;
            case 1:  // LDR literal 64-bit variant
                __ Mov(xt, pcrel_address);
                __ Ldr(xt, MemOperand(xt, 0, Offset));
                break;
            case 2:  // LDR Signed Word (literal)
                __ Mov(xt, pcrel_address);
                __ Ldrsw(xt, MemOperand(xt, 0, Offset));
                break;
            case 3:  // PRFM Prefetch
                __ Push(x0);
                __ Mov(x0, pcrel_address);
                __ Ldrsw(x0, MemOperand(xt, 0, Offset));
                __ Pop(x0);
                break;
            default:
                LOG(WARNING) << "Unallocated ldr(literal) opc : " << opc;
                EmitCode(insn);
                break;
        }
    } else if (v == 1) {
        __ Push(x0);
        __ Mov(x0, pcrel_address);
        switch (opc) {
            case 0:  // LDR (literal, SIMD&FP) 32-bit variant
                __ Ldr(SReg(0), MemOperand(x0, 0, Offset));
                break;
            case 1:  // LDR (literal, SIMD&FP) 64-bit variant
                __ Ldr(DReg(0), MemOperand(x0, 0, Offset));
                break;
            case 2:  // LDR (literal, SIMD&FP) 128-bit variant
                __ Ldr(VReg(0), MemOperand(x0, 0, Offset));
                break;
            default:
                LOG(WARNING) << "Unallocated ldr(literal, SIMD&FP) opc : " << opc;
                EmitCode(insn);
                break;
        }
        __ Pop(x0);

    } else {
        LOG(WARNING) << "Unallocated ldr_literal v : " << v;
        EmitCode(insn);
    }
}


}  // namespace arm64
}  // namespace whale


