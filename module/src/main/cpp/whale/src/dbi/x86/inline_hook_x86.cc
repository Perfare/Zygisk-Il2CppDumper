#include <sys/mman.h>
#include <dbi/x86/distorm/distorm.h>
#include "dbi/x86/inline_hook_x86.h"
#include "dbi/x86/instruction_rewriter_x86.h"
#include "assembler/x86/assembler_x86.h"
#include "platform/memory.h"

#define __ masm.

namespace whale {
namespace x86 {

void X86InlineHook::StartHook() {

    DCHECK(address_ != 0 && replace_ != 0);
    X86Assembler masm;
    __ movl(EDX, Immediate(replace_));
    __ jmp(EDX);
    masm.FinalizeCode();

    size_t backup_size = masm.GetBuffer()->Size();
    size_t code_aligned_size = 0;
    do {
        u1 *code = reinterpret_cast<u1 *>(address_) + code_aligned_size;
        u1 size = Decode(code, UINT8_MAX, 0).size;
        code_aligned_size += size;
    } while (code_aligned_size < backup_size);

    backup_size = code_aligned_size;

    backup_code_ = new BackupCode(GetTarget<u1 *>(), backup_size);

    if (backup_ != nullptr) {
        intptr_t tail = address_ + backup_size;
        intptr_t trampoline = BuildTrampoline(static_cast<u4>(tail));
        *backup_ = trampoline;
    }

    ScopedMemoryPatch patch(GetTarget<void *>(), masm.GetBuffer()->contents(),
                            masm.GetBuffer()->Size());
}

intptr_t X86InlineHook::BuildTrampoline(u4 tail) {
    X86Assembler masm;
    X86InstructionRewriter rewriter(&masm, backup_code_, GetTarget<u4>(), tail);
    rewriter.Rewrite();

    __ movl(EDX, Immediate(tail));
    __ jmp(EDX);
    masm.FinalizeCode();
    size_t size = masm.GetBuffer()->Size();

    trampoline_addr_ = mmap(nullptr, GetPageSize(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    memcpy(trampoline_addr_, masm.GetBuffer()->contents(), size);
    mprotect(trampoline_addr_, GetPageSize(), PROT_READ | PROT_EXEC);
    return reinterpret_cast<intptr_t>(trampoline_addr_);
}


void X86InlineHook::StopHook() {
    size_t code_size = backup_code_->GetSizeInBytes();
    void *insns = backup_code_->GetInstructions<void>();
    ScopedMemoryPatch patch(GetTarget<void *>(), insns, code_size);
    memcpy(GetTarget<void *>(), insns, code_size);
    if (trampoline_addr_ != nullptr) {
        munmap(trampoline_addr_, GetPageSize());
    }
}

}  // namespace x86
}  // namespace whale
