//
// Created by Perfare on 2020/7/4.
//

#ifndef RIRU_IL2CPPDUMPER_IL2CPP_H
#define RIRU_IL2CPPDUMPER_IL2CPP_H

#include "game.h"

#define STR(x) #x
#define STRINGIFY_MACRO(x) STR(x)
#define EXPAND(x) x
#define IL2CPPHEADER(a, b, c) STRINGIFY_MACRO(EXPAND(a)EXPAND(b)EXPAND(c))
#define IL2CPPAPIDIR il2cppapi/
#define IL2CPPCLASS IL2CPPHEADER(IL2CPPAPIDIR, UnityVersion, /il2cpp-class.h)
#define IL2CPPAPI IL2CPPHEADER(IL2CPPAPIDIR, UnityVersion, /il2cpp-api-functions.h)

void il2cpp_dump(void *handle, char *outDir);

#endif //RIRU_IL2CPPDUMPER_IL2CPP_H
