#include "IBT4/IBT4.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <INA226.h>
#include <Wire.h>

/*
 - [x] IGB4 driver
 - [x] i2c display SSD1306
 - [ ] rotary switch
 - [ ] voltage/current measurement
 - [ ] wifi, also AP mode
 - [ ] make it all work together
*/

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

IBT4 winch(0, 1);

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(2, 3, 4, -1, 4);
void IRAM_ATTR readEncoderISR()
{
    rotaryEncoder.readEncoder_ISR();
}

void setup()
{
    winch.begin();
    Wire.begin(6, 7);

    Serial.begin(115200);

    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    rotaryEncoder.setBoundaries(0, 10, true); // minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotaryEncoder.disableAcceleration();      // acceleration is now enabled by default - disable if you dont need it

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    display.clearDisplay();

    display.setTextSize(2); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.println(F("Test text"));
    display.setCursor(10, 15);
    display.println(F("Line 2"));
    display.display(); // Show initial text
    delay(100);
}

void loop()
{
    /*
     * winch working example
    winch.setIn1High(255);
    delay(1000);

    winch.stop();
    delay(1000);
    winch.setIn2High(100);

    delay(1000);
    winch.stop();
    delay(1000);
    */
}