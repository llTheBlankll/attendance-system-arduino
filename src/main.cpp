#include <MFRC522.h>
#include <SPI.h>
#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <ArduinoJson.h>
#include <neotimer.h>

#include <lcd_controller.h>
#include <face_controller.h>

#include "buzzer_controller.h"

#define RST_PIN 0
#define SS_PIN 5
#define BUTTON_PIN 32

#define ssid "PLDTHOMEFIBR9u7w4"
#define pass "PLDTWIFIkr39h"
#define WS_URL "ws://roundhouse.proxy.rlwy.net:42552/websocket/scanner"

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
websockets::WebsocketsClient wsClient;
Neotimer checkWSTimer(99);
Neotimer faceTimer(500);

byte hash[34];
byte blockSize = 4;

int lastMode = LOW;
int buttonMode = 0;

bool buttonState = false;
bool hasResponse = false;
bool hasSent = false;

// Controllers
LCDController lcdController;
BuzzerController buzzerController;
FaceController faceController;

String mode = "in";
// enum Faces {IDLE, WELCOME, LATE, INVALID}

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


  if (messageResponse == "You are LATE") {
  } else if (messageResponse == "You are ONTIME" || messageResponse == "Already Scanned!" || messageResponse ==
             "Already Arrived!") {
    faceController.faceMode = 1;
  } else if (messageResponse == "Invalid") {
    faceController.faceMode = 3;
  } else {
    faceController.faceMode = 1;
  }

  hasResponse = true;
}

void onEventCallback(const websockets::WebsocketsEvent event, const websockets::WebsocketsMessage &message) {
  if (event == websockets::WebsocketsEvent::ConnectionClosed) {
    while (wsClient.available() == false) {
      lcdController.changeLcdText("Reconnecting...");
      wsClient.connect(WS_URL);
      delay(100);
    }

    lcdController.changeLcdText("Reconnected!");
  }
}

void configureWebSockets() {
  wsClient.addHeader("Authorization", "Basic ZXNwMzI6MTIzNA==");
  wsClient.connect(WS_URL);
  wsClient.setInsecure();
  wsClient.onMessage(ws_message_callback);

  while (wsClient.available() == false) {
    lcdController.changeLcdText("Connecting server...");
    wsClient.connect(WS_URL);
    delay(100);
  }
}

void setRFIDKey() {
  for (unsigned char &keyCount: key.keyByte) {
    keyCount = 0xFF;
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  // CONFIGURE WIFI
  WiFiClass::mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFiClass::status() != WL_CONNECTED) {
    lcdController.changeLcdText("Connecting...");
    delay(50);
  }

  lcdController.changeLcdText("Connected!");
  Serial.println(WiFi.localIP());

  setRFIDKey();
  configureWebSockets();

  // PIN MODE
  pinMode(BUTTON_PIN, INPUT);
  buttonMode = digitalRead(BUTTON_PIN);

  // Test Buzzer
  buzzerController.buzz(1000);
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
    lcdController.changeLcdText("Check " + String(mode));
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
      lcdController.changeLcdText("WebSocket");
      return false;
    }
  }

  return false;
}

void updateGTO() {
  if (faceTimer.repeat()) {
    faceController.displayFace();
  }
}

void WSConnectionCheck() {
  // Checks websocket connection.
  checkWebSocketConnection();

  // Check if there is a response from the server.
  if (hasResponse == true) {
    hasResponse = false;
  }

  wsClient.poll();
}

void loop() {
  // Display Face
  updateGTO();
  // Check if the state of button has changed.
  checkMode();

  if (!rfid.PICC_IsNewCardPresent()) {
    lcdController.changeLcdText("Processing...");
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    lcdController.changeLcdText("No card found.");
    return;
  }

  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &rfid.uid);
  if (status == MFRC522::STATUS_OK) {
    // Read the first hash.
    status = rfid.MIFARE_Read(4, firstHash, &firstHashSize);

    if (status == MFRC522::STATUS_OK) {
      // Proceed getting the second hash.
      status = rfid.MIFARE_Read(5, secondHash, &secondHashSize);
      if (status == MFRC522::STATUS_OK) {
        lcdController.changeLcdText("Processing...");
        // If success, concatenate firstHash and secondHash to hash variable.
        setPartsOfHash();

        const String hashString = concatenateBytes(hash, sizeof(hash) - 2);
        const String jsonData = R"({"mode": ")" + mode + R"(", "hashedLrn": ")" + hashString + "\"}";
        // Serial.println(jsonData);
        // Serial.println("Sending to websocket: " + hashString);
        if (wsClient.send(jsonData)) {
          hasSent = true;
          buzzerController.buzz(100);
        }
      } else {
        lcdController.changeLcdText("Scan again...");
      }
    } else {
      lcdController.changeLcdText("Scan again...");
      Serial.println(MFRC522::GetStatusCodeName(status));
    }

    // RESET SIZE, FIXES BUFFER ISSUES
    firstHashSize = sizeof(firstHash);
    secondHashSize = sizeof(secondHash);
  } else {
    lcdController.changeLcdText("Too fast!");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  // lcdController.changeLcdText("Card scanned!");
}
