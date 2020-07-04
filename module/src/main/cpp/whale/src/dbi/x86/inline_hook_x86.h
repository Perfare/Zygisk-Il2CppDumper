#ifndef WHALE_ARCH_X86_INLINE_HOOK_X86_H_
#define WHALE_ARCH_X86_INLINE_HOOK_X86_H_

#include "dbi/hook_common.h"
#include "base/primitive_types.h"
#include "dbi/backup_code.h"

namespace whale {
namespace x86 {

class X86InlineHook : public InlineHook {
 public:
    X86InlineHook(intptr_t address, intptr_t replace, intptr_t *backup)
            : InlineHook(address, replace, backup), backup_code_(nullptr),
              trampoline_addr_(nullptr) {}

    ~X86InlineHook() override {
        delete backup_code_;
    }

    void StartHook() override;

    void StopHook() override;

 private:
    BackupCode *backup_code_;
    void *trampoline_addr_;

    intptr_t BuildTrampoline(u4 tail);
};

}  // namespace x86
}  // namespace whale

#endif  // WHALE_ARCH_X86_INLINE_HOOK_X86_H_
