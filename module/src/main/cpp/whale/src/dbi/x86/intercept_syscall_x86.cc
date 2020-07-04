#include "dbi/x86/intercept_syscall_x86.h"
#include "base/primitive_types.h"

namespace whale {
namespace x86 {


void X86InterceptSysCallHook::FindSysCalls(uintptr_t start_addr, uintptr_t end_addr) {
    u1 *start = reinterpret_cast<u1 *>(start_addr);
    u1 *end = reinterpret_cast<u1 *>(end_addr) - 1;
    while (start < end) {
        // int 80h
        if (*start == 0xcd & *(++start) == 0x80) {
            // eax: sysnum
            // ebx: arg0
            // ecx: arg1
            // edx: arg2
            // esi: arg3
            // edi: arg4
            // ebp: arg5
        }
    }

}


}  // namespace x86
}  // namespace whale
