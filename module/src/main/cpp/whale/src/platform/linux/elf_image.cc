#include "platform/linux/elf_image.h"

#define SHT_GNU_HASH 0x6ffffff6

namespace whale {

static u4 ElfHash(const char *name) {
    const u1 *name_bytes = reinterpret_cast<const u1 *>(name);
    u4 h = 0, g;
    while (*name_bytes) {
        h = (h << 4) + *name_bytes++;
        g = h & 0xf0000000;
        h ^= g;
        h ^= g >> 24;
    }
    return h;
}

static u4 GnuHash(const char *name) {
    const u1 *name_bytes = reinterpret_cast<const u1 *>(name);
    u4 h = 5381;
    while (*name_bytes) {
        h += (h << 5) + *name_bytes++;
    }
    return h;
}

Elf_Sym *ElfParser::LinearLookup(const char *name, Elf_Sym *symtab,
                                 uintptr_t symcnt, const char *strtab) {

    while (symcnt > 0) {
        --symcnt;
        if ((symtab[symcnt].st_info & (STT_OBJECT | STT_FUNC)) != 0) {
            if (strcmp(strtab + symtab[symcnt].st_name, name) == 0) {
                return symtab + symcnt;
            }
        }
    }
    return nullptr;
}

Elf_Sym *ElfParser::ElfLookup(const char *name) {
    uint_fast32_t hash = ElfHash(name);

    for (uintptr_t n = bucket_[hash % nbucket_]; n != 0; n = chain_[n]) {
        Elf_Sym *sym = dynsym_ + n;
        if (strcmp(dynstr_ + sym->st_name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}

Elf_Sym *ElfParser::GnuLookup(const char *name) {
    static constexpr uint_fast32_t bloom_mask_bits = sizeof(ElfW(Addr)) * 8;

    uint32_t hashval = GnuHash(name);
    uint32_t hashval2 = hashval >> gnu_shift2_;
    Elf_Addr bloom_word = gnu_bloom_filter_[(hashval / bloom_mask_bits) &
                                            gnu_maskwords_bm_];

    if ((1 & (bloom_word >> (hashval % bloom_mask_bits)) &
         (bloom_word >> (hashval2 % bloom_mask_bits))) != 0) {
        uint_fast32_t sym_index = gnu_bucket_[hashval % gnu_nbucket_];
        if (sym_index != 0) {
            do {
                Elf_Sym *sym = dynsym_ + sym_index;
                if (((gnu_chain_[sym_index] ^ hashval) >> 1) == 0
                    && strcmp(dynstr_ + sym->st_name, name) == 0) {
                    return sym;
                }
            } while ((gnu_chain_[sym_index++] & 1) == 0);
        }
    }
    return nullptr;
}

uintptr_t ElfParser::FindSymbolOffset(const char *name) {
    Elf_Sym *sym = nullptr;
    if (gnu_nbucket_ > 0) {
        sym = GnuLookup(name);
    }
    if (sym == nullptr && nbucket_ > 0) {
        sym = ElfLookup(name);
    }
    if (sym == nullptr && symtab_ != nullptr) {
        sym = LinearLookup(name, symtab_,
                           symcnt_, strtab_);
    }
    if (sym != nullptr) {
        return static_cast<uintptr_t>(sym->st_value + load_bias_);
    }
    return 0;
}

bool ElfParser::Parse(uintptr_t base) {
    load_bias_ = INT_MAX;
    ehdr_ = reinterpret_cast<Elf_Ehdr *>(base);
    phdr_ = OffsetOf<Elf_Phdr *>(base, ehdr_->e_phoff);
    shdr_ = OffsetOf<Elf_Shdr *>(base, ehdr_->e_shoff);

    if (ehdr_->e_shnum <= 0) {
        return false;
    }
    shstrtab_ = OffsetOf<char *>(ehdr_, shdr_[ehdr_->e_shstrndx].sh_offset);
    dynstr_ = nullptr;
    for (int i = 0; i < ehdr_->e_shnum; ++i) {
        switch (shdr_[i].sh_type) {
            case SHT_SYMTAB:
                symtab_ = OffsetOf<Elf_Sym *>(ehdr_, shdr_[i].sh_offset);
                symcnt_ = shdr_[i].sh_size / sizeof(Elf_Sym);
                break;
            case SHT_DYNSYM:
                dynsym_ = OffsetOf<Elf_Sym *>(ehdr_, shdr_[i].sh_offset);
                dyncnt_ = shdr_[i].sh_size / sizeof(Elf_Sym);
                break;
            case SHT_STRTAB: {
                char *name = shstrtab_ + shdr_[i].sh_name;
                char *table = OffsetOf<char *>(ehdr_, shdr_[i].sh_offset);
                if (dynstr_ == nullptr) {
                    dynstr_ = table;
                } else if (!strcmp(name, ".strtab")) {
                    strtab_ = table;
                }
                break;
            }
            case SHT_PROGBITS: {
                if (load_bias_ == INT_MAX) {
                    load_bias_ = static_cast<uintptr_t>(shdr_[i].sh_offset - shdr_[i].sh_addr);
                } else {
                    char *name = shstrtab_ + shdr_[i].sh_name;
                    if (!strcmp(name, ".got")) {
                        got_ = &shdr_[i];
                    } else if (!strcmp(name, ".got.plt")) {
                        got_plt_ = &shdr_[i];
                    }
                }
                break;
            }
            case SHT_HASH: {
                Elf_Word *d_un = OffsetOf<Elf_Word *>(ehdr_, shdr_[i].sh_offset);
                nbucket_ = d_un[0];
                nchain_ = d_un[1];
                bucket_ = d_un + 2;
                chain_ = bucket_ + nbucket_;
                break;
            }
            case SHT_GNU_HASH: {
                Elf_Word *d_buf = OffsetOf<Elf_Word *>(ehdr_, shdr_[i].sh_offset);
                gnu_nbucket_ = d_buf[0];
                gnu_symndx_ = d_buf[1];
                gnu_maskwords_bm_ = d_buf[2];
                if (ehdr_->e_ident[EI_CLASS] == ELFCLASS64) {
                    gnu_maskwords_bm_ *= 2;
                }
                gnu_shift2_ = d_buf[3];
                gnu_bloom_filter_ = d_buf + 4;
                gnu_bucket_ = d_buf + 4 + gnu_maskwords_bm_;
                gnu_chain_ = gnu_bucket_ + gnu_nbucket_ -
                             gnu_symndx_;
                --gnu_maskwords_bm_;
                break;
            }
            default: {
                char *name = shstrtab_ + shdr_[i].sh_name;
                if (!strcmp(name, ".rel.dyn")) {
                    rel_dyn_ = &shdr_[i];
                } else if (!strcmp(name, ".rel.plt")) {
                    rel_plt_ = &shdr_[i];
                } else if (!strcmp(name, ".rela.dyn")) {
                    rela_dyn_ = &shdr_[i];
                } else if (!strcmp(name, ".rela.plt")) {
                    rela_plt_ = &shdr_[i];
                }
                break;
            }
        }  // end switch
    }
    return true;
}

bool ElfReader::Open(const char *path) {
    // open will return -1 in houdini, so we use fopen.
    FILE *file = fopen(path, "rbe");
    if (file == nullptr) {
        LOG(ERROR) << "failed to open: " << path << ", err: " << strerror(errno);
        return false;
    }
    int fd = fileno(file);
    struct stat stat;
    if (fstat(fd, &stat) != 0) {
        return false;
    }
    len_ = static_cast<size_t>(stat.st_size);
    base_ = mmap(nullptr, len_, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base_ == MAP_FAILED) {
        return false;
    }
    TEMP_FAILURE_RETRY(read(fd, base_, len_));
    return true;
}

}  // namespace whale
