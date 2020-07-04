#ifndef WHALE_FFI_CXX_H_
#define WHALE_FFI_CXX_H_

#include <cstdarg>
#include <list>
#include <mutex>
#include "ffi.h"
#include "base/macros.h"

enum class FFIType {
    kFFITypeVoid,
    kFFITypeU1,
    kFFITypeU2,
    kFFITypeU4,
    kFFITypeU8,
    kFFITypeS1,
    kFFITypeS2,
    kFFITypeS4,
    kFFITypeS8,
    kFFITypePointer,
    kFFITypeFloat,
    kFFITypeDouble,
};

class FFICallInterface;

class FFIClosure;

typedef void (*FFICallback)(FFIClosure *closure, void *ret, void **args, void *userdata);

class FFIClosure {
 public:
    FFIClosure(FFICallInterface *cif, void *userdata, FFICallback callback) : cif_(cif),
                                                                              userdata_(userdata),
                                                                              callback_(callback) {
        closure_ = reinterpret_cast<ffi_closure *>(ffi_closure_alloc(sizeof(ffi_closure), &code_));
    }

    ~FFIClosure() {
        ffi_closure_free(closure_);
    }

    void *GetCode() {
        return code_;
    }

    void *GetUserData() {
        return userdata_;
    }

    FFICallInterface *GetCif() {
        return cif_;
    }

    FFICallback GetCallback() {
        return callback_;
    }

 private:
    friend class FFICallInterface;

    FFICallInterface *cif_;
    ffi_closure *closure_;
    FFICallback callback_;
    void *code_;
    void *userdata_;

};

class FFICallInterface {
 public:
    FFICallInterface(const FFIType return_type) : return_type_(return_type) {}

    ~FFICallInterface();

    FFICallInterface *Parameter(const FFIType parameter) {
        parameters_.push_back(parameter);
        return this;
    }

    FFICallInterface *Parameters(unsigned int count, ...) {
        va_list ap;
        va_start(ap, count);
        while (count-- > 0) {
            Parameter(va_arg(ap, FFIType));
        }
        va_end(ap);
        return this;
    }

    FFICallInterface *FinalizeCif();

    size_t GetParameterCount() {
        return parameters_.size();
    }

    std::list<FFIType> GetParameters() {
        return parameters_;
    }

    FFIClosure *CreateClosure(void *userdata, FFICallback callback);

    void RemoveClosure(FFIClosure *closure) {
        std::lock_guard<std::mutex> guard(lock_);
        closures_.remove(closure);
    }

 private:
    std::mutex lock_;
    ffi_cif *cif_;
    ffi_type **types_;
    std::list<FFIType> parameters_;
    const FFIType return_type_;
    std::list<FFIClosure *> closures_;
};

#endif //WHALE_FFI_CXX_H_
