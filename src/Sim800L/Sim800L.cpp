//
// Created by maba_kalox on 4/30/21.
//

#include "Sim800L.h"

Sim800L::Sim800L(Stream *_stream, bool _is_debug) :
        is_debug(_is_debug), stream(_stream) {}

bool Sim800L::initGPRS(const char APN_data[]) {
    char base_command[] = "AT+CSTT=";
    auto cmd = concatStr(base_command, APN_data);
    sendCommand(cmd.get(), "OK");
    sendCommand("AT+CIICR", "OK");
    sendCommand("AT+CIFSR", "");
    return true;
}

bool Sim800L::initBASE() {
    return (
            sendCommand("AT", "OK") &&
            sendCommand("AT+CPIN?", "+CPIN: READY")
    );
}


bool Sim800L::sendCommand(const char cmd[], const char expectedResponse[]) {
    const size_t buff_size = 128;
    char buff[buff_size];
    size_t buff_taken = 1;
    stream->println(cmd);
    stream->flush();
    while ((buff_taken < 3 || buff[buff_taken - 3] != '\r') && waitForStream()) {
        size_t bytes_read = stream->readBytes(buff + buff_taken - 1, buff_size - buff_taken);
        buff_taken += bytes_read;
        buff[buff_taken - 1] = '\0';
    }
    char *tmp = buff;
    size_t prev_len = 0;
    for (size_t i = 0; i < buff_taken - 1; i++) {
        if (buff[i] == '\n') {
            buff[i] = '\0';
            Serial.print(strlen(tmp));
            Serial.print(':');
            Serial.println(tmp);
            tmp = &buff[i + 1];
        } else if (buff[i] == '\r') {
            buff[i] = '\0';
        }
    }
    return true;
}

byte Sim800L::waitForStream() {
    unsigned long waitStartTime = millis();
    while (!stream->available() && millis() - waitStartTime < timeOut_ms) {
        delay(1);
    }
    byte prevAm = 0;
    uint32_t tmr = millis();
    while (millis() - tmr < 50 && prevAm <= 64) {
        byte am = stream->available();
        if (am != prevAm) {
            tmr = millis();
            prevAm = am;
        }
    }
    return prevAm;
}
