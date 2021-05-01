//
// Created by maba_kalox on 4/26/21.
//

#include "concatStr.h"

std::unique_ptr<char[]> concatStr(const char *str1, const char *str2, const char *str3) {
    auto new_str = std::unique_ptr<char[]>(
            new char[strlen(str1) + strlen(str2) + strlen(str3) + 1]);
    strcpy(new_str.get(), str1);
    strcat(new_str.get(), str2);
    strcat(new_str.get(), str3);
    return new_str;
}
