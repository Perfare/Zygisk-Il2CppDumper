#ifndef WHALE_CODE_INTERCEPTOR_H_
#define WHALE_CODE_INTERCEPTOR_H_

#include <list>
#include <cstdint>
#include "base/logging.h"
#include "dbi/instruction_set.h"
#include "dbi/hook_common.h"

namespace whale {

class Interceptor {
 public:
    static Interceptor *Instance();

    void AddHook(std::unique_ptr<Hook> &hook);

    void RemoveHook(int id);

    void RemoveHook(std::unique_ptr<Hook> &hook) {
        RemoveHook(hook->id_);
    }
    void TraverseHooks(std::function<void(std::unique_ptr<Hook>&)> visitor);

 private:
    std::list<std::unique_ptr<Hook>> hook_list_;
};

}  // namespace whale

#endif  // WHALE_CODE_INTERCEPTOR_H_

