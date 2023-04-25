//
// Created by StarHeartHunt on 2023/4/3.
//

#include "string_utils.h"

std::string Utf16ToUtf8(const Il2CppChar *utf16String, int maximumSize) {
    const Il2CppChar *ptr = utf16String;
    size_t length = 0;
    while (*ptr) {
        ptr++;
        length++;
        if (maximumSize != -1 && length == maximumSize)
            break;
    }

    std::string utf8String;
    utf8String.reserve(length);
    utf8::unchecked::utf16to8(utf16String, ptr, std::back_inserter(utf8String));

    return utf8String;
}

std::string Utf16ToUtf8(const Il2CppChar *utf16String) {
    return Utf16ToUtf8(utf16String, -1);
}

std::string Il2CppStringToStdString(Il2CppString *str) {
    auto chars = il2cpp_string_chars(str);
    return Utf16ToUtf8(chars);
}