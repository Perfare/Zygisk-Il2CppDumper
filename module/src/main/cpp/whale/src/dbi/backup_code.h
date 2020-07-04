#ifndef ARCH_BACKUP_CODE_H_
#define ARCH_BACKUP_CODE_H_

#include <memory>
#include "base/primitive_types.h"
#include "base/macros.h"
#include "base/logging.h"


namespace whale {

class BackupCode {
 public:
    BackupCode(const void *address, const size_t size) : size_(size) {
        insns_ = malloc(size);
        memcpy(insns_, address, size);
    }

    ~BackupCode() {
        free(insns_);
    }

    size_t GetSizeInBytes() {
        return size_;
    }

    size_t GetCount(size_t insn_size) {
        return size_ / insn_size;
    }

    template<typename T>
    size_t GetCount() {
        return GetCount(sizeof(T));
    }

    template<typename T>
    T *GetInstructions() {
        return reinterpret_cast<T *>(insns_);
    }

    intptr_t GetInstructions() {
        return reinterpret_cast<intptr_t>(insns_);
    }

 private:
    void *insns_;
    size_t size_;

 private:
    DISALLOW_COPY_AND_ASSIGN(BackupCode);
};

}  // namespace whale

#endif  // ARCH_BACKUP_CODE_H_

