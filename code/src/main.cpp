#include "ConfigFile/configfile.h"
#include "Esplog/esplog.h"
#include "IBT4/IBT4.h"
#include "MotorDirection/motordirection.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AiEsp32RotaryEncoder.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <INA226.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <Wire.h>

#include <NimBLEDevice.h>

// include OLED icons
#include "resources/arrow-ccw.h"
#include "resources/arrow-cw.h"
#include "resources/ble-connected.h"
#include "resources/ble-disconnected.h"
#include "resources/kitereel-logo.h"
#include "resources/stop-icon.h"

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
IBT4 winch(0, 1);
INA226 ina(0x40);

ConfigFile config;
motordirection BLEdirection = motordirection();

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(2, 3, 4, -1, 4);

NimBLEServer *pServer = NULL;
NimBLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool haveWiFi = false;

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Create classes, objects, structures
class MyServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        deviceConnected = true;
    };

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        deviceConnected = false;
    }
};

struct BLEMessage {
    String id = "";
    String value = "";
    int example = 0;
};

/*
 * the code is based on using the MicroBlue app
 */
class MyCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        std::string message = pCharacteristic->getValue();

        BLEMessage msg;
        int counter = 0;
        String alphanumeric = "abcdefghijklmnopqrstuvwxyz,1234567890";

        for (int i = 0; i < message.length(); i++) {
            if (alphanumeric.indexOf(message[i]) == -1) {
                counter += 1;
            }

            if (counter == 1 && i > 0) {
                msg.id.concat(message[i]);
            } else if (counter == 2 && i > msg.id.length() + 1) {
                msg.value.concat(message[i]);
            } else if (msg.id.length() > 0 && msg.value.length() > 0) {
                logger.vlogf(LOG_DEBUG, "id: %s value: %s", msg.id, msg.value);
            }
        }

        if (msg.id == "stop") {
            logger.vlogf(LOG_DEBUG, " -> stop = %d", msg.value.toInt());
            if (msg.value.toInt() == 1) {
                /* feedback to the app is not (hopefully yet) working
                std::string notify_stop = "␁rotate␂50␃";
                pTxCharacteristic->setValue(notify_stop);
                pTxCharacteristic->notify();
                */
                BLEdirection.setValue(0, true);
            }
        }
        if (msg.id == "rotate") {
            int value = msg.value.toInt();
            if (value <= 25) {
                BLEdirection.setValue(1);
            } else if (value >= 75) {
                BLEdirection.setValue(-1);
            } else {
                BLEdirection.setValue(0);
            }
            logger.vlogf(LOG_DEBUG, " -> rotate = (%d) %d", BLEdirection.getValue(false), value);
        }
        // joystick or d-pad, wont do (yet)
        /* if (msg.id == "d0") {
           }
         */
    }
};

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
            logger.log(LOG_DEBUG, "Button pressed");
            // stop
            delay(100);
        }
        wasButtonDown = true;
        return true;
    }
    wasButtonDown = false;
    return false;
}

void displayFirmwareMessage(const char *message)
{
    display.fillRect(0, 0, 24, 128, SSD1306_BLACK);
    display.setCursor(5, 5);
    display.print(message);
    display.display();
}

void startFWUpdate(int size, const char *type)
{
    logger.vlogf(LOG_INFO, "Updating %s...", type);
    display.clearDisplay();
    displayFirmwareMessage(" Update ");
}

void displayUpdateFWUpdate(int size, int currentSize)
{
    Serial.print(".");
    display.drawRect(5, 32, 128 - 5, 30, SSD1306_WHITE);
    display.fillRect(5, 32, ((128 - 5) * currentSize) / size, 30, SSD1306_WHITE);
    display.display();
}

