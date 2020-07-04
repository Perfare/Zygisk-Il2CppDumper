#ifndef PLATFORM_SCOPED_MEMORY_ACCESS_H_
#define PLATFORM_SCOPED_MEMORY_ACCESS_H_

#include <cstddef>

namespace whale {

class ScopedMemoryPatch {
public:
 ScopedMemoryPatch(void *address, void *patch, size_t size);

 ~ScopedMemoryPatch();

private:
 void *address_;
 void *patch_;
 size_t size_;
};

}  // namespace whale

#endif  // PLATFORM_SCOPED_MEMORY_ACCESS_H_
