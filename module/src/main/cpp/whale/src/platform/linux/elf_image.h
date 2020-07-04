#ifndef WHALE_PLATFORM_LINUX_ELF_READER_H_
#define WHALE_PLATFORM_LINUX_ELF_READER_H_

#include <cstdint>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <syscall.h>
#include <dlfcn.h>
#include "base/cxx_helper.h"
#include "base/primitive_types.h"
#include "base/logging.h"

#if defined(__LP64__)
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Phdr Elf64_Phdr
#define Elf_Sym  Elf64_Sym
#define Elf_Word  Elf64_Word
#define Elf_Addr  Elf64_Addr
#define ElfW(what) Elf64_ ## what
#define ELF_R_SYM ELF64_R_SYM
#define ELF_R_TYPE ELF64_R_TYPE
#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Phdr Elf32_Phdr
#define Elf_Sym  Elf32_Sym
#define Elf_Word  Elf32_Word
#define Elf_Addr  Elf32_Addr
#define ElfW(what) Elf32_ ## what
#define ELF_R_SYM ELF32_R_SYM
#define ELF_R_TYPE ELF32_R_TYPE
#endif

namespace whale {

class ElfParser {
 public:
    ElfParser() {}

    intptr_t GetLoadBias() {
        return load_bias_;
    }

    Elf_Sym *
    LinearLookup(const char *name, Elf_Sym *symtab, uintptr_t symcnt, const char *strtab);

    Elf_Sym *ElfLookup(const char *name);

    Elf_Sym *GnuLookup(const char *name);


    uintptr_t FindSymbolOffset(const char *name);

    bool Parse(uintptr_t base);

 private:
    Elf_Ehdr *ehdr_;
    Elf_Phdr *phdr_;
    Elf_Shdr *shdr_;
    uintptr_t load_bias_;

    Elf_Shdr *got_;
    Elf_Shdr *got_plt_;
    Elf_Shdr *rel_dyn_;
    Elf_Shdr *rel_plt_;
    Elf_Shdr *rela_dyn_;
    Elf_Shdr *rela_plt_;

    char *shstrtab_;
    char *strtab_;
    char *dynstr_;
    Elf_Sym *symtab_;
    uintptr_t symcnt_;
    Elf_Sym *dynsym_;
    uintptr_t dyncnt_;

    u4 nbucket_{};
    u4 nchain_{};
    u4 *bucket_{};
    u4 *chain_{};

    u4 gnu_nbucket_{};
    u4 gnu_symndx_{};
    u4 gnu_maskwords_bm_;
    u4 gnu_shift2_;
    u4 *gnu_bloom_filter_;
    u4 *gnu_bucket_;
    u4 *gnu_chain_;
};

class ElfReader {
 public:
    ElfReader() : base_(nullptr), len_(0), fp_(nullptr) {}

    ~ElfReader() {
        if (fp_) {
            fclose(fp_);
        }
        if (base_ != nullptr && base_ != MAP_FAILED) {
            munmap(base_, len_);
        }
    }

    bool Open(const char *path);

    bool ReadSectionHeaders() {
        return parser_.Parse(reinterpret_cast<uintptr_t>(base_));
    }

    intptr_t GetLoadBias() {
        return parser_.GetLoadBias();
    }

    uintptr_t FindSymbolOffset(const char *name) {
        return parser_.FindSymbolOffset(name);
    }

 private:
    void *base_;
    size_t len_;
    FILE *fp_;
    ElfParser parser_;
};


class ElfImage {
 public:
    ElfImage() : base_(0) {}

    bool Open(const char *path, uintptr_t base) {
        base_ = base;
        return reader_.Open(path) && reader_.ReadSectionHeaders();
    }

    template<typename T>
    T FindSymbol(const char *name) {
        uintptr_t offset = reader_.FindSymbolOffset(name);
        if (offset > 0) {
            uintptr_t ptr = base_ + offset;
            return ForceCast<T>(reinterpret_cast<ptr_t>(ptr));
        }
        return (T) nullptr;
    }


 private:
    ElfReader reader_;
    uintptr_t base_;
};

}  // namespace whale

#endif  // WHALE_PLATFORM_LINUX_ELF_READER_H_
