//
// Created by Perfare on 2020/7/4.
//

#include "hack.h"
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/system_properties.h>
#include <dlfcn.h>
#include "il2cpp_dump.h"
#include "log.h"
#include "xdl.h"

static int GetAndroidApiLevel() {
    char prop_value[PROP_VALUE_MAX];
    __system_property_get("ro.build.version.sdk", prop_value);
    return atoi(prop_value);
}

void hack_start(const char *game_data_dir) {
    void *handle = xdl_open("libil2cpp.so", 0);
    if (handle) {
        il2cpp_api_init(handle);
        il2cpp_dump(game_data_dir);
    } else {
        LOGI("libil2cpp.so not found in thread %d", gettid());
    }
}

void hack_prepare(const char *game_data_dir) {
    LOGI("hack thread: %d", gettid());
    int api_level = GetAndroidApiLevel();
    LOGI("api level: %d", api_level);
    sleep(5);

    hack_start(game_data_dir);
}