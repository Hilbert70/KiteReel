#include "Esplog/esplog.h"
#include "IBT4/IBT4.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <INA226.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <Wire.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
IBT4 winch(0, 1);
INA226 ina(0x40);
Preferences preferences;

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(2, 3, 4, -1, 4);
void IRAM_ATTR readEncoderISR()
{
    rotaryEncoder.readEncoder_ISR();
}

bool handle_rotary_button()
{
    static bool wasButtonDown = false;
    bool isEncoderButtonDown = rotaryEncoder.isEncoderButtonDown();

    if (isEncoderButtonDown) {
        if (!wasButtonDown) {
            // update display
            Serial.println("STOP");
            display.clearDisplay();
            display.setCursor(10, 0);
            display.print("STOP");
            display.display();
            delay(100);
        }
        wasButtonDown = true;
        return true;
    }
    wasButtonDown = false;
    return false;
}

void setup()
{
    delay(1000); // Delay for USB CDC initialization

    uint32_t chipID = 0;
    for (int i = 0; i < 17; i = i + 8) {
        chipID |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    winch.begin();
    Wire.begin(6, 7);

    Serial.begin(115200);
    logger.setup(true, LOG_INFO); // for nor LOG
    logger.log(LOG_INFO, "Started setup.");
    preferences.begin("KiteReel", false);

    if (!SPIFFS.begin(true)) {
        logger.log(LOG_ERROR, "An Error has occurred while mounting SPIFFS");
        return;
    }
    loglevel nvsloglevel = (loglevel)preferences.getInt("loglevel", LOG_DEBUG); // for now
    logger.setLoglevel(nvsloglevel);

    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    rotaryEncoder.setBoundaries(-1, 1, false); // minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotaryEncoder.disableAcceleration();       // acceleration is now enabled by default - disable if you dont need it

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        logger.log(LOG_FATAL, "SSD1306 allocation failed");
        for (;;)
            ; // Don't proceed, loop forever
    }

    if (!ina.begin()) {
        logger.log(LOG_ERROR, "Could not connect to INA226!");
    }

    ina.setAverage(2);
    delay(100);

    int inaRetcode = ina.setMaxCurrentShunt(0.8, 0.1);

    display.clearDisplay();

    display.setTextSize(2); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.println(F("Test text"));
    display.setCursor(10, 15);
    display.print(F("i: "));
    display.println(inaRetcode);
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
    const uint8_t rampStep = 20;   // determine if this is fast enough
    const long rampTimeStep = 100; // just to start somewhere
    static long rampTimer = millis();
    static int rampValue = 0;  // motor is initially stopped
    static int rampTarget = 0; // motor is initially stopped
    static bool rampStarted = false;

    if (rotaryEncoder.encoderChanged()) {
        long value = rotaryEncoder.readEncoder();
        logger.vlogf(LOG_INFO, "Value: %d", value);

        display.clearDisplay();
        display.setCursor(10, 0);
        display.print("Value ");
        display.println(value);
        display.display();

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
    }

    // start doing stuff with the motor
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
            } else if (rampValue > 0) {
                rampValue -= rampStep;
            }
            // else rampValue = 0
        }
        // check the stop
        if ((rampValue < rampStep - 5) &&
            (rampValue > -rampStep + 5)) {
            rampValue = 0;
            if (rampTarget == 0) {
                rampStarted = false;
            }
        }
        logger.vlogf(LOG_DEBUG, "rampValue: %d rampTarget %d", rampValue, rampTarget);
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

    // see if the button is pressed
    if (handle_rotary_button()) {
        // we had a stop
        rampValue = 0;
        rampTarget = 0;
        rampStarted = true;
        rotaryEncoder.setEncoderValue(0);
    }

    // do current and voltage measurement
    if (ina.getCurrent() > 0.8) {
        rampValue = 0;
        rampTarget = 0;
        rampStarted = true;
        rotaryEncoder.setEncoderValue(0);
    }
}