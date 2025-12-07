//
// Created by Perfare on 2020/7/4.
//

#ifndef ZYGISK_IL2CPPDUMPER_GAME_H
#define ZYGISK_IL2CPPDUMPER_GAME_H

#include <string>
#include <vector>
#include <sstream>
#include <fcntl.h>
#include "log.h"

bool isTargetGame(const char *package_name, int module_dir_fd) {
    int fd = openat(module_dir_fd, "targets.txt", O_RDONLY);
    if (fd == -1) {
        return false;
    }

    struct stat st{};
    fstat(fd, &st);
    char *buf = new char[st.st_size + 1];
    read(fd, buf, st.st_size);
    buf[st.st_size] = 0;
    close(fd);

    std::istringstream iss(buf);
    std::string target;
    while (std::getline(iss, target, '\n')) {
        if (strcmp(package_name, target.c_str()) == 0) {
            delete[] buf;
            return true;
        }
    }

    delete[] buf;
    return false;
}

#endif //ZYGISK_IL2CPPDUMPER_GAME_H
