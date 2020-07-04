#include "dbi/hook_common.h"
#include "hook_common.h"

#if defined(linux)

#include "platform/linux/process_map.h"

#endif

namespace whale {

bool FindStdLibCallback(const char *path, bool *stop) {
#if defined(linux)
    if (strstr(path, "system/") && strstr(path, "/libc.so")) {
        *stop = true;
        return true;
    }
#elif defined(__APPLE__)
    if (strstr(path, "libc.dylib")) {
        *stop = true;
        return true;
    }
#endif
    return false;
}

InterceptSysCallHook::InterceptSysCallHook(MemoryRangeCallback callback)
        : Hook() {
    if (callback == nullptr) {
        callback = FindStdLibCallback;
    }
    callback_ = callback;
}

void InterceptSysCallHook::StartHook() {
    bool stop_foreach = false;
#if defined(linux)
    ForeachMemoryRange(
            [&](uintptr_t begin, uintptr_t end, char *perm, char *mapname) -> bool {
                if (strstr(perm, "x") && strstr(perm, "r")) {
                    if (callback_(mapname, &stop_foreach)) {
                        FindSysCalls(begin, end);
                    }
                }
                return stop_foreach;
            });
#endif
}

void InterceptSysCallHook::StopHook() {

}
}  // namespace whale
