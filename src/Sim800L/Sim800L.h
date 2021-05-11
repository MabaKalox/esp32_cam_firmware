//
// Created by maba_kalox on 4/30/21.
//

#ifndef ESP32_GSM_MINE_SIM800L_H
#define ESP32_GSM_MINE_SIM800L_H

#include <Arduino.h>
#include <helpers/concatStr.h>
#include "SoftwareSerial.h"
#include <memory>
#include <SPIFFS.h>

#define default_timeOut_ms 20000
enum NetworkRegistration {
    NOT_REGISTERED, REGISTERED_HOME, SEARCHING, DENIED, NET_UNKNOWN, REGISTERED_ROAMING, NET_ERROR
};

class Sim800L {
private:
    bool is_debug;
    Stream *stream = nullptr;

    byte waitForStream(uint32_t timeOut_ms = default_timeOut_ms);

    void (*rst_handler)();

protected:
    std::unique_ptr<char[]> sendCommand(
            const char cmd[], uint32_t timeOut_ms = default_timeOut_ms);

    std::unique_ptr<char[]> readResponse(const uint32_t& timeOut_ms = default_timeOut_ms);

public:
    int getSignal();

    NetworkRegistration getRegistrationStatus();

    Sim800L(Stream *_stream, void (*_rst_handler)(), bool _is_debug = false);

    static bool has_OK(char response[]);

    bool initBASE();

    bool initGPRS(const char APN_data[]);

    bool send_file(const uint8_t* file_buff, size_t file_buff_len);

    bool setSSL();
};


#endif //ESP32_GSM_MINE_SIM800L_H
