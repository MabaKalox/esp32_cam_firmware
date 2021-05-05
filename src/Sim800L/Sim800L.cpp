//
// Created by maba_kalox on 4/30/21.
//

#include "Sim800L.h"

#define AT_CMD_BASE "AT"
#define AT_CMD_START_GPRS "AT+CIICR"
#define AT_CMD_OBTAIN_IP "AT+CIFSR"
#define AT_GET_CPIN "AT+CPIN?"
#define AT_SET_APN "AT+CSTT="
#define AT_GET_REG "AT+CSQ"
#define AT_SET_SSL "AT+CIPSSL=1"

#define AT_RSP_OK "OK"
#define AT_RSP_CPIN_READY "+CPIN: READY"
#define CONNECT_OK "CONNECT OK"

Sim800L::Sim800L(Stream *_stream, void (*_rst_handler)(), bool _is_debug) :
        is_debug(_is_debug), stream(_stream), rst_handler(_rst_handler) {}

bool Sim800L::initGPRS(const char APN_data[]) {

    auto apn_cmd_response = sendCommand(
            concatStr(AT_SET_APN, APN_data).get()
    );
    if (!apn_cmd_response || !has_OK(apn_cmd_response.get())) {
        if (is_debug) Serial.print(apn_cmd_response.get());
        return false;
    }

    auto gprs_start_response = sendCommand(AT_CMD_START_GPRS);
    if (!gprs_start_response || !has_OK(gprs_start_response.get())) {
        if (is_debug) Serial.print(gprs_start_response.get());
        return false;
    }

    auto ip_addr = sendCommand(AT_CMD_OBTAIN_IP);
    if (!ip_addr) {
        return false;
    }
    return true;
}

bool Sim800L::initBASE() {
    rst_handler();
    std::unique_ptr<char[]> response0 = nullptr;
    while (!response0 || !has_OK(response0.get())) {
        response0.reset();
        response0 = sendCommand(AT_CMD_BASE, 1000);
    }
    while (getSignal() <= 0) {
        delay(1000);
    }
    auto n_stat = getRegistrationStatus();
    while (n_stat != REGISTERED_HOME && n_stat != REGISTERED_ROAMING) {
        delay(1000);
        n_stat = getRegistrationStatus();
    }
    auto cpin_status = sendCommand(AT_GET_CPIN);
    if (!cpin_status || !has_OK(cpin_status.get()) || !strstr(cpin_status.get(), AT_RSP_CPIN_READY)) {
        if (is_debug) {
            Serial.print(cpin_status.get());
        }
        return false;
    }
    return true;
}

std::unique_ptr<char[]> Sim800L::sendCommand(const char cmd[], uint32_t timeOut_ms) {
    const size_t buff_size = 129;
    char buff[buff_size];
    size_t char_stored = 0;
    stream->println(cmd);
    stream->flush();
    bool was_response;
    while ((char_stored < 2 || (buff[char_stored - 2] != '\r' && buff[char_stored - 1] != '\n')) &&
           (was_response = waitForStream(timeOut_ms))) {
        size_t bytes_read = stream->readBytes(buff + char_stored, buff_size - char_stored - 1);
        char_stored += bytes_read;
        buff[char_stored] = '\0';
    }
    if (was_response) {
        char *new_line_ptr = strchr(buff, '\n');
        *new_line_ptr = '\0';
        if (strncmp(buff, cmd, strlen(cmd)) == 0) {
            if (is_debug) {
                Serial.print("Command was: ");
                Serial.println(buff);
                Serial.print("Response is: ");
                Serial.print(new_line_ptr + 1);
            }
            auto new_str = std::unique_ptr<char[]>(new char[strlen(new_line_ptr + 1) + 1]);
            strcpy(new_str.get(), new_line_ptr + 1);
            return new_str;
        } else {
            if (is_debug) {
                Serial.println("=======");
                Serial.println("Command was not delivered!");
                Serial.print("Command was: ");
                Serial.println(cmd);
                Serial.println("=======");
            }
        }
    } else {
        if (is_debug) {
            Serial.println("=======");
            Serial.println("Response Timeout!");
            Serial.print("On command: ");
            Serial.println(cmd);
            Serial.println("=======");
        }
    }
    return nullptr;
}

byte Sim800L::waitForStream(uint32_t timeOut_ms) {
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

int Sim800L::getSignal() {
    auto response = sendCommand(AT_GET_REG);
    if (response && has_OK(response.get()) && strncmp(response.get(), "+CSQ: ", 6) == 0) {
        int signal = 0;
        if (response[7] != ',') {
            signal += 10 * (response[6] - '0');
            signal += response[7] - '0';
        } else {
            signal += response[6] - '0';
        }
        if (is_debug) {
            Serial.print("Signal is: ");
            Serial.println(signal);
        }
        if (signal <= 31) {
            return signal;
        } else {
            return -1;
        }
    }
    return 0;
}

NetworkRegistration Sim800L::getRegistrationStatus() {
    auto response = sendCommand("AT+CREG?");
    if (response && strncmp(response.get(), "+CREG: ", 7) == 0) {
        char val = response[9];
        switch (val) {
            case '0' :
                return NOT_REGISTERED;
            case '1' :
                return REGISTERED_HOME;
            case '2' :
                return SEARCHING;
            case '3' :
                return DENIED;
            case '5' :
                return REGISTERED_ROAMING;
            default  :
                return NET_UNKNOWN;
        }
    }
    return NET_ERROR;
}

bool Sim800L::has_OK(char response[]) {
    return strstr(response, AT_RSP_OK);
}

bool Sim800L::setSSL() {
    auto response = sendCommand(AT_SET_SSL);
    return response && has_OK(response.get());
}

bool Sim800L::send_file() {
    auto init_tcp_response = sendCommand(R"(AT+CIPSTART="TCP","142.93.111.54",443)");
    waitForStream();

    delay(5000);
    auto stop_tcp_response = sendCommand("AT+CIPSHUT");

    return true;
}
