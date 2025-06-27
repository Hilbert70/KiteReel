#include "ConfigFile/configfile.h"
#include "Esplog/esplog.h"
#include "IBT4/IBT4.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <INA226.h>
#include <SPIFFS.h>
#include <Wire.h>

#include "resources/arrow-ccw.h"
#include "resources/arrow-cw.h"
#include "resources/kitereel-logo.h"
#include "resources/stop-icon.h"

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 32    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
IBT4 winch(0, 1);
INA226 ina(0x40);

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
            logger.log(LOG_INFO, "STOP");
            // stop
            display.fillRect(0, 0, 32, 32, SSD1306_BLACK);
            display.drawBitmap(0, 0, image_data_stopicon, 32, 32, SSD1306_WHITE);
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
    logger.vlogf(LOG_INFO, "kitereel %s", AUTO_VERSION);
    logger.log(LOG_INFO, "Started setup.");

    ConfigFile config = ConfigFile();

    logger.vlogf(LOG_FATAL, "loglevel %d", config.getLoglevel());
    logger.setLoglevel(config.getLoglevel());

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
    display.drawBitmap(0, 0, image_data_kitereel, 128, 32, SSD1306_WHITE);
    display.display();
    delay(2000);

    if (!config.getFastBoot()) {
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(5, 0);
        display.println(F("Version"));
        display.setCursor(5, 16);
        display.print(AUTO_VERSION);
        display.display();
        delay(2000);

        // display some config settings
        display.clearDisplay();
        display.setCursor(10, 0);
        display.print(F("Loglvl "));
        display.print(config.getLoglevel());
        display.setCursor(10, 16);
        display.print(F("pwrs: "));
        display.println(inaRetcode == 0 ? "ok" : "nok");
        display.display(); // Show initial text
        delay(2000);
    }
    display.clearDisplay();
    display.drawBitmap(0, 0, image_data_stopicon, 32, 32, SSD1306_WHITE);
    display.display();
    delay(2000);
    delay(100);
}

void loop()
{
    const uint8_t rampStep = 20;   // determine if this is fast enough
    const long rampTimeStep = 100; // just to start somewhere
    static long rampTimer = millis();
    static long displayTimer = millis();
    static int rampValue = 0;  // motor is initially stopped
    static int rampTarget = 0; // motor is initially stopped
    static bool rampStarted = false;

    if (rotaryEncoder.encoderChanged()) {
        long value = rotaryEncoder.readEncoder();
        logger.vlogf(LOG_INFO, "Value: %d", value);

        if (value > 0) {
            // ccw
            // display.clearDisplay();
            display.fillRect(0, 0, 32, 32, SSD1306_BLACK);
            display.drawBitmap(0, 0, image_data_arrowccw, 32, 32, SSD1306_WHITE);
            display.display();
        } else if (value < 0) {
            // cw
            // display.clearDisplay();
            display.fillRect(0, 0, 32, 32, SSD1306_BLACK);
            display.drawBitmap(0, 0, image_data_arrowcw, 32, 32, SSD1306_WHITE);
            display.display();
        } else {
            // stop
            // display.clearDisplay();
            display.fillRect(0, 0, 32, 32, SSD1306_BLACK);
            display.drawBitmap(0, 0, image_data_stopicon, 32, 32, SSD1306_WHITE);
            display.display();
        }

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
    // stop if the motor current is larger than 0.8 A (0.1R), change when R = 0.02
    if (ina.getCurrent() > 0.8) {
        rampValue = 0;
        rampTarget = 0;
        rampStarted = true;
        rotaryEncoder.setEncoderValue(0);
        display.fillRect(0, 0, 32, 32, SSD1306_BLACK);
        display.drawBitmap(0, 0, image_data_stopicon, 32, 32, SSD1306_WHITE);
        display.display();
    }
    // stop if vbus is less that 7.6 volt
    if (ina.getBusVoltage() < 7.6) {
        rampValue = 0;
        rampTarget = 0;
        rampStarted = true;
        rotaryEncoder.setEncoderValue(0);
        display.fillRect(0, 0, 32, 32, SSD1306_BLACK);
        display.drawBitmap(0, 0, image_data_stopicon, 32, 32, SSD1306_WHITE);
        display.display();
    }

    if (displayTimer + 1000 < millis()) {
        // update display
        display.fillRect(32, 0, 128 - 32, 32, SSD1306_BLACK);
        display.setCursor(37, 0);
        float vbat = ina.getBusVoltage();
        if (vbat < 7.6) {
            static int count = 0;
            if (count < 1) {
                display.setTextColor(SSD1306_WHITE);
            } else {
                display.setTextColor(SSD1306_BLACK);
                if (count >= 1)
                    count = -1;
            }
            count++;
            display.setCursor(37, 0);
            display.print("BATTERY");
            display.setTextColor(SSD1306_WHITE);
            display.setCursor(37, 0);
            display.setCursor(37, 16);
            display.print("VB ");
            display.println(vbat);
        } else {
            display.print("VB ");
            display.println(vbat);
            display.setCursor(37, 16);
            display.print("I  ");
            display.print(ina.getCurrent());
        }
        display.display();
        displayTimer = millis();
    }
}