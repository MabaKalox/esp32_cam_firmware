//
// Created by maba_kalox on 4/30/21.
//

#include "Sim800L.h"

#define AT_CMD_BASE "AT"
#define AT_CMD_START_GPRS "AT+CIICR"
#define AT_CMD_OBTAIN_IP "AT+CIFSR"
#define AT_GET_CPIN "AT+CPIN?"
#define AT_SET_APN "AT+CSTT="
#define AT_GET_SIGNAL "AT+CSQ"
#define AT_GET_REG "AT+CREG?"
#define AT_SET_SSL "AT+CIPSSL=1"

#define AT_RSP_OK "OK"
#define AT_RSP_CPIN_READY "+CPIN: READY"
#define AT_RSP_CONNECT_OK "CONNECT"

#define HEADERS1 "POST /images/upload_image/?auto_name=true HTTP/1.1\r\nHost: 142.93.111.54\r\nContent-Length: "
#define HEADERS2 "\r\nContent-Type: multipart/form-data; boundary=---------------------------9755227392164441747325250978\r\n\r\n"
#define BODY_HEAD "-----------------------------9755227392164441747325250978\r\nContent-Disposition: form-data; name=\"file\"; filename=\"1x1.png\"\r\nContent-Type: image/jpeg\r\n\r\n"
#define BODY_TAIL "\r\n-----------------------------9755227392164441747325250978--\r\n\r\n"

Sim800L::Sim800L(Stream *_stream, void (*_rst_handler)(), bool _is_debug) :
        is_debug(_is_debug), stream(_stream), rst_handler(_rst_handler) {}

bool Sim800L::initGPRS(const char APN_data[]) {
    auto response = sendCommand("AT+CIPMODE=1");
    if (!response) {
        return false;
    }
    response.reset();

    response = sendCommand("AT+CIPCCFG=8,1,1024,1,0,1460,50");
    if (!response || !has_OK(response.get())) {
        Serial.println(response.get());
        return false;
    }

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
//    rst_handler();
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
    stream->println(cmd);
    stream->flush();

    auto serial_response = readResponse(timeOut_ms);
    if (serial_response) {
        char *new_line_ptr = strchr(serial_response.get(), '\n');
        *new_line_ptr = '\0';
        if (strncmp(serial_response.get(), cmd, strlen(cmd)) == 0) {
            if (is_debug) {
                Serial.print("Command was: ");
                Serial.println(serial_response.get());
                Serial.print("Response is: ");
                Serial.print(new_line_ptr + 1);
            }
            auto new_str = std::unique_ptr<char[]>(new char[strlen(new_line_ptr + 1) + 1]);
            strcpy(new_str.get(), new_line_ptr + 1);
            return new_str;
        } else {
            if (is_debug) {
                Serial.println("=======");
                Serial.println("Not delivered!");
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
    auto response = sendCommand(AT_GET_SIGNAL);
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
    auto response = sendCommand(AT_GET_REG);
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

bool Sim800L::send_file(const uint8_t* file_buff, size_t file_buff_len) {
    auto init_tcp_response = sendCommand(R"(AT+CIPSTART="TCP","142.93.111.54",443)");
    auto is_connected = readResponse();
    if (!is_connected || !strstr(is_connected.get(), AT_RSP_CONNECT_OK)) {
        if (is_debug) Serial.print(is_connected.get());
        return false;
    }

    char body_ln[11];
    sprintf(body_ln, "%u", strlen(BODY_HEAD) + file_buff_len + strlen(BODY_TAIL));
    auto headers = concatStr(HEADERS1, body_ln, HEADERS2);
    stream->write(headers.get());

    stream->write(BODY_HEAD);
    stream->flush();
    delay(120);

    const uint8_t *file_ptr = file_buff;
    for (size_t n = 0; n < file_buff_len; n = n + 1024) {
        if (n + 1024 < file_buff_len) {
            stream->write(file_ptr, 1024);
            stream->flush();
            delay(120);
            file_ptr += 1024;
        } else if (file_buff_len % 1024 > 0) {
            size_t remainder = file_buff_len % 1024;
            stream->write(file_ptr, remainder);
            stream->flush();
            delay(120);
        }
    }

    stream->write(BODY_TAIL);
    stream->flush();

    delay(2000);
    stream->write("+++");
    delay(1200);
    Serial.println("Upload DONE");

//    auto stop_tcp_response = sendCommand("AT+CIPSHUT");
//    if (!stop_tcp_response || !strstr(stop_tcp_response.get(), "SHUT OK")) {
//        if (is_debug) Serial.print(stop_tcp_response.get());
//        return false;
//    }
    return true;
}

std::unique_ptr<char[]> Sim800L::readResponse(const uint32_t &timeOut_ms) {
    size_t char_stored = 0;
    std::unique_ptr<char[]> response = nullptr;
    byte available;
    while ((char_stored < 2 || (response[char_stored - 2] != '\r' && response[char_stored - 1] != '\n')) &&
           (available = waitForStream(timeOut_ms))) {
        char *ptr = response.release();
        ptr = (char *) realloc(ptr, char_stored + available + 1);
        response = std::unique_ptr<char[]>(ptr);

        stream->readBytes(response.get() + char_stored, available);
        char_stored += available;
        response[char_stored] = '\0';
    }
    // check was while exit due to right line ending or because of timeout for new chars
    if (available) {
        return response;
    } else {
        return nullptr;
    }
}
