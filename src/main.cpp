#include <Arduino.h>
#include "SoftwareSerial.h"
#include <EncButton/EncButton.h>
#include <Sim800L/Sim800L.h>

String Arsp, Grsp;
SoftwareSerial SIM800L_Serial(16, 17);
Sim800L *gsmM = nullptr;
#define GSM_RESET_PIN 18

EncButton<EB_CALLBACK, 22> butt;
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void restart_gsm() {
    Serial.println("Restarting...");
    digitalWrite(GSM_RESET_PIN, LOW);
    timerRestart(timer);
    timerAlarmEnable(timer);
}

void IRAM_ATTR onTimer() {
    portENTER_CRITICAL_ISR(&timerMux);
    digitalWrite(GSM_RESET_PIN, HIGH);
    timerAlarmDisable(timer);
    portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
    Serial.begin(115200);
    Serial.setTimeout(5);

    pinMode(GSM_RESET_PIN, OUTPUT);
    digitalWrite(GSM_RESET_PIN, HIGH);

    timer = timerBegin(0, 180, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000, true);
    timerAlarmDisable(timer);

    SIM800L_Serial.begin(38400);
    SIM800L_Serial.setTimeout(5);
    gsmM = new Sim800L(&SIM800L_Serial, restart_gsm, true);
    gsmM->initBASE();
    gsmM->initGPRS(R"("internet.lmt.lv","","")");
    gsmM->setSSL();
    gsmM->send_file();

    butt.pullUp();
    butt.attach(HOLDED_HANDLER, restart_gsm);
}

unsigned long last_check;

void loop() {
    butt.tick();

    if (Serial.available()) {
        SIM800L_Serial.write(Serial.read());
    }

    if (SIM800L_Serial.available()) {
        Serial.write(SIM800L_Serial.read());
    }
}