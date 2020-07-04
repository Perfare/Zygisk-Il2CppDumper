#ifndef ARCH_ARM_INLINEHOOK_ARM_H_
#define ARCH_ARM_INLINEHOOK_ARM_H_

#include "dbi/backup_code.h"
#include "dbi/hook_common.h"
#include "base/primitive_types.h"
#include "base/align.h"

namespace whale {
namespace arm {

class ArmInlineHook : public InlineHook {
 public:
    ArmInlineHook(intptr_t address, intptr_t replace, intptr_t *backup)
            : InlineHook(address, replace, backup) {
        is_thumb_ = static_cast<bool>(address & 0x1);
        if (is_thumb_) {
            address_ = RoundDown(address, 2);
        }
    }

    void StartHook() override;

    void StopHook() override;

 private:
    bool is_thumb_;
    BackupCode *backup_code_;
    void *trampoline_addr_;

    intptr_t BuildTrampoline(u4 tail);
};

}  // namespace arm
}  // namespace whale

#endif  // ARCH_ARM_INLINEHOOK_ARM_H_

