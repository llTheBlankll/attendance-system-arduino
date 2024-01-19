#include <MFRC522.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <neotimer.h>
#include <faces.h>

#define RST_PIN 0
#define SS_PIN 5
#define BUTTON_PIN 32
#define BUZZER_PIN 15
#define OLED_RST (-1)

#define ssid "PLDTHOMEFIBR9u7w4"
#define pass "PLDTWIFIkr39h"
#define WS_URL "ws://roundhouse.proxy.rlwy.net:42552/websocket/scanner"
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define BITMAP_HEIGHT 128
#define BITMAP_WIDTH 32

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
LiquidCrystal_I2C lcd(0x27, 16, 2);
websockets::WebsocketsClient wsClient;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
TaskHandle_t displayTask;
Neotimer buzzTimer(1000);
Neotimer faceTimer(200);
Neotimer checkWSocket(100);

byte firstHash[18];
byte secondHash[18];
byte hash[34];
byte blockSize = sizeof(firstHash);
byte firstHashSize = sizeof(firstHash);
byte secondHashSize = sizeof(secondHash);

int lastMode = LOW;
int buttonMode = 0;
int faceMode = 0;
int frame = 0;
int frameStop = 2;
int frameCount = 0;

bool buttonState = false;
bool hasResponse = false;
bool hasSent = false;

String currentText = "Need restart";
String mode = "in";
// enum Faces {IDLE, WELCOME, LATE, INVALID}
constexpr int allArray_LEN[4] = {16, 20, 14, 13};


void changeLcdText(const String &value) {
    if (currentText != value) {
        lcd.clear();
        lcd.print(currentText);
        currentText = value;
    }
}

String byteArrayToString(const byte *array, const byte size) {
    String str = "";
    for (int i = 0; i < size; i++) {
        str += String(array[i], HEX);
    }

    return str;
}

String hexToText(const String &hexString) {
    String result = "";
    const unsigned int strLen = hexString.length();
    for (int i = 0; i < strLen; i += 2) {
        String hexByte = hexString.substring(i, i + 2);
        const char charByte = static_cast<char>(strtol(hexByte.c_str(), nullptr, 16));
        result += charByte;
    }

    return result;
}

String concatenateBytes(const byte *value, const byte size) {
    return hexToText(byteArrayToString(value, size));
}

void ws_message_callback(const websockets::WebsocketsMessage &message) {
    DynamicJsonDocument response(2048);
    deserializeJson(response, message.data());
    const String messageResponse = response["message"];

    switch (messageResponse) {
        case "You are LATE":
            faceMode = 2;
            break;

        case "You are ONTIME":
            faceMode = 1;
            break;

        case "Invalid":
            faceMode = 3;
            break;

        case "Already Scanned!":
            faceMode = 1;
            break;

        case "Already arrived!":
            faceMode = 1;
            break;

        default:
            faceMode = 1;
    }

    changeLcdText(messageResponse);
    hasResponse = true;
    hasSent = false;
}


void displayFace() {
    if (faceTimer.repeat()) {
        if (frameCount == frameStop && faceMode != 0) {
            changeLcdText("Ready!");
            faceMode = 0;
            frameCount = 0;
        }

        if (frame == allArray_LEN[faceMode] - 1) {
            frame = 0;
            if (faceMode != 0) {
                frameCount += 1;
            }
        }

        display.clearDisplay();
        switch (faceMode) {
            case 0:
                if (frame >= sizeof(idle_face_allArray) / 4) {
                    frame = 0;
                } else {
                    display.drawBitmap(0, 0, idle_face_allArray[frame], 128, 32, WHITE);
                }
                break;
            case 1:
                if (frame >= sizeof(welcome_face_allArray) / 4) {
                    frame = 0;
                } else {
                    display.drawBitmap(0, 0, welcome_face_allArray[frame], 128, 32, WHITE);
                }
                break;
            case 2:

                if (frame >= sizeof(late_face_allArray) / 4) {
                    frame = 0;
                } else {
                    display.drawBitmap(0, 0, late_face_allArray[frame], 128, 32, WHITE);
                }
                break;
            case 3:
                if (frame >= sizeof(invalid_face_allArray) / 4) {
                    frame = 0;
                } else {
                    Serial.println("Frame: " + String(frame) + ", Size: " + String(sizeof(invalid_face_allArray) / 4));
                    display.drawBitmap(0, 0, invalid_face_allArray[frame], 128, 32, WHITE);
                }
                break;
            default:
                // display.drawBitmap(0, 0, idle_face_allArray[frame], 128, 32, WHITE);
                break;
        }
        display.display();
        frame += 1;
    }
}

void buzzTimerPoll() {
    if (buzzTimer.done()) {
        noTone(BUZZER_PIN);
        buzzTimer.reset();
    }
}

