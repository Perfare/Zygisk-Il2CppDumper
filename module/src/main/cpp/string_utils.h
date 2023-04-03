//
// Created by StarHeartHunt on 2023/4/3.
//

#ifndef ZYGISK_IL2CPPDUMPER_STRING_UTILS_H
#define ZYGISK_IL2CPPDUMPER_STRING_UTILS_H

#include "utf8.h"

std::string Utf16ToUtf8(const Il2CppChar *utf16String, int maximumSize);
std::string Utf16ToUtf8(const Il2CppChar *utf16String);
std::string Il2CppStringToStdString(Il2CppString *str);

#endif //ZYGISK_IL2CPPDUMPER_STRING_UTILS_H
