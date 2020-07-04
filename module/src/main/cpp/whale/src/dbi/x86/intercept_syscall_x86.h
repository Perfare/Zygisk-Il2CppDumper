#ifndef WHALE_DBI_X86_INTERCEPT_SYSCALL_HOOK_H_
#define WHALE_DBI_X86_INTERCEPT_SYSCALL_HOOK_H_

#include "dbi/hook_common.h"

namespace whale {
namespace x86 {

class X86InterceptSysCallHook : public InterceptSysCallHook {
 public:
    X86InterceptSysCallHook(MemoryRangeCallback callback) : InterceptSysCallHook(callback) {}

 protected:
    void FindSysCalls(uintptr_t start, uintptr_t end);
};

}  // namespace x86
}  // namespace whale

#endif // WHALE_DBI_X86_INTERCEPT_SYSCALL_HOOK_H_
