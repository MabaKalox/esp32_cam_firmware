//
// Created by maba_kalox on 4/30/21.
//

#ifndef ESP32_GSM_MINE_SIM800L_H
#define ESP32_GSM_MINE_SIM800L_H

#include <Arduino.h>
#include <helpers/concatStr.h>
#include <memory>

class Sim800L {
private:
    bool is_debug;
    const unsigned long timeOut_ms = 10000;
    Stream *stream = nullptr;
    byte waitForStream();
protected:
public:
    std::unique_ptr<char[]> sendCommand(const char cmd[]);
    explicit Sim800L(Stream *_stream, bool _is_debug = false);
    bool initBASE();
    bool initGPRS(const char APN_data[]);
};


#endif //ESP32_GSM_MINE_SIM800L_H
