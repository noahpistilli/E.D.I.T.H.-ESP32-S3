#include <Arduino.h>
#include "photomath.h"
#include <Display.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <camera.h>

Display *display;
constexpr int MIC_PIN = 3;
constexpr int SAMPLE_RATE = 16000; // 16 kHz
constexpr int RECORD_S = 2;
constexpr int TOTAL_SAMPLES = SAMPLE_RATE * RECORD_S;
hw_timer_t* timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile bool sample_flag = false;
volatile int16_t audio_buffer[TOTAL_SAMPLES];
volatile int sample_index = 0;

void IRAM_ATTR onTimer() {
    sample_flag = true;
}


void initWiFi() {
    WiFi.begin("cui028", "cui028wifi");
    WiFi.setSleep(false);
    display->print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        display->print(".");
        delay(1000);
    }

    display->println("");
    display->println("Wifi connected");
}


void setup() {
    Wire.setPins(41, 42);
    Serial.begin(115200);

    display = new Display();
    delay(2000);
    display->println("E.D.I.T.H. starting...");
    initWiFi();
    init_camera();
    delay(2000);
    display->clear();

    analogReadResolution(12);

    // Init 16kHz timer
    timer = timerBegin(0, 80, true);  // 80MHz / 80 = 1us tick
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000 / SAMPLE_RATE, true);
}

void loop() {
    int v = analogRead(MIC_PIN);
    if (v > 2500) {
        delay(1000);
        display->println("Listening...");

        sample_index = 0;
        timerAlarmEnable(timer);

        // Capture audio for 2 seconds
        while (sample_index < TOTAL_SAMPLES) {
            if (sample_flag) {
                sample_flag = false;

                v = analogRead(MIC_PIN);
                // Bit-extend to 16 bits from 12
                int16_t pcm = static_cast<int16_t>((v - 2048) << 4);

                // Enter the interrupt then exit
                portENTER_CRITICAL(&timerMux);
                audio_buffer[sample_index] = pcm;
                sample_index++;
                portEXIT_CRITICAL(&timerMux);
            }
        }

        timerAlarmDisable(timer);
        display->println("Done.");
        HTTPClient http;
        http.begin("http://192.168.50.232/audio");
        http.addHeader("Content-Type", "application/octet-stream");

        // Send raw PCM buffer
        int httpCode = http.POST((uint8_t*)audio_buffer, sizeof(audio_buffer));

        if (httpCode <= 0) {
            display->printf("POST failed: %s\n", http.errorToString(httpCode).c_str());
        }

        const String resp = http.getString();
        http.end();

        // Equation solver keywords, send off to proxy server for Photomath.
        if (resp == "solve equation") {
            display->clear();
            display->println("Solving equation...");
            auto photo = Photomath();
            photo.createRequest();
            display->print(photo.parseEquation());
            display->print(" = ");
            display->println(photo.parseSolution());
        }
        delay(10000);
    }
}