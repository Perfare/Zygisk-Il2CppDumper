#include <dlfcn.h>
#include "interceptor.h"
#include "dbi/darwin/macho_import_hook.h"
#include "macho_import_hook.h"


namespace whale {
namespace darwin {

static void NewDylibCallback(const struct mach_header *mh, intptr_t vmaddr_slide) {
    Interceptor::Instance()->TraverseHooks([&](std::unique_ptr<Hook> &hook) {
        if (hook->GetType() == HookType::kImportHook) {
            MachoImportHook *import_hook = dynamic_cast<MachoImportHook *>(hook.get());
            import_hook->ImportHookOneMachO(reinterpret_cast<const macho_header *>(mh), vmaddr_slide);
        }
    });
};

void MachoImportHook::StartHook() {
    static Singleton<void> register_dyld_once([](void *instance) -> void {
        _dyld_register_func_for_add_image(NewDylibCallback);
    });
    register_dyld_once.Ensure();
    for (u4 i = 0; i < _dyld_image_count(); ++i) {
        ImportHookOneMachO(
                reinterpret_cast<const macho_header *>(_dyld_get_image_header(i)),
                _dyld_get_image_vmaddr_slide(i)
        );
    }
}

void MachoImportHook::ImportHookOneMachO(const macho_header *mh, intptr_t slide) {
    void **address = GetImportAddress(
            mh,
            slide
    );
    if (address != nullptr) {
        address_map_.insert(std::make_pair(address, *address));
        *address = replace_;
    }
    if (backup_ != nullptr) {
        *backup_ = dlsym(RTLD_DEFAULT, symbol_name_);
    }
}

void MachoImportHook::StopHook() {
    for (auto &entry : address_map_) {
        void **address = entry.first;
        void *origin_function = entry.second;
        *address = origin_function;
    }
}

void **MachoImportHook::GetImportAddress(const macho_header *mh, intptr_t slide) {
    macho_nlist *symbol_table = nullptr;
    const char *string_table = nullptr;
    u1 *link_edit_base = nullptr;
    u4 *indirect_symbol_table = nullptr;

    u4 cmd_count = mh->ncmds;
    load_command *first_cmd = OffsetOf<load_command *>(const_cast<macho_header *>(mh), sizeof(macho_header));
    load_command *cmd = first_cmd;
    for (int i = 0; i < cmd_count; ++i) {
        if (cmd->cmd == LC_SEGMENT_COMMAND) {
            macho_segment_command *seg = reinterpret_cast<macho_segment_command *>(cmd);
            if (!strcmp(seg->segname, SEG_LINKEDIT)) {
                link_edit_base = reinterpret_cast<u1 *>(seg->vmaddr + slide - seg->fileoff);
                break;
            }
        }
        cmd = OffsetOf<load_command *>(cmd, cmd->cmdsize);
    }
    cmd = first_cmd;
    for (int i = 0; i < cmd_count; ++i) {
        switch (cmd->cmd) {
            case LC_SYMTAB: {
                symtab_command *symtab = reinterpret_cast<symtab_command *>(cmd);
                string_table = (char *) &link_edit_base[symtab->stroff];
                symbol_table = (macho_nlist *) (&link_edit_base[symtab->symoff]);
                break;
            }
            case LC_DYSYMTAB: {
                dysymtab_command *dsymtab = reinterpret_cast<dysymtab_command *>(cmd);
                indirect_symbol_table = (u4 *) (&link_edit_base[dsymtab->indirectsymoff]);
                break;
            }
            default:
                break;
        }
        cmd = OffsetOf<load_command *>(cmd, cmd->cmdsize);
    }
    cmd = first_cmd;
    for (int i = 0; i < cmd_count; ++i) {
        if (cmd->cmd == LC_SEGMENT_COMMAND) {
            macho_segment_command *seg = reinterpret_cast<macho_segment_command *>(cmd);
            macho_section *sect_start = OffsetOf<macho_section *>(seg, sizeof(macho_segment_command));
            macho_section *sect_end = &sect_start[seg->nsects];
            macho_section *sect;
            for (sect = sect_start; sect < sect_end; ++sect) {
                int type = sect->flags & SECTION_TYPE;
                if (type == S_LAZY_DYLIB_SYMBOL_POINTERS
                    || type == S_LAZY_SYMBOL_POINTERS
                    || type == S_NON_LAZY_SYMBOL_POINTERS) {

                    size_t ptr_count = sect->size / sizeof(void *);
                    void **symbol_pointers = reinterpret_cast<void **>(sect->addr + slide);
                    uint32_t indirect_table_offset = sect->reserved1;
                    for (int lazy_index = 0; lazy_index < ptr_count; lazy_index++) {
                        uint32_t symbol_index = indirect_symbol_table[indirect_table_offset + lazy_index];
                        if (symbol_index != INDIRECT_SYMBOL_ABS && symbol_index != INDIRECT_SYMBOL_LOCAL) {
                            const char *current_symbol_name = &string_table[symbol_table[symbol_index].n_un.n_strx];
                            if (!strcmp(current_symbol_name, symbol_name_)) {
                                void **result = symbol_pointers + lazy_index;
                                return result;
                            }
                        }
                    }  // end sym foreach
                }

            }  // end section foreach
        }
        cmd = OffsetOf<load_command *>(cmd, cmd->cmdsize);
    }  // end cmd foreach
    return nullptr;
}

}  // namespace darwin
}  // namespace whale
