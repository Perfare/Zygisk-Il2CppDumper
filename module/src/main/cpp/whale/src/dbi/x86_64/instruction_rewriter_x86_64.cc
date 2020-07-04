#include "dbi/x86/instruction_rewriter_x86.h"
#include "instruction_rewriter_x86_64.h"
#include "dbi/x86/distorm/distorm.h"
#include "dbi/x86/distorm/mnemonics.h"

#define __ masm_->

namespace whale {
namespace x86_64 {

constexpr static const unsigned kRIP_index = 74;

void X86_64InstructionRewriter::Rewrite() {
    u1 *instructions = code_->GetInstructions<u1>();
    int pos = 0;
    size_t count = code_->GetCount<u1>();
    u8 pc = cfg_pc_;
    while (pos < code_->GetCount<u1>()) {
        u1 *current = instructions + pos;
        _DInst insn = Decode(current, count - pos, 1);
        pc += insn.size;
        switch (insn.opcode) {
            case I_MOV:
                Rewrite_Mov(current, pc, insn);
                break;
            case I_CALL:
                Rewrite_Call(current, pc, insn);
                break;
            case I_JMP:
                Rewrite_Jmp(current, pc, insn);
                break;
            case I_JECXZ:
            case I_JRCXZ:
                Rewrite_JRCXZ(current, pc, insn);
                break;
            default:
                EmitCode(current, insn.size);
                break;
        }
        pos += insn.size;
    }
}

void X86_64InstructionRewriter::Rewrite_Mov(u1 *current, u8 pc, _DInst insn) {
    _Operand op0 = insn.ops[0];
    _Operand op1 = insn.ops[1];

    if (op0.type == O_REG && op1.type == O_SMEM && op1.index == kRIP_index) {
        //  mov rd, nword ptr [rip + disp]
        int rd = insn.ops[0].index % 16;
        __ movq(rd, Immediate(pc + insn.disp));
        __ movl(rd, Address(rd, 0));
    } else if (op0.type == O_SMEM && op1.type == O_IMM && op1.index == kRIP_index) {
        //  mov nword ptr [rip + disp], imm
        __ pushq(RAX);
        __ movq(RAX, Immediate(pc + insn.disp));
        if (op1.size <= 32) {
            __ movl(Address(RAX, 0), Immediate(insn.imm.dword));
        } else {
            __ movq(Address(RAX, 0), Immediate(insn.imm.qword));
        }
        __ popq(RAX);
    } else {
        EmitCode(current, insn.size);
    }
}

void X86_64InstructionRewriter::Rewrite_Call(u1 *current, u8 pc, _DInst insn) {
    _Operand op = insn.ops[0];
    if (op.type == O_PC) {
        __ movq(R11, Immediate(pc + insn.imm.qword));
        __ call(R11);
    } else {
        EmitCode(current, insn.size);
    }
}

void X86_64InstructionRewriter::Rewrite_Jmp(u1 *current, u8 pc, _DInst insn) {
    _Operand op = insn.ops[0];
    if (op.type == O_PC) {
        __ movq(R11, Immediate(pc + insn.imm.qword));
        __ jmp(R11);
    } else {
        EmitCode(current, insn.size);
    }
}

void X86_64InstructionRewriter::Rewrite_JRCXZ(u1 *current, u8 pc, _DInst insn) {
    bool rewritten = false;
    u8 pcrel_address = pc + insn.imm.qword;
    if (pcrel_address >= tail_pc_) {
        NearLabel true_label, false_label;

        __ jrcxz(&true_label);
        __ jmp(&false_label);

        __ Bind(&true_label);
        __ movq(R11, Immediate(pcrel_address));
        __ jmp(R11);

        __ Bind(&false_label);
        rewritten = true;
    }
    if (!rewritten) {
        EmitCode(current, insn.size);
    }
}


}  // namespace x86
}  // namespace whale
