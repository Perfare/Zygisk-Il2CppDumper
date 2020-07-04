#ifndef WHALE_PLATFORM_PROCESS_MAP_H_
#define WHALE_PLATFORM_PROCESS_MAP_H_

#include <malloc.h>
#include <functional>
#include "base/primitive_types.h"

namespace whale {

struct MemoryRange {
 public:
    MemoryRange() : base_(0), end_(0), path_(nullptr) {}

    ~MemoryRange() {
        if (path_ != nullptr) {
            free((void *) path_);
            path_ = nullptr;
        }
    }

    bool IsValid() {
        return path_ != nullptr && base_ < end_;
    }

    bool IsInRange(uintptr_t address) {
        return IsValid() && base_ <= address && end_ >= address;
    }

    const char *path_;
    uintptr_t base_;
    uintptr_t end_;
};

bool IsFileInMemory(const char *name);

std::unique_ptr<MemoryRange> FindExecuteMemoryRange(const char *name);

std::unique_ptr<MemoryRange> FindFileMemoryRange(const char *name);

void ForeachMemoryRange(std::function<bool(uintptr_t, uintptr_t, char *, char *)> callback);

}  // namespace whale

#endif  // WHALE_PLATFORM_PROCESS_MAP_H_
