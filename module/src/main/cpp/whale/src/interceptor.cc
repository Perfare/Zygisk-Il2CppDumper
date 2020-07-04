#include "interceptor.h"

namespace whale {

Interceptor *Interceptor::Instance() {
    static Interceptor instance;
    return &instance;
}

void Interceptor::AddHook(std::unique_ptr<Hook> &hook) {
    hook->id_ = static_cast<int>(hook_list_.size());
    hook->StartHook();
    hook_list_.push_back(std::move(hook));
}

void Interceptor::RemoveHook(int id) {
    for (auto &entry : hook_list_) {
        if (entry->id_ == id) {
            hook_list_.remove(entry);
            entry->StopHook();
            break;
        }
    }
}

void Interceptor::TraverseHooks(std::function<void(std::unique_ptr<Hook> &)> visitor) {
    for (auto &hook : hook_list_) {
        visitor(hook);
    }
}

}  // namespace whale
