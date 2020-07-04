#include <sys/mman.h>

#include "platform/memory.h"
#include "assembler/vixl/aarch32/macro-assembler-aarch32.h"
#include "dbi/arm/inline_hook_arm.h"
#include "dbi/arm/instruction_rewriter_arm.h"
#include "base/logging.h"
#include "base/primitive_types.h"

#ifdef WHALE_DISASM_AFTER_REWRITE
#include "assembler/vixl/aarch32/disasm-aarch32.h"
#endif

#define __ masm.

namespace whale {
namespace arm {

using namespace vixl::aarch32;  // NOLINT

#ifdef WHALE_DISASM_AFTER_REWRITE

void Disassemble(MacroAssembler *masm) {
    whale::LogMessage log("Disassembler", static_cast<int>(masm->GetSizeOfCodeGenerated()));
    vixl::aarch32::PrintDisassembler disassembler(log.stream());
    disassembler.DisassembleA32Buffer(masm->GetBuffer()->GetStartAddress<u4 *>(),
                                      masm->GetBuffer()->GetSizeInBytes());
}

#endif


void ArmInlineHook::StartHook() {
    DCHECK(address_ != 0 && replace_ != 0);
    MacroAssembler masm;
    is_thumb_ ? masm.UseT32() : masm.UseA32();
    Literal<u4> replace(static_cast<const u4>(replace_));
    if (is_thumb_) {
        if (address_ % 4 != 0) {
            __ Nop();
        }
    }
    __ Ldr(pc, &replace);
    __ Place(&replace);
    masm.FinalizeCode();

    size_t backup_size = masm.GetSizeOfCodeGenerated();
    u2 *target = GetTarget<u2 *>();
    if (is_thumb_) {
        if (!Is32BitThumbInstruction(target[backup_size / sizeof(u2) - 2])
            && Is32BitThumbInstruction(target[backup_size / sizeof(u2) - 1])) {
            backup_size += sizeof(u2);
        }
    }
    backup_code_ = new BackupCode(GetTarget<u2 *>(), backup_size);

    if (backup_ != nullptr) {
        intptr_t tail = address_ + backup_size;
        intptr_t trampoline = BuildTrampoline(static_cast<u4>(tail));
        *backup_ = trampoline;
    }

    ScopedMemoryPatch patch(GetTarget<void *>(), masm.GetBuffer()->GetStartAddress<void *>(),
                            masm.GetBuffer()->GetSizeInBytes());
    memcpy(GetTarget<void *>(), masm.GetBuffer()->GetStartAddress<void *>(),
           masm.GetBuffer()->GetSizeInBytes());
}

intptr_t
ArmInlineHook::BuildTrampoline(u4 tail) {
    MacroAssembler masm;
    is_thumb_ ? masm.UseT32() : masm.UseA32();
    ArmInstructionRewriter rewriter(&masm, backup_code_, GetTarget<u4>(), tail, is_thumb_);
    rewriter.Rewrite();
    if (is_thumb_) {
        tail |= 1;
    }
    Literal<u4> target(static_cast<const u4>(tail));
    __ Ldr(pc, &target);
    __ Place(&target);

    masm.FinalizeCode();

#ifdef WHALE_DISASM_AFTER_REWRITE
    Disassemble(&masm);
#endif

    size_t size = masm.GetBuffer()->GetSizeInBytes();
    trampoline_addr_ = mmap(nullptr, GetPageSize(), PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    memcpy(trampoline_addr_, masm.GetBuffer()->GetStartAddress<void *>(), size);
    mprotect(trampoline_addr_, GetPageSize(), PROT_READ | PROT_EXEC);
    auto trampoline_addr = reinterpret_cast<intptr_t>(trampoline_addr_);
    if (is_thumb_) {
        trampoline_addr |= 1;
    }
    return trampoline_addr;
}


void ArmInlineHook::StopHook() {
    size_t code_size = backup_code_->GetSizeInBytes();
    void *insns = backup_code_->GetInstructions<void>();
    ScopedMemoryPatch patch(GetTarget<void *>(), insns, code_size);
    memcpy(GetTarget<void *>(), insns, code_size);
    if (trampoline_addr_ != nullptr) {
        munmap(trampoline_addr_, GetPageSize());
    }
}

}  // namespace arm
}  // namespace whale