/*
 *          | |
 *  ___  ___| |_ _   _ _ __
 * / __|/ _ \ __| | | | '_ \
 * \__ \  __/ |_| |_| | |_) |
 * |___/\___|\__|\__,_| .__/
 *                    | |
 *                    |_|
 */
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

    config = ConfigFile();

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

    // int inaRetcode = ina.setMaxCurrentShunt(0.8, 0.1);
    int inaRetcode = ina.setMaxCurrentShunt(3.0, 0.02);

    display.clearDisplay();
    display.drawBitmap(0, 0, image_data_kitereel, 128, 32, SSD1306_WHITE);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 32);
    display.println(F("Version"));
    display.setCursor(5, 48);
    display.print(AUTO_VERSION);
    display.display();
    delay(2000);
    logger.vlogf(LOG_INFO, "As \"%s\" connecting to wifi: \"%s\"?", config.getHostname().c_str(),config.getWifiSSID().c_str());
    if (rotaryEncoder.isEncoderButtonDown() && config.getWifiSSID() != "") {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        logger.vlogf(LOG_INFO, "Connecting to wifi: %s.", config.getWifiSSID().c_str());
        display.print("Connect to WiFi ");
        // otherwise the default esp generated hostname
        if (config.getHostname() != "")
            WiFi.setHostname(config.getHostname().c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(config.getWifiSSID().c_str(), config.getWifiPassword().c_str());
        while (WiFi.status() != WL_CONNECTED) {
            Serial.print('.');
            display.print(".");
            delay(1000);
        }
        logger.vlogf(LOG_INFO, "Connected to wifi (%d).", (WiFi.status() == WL_CONNECTED));
        display.println("WIFI.");
        haveWiFi = WiFi.status() == WL_CONNECTED;

        ArduinoOTA.onStart([]() {
            Serial.println("Start");
            const char *type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "firmware";
            else { // U_SPIFFS
                type = "filesystem";
                SPIFFS.end();
            }
            startFWUpdate(0, type);
        });

        if (config.getOTAPassword() != "") {
            ArduinoOTA.setPassword(config.getOTAPassword().c_str());
        }

        ArduinoOTA.onEnd([]() {
            Serial.println("\nSuccess!");
            displayFirmwareMessage("Success!");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            displayUpdateFWUpdate(total, progress);
        });

        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) {
                Serial.println("Auth Failed");
                displayFirmwareMessage("Auth Failed!");
            } else if (error == OTA_BEGIN_ERROR) {
                Serial.println("Begin Failed");
                displayFirmwareMessage("Begin Failed!");
            } else if (error == OTA_CONNECT_ERROR) {
                Serial.println("Connect Failed");
                displayFirmwareMessage("Connect Failed!");
            } else if (error == OTA_RECEIVE_ERROR) {
                Serial.println("Receive Failed");
                displayFirmwareMessage("Receive Failed!");
            } else if (error == OTA_END_ERROR) {
                Serial.println("End Failed");
                displayFirmwareMessage("End Failed!");
            }
            delay(1000); // Wait a second
            display.clearDisplay();
        });
        ArduinoOTA.begin();
    }

    if (!config.getFastBoot()) {
        // display some config settings
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print(F("Loglvl "));
        display.println(config.getLoglevel());
        display.print(F("pwrs: "));
        display.println(inaRetcode == 0 ? "ok" : "nok");
        display.print(F("MaxI "));
        display.println(config.getMaxCurrent());
        display.print(F("minV: "));
        display.println(config.getMinVoltage());
        display.print(F("BLE: "));
        if (config.getBluetooth()) {
            display.println(F("on"));
        } else {
            display.println(F("off"));
        }
        display.print("BLE name:");
        display.print("\"");
        display.print(config.getBLEName());
        display.println("\"");
        display.print("BLE pin:");
        display.println(config.getBLEPIN());

        display.display(); // Show initial text
        display.setTextSize(2);
        delay(4000);
    }

    if (config.getBluetooth()) {
        // Create the BLE Device

        NimBLEDevice::init(std::string(config.getBLEName().c_str()));
        // Create the BLE Server
        pServer = NimBLEDevice::createServer();
        pServer->setCallbacks(new MyServerCallbacks());

        // Create the BLE Service
        NimBLEService *pService = pServer->createService(SERVICE_UUID);

        // Create a BLE Characteristic
        pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);

        NimBLECharacteristic *pRxCharacteristic =
            pService->createCharacteristic(CHARACTERISTIC_UUID_RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN);

        pRxCharacteristic->setCallbacks(new MyCallbacks());

        NimBLEDevice::setSecurityAuth(true, true, true);
        NimBLEDevice::setSecurityPasskey(config.getBLEPIN());
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
        // Start the service
        pService->start();

        // Start advertising
        pServer->getAdvertising()->start();

        logger.log(LOG_INFO, "Waiting a client connection to notify...");
    }
    display.clearDisplay();
    display.drawBitmap(0, 0, image_data_stopicon, 32, 32, SSD1306_WHITE);
    if (config.getBluetooth()) {
        display.drawBitmap(104, 64 - 24, image_data_bledisconnected, 24, 24, SSD1306_WHITE);
    }
    display.display();
}

