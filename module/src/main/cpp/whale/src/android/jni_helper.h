#ifndef WHALE_ANDROID_ART_JNI_HELPER_H_
#define WHALE_ANDROID_ART_JNI_HELPER_H_

#include <jni.h>
#include <base/macros.h>

#define NATIVE_METHOD(className, functionName, signature) \
    { #functionName, signature, reinterpret_cast<void*>(className ## _ ## functionName) }

#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

#define JNI_START JNIEnv *env, jclass cl

static inline void JNIExceptionClear(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
}

static inline bool JNIExceptionCheck(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        jthrowable e = env->ExceptionOccurred();
        env->Throw(e);
        env->DeleteLocalRef(e);
        return true;
    }
    return false;
}

static inline void JNIExceptionClearAndDescribe(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

template<typename T>
class ScopedLocalRef {
 public:
    ScopedLocalRef(JNIEnv *env, T localRef) : mEnv(env), mLocalRef(localRef) {
    }

    ~ScopedLocalRef() {
        reset();
    }

    void reset(T ptr = nullptr) {
        if (ptr != mLocalRef) {
            if (mLocalRef != nullptr) {
                mEnv->DeleteLocalRef(mLocalRef);
            }
            mLocalRef = ptr;
        }
    }

    T release() {
        T localRef = mLocalRef;
        mLocalRef = nullptr;
        return localRef;
    }

    T get() const {
        return mLocalRef;
    }

 private:
    JNIEnv *const mEnv;
    T mLocalRef;

    DISALLOW_COPY_AND_ASSIGN(ScopedLocalRef);
};

#endif  // WHALE_ANDROID_ART_JNI_HELPER_H_
