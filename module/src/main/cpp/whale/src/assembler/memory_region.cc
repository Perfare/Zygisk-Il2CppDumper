#include <cstddef>
#include "assembler/memory_region.h"

namespace whale {

void MemoryRegion::CopyFrom(size_t offset, const MemoryRegion &from) const {
    CHECK(from.pointer() != nullptr);
    CHECK_GT(from.size(), 0U);
    CHECK_GE(this->size(), from.size());
    CHECK_LE(offset, this->size() - from.size());
    memmove(reinterpret_cast<void *>(begin() + offset), from.pointer(), from.size());
}

}  // namespace whale