/*
 * | |
 * | | ___   ___  _ __
 * | |/ _ \ / _ \| '_ \
 * | | (_) | (_) | |_) |
 * |_|\___/ \___/| .__/
 *               | |
 *               |_|
 */
void loop()
{
    const uint8_t rampStep = 20;   // determine if this is fast enough
    const long rampTimeStep = 100; // just to start somewhere
    static long rampTimer = millis();
    static long displayTimer = millis() - 1000; // display immediate ;
    static int rampValue = 0;                   // motor is initially stopped
    static int rampTarget = 0;                  // motor is initially stopped
    static bool rampStarted = false;

    if (config.getBluetooth()) {
        // disconnecting
        if (!deviceConnected && oldDeviceConnected) {
            delay(500);                  // give the bluetooth stack the chance to get things ready
            pServer->startAdvertising(); // restart advertising
            logger.log(LOG_INFO, "start advertising");
            oldDeviceConnected = deviceConnected;
            display.fillRect(104, 64 - 24, 24, 24, SSD1306_BLACK);
            display.drawBitmap(104, 64 - 24, image_data_bledisconnected, 24, 24, SSD1306_WHITE);
            display.display();
        }
        // connecting
        if (deviceConnected && !oldDeviceConnected) {
            // do stuff here on connecting
            oldDeviceConnected = deviceConnected;
            display.fillRect(104, 64 - 24, 24, 24, SSD1306_BLACK);
            display.drawBitmap(104, 64 - 24, image_data_bleconnected, 24, 24, SSD1306_WHITE);
            display.display();
        }
    }
    // Handle rotary button
    bool rotChanged = rotaryEncoder.encoderChanged();
    if (rotChanged || BLEdirection.isChanged()) {
        long value;
        if (rotChanged) {
            value = rotaryEncoder.readEncoder();
        } else {
            value = BLEdirection.getValue();
        }
        logger.vlogf(LOG_DEBUG, "Value: %d", value);

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
        } else if (value < 0) {
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
        // logger.vlogf(LOG_DEBUG, "rampValue: %d rampTarget %d", rampValue, rampTarget);
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
    // stop if the motor current is larger than 0.8 A (0.1R), change when R = 0.02 to 2.0 A
    // stop if vbus is less that 7.6 volt
    if (handle_rotary_button() ||
        BLEdirection.isImmediateStop() ||
        ina.getCurrent() > config.getMaxCurrent() ||
        ina.getBusVoltage() < config.getMinVoltage()) {
        // we had a stop
        rampValue = 0;
        rampTarget = 0;
        rampStarted = true;
        rotaryEncoder.setEncoderValue(0);
        display.fillRect(0, 0, 32, 32, SSD1306_BLACK);
        display.drawBitmap(0, 0, image_data_stopicon, 32, 32, SSD1306_WHITE);
        display.display();
    }

    // Display battery voltage or flash "BATTERY" if the battery voltage is too low.
    if (displayTimer + 1000 < millis()) {
        // update display
        float vbat = ina.getBusVoltage();
        if (vbat < config.getMinVoltage()) {
            static int count = 0;
            display.fillRect(32, 0, 128 - 32, 32, SSD1306_BLACK);
            display.setCursor(37, 0);
            if (count % 2) {
                display.setTextColor(SSD1306_WHITE);
            } else {
                display.setTextColor(SSD1306_BLACK);
            }
            count = (++count) % 2;
            display.setCursor(37, 0);
            display.print("BATTERY");
            display.setCursor(37, 16);
            display.print("LOW");
            display.setTextColor(SSD1306_WHITE);
        }
        display.fillRect(0, 32, 96, 32, SSD1306_BLACK);
        display.setCursor(0, 32);
        display.print("V");
        display.setCursor(12, 40);
        display.setTextSize(1);
        display.print("BAT");
        display.setTextSize(2);
        display.setCursor(36, 32);
        display.println(vbat, 1);
        display.setCursor(0, 48);
        display.print("I");
        display.setCursor(12, 56);
        display.setTextSize(1);
        display.print("MOT");
        display.setTextSize(2);
        display.setCursor(36, 48);
        display.print(ina.getCurrent());

        display.display();
        displayTimer = millis();
    }
    if (haveWiFi)
        ArduinoOTA.handle();
}