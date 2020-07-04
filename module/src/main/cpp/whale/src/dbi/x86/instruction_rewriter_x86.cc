#include "dbi/x86/distorm/distorm.h"
#include <dbi/x86/distorm/mnemonics.h>
#include "dbi/x86/instruction_rewriter_x86.h"
#include "instruction_rewriter_x86.h"

#define __ masm_->

namespace whale {
namespace x86 {

void X86InstructionRewriter::Rewrite() {
    __ addl(ESP, Immediate(4));
    u1 *insns = code_->GetInstructions<u1>();
    int pos = 0;
    size_t count = code_->GetCount<u1>();
    u4 pc = cfg_pc_;
    while (pos < count) {
        u1 *current = insns + pos;
        _DInst insn = Decode(current, count - pos, 0);
        pc += insn.size;
        switch (insn.opcode) {
            case I_CALL:
                Rewrite_Call(current, pc, insn);
                break;
            case I_JMP:
                Rewrite_Jmp(current, pc, insn);
                break;
            case I_JECXZ:
            case I_JCXZ:
                Rewrite_JRCXZ(current, pc, insn);
                break;
            default:
                EmitCode(current, insn.size);
                break;
        }
        pos += insn.size;
    }
}

void X86InstructionRewriter::Rewrite_Call(u1 *current, u4 pc, _DInst insn) {
    bool rewritten = false;
    if (insn.ops[0].type == O_PC) {
        u4 pcrel_address = pc + insn.imm.dword;
        Register reg;
        if (IsGetPCThunkToRegister(pcrel_address, &reg)) {
            __ movl(reg, Immediate(pc));
            rewritten = true;
        } else if (pcrel_address >= tail_pc_) {
            __ pushl(Immediate(pc));
            __ movl(EDX, Immediate(pcrel_address));
            __ jmp(EDX);
            rewritten = true;
        }
    }
    if (!rewritten) {
        EmitCode(current, insn.size);
    }
}

void X86InstructionRewriter::Rewrite_Jmp(u1 *current, u4 pc, _DInst insn) {
    bool rewritten = false;
    if (insn.ops[0].type == O_PC) {
        u4 pcrel_address = pc + insn.imm.dword;
        if (pcrel_address >= tail_pc_) {
            __ movl(EDX, Immediate(pcrel_address));
            __ jmp(EDX);
            rewritten = true;
        }
    }
    if (!rewritten) {
        EmitCode(current, insn.size);
    }
}

void X86InstructionRewriter::Rewrite_JRCXZ(u1 *current, u4 pc, _DInst insn) {
    bool rewritten = false;
    u4 pcrel_address = pc + insn.imm.dword;
    if (pcrel_address >= tail_pc_) {
        NearLabel true_label, false_label;

        __ jecxz(&true_label);
        __ jmp(&false_label);

        __ Bind(&true_label);
        __ movl(EDX, Immediate(pcrel_address));
        __ jmp(EDX);

        __ Bind(&false_label);
        rewritten = true;
    }
    if (!rewritten) {
        EmitCode(current, insn.size);
    }
}

/**
 * Find the following scheme:
 *
 * _x86_get_pc_thunk_bx:
 *      mov ebx, [esp+0]
 *      ret
 *
 */
bool X86InstructionRewriter::IsGetPCThunkToRegister(u4 address, Register *reg) {
    u1 *current = reinterpret_cast<u1 *>(address);
    _DInst insn0 = Decode(current, UINT8_MAX, 0);
    if (insn0.opcode != I_MOV || insn0.ops[0].type != O_REG || insn0.ops[1].type != O_SMEM) {
        return false;
    }
    _DInst insn1 = Decode(current + insn0.size, UINT8_MAX, 0);
    if (insn1.opcode != I_RET) {
        return false;
    }
    *reg = static_cast<Register>(insn0.ops[0].index % 16);
    return true;
}

}  // namespace x86
}  // namespace whale
