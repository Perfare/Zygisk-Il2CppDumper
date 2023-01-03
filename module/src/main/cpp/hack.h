//
// Created by Perfare on 2020/7/4.
//

#ifndef ZYGISK_IL2CPPDUMPER_HACK_H
#define ZYGISK_IL2CPPDUMPER_HACK_H

#include <jni.h>
#include "log.h"

static int enable_hack;
static char *game_data_dir = NULL;

int isGame(JNIEnv *env, jstring appDataDir);

void *hack_thread(void *arg);

#endif //ZYGISK_IL2CPPDUMPER_HACK_H
