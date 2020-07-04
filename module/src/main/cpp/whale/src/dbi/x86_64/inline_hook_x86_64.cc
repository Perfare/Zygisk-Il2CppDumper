#include <sys/mman.h>
#include <platform/memory.h>
#include <dbi/x86/distorm/decoder.h>
#include "assembler/x86_64/assembler_x86_64.h"
#include "dbi/x86_64/inline_hook_x86_64.h"
#include "dbi/x86_64/instruction_rewriter_x86_64.h"
#include "dbi/x86/distorm/distorm.h"

#define __ masm.

namespace whale {
namespace x86_64 {


void X86_64InlineHook::StartHook() {
    CHECK(address_ != 0 && replace_ != 0);
    X86_64Assembler masm;

    __ movq(RAX, Immediate(replace_));
    __ jmp(RAX);
    masm.FinalizeCode();

    size_t backup_size = masm.GetBuffer()->Size();
    size_t code_aligned_size = 0;
    do {
        u1 *code = reinterpret_cast<u1 *>(address_) + code_aligned_size;
        u1 size = Decode(code, UINT8_MAX, 1).size;
        code_aligned_size += size;
    } while (code_aligned_size < backup_size);

    backup_size = code_aligned_size;
    backup_code_ = new BackupCode(GetTarget<u1 *>(), backup_size);

    if (backup_ != nullptr) {
        intptr_t tail = address_ + backup_size;
        intptr_t trampoline = BuildTrampoline(static_cast<u8>(tail));
        *backup_ = trampoline;
    }

    ScopedMemoryPatch patch(GetTarget<void *>(), masm.GetBuffer()->contents(),
                            masm.GetBuffer()->Size());
}

intptr_t X86_64InlineHook::BuildTrampoline(u8 tail) {
    X86_64Assembler masm;
    X86_64InstructionRewriter rewriter(&masm, backup_code_, GetTarget<u8>(), tail);
    rewriter.Rewrite();

    __ movq(R12, Immediate(tail));
    __ jmp(R12);

    masm.FinalizeCode();

    size_t size = masm.GetBuffer()->Size();
    trampoline_addr_ = mmap(nullptr, GetPageSize(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    memcpy(trampoline_addr_, masm.GetBuffer()->contents(), size);
    mprotect(trampoline_addr_, GetPageSize(), PROT_READ | PROT_EXEC);
    return reinterpret_cast<intptr_t>(trampoline_addr_);
}


void X86_64InlineHook::StopHook() {
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
