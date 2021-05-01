//
// Created by maba_kalox on 4/26/21.
//

#ifndef ESP32_CAM_CONCAT_STR_H
#define ESP32_CAM_CONCAT_STR_H

#include <Arduino.h>
#include <memory>

std::unique_ptr<char[]> concatStr(
        const char *str1,
        const char *str2,
        const char *str3 = "");


#endif //ESP32_CAM_CONCAT_STR_H
