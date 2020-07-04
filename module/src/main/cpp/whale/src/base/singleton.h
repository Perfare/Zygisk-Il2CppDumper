#ifndef WHALE_BASE_SINGLETON_H_
#define WHALE_BASE_SINGLETON_H_

template<typename T>
class Singleton {
 public:
    Singleton(std::function<void(T *)> init_function) : init_function_(init_function),
                                                  initialized_(false) {}

    void Ensure() {
        if (!initialized_) {
            std::lock_guard<std::mutex> guard(lock_);
            if (!initialized_) {
                init_function_(&instance_);
                initialized_ = true;
            }
        }
    }

    T Get() {
        Ensure();
        return instance_;
    }

 private:
    typename std::conditional<std::is_void<T>::value, bool, T>::type instance_;
    std::mutex lock_;
    std::function<void(T *)> init_function_;
    bool initialized_;
};

#endif  // WHALE_BASE_SINGLETON_H_
