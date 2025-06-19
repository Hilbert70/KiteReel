#include "IBT4/IBT4.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <INA226.h>
#include <Wire.h>

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

void handle_rotary_button()
{
    static bool wasButtonDown = false;

    bool isEncoderButtonDown = rotaryEncoder.isEncoderButtonDown();

    if (isEncoderButtonDown) {
        if (!wasButtonDown) {
            rotaryEncoder.setEncoderValue(0);
            winch.stop();

            // update display
            Serial.print("STOP");
            display.clearDisplay();
            display.setCursor(10, 0);
            display.print("STOP");
            display.display();
            delay(100);
        }
        wasButtonDown = true;
        return;
    }
    wasButtonDown = false;
}

void setup()
{
    winch.begin();
    Wire.begin(6, 7);

    Serial.begin(115200);

    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    rotaryEncoder.setBoundaries(-10, 10, false); // minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotaryEncoder.disableAcceleration();         // acceleration is now enabled by default - disable if you dont need it

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
    delay(5000);
    // do some config stuff, STA or AP mode, this can be done later, it is a nice to have
    display.clearDisplay();
    display.setCursor(10, 0);
    display.print("STOP");
    display.display();
    delay(100);
}

void loop()
{
    const uint8_t rampStep = 20;    // determine if this is fast enough
    const long rampTimeStep = 100; // just to start somewhere
    static long rampTimer = millis();
    static int rampValue = 0;  // motor is initially stopped
    static int rampTarget = 0; // motor is initially stopped
    static bool rampStarted = false;

    if (rotaryEncoder.encoderChanged()) {
        long value = rotaryEncoder.readEncoder();
        Serial.print("Value: ");
        Serial.println(value);
        display.clearDisplay();
        display.setCursor(10, 0);
        display.print("Value ");
        display.println(value);
        display.display();

        /*
         * lets assume we do not go from - straight to +, alway from + to 0 and
         * from 0 to - and vice versa
         * (maybe later ;-)
         */
        if (value == 0) {
            if (rampValue != 0) {
                // start ramp down
                rampTarget = 0;
                rampStarted = true;
                rampTimer = millis();
            }
        } else if (value > 0) {
            rampTarget = 255;
            rampStarted = true;
            rampTimer = millis();

            // do in1
        } else if (value < 0) {
            // do in2

            rampTarget = -255;
            rampStarted = true;
            rampTimer = millis();
        }
        // if encoder value is + -> ramp up in1
        // if encoder value is - -> ramp up in2
        // if encoder value = 0 ramp down to stop
    }
    if (rampStarted && rampTimer + rampTimeStep < millis()) {
        if (rampTarget == 255) {
            rampValue += rampStep;
            if (rampValue > 255) {
                rampValue = 255;
                rampStarted = false;
            }
        } else if (rampTarget == -255) {
            rampValue -= rampStep;
            if (rampValue < -255) {
                rampValue = -255;
                rampStarted = false;
            }
        } else if (rampTarget == 0) {
            if (rampValue < 0) {
                rampValue += rampStep;
            } else {
                rampValue -= rampStep;
            }
        }
        // check the stop
        if ((rampValue < rampStep - 5) &&
            (rampValue > -rampStep + 5)) {
            rampValue = 0;
            if (rampTarget == 0) {
                rampStarted = false;
            }
        }
        Serial.print("rampValue: ");
        Serial.print(rampValue);
        Serial.print(" rampTarget: ");
        Serial.println(rampTarget);
        if (rampValue == 0) {
            winch.stop();
        } else if (rampValue > 0) {
            // do in1 stuff
            winch.setIn1High(rampValue);
        } else {
            winch.setIn2High(-rampValue);
        }
        rampTimer = millis();
    }
    handle_rotary_button();
}