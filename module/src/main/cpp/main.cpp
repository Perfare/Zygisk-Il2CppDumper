#include <cstring>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cinttypes>
#include "hack.h"
#include "zygisk.hpp"
#include "game.h"
#include "log.h"

using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        auto package_name = env->GetStringUTFChars(args->nice_name, nullptr);
        auto app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        preSpecialize(package_name, app_data_dir);
        env->ReleaseStringUTFChars(args->nice_name, package_name);
        env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (enable_hack) {
            std::thread hack_thread(hack_prepare, game_data_dir, data, length);
            hack_thread.detach();
        }
    }

private:
    Api *api;
    JNIEnv *env;
    bool enable_hack;
    char *game_data_dir;
    void *data;
    size_t length;

    void preSpecialize(const char *package_name, const char *app_data_dir) {
        const char* target = GamePackageName;
        char buf[256];
        int count;
        unsigned custom_package = 0;
        int fd = api->connectCompanion();
        read(fd, &custom_package, sizeof(custom_package));
        if (custom_package){
            bzero(buf, 256);
            count = read(fd, buf, 256);
            if (count > 0){
                target = buf;
//                LOGD("process=[%s], custom_package=[%u], target[%s]\n", package_name, custom_package, target);
            }
        }
        close(fd);
        if (strcmp(package_name, target) == 0) {
            LOGI("detect game: %s", package_name);
            enable_hack = true;
            game_data_dir = new char[strlen(app_data_dir) + 1];
            strcpy(game_data_dir, app_data_dir);

#if defined(__i386__)
            auto path = "zygisk/armeabi-v7a.so";
#endif
#if defined(__x86_64__)
            auto path = "zygisk/arm64-v8a.so";
#endif
#if defined(__i386__) || defined(__x86_64__)
            int dirfd = api->getModuleDir();
            int fd = openat(dirfd, path, O_RDONLY);
            if (fd != -1) {
                struct stat sb{};
                fstat(fd, &sb);
                length = sb.st_size;
                data = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
                close(fd);
            } else {
                LOGW("Unable to open arm file");
            }
#endif
        } else {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }
    }
};

static void companion_handler(int i) {
    const char filepath[] = "/data/adb/il2cpp_dumper_target.txt";
    unsigned custom_package = 0;
    char buf[256];
    bzero(buf, 256);
    if (access(filepath, F_OK) == 0){
        FILE* fp = fopen(filepath, "r");
        if (fp){
            if (fgets(buf, 256, fp) != nullptr){
                char *tmp;
                if ((tmp = strstr(buf, "\n"))){
                    *tmp = '\0';
                }
                custom_package = 1;
                write(i, &custom_package, sizeof(custom_package));
                write(i, buf, strlen(buf));
                return;
            }
        }
    }
    write(i, &custom_package, sizeof(custom_package));
//    LOGD("companion custom_package=[%u]\n", custom_package);
}

REGISTER_ZYGISK_MODULE(MyModule)
REGISTER_ZYGISK_COMPANION(companion_handler)