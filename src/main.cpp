#include <Arduino.h>
#include <HardwareSerial.h>
#include <Sim800L/Sim800L.h>
#include <esp_camera.h>
#include "camera/init_camera.h"
#include <soc/soc.h>           // Disable brownour problems
#include <soc/rtc_cntl_reg.h>  // Disable brownour problems

#define WAKE_UP_PERIOD_S 20
#define WAKE_UP_PIN GPIO_NUM_12

String Arsp, Grsp;
//SoftwareSerial SIM800L_Serial(15, 13);
HardwareSerial SIM800L_Serial(2);
Sim800L *gsmM = nullptr;
#define GSM_RESET_PIN 14

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

void make_and_upload_photo() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) {
        bool was_uploaded = gsmM->send_file(fb->buf, fb->len);;
        if (was_uploaded) {
            Serial.println("Uploaded");
        } else {
            Serial.println("Upload failed");
        }
    } else {
        Serial.println("Camera capture failed");
    }
    esp_camera_fb_return(fb);
}

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    Serial.setTimeout(10);
    Serial.println("Waking Up");

    pinMode(GSM_RESET_PIN, OUTPUT);
    digitalWrite(GSM_RESET_PIN, HIGH);

    timer = timerBegin(2, 180, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000, true);
    timerAlarmDisable(timer);

    SIM800L_Serial.begin(9600, SERIAL_8N1, 15, 13);
    SIM800L_Serial.setTimeout(30);
    gsmM = new Sim800L(&SIM800L_Serial, restart_gsm, true);
    restart_gsm();
    gsmM->initBASE();
    gsmM->initGPRS(R"("internet.lmt.lv","","")");
    gsmM->setSSL();

    pinMode(WAKE_UP_PIN, INPUT_PULLUP);
    esp_sleep_enable_ext0_wakeup(WAKE_UP_PIN, 0);
    if (init_camera(5000)) {
        make_and_upload_photo();
    }
    Serial.println("Setup complete");
//    esp_deep_sleep(WAKE_UP_PERIOD_S * 1000000);
}

void loop() {
    if (Serial.available()) {
        SIM800L_Serial.write(Serial.read());
    }

    if (SIM800L_Serial.available()) {
        Serial.write(SIM800L_Serial.read());
    }
}