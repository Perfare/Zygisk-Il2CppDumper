#ifndef WHALE_ARCH_HOOK_H_
#define WHALE_ARCH_HOOK_H_

#include <cstdint>
#include <map>
#include "dbi/instruction_set.h"

namespace whale {

enum class HookType {
    kNone,
    kInlineHook,
    kImportHook,
};

class Hook {
 public:
    int id_{};

    virtual ~Hook() = default;

    virtual InstructionSet getISA() {
        return InstructionSet::kNone;
    }

    virtual HookType GetType() {
        return HookType::kNone;
    }

    virtual void StartHook() = 0;

    virtual void StopHook() = 0;
};

class ImportHook : public Hook {
 public:
    ImportHook(const char *symbol_name, void *replace, void **backup)
            : symbol_name_(symbol_name),
              replace_(replace),
              backup_(backup) {}

    virtual ~ImportHook() override = default;

    HookType GetType() override {
        return HookType::kImportHook;
    }

    const char *GetSymbolName() {
        return symbol_name_;
    }

    template<typename T>
    ALWAYS_INLINE T GetReplaceAddress() {
        return (T) replace_;
    }

 protected:
    const char *symbol_name_;
    std::map<void **, void *> address_map_;
    void *replace_;
    void **backup_;
};

class InlineHook : public Hook {
 public:
    InlineHook(intptr_t address, intptr_t replace, intptr_t *backup)
            : address_(address),
              replace_(replace),
              backup_(backup) {}

    virtual ~InlineHook() override = default;

    template<typename T>
    ALWAYS_INLINE T GetTarget() {
        return (T) address_;
    }

    template<typename T>
    ALWAYS_INLINE T GetReplaceAddress() {
        return (T) replace_;
    }

    HookType GetType() override {
        return HookType::kInlineHook;
    }

 protected:
    intptr_t address_;
    intptr_t replace_;
    intptr_t *backup_;
};

typedef bool (*MemoryRangeCallback)(const char *path, bool *stop);

class InterceptSysCallHook : public Hook {
 public:
    InterceptSysCallHook(MemoryRangeCallback callback);

    void StartHook();

    void StopHook();

 protected:
    virtual void FindSysCalls(uintptr_t start, uintptr_t end) = 0;

    MemoryRangeCallback callback_;
};

}  // namespace whale

#endif  // WHALE_ARCH_HOOK_H_

