#pragma once

#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Arduino.h>
#include <string>

class Display {
public:
    Display() {
        display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
        if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            Serial.println(F("SSD1306 allocation failed"));
            while (true);
        }

        delay(2000);
        clear();
    }

    void clear() {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 10);
    }

    void println(const std::string& str) {
        if (display.getCursorY() > SCREEN_HEIGHT-8) {
            clear();
        }

        display.println(str.c_str());
        display.display();
    }

    void printf(const char* format, ...) {
        if (display.getCursorY() > SCREEN_HEIGHT-8) {
            clear();
        }

        display.printf(format);
    }

    void print(const std::string& str) {
        if (display.getCursorY() > SCREEN_HEIGHT-8) {
            clear();
        }

        display.print(str.c_str());
        display.display();
    }

private:
    Adafruit_SSD1306 display;
    static constexpr int SCREEN_WIDTH = 128;
    static constexpr int SCREEN_HEIGHT = 64;
};