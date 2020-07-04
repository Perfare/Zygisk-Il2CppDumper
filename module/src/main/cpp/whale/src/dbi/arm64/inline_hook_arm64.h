#ifndef ARCH_ARM64_INLINEHOOK_ARM64_H_
#define ARCH_ARM64_INLINEHOOK_ARM64_H_

#include "dbi/backup_code.h"
#include "dbi/hook_common.h"
#include "base/primitive_types.h"

namespace whale {
namespace arm64 {

class Arm64InlineHook : public InlineHook {
 public:
    Arm64InlineHook(intptr_t address, intptr_t replace, intptr_t *backup)
            : InlineHook(address, replace, backup), backup_code_(nullptr), trampoline_addr_(nullptr) {}

    ~Arm64InlineHook() override {
        delete backup_code_;
    }

    void StartHook() override;

    void StopHook() override;

 private:
    BackupCode *backup_code_;
    void *trampoline_addr_;

    intptr_t BuildTrampoline(u8 tail);
};

}  // namespace arm64
}  // namespace whale

#endif  // ARCH_ARM64_INLINEHOOK_ARM64_H_

