//
// Created by maba_kalox on 4/30/21.
//

#include "Sim800L.h"

Sim800L::Sim800L(Stream *_stream, bool _is_debug) :
        is_debug(_is_debug), stream(_stream) {}

bool Sim800L::initGPRS(const char APN_data[]) {
    char base_command[] = "AT+CSTT=";
    auto cmd = concatStr(base_command, APN_data);
    auto response0 = sendCommand(cmd.get());
    if (response0) {
        Serial.print(response0.get());
        auto response1 = sendCommand("AT+CIICR");
        if (response1) {
            Serial.print(response1.get());
            auto response2 = sendCommand("AT+CIFSR");
            if (response2) {
                Serial.print(response2.get());
                return true;
            }
        }
    }
    return false;
}

bool Sim800L::initBASE() {
    auto response0 = sendCommand("AT");
    if (response0) {
        Serial.print(response0.get());
        auto response1 = sendCommand("AT+CPIN?");
        if (response1) {
            Serial.print(response1.get());
            return true;
        }
    }
    return false;
}


std::unique_ptr<char[]> Sim800L::sendCommand(const char cmd[]) {
    const size_t buff_size = 129;
    char buff[buff_size];
    size_t char_stored = 0;
    stream->println(cmd);
    stream->flush();
    bool was_response;
    while ((char_stored < 2 || (buff[char_stored - 2] != '\r' && buff[char_stored - 1] != '\n')) &&
           (was_response = waitForStream())) {
        size_t bytes_read = stream->readBytes(buff + char_stored, buff_size - char_stored - 1);
        char_stored += bytes_read;
        buff[char_stored] = '\0';
    }
    if (was_response) {
        char *new_line_ptr = strchr(buff, '\n');
        *new_line_ptr = '\0';
        if (strncmp(buff, cmd, strlen(cmd)) == 0) {
            Serial.println("Command received good");
            auto new_str = std::unique_ptr<char[]>(new char[strlen(new_line_ptr+1) + 1]);
            strcpy(new_str.get(), new_line_ptr+1);
            return new_str;
        } else {
            Serial.println("=======");
            Serial.println("Command was not delivered!");
            Serial.print("Command was: ");
            Serial.println(cmd);
            Serial.println("=======");
        }
    } else {
        Serial.println("=======");
        Serial.println("Response Timeout!");
        Serial.print("On command: ");
        Serial.println(cmd);
        Serial.println("=======");
    }
    return nullptr;
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
