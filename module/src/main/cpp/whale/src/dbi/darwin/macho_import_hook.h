#ifndef WHALE_DBI_MACHO_IMPORT_HOOK_H_
#define WHALE_DBI_MACHO_IMPORT_HOOK_H_

#include "dbi/hook_common.h"
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/dyld.h>
#include "base/cxx_helper.h"
#include "base/singleton.h"

#ifndef S_LAZY_DYLIB_SYMBOL_POINTERS
#define S_LAZY_DYLIB_SYMBOL_POINTERS  0x10
#endif

#if __LP64__
#define LC_SEGMENT_COMMAND            LC_SEGMENT_64
typedef mach_header_64 macho_header;
typedef section_64 macho_section;
typedef nlist_64 macho_nlist;
typedef segment_command_64 macho_segment_command;
#else
#define LC_SEGMENT_COMMAND		LC_SEGMENT
typedef  mach_header		macho_header;
typedef  section			macho_section;
typedef  nlist			macho_nlist;
typedef  segment_command	macho_segment_command;
#endif

namespace whale {
namespace darwin {

class MachoImportHook final : public ImportHook {
 public:
    MachoImportHook(const char *symbol_name, void *replace, void **backup)
            : ImportHook(symbol_name, replace, backup) {}

    void StartHook() override;

    void StopHook() override;

    void ImportHookOneMachO(const macho_header *mh, intptr_t slide);

 private:
    void **GetImportAddress(const macho_header *mh, intptr_t slide);
};

}  // namespace darwin
}  // namespace whale

#endif  // WHALE_DBI_MACHO_IMPORT_HOOK_H_