void buzz(int time) {
    buzzTimer.set(time);
    buzzTimer.start();

    if (buzzTimer.done()) {
        noTone(BUZZER_PIN);
    }

    if (buzzTimer.waiting()) {
        tone(BUZZER_PIN, 1000);
    }
}

void onEventCallback(const websockets::WebsocketsEvent event, const websockets::WebsocketsMessage& message) {
    if (event == websockets::WebsocketsEvent::ConnectionClosed) {
        while (wsClient.available() == false) {
            changeLcdText("Reconnecting...");
            wsClient.connect(WS_URL);
            delay(100);
        }

        changeLcdText("Reconnected!");
    }
}

void configureWebSockets() {
    wsClient.addHeader("Authorization", "Basic ZXNwMzI6MTIzNA==");
    wsClient.connect(WS_URL);
    wsClient.setInsecure();
    wsClient.onMessage(ws_message_callback);

    while (wsClient.available() == false) {
        changeLcdText("Connecting server...");
        wsClient.connect(WS_URL);
        delay(100);
    }
}

void setRFIDKey() {
    for (unsigned char & keyCount : key.keyByte) {
        keyCount = 0xFF;
    }
}

void setup() {
    Serial.begin(115200);
    SPI.begin();
    rfid.PCD_Init();

    // Initialize Display
    lcd.clear();
    lcd.init();
    lcd.backlight();
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    // CONFIGURE WIFI
    WiFiClass::mode(WIFI_STA);
    WiFi.begin(ssid, pass);

    while ((WiFiClass::status() != WL_CONNECTED)) {
        changeLcdText("Connecting...");
        delay(50);
    }

    changeLcdText("Connected!");
    Serial.println(WiFi.localIP());

    setRFIDKey();
    configureWebSockets();

    // PIN MODE
    pinMode(BUTTON_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    buttonMode = digitalRead(BUTTON_PIN);

    // Test Buzzer
    buzz(500);
}

/**
 * Checks the mode and updates the button state, mode, and LCD text accordingly.
 *
 * @throws None
 */
void checkMode() {
    buttonMode = digitalRead(BUTTON_PIN);
    if (buttonMode) {
        buttonState = !buttonState;
        mode = !buttonState ? "out" : "in";
        changeLcdText("Check " + String(mode));
        delay(200);
    }
}

/**
 * Sets the parts of the hash by copying the characters from the firstHash and secondHash arrays to the hash array.
 *
 * @throws None
 */
void setPartsOfHash() {
    for (int character = 0; character < sizeof(firstHash) - 2; character++) {
        hash[character] = firstHash[character];
    }

    for (int character = 0; character < sizeof(secondHash) - 2; character++) {
        hash[(sizeof(hash) / 2) - 1 + character] = secondHash[character];
    }
}

bool checkWebSocketConnection() {
    if (checkWSocket.repeat()) {
        if (wsClient.available()) {
            return true;
        } else {
            wsClient.connect(WS_URL);
            changeLcdText("WebSocket");
            return false;
        }
    }

    return false;
}

void loop() {
    // Websocket client and check websocket connection if it is up.
    wsClient.poll();
    checkWebSocketConnection();

    // Stop if the buzzer has reached its timer.
    buzzTimerPoll();

    // Display Face
    displayFace();

    if (hasSent == true) {
        return;
    }

    if (hasResponse == true) {
        hasResponse = false;
        return;
    }

    // Check if the state of button has changed.
    checkMode();

    if (!rfid.PICC_IsNewCardPresent()) {
        changeLcdText("Processing...");
        return;
    }

    if (!rfid.PICC_ReadCardSerial()) {
        changeLcdText("No card found.");
        return;
    }

    status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(rfid.uid));
    if (status == MFRC522::STATUS_OK) {
        // Read the first hash.
        status = rfid.MIFARE_Read(4, firstHash, &firstHashSize);

        if (status == MFRC522::STATUS_OK) {
            // Proceed getting the second hash.
            status = rfid.MIFARE_Read(5, secondHash, &secondHashSize);
            if (status == MFRC522::STATUS_OK) {
                changeLcdText("Processing...");
                // If success, concatenate firstHash and secondHash to hash variable.
                setPartsOfHash();

                String hashString = concatenateBytes(hash, sizeof(hash) - 2);
                String jsonData = "{\"mode\": \"" + mode + "\", \"hashedLrn\": \"" + hashString + "\"}";
                // Serial.println(jsonData);
                // Serial.println("Sending to websocket: " + hashString);
                if (wsClient.send(jsonData)) {
                    hasSent = true;
                    buzz(50);
                }
            } else {
                changeLcdText("Scan again...");
            }
        } else {
            changeLcdText("Scan again...");
            Serial.println(rfid.GetStatusCodeName(status));
        }

        // RESET SIZE, FIXES BUFFER ISSUES
        firstHashSize = sizeof(firstHash);
        secondHashSize = sizeof(secondHash);
    } else {
        changeLcdText("Too fast!");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    // changeLcdText("Card scanned!");
}
