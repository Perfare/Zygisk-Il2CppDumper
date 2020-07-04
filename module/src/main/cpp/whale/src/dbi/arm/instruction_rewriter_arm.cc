#include "assembler/vixl/aarch32/macro-assembler-aarch32.h"
#include "dbi/arm/instruction_rewriter_arm.h"
#include "dbi/arm/decoder_arm.h"
#include "dbi/arm/decoder_thumb.h"
#include "dbi/arm/registers_arm.h"
#include "dbi/backup_code.h"
#include "dbi/instruction_rewriter.h"
#include "dbi/instruction_set.h"
#include "base/macros.h"
#include "base/align.h"
#include "base/primitive_types.h"
#include "base/logging.h"

#define __ masm_->
#define UNREACHABLE_BRANCH() LOG(FATAL) << "Unreachable branch";  \
                       UNREACHABLE()


namespace whale {
namespace arm {

using namespace vixl::aarch32;  // NOLINT

void arm::ArmInstructionRewriter::Rewrite() {
    if (is_thumb_) {
        RewriteThumb();
    } else {
        RewriteArm();
    }
}

void ArmInstructionRewriter::RewriteArm() {
    u4 pc = cfg_pc_ + 8;
    u4 *instructions = code_->GetInstructions<u4>();
    for (int i = 0; i < code_->GetCount<u4>(); ++i) {
        u4 insn = instructions[i];
        ArmInsnType type = DecodeArm(insn);
        switch (type) {
            case ArmInsnType::kARM_LDR:
                RewriteArm_LDR(type, pc, insn);
                break;
            case ArmInsnType::kARM_ADD:
                RewriteArm_Add(type, pc, insn);
                break;
            case ArmInsnType::kARM_B:
            case ArmInsnType::kARM_BL:
                RewriteArm_B(type, pc, insn);
                break;
            default:
                EmitA32(insn);
                break;
        }
        pc += 4;
    }
}

void ArmInstructionRewriter::RewriteThumb() {
    u4 pc = cfg_pc_ + 4;
    InstructionIterator it(code_->GetInstructions(), code_->GetSizeInBytes());
    while (it.HasNext()) {
        u2 insn_16 = it.GetCurrent<u2>();
        u4 insn_32 = 0;
        bool is_32bit = Is32BitThumbInstruction(insn_16);
        if (is_32bit) {
            insn_32 = ((insn_16) << 16) | *(it.GetCurrentRef<u2>() + 1);
        }
        u4 width = is_32bit ? 4 : 2;
        ThumbInsnType type = is_32bit ? DecodeThumb32(insn_32) : DecodeThumb16(insn_16);
        switch (type) {
            case kTHUMB_B_COND16:
                RewriteThumb_B_COND16(pc, insn_16);
                break;
            case kTHUMB_B16:
                RewriteThumb_B16(pc, insn_16);
                break;
            case kTHUMB_BX16:
            case kTHUMB_BLX16:
                RewriteThumb_BX16(type, pc, insn_16);
                break;
            case kTHUMB_B32:
            case kTHUMB_BL32:
            case kTHUMB_BL_ARM32:
                RewriteThumb_B32(type, pc, insn_16);
                break;
            case kTHUMB_CBZ16:
            case kTHUMB_CBNZ16:
                RewriteThumb_CBZ16(type, pc, insn_16);
                break;
            case kTHUMB_LDR_PC_16:
                RewriteThumb_LDR_PC16(pc, insn_16);
                break;
            case kTHUMB_ADD_FROM_PC16:
                RewriteThumb_ADD_FROM_PC16(pc, insn_16);
                break;
            case kTHUMB_ADDH16:
            case kTHUMB_CMPH16:
            case kTHUMB_MOVH16:
                RewriteThumb_DataProcessing16(type, pc, insn_16);
                break;
            case kTHUMB_LDRBL32:
            case kTHUMB_LDRHL32:
            case kTHUMB_LDRL32:
            case kTHUMB_LDRSBL32:
            case kTHUMB_LDRSHL32:
                RewriteThumb_LDRL32(type, pc, insn_32);
                break;
            default:
                if (is_32bit) {
                    EmitT32(insn_32);
                } else {
                    EmitT16(insn_16);
                }
                break;
        }  // end switch
        pc += width;
        it.Step(width);
    }  // end while
}

/**
 * Rewrite Instruction for this scheme:
 * ldr rd, [pc, #offset]
 */
void ArmInstructionRewriter::RewriteThumb_LDR_PC16(u4 pc, u2 insn) {
    int imm8 = (insn >> 0) & 0xff;
    auto rd = Reg(((insn >> 8) & 0x7));
    int offset = imm8 << 2;
    u4 pcrel_address = RoundDown(pc + offset, 4);
    /*
     * How to reproduce this case:
     *     ldr r0, [pc]
     *     b #2
     *     .word 1234
     *     bx lr
     */
    if (pcrel_address < tail_pc_) {
        EmitT16(insn);
    } else {
        __ Mov(rd, pcrel_address);
        __ Ldr(rd, MemOperand(rd, 0));
    }
}

/**
 * Rewrite Instruction for the following scheme:
 * pcrel add, cmp, mov
 */
void
ArmInstructionRewriter::RewriteThumb_DataProcessing16(ThumbInsnType type, u4 cfg_pc, u2 insn) {
    int dn = (insn >> 7) & 0x1;
    auto rm = Reg((insn >> 3) & 0xf);
    auto rd = Reg(((insn >> 0) & 0x7) | (dn << 3));

    if (!rd.IsPC() && !rm.IsPC()) {
        EmitT16(insn);
        return;
    }
    if (rd.Is(rm)) {
        EmitT16(insn);
    } else if (rm.IsPC()) {
        auto scratch_reg = rd.Is(r0) ? r1 : r0;
        __ Push(scratch_reg);
        __ Mov(scratch_reg, cfg_pc);
        switch (type) {
            case kTHUMB_ADDH16:
                __ Add(rd, rd, scratch_reg);
                break;
            case kTHUMB_CMPH16:
                __ Cmp(rd, scratch_reg);
                break;
            case kTHUMB_MOVH16:
                __ Mov(rd, scratch_reg);
                break;
            default:
            UNREACHABLE_BRANCH();
        }
        __ Pop(scratch_reg);
    } else {  // rd == pc
        Register scratch_reg = (rm.Is(r0)) ? r1 : r0;
        __ Push(scratch_reg);
        __ Mov(scratch_reg, cfg_pc);
        switch (type) {
            case kTHUMB_ADDH16:
                __ Add(pc, pc, scratch_reg);
                break;
            case kTHUMB_CMPH16:
                __ Cmp(pc, scratch_reg);
                break;
            case kTHUMB_MOVH16:
                __ Mov(pc, scratch_reg);
                break;
            default:
            UNREACHABLE_BRANCH();
        }
        __ Pop(scratch_reg);
    }
}

void ArmInstructionRewriter::RewriteThumb_LDRL32(ThumbInsnType type, u4 pc, u4 insn) {
    Register rt = Reg((insn >> 12) & 0xf);
    u4 imm12 = (insn >> 0) & 0xfff;
    u4 upwards = (insn >> 23) & 0x1;
    u4 pcrel_address = RoundDown((pc + (upwards ? imm12 : -imm12)), 4);
    if (pcrel_address < tail_pc_) {
        EmitT32(insn);
        return;
    }
    __ Mov(rt, pcrel_address);
    switch (type) {
        case kTHUMB_LDRBL32:
            __ Ldrb(rt, MemOperand(rt, 0));
            break;
        case kTHUMB_LDRHL32:
            __ Ldrh(rt, MemOperand(rt, 0));
            break;
        case kTHUMB_LDRL32:
            __ Ldr(rt, MemOperand(rt, 0));
            break;
        case kTHUMB_LDRSBL32:
            __ Ldrsb(rt, MemOperand(rt, 0));
            break;
        case kTHUMB_LDRSHL32:
            __ Ldrsh(rt, MemOperand(rt, 0));
            break;
        default:
        UNREACHABLE_BRANCH();
    }
}

void ArmInstructionRewriter::RewriteThumb_B_COND16(u4 cfg_pc, u2 insn) {
    u2 cond = (insn >> 8) & 0xf;
    int imm8 = (insn >> 0) & 0xff;
    int branch_offset = (static_cast<u1>(imm8)) << 1;
    u4 pcrel_address = RoundDown(cfg_pc + branch_offset, 4) | 1;

    if (pcrel_address < tail_pc_) {
        EmitT16(insn);
        return;
    }
    if (cond == al) {
        __ Push(r0);
        __ Mov(r0, pcrel_address);
        __ Mov(pc, r0);
        __ Pop(r0);
    } else {
        Label true_label, false_label;
        __ B(Condition(cond), &true_label);
        __ B(&false_label);
        __ Bind(&true_label);
        __ Push(r0);
        __ Mov(r0, pcrel_address);
        __ Mov(pc, r0);
        __ Pop(r0);
        __ Bind(&false_label);
    }
    masm_->FinalizeCode();
}

void ArmInstructionRewriter::RewriteThumb_B16(u4 cfg_pc, u2 insn) {
    int imm11 = (insn >> 0) & 0x7ff;
    int branch_offset = (imm11 & 0x400) ? 0xFFFFF000 : 0;
    branch_offset |= imm11 << 1;
    u4 pcrel_address = RoundDown(cfg_pc + branch_offset, 4);
    if (pcrel_address < tail_pc_) {
        EmitT16(insn);
        return;
    }
    Literal<u4> target(pcrel_address | 1);
    Label skip_literal;
    __ Ldr(pc, &target);
    __ B(&skip_literal);
    __ Place(&target);
    __ Bind(&skip_literal);
    masm_->FinalizeCode();
}


void ArmInstructionRewriter::RewriteThumb_BX16(ThumbInsnType type, u4 pc, u2 insn) {
    Register rm = Reg((insn >> 3) & 0xf);
    if (rm.IsPC()) {
        // This instruction may cause the CPU status change.
        // Thumb ========> ARM
        // keep an eye on this.
        LOG(WARNING) << "Un-rewritten instruction: bx pc";
    }
    EmitT16(insn);
}

void ArmInstructionRewriter::RewriteThumb_B32(ThumbInsnType type, u4 cfg_pc, u4 insn) {
    int sign_bit = (insn >> 26) & 0x1;
    int offset_high = (insn >> 16) & 0x3ff;
    int link = (insn >> 14) & 0x1;
    int j1 = (insn >> 13) & 0x1;
    int thumb_mode = (insn >> 12) & 0x1;
    int j2 = (insn >> 11) & 0x1;
    int offset_low = (insn >> 0) & 0x7ff;
    int branch_offset = sign_bit ? 0xFF000000 : 0;
    branch_offset |= (j1 ^ sign_bit) ? 0 : 1 << 23;
    branch_offset |= (j2 ^ sign_bit) ? 0 : 1 << 22;
    branch_offset |= offset_high << 12;
    branch_offset |= offset_low << 1;
    u4 pcrel_address = cfg_pc + branch_offset;
    if (pcrel_address < tail_pc_) {
        EmitT32(insn);
    } else {
        if (link) {
            __ Add(lr, pc, 4);
        }
        if (thumb_mode) {
            pcrel_address |= 1;
        }
        Literal<u4> target(pcrel_address);
        Label skip_literal;
        __ Ldr(pc, &target);
        __ B(&skip_literal);
        __ Place(&target);
        __ Bind(&skip_literal);
        masm_->FinalizeCode();
    }
}

void ArmInstructionRewriter::RewriteThumb_CBZ16(ThumbInsnType type, u4 cfg_pc, u2 insn) {
    int n = (insn >> 11) & 0x1;
    int imm1 = (insn >> 9) & 0x1;
    int imm5 = (insn >> 3) & 0x1f;
    Register rn = Reg((insn >> 0) & 0x7);
    int branch_offset = (imm1 << 6) | (imm5 << 1);
    u4 pcrel_address = RoundDown(cfg_pc + branch_offset, 4) | 1;
    if (pcrel_address < tail_pc_) {
        EmitT16(insn);
        return;
    }
    Label true_label, false_label;
    __ Cbz(rn, &true_label);
    __ B(&false_label);
    __ Bind(&true_label);
    __ Push(r0);
    __ Mov(r0, pcrel_address);
    __ Mov(pc, r0);
    __ Pop(r0);
    __ Bind(&false_label);
    masm_->FinalizeCode();
}

void ArmInstructionRewriter::RewriteThumb_ADD_FROM_PC16(u4 pc, u2 insn) {
    Register rd = Reg((insn >> 8) & 0x7);
    int imm8 = (insn >> 0) & 0xff;
    int offset = imm8 << 2;
    u4 pcrel_address = RoundDown(pc + offset, 4);
    __ Mov(rd, pcrel_address);
}

void ArmInstructionRewriter::RewriteArm_LDR(ArmInsnType type, u4 pc, u4 insn) {
    int updown = (insn >> 23) & 0x1;
    int imm = insn & 0xFFF;
    Register rd = Reg((insn & 0xF000) >> 12);
    u4 pcrel_address = updown ? pc + imm : pc - imm;
    if (pcrel_address < tail_pc_) {
        EmitA32(insn);
    } else {
        __ Mov(rd, pcrel_address);
        __ Ldr(rd, MemOperand(rd, 0));
        masm_->FinalizeCode();
    }
}

void ArmInstructionRewriter::RewriteArm_Add(ArmInsnType type, u4 pc, u4 insn) {
    Register rd = Reg((insn & 0xF000) >> 12);
    Register rm = Reg(insn & 0xF);
    Register scratch_reg = ip;
    for (unsigned reg = 0; reg < R7; ++reg) {
        if (rd.GetCode() != reg && rm.GetCode() != reg) {
            scratch_reg = Reg(reg);
            break;
        }
    }
    __ Push(scratch_reg);
    __ Mov(scratch_reg, pc);
    __ Add(rd, scratch_reg, rd);
    __ Pop(scratch_reg);
}

void ArmInstructionRewriter::RewriteArm_B(ArmInsnType type, u4 cfg_pc, u4 insn) {
    u4 offset = (insn >> 0) & 0xffffff;
    u4 branch_offset = (offset & 0x800000) ? 0xFC000000 : 0;
    branch_offset |= (offset << 2);
    u4 pcrel_address = cfg_pc + branch_offset;
    if (pcrel_address < tail_pc_) {
        EmitA32(insn);
        return;
    }
    if (type == ArmInsnType::kARM_BL) {
        __ Add(lr, pc, 4);
    }
    Literal<u4> target(pcrel_address);
    Label skip_literal;
    __ Ldr(pc, &target);
    __ B(&skip_literal);
    __ Place(&target);
    __ Bind(&skip_literal);
    masm_->FinalizeCode();
}


}  // namespace arm
}  // namespace whale
