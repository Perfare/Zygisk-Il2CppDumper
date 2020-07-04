#include "ffi_cxx.h"

FFICallInterface::~FFICallInterface() {
    for (FFIClosure *closure : closures_) {
        delete closure;
    }
    delete cif_;
    delete types_;
}


void FFIDispatcher(ffi_cif *cif OPTION, void *ret, void **args, void *userdata) {
    FFIClosure *closure = reinterpret_cast<FFIClosure *>(userdata);
    FFICallback callback = closure->GetCallback();
    callback(closure, ret, args, closure->GetUserData());
}


FFIClosure *FFICallInterface::CreateClosure(void *userdata, FFICallback callback) {
    std::lock_guard<std::mutex> guard(lock_);
    FFIClosure *closure = new FFIClosure(this, userdata, callback);
    ffi_prep_closure_loc(closure->closure_, cif_, FFIDispatcher, closure, closure->code_);
    closures_.push_back(closure);
    return closure;
}

static ffi_type *FFIGetCType(FFIType type) {
    switch (type) {
        case FFIType::kFFITypeVoid:
            return &ffi_type_void;
        case FFIType::kFFITypeU1:
            return &ffi_type_uint8;
        case FFIType::kFFITypeU2:
            return &ffi_type_uint16;
        case FFIType::kFFITypeU4:
            return &ffi_type_uint32;
        case FFIType::kFFITypeU8:
            return &ffi_type_uint64;
        case FFIType::kFFITypeS1:
            return &ffi_type_sint8;
        case FFIType::kFFITypeS2:
            return &ffi_type_sint16;
        case FFIType::kFFITypeS4:
            return &ffi_type_sint32;
        case FFIType::kFFITypeS8:
            return &ffi_type_sint64;
        case FFIType::kFFITypePointer:
            return &ffi_type_pointer;
        case FFIType::kFFITypeFloat:
            return &ffi_type_float;
        case FFIType::kFFITypeDouble:
            return &ffi_type_double;
    }
}

FFICallInterface *FFICallInterface::FinalizeCif() {
    cif_ = new ffi_cif;
    types_ = new ffi_type *[parameters_.size()];
    int idx = 0;
    for (FFIType type : parameters_) {
        types_[idx] = FFIGetCType(type);
        idx++;
    }
    ffi_prep_cif(cif_, FFI_DEFAULT_ABI,
                 (unsigned int) parameters_.size(), FFIGetCType(return_type_), types_);
    return this;
}

