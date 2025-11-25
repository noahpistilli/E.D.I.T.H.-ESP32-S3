#include <Arduino.h>
#include "photomath.h"
#include <Display.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <camera.h>

Display *display;
// Almost every pin on the ESP is being used, this is one of the few that isn't.
constexpr int MIC_PIN = 3;
// 16 kHz
constexpr int SAMPLE_RATE = 16000;

// We can't record for too long as the ESP memory isn't the largest.
// Large enough for simple, concise voice commands, however.
constexpr int RECORD_S = 2;
constexpr int TOTAL_SAMPLES = SAMPLE_RATE * RECORD_S;
hw_timer_t* timer = nullptr;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile bool sample_flag = false;
volatile int16_t audio_buffer[TOTAL_SAMPLES];
volatile int sample_index = 0;

// Interrupt timer
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

    // Allows for 4096 as the max value. Higher can corrupt the packets and introduce latency.
    analogReadResolution(12);

    // Init 16kHz timer
    // 80MHz / 80 = 1us tick
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000 / SAMPLE_RATE, true);
}

void loop() {
    // NOTE: I attempted to train a machine learning model however the samples required to make it accurate
    // took up too much space on the ESP, making it an unviable option. Due to this I opted into detecting a high audio
    // spike, then processing commands based on that. The command is then sent to a proxy server where Google Cloud's
    // Speech to Text API is utilized. Example of what I attempted with the machine learning model is in the final
    // report.
    int v = analogRead(MIC_PIN);
    if (v > 2500) {
        delay(1000);
        display->println("Listening...");

        // Clear audio buffer
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

        // Send voice prompt to proxy server
        HTTPClient http;
        http.begin("http://192.168.50.232/audio");
        http.addHeader("Content-Type", "application/octet-stream");

        // Send raw PCM buffer
        int httpCode = http.POST((uint8_t*)audio_buffer, sizeof(audio_buffer));

        if (httpCode <= 0) {
            display->printf("POST failed: %s\n", http.errorToString(httpCode).c_str());
            display->clear();
            delay(3000);
            display->print("");
            return;
        }

        const String resp = http.getString();
        http.end();

        // Equation solver keywords, send off to proxy server for Photomath.
        if (resp == "solve equation") {
            display->clear();
            display->println("Solving equation...");
            auto photo = Photomath();
            bool success = photo.createRequest();
            if (!success) {
                display->println("Failed to solve equation.");
                display->clear();
                delay(3000);
                display->print("");
                return;
            }

            display->print(photo.parseEquation());
            display->print(" = ");
            display->println(photo.parseSolution());
        }

        // 10-second delay is enough for a person to read the answer.
        delay(10000);
    }
}