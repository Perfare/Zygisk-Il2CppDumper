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
#include "game.h"
#include <xdl.h>

int isGame(JNIEnv *env, jstring appDataDir) {
    if (!appDataDir)
        return 0;
    const char *app_data_dir = env->GetStringUTFChars(appDataDir, nullptr);
    int user = 0;
    static char package_name[256];
    if (sscanf(app_data_dir, "/data/%*[^/]/%d/%s", &user, package_name) != 2) {
        if (sscanf(app_data_dir, "/data/%*[^/]/%s", package_name) != 1) {
            package_name[0] = '\0';
            LOGW("can't parse %s", app_data_dir);
            return 0;
        }
    }
    int flag = 0;
    if (strcmp(package_name, GamePackageName) == 0) {
        LOGI("detect game: %s", package_name);
        game_data_dir = new char[strlen(app_data_dir) + 1];
        strcpy(game_data_dir, app_data_dir);
        flag = 1;
    }
    env->ReleaseStringUTFChars(appDataDir, app_data_dir);
    return flag;
}

static int GetAndroidApiLevel() {
    char prop_value[PROP_VALUE_MAX];
    __system_property_get("ro.build.version.sdk", prop_value);
    return atoi(prop_value);
}

void *hack_thread(void *arg) {
    LOGI("hack thread: %d", gettid());
    int api_level = GetAndroidApiLevel();
    LOGI("api level: %d", api_level);
    sleep(5);
    void *handle = xdl_open("libil2cpp.so", 0);
    if (handle) {
        il2cpp_dump(handle, game_data_dir);
    } else {
        LOGI("libil2cpp.so not found in thread %d", gettid());
    }
    return nullptr;
}