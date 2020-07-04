#include <sys/mman.h>
#include "platform/memory.h"
#include "base/align.h"
#include "base/cxx_helper.h"
#include "base/logging.h"
#include "base/macros.h"

#ifdef __APPLE__

#include <mach/mach.h>
#include <mach/vm_map.h>
#include <libkern/OSCacheControl.h>
#include <sys/sysctl.h>

C_API kern_return_t mach_vm_remap(vm_map_t, mach_vm_address_t *, mach_vm_size_t,
                                       mach_vm_offset_t, int, vm_map_t, mach_vm_address_t,
                                       boolean_t, vm_prot_t *, vm_prot_t *, vm_inherit_t);

#endif

namespace whale {

ScopedMemoryPatch::ScopedMemoryPatch(void *address, void *patch, size_t size) : address_(address), patch_(patch),
                                                                                size_(size) {
    CHECK(address != nullptr && size > 0);
    intptr_t page_start = PageStart(reinterpret_cast<intptr_t>(address));
    size_t page_offset = static_cast<size_t>(reinterpret_cast<intptr_t>(address) - page_start);
    intptr_t page_end = PageAlign(reinterpret_cast<intptr_t>(address) + size);
    size_t page_size = static_cast<size_t>(page_end - page_start);
    bool use_rwx_page =
            mprotect(reinterpret_cast<void *>(page_start), page_size, PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
    if (use_rwx_page) {
        memcpy(address, patch, size);
    } else {
#ifdef __APPLE__
        //
        // Only rw- and r-x page permissions are available on IOS.
        //
        void *remap_page = mmap(nullptr, GetPageSize(), PROT_READ | PROT_WRITE,
                                MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
        mach_port_t task_self = mach_task_self();
        vm_address_t vm_page_start = static_cast<vm_address_t>(page_start);
        vm_size_t region_size = 0;
        vm_region_submap_short_info_64 info;
        mach_msg_type_number_t info_count = VM_REGION_SUBMAP_SHORT_INFO_COUNT_64;
        natural_t max_depth = UINT8_MAX;
        kern_return_t kr = vm_region_recurse_64(task_self,
                                                &vm_page_start, &region_size, &max_depth,
                                                (vm_region_recurse_info_t) &info,
                                                &info_count);
        if (kr != KERN_SUCCESS) {
            return;
        }
        vm_copy(task_self, vm_page_start, page_size, (vm_address_t) remap_page);
        memcpy(OffsetOf<void *>(remap_page, page_offset), patch, size);

        mprotect(remap_page, page_size, PROT_READ | PROT_EXEC);

        vm_prot_t cur_protection, max_protection;
        mach_vm_address_t mach_vm_page_start = static_cast<vm_address_t>(page_start);
        mach_vm_remap(task_self, &mach_vm_page_start, page_size, 0, VM_FLAGS_OVERWRITE,
                      task_self, (mach_vm_address_t) remap_page, TRUE, &cur_protection, &max_protection,
                      info.inheritance);
#endif
    }
}

ScopedMemoryPatch::~ScopedMemoryPatch() {
#ifdef __APPLE__
    sys_icache_invalidate(reinterpret_cast<char *>(address_), size_);
#else
    __builtin___clear_cache(
            reinterpret_cast<char *>(address_),
            reinterpret_cast<char *>(reinterpret_cast<char *>(address_) + size_)
    );
#endif
}


}  // namespace whale
