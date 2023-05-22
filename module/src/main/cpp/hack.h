//
// Created by Perfare on 2020/7/4.
//

#ifndef ZYGISK_IL2CPPDUMPER_HACK_H
#define ZYGISK_IL2CPPDUMPER_HACK_H

#include <stddef.h>

struct ArmLoader {
    void *arm;
    size_t arm_length;
    void *arm64;
    size_t arm64_length;
};

void hack_prepare(const char *game_data_dir, ArmLoader *loader);

#endif //ZYGISK_IL2CPPDUMPER_HACK_H
