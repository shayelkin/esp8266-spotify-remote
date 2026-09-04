/**The MIT License (MIT)

 Copyright (c) 2018 by ThingPulse Ltd., https://thingpulse.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_NeoPixel.h>
#include <LittleFS.h>

#include "SpotifyClient.h"
#include "pins.h"
#include "secrets.h"

#define BUTTON_ACTION_COOLDOWN_MS 250
#define SONG_TITLE_DURATION_MS 5000UL
#define ALBUM_NAME_DURATION_MS 2000UL
#define IDLE_SLEEP_TIMEOUT_MS (2UL * 60UL * 1000UL)
#define SLEEP_BUTTON_HOLD_MS 1500UL
#define SCROLL_SPEED_PX_PER_SEC 20.0f
#define SCROLL_PAUSE_MS 1200UL

void setClock();
String formatTime(uint32_t time);
void saveRefreshToken(String refreshToken);
String loadRefreshToken();
void displayLogo();
void drawProgress(uint64_t progressMs, uint64_t durationMs, const String &firstLine, const String &artistName, boolean isPlaying, boolean isPlayerActive);

void drawSongInfo();
DrawingCallback drawSongInfoCallback = &drawSongInfo;
void drawScrollingLine(uint8_t x0, uint8_t x1, uint8_t y, uint8_t clipYTop, uint8_t clipYBottom, const String &text, unsigned long &scrollStartMillis, String &lastText);

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);
Adafruit_NeoPixel statusLed(1, PIN_WS2812, NEO_GRB + NEO_KHZ800);

#define COLOR_WIFI_CONNECTING statusLed.Color(0, 0, 255)
#define COLOR_OAUTH_PENDING statusLed.Color(200, 0, 200)
#define COLOR_PLAYING statusLed.Color(0, 180, 0)
#define COLOR_PAUSED statusLed.Color(255, 120, 0)
#define COLOR_IDLE statusLed.Color(0, 0, 0)
#define COLOR_ERROR statusLed.Color(255, 0, 0)

void setStatusColor(uint32_t color) {
  statusLed.setPixelColor(0, color);
  statusLed.show();
}

WiFiClientSecure wifiClient;
SpotifyClient client(clientId, clientSecret, redirectUri, &wifiClient);
SpotifyData data;
SpotifyAuth auth;

uint32_t lastDrawingUpdate = 0;
unsigned long firstLineShownSince = 0;
bool showingSongTitle = true;
bool playerWasActive = false;
long lastUpdate = 0;
uint32_t lastUpMillis = 0;
uint32_t lastDownMillis = 0;
bool isIdle = false;
uint32_t idleSince = 0;
uint32_t selectPressStartMillis = 0;
unsigned long titleLineScrollStart = 0;
String titleLineLastText = "";
unsigned long artistLineScrollStart = 0;
String artistLineLastText = "";

bool buttonPressed(uint8_t pin, uint32_t &lastActionMillis) {
  if (digitalRead(pin) == LOW && millis() - lastActionMillis > BUTTON_ACTION_COOLDOWN_MS) {
    lastActionMillis = millis();
    return true;
  }
  return false;
}

void enterSleep() {
  u8g2.setPowerSave(1);
  statusLed.setPixelColor(0, 0);
  statusLed.show();
  Serial.println("Entering deep sleep. Wake with RST.");
  ESP.deepSleep(0);
}

void printFreeHeap(String msg) {
  #ifdef MEMORY_DEBUG
  Serial.println("*** Memory stats " + msg + " ***");
  Serial.printf("\tFree heap: %d\n", ESP.getFreeHeap());
  Serial.printf("\tMax free block size: %d\n", ESP.getMaxFreeBlockSize());
  Serial.printf("\tHeap fragmentation: %d%\n\n", ESP.getHeapFragmentation());
  #endif
}

void setup() {
  Serial.begin(115200);
  Serial.println("");
  printFreeHeap("right after setup()");
  wifiClient.setBufferSizes(512, 512);
  // No trust anchor is configured, so BearSSL requires this explicitly or connect() always fails.
  wifiClient.setInsecure();

  statusLed.begin();
  statusLed.setBrightness(54);
  setStatusColor(COLOR_WIFI_CONNECTING);

  pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  u8g2.begin();
  displayLogo();

  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected at IP address: ");
  Serial.println(WiFi.localIP());

  printFreeHeap("after WiFi connection");

  setClock();

  boolean mounted = LittleFS.begin();
  if (!mounted) {
    Serial.println("FS not formatted. Doing that now");
    LittleFS.format();
    Serial.println("FS formatted...");
    LittleFS.begin();
  }

  printFreeHeap("after LittleFS mounted");

  String code = "";
  String grantType = "";
  String refreshToken = loadRefreshToken();
  if (refreshToken == "") {
    Serial.println("No refresh token found. Requesting through browser");
    setStatusColor(COLOR_OAUTH_PENDING);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(0, 10, "Authentication required.");
    u8g2.drawStr(0, 22, "Open browser at");
    u8g2.drawStr(0, 34, ("http://" + espotifierNodeName + ".local").c_str());
    u8g2.sendBuffer();

    code = client.startConfigPortal(espotifierNodeName);
    grantType = "authorization_code";
  } else {
    Serial.println("Using refresh token found on the FS");
    code = refreshToken;
    grantType = "refresh_token";
  }
  client.getToken(&auth, grantType, code);
  Serial.printf("Refresh token: %s\nAccess Token: %s\n", auth.refreshToken.c_str(), auth.accessToken.c_str());
  if (auth.refreshToken != "") {
    saveRefreshToken(auth.refreshToken);
  }
  client.setDrawingCallback(&drawSongInfoCallback);
}

void loop() {
  if (millis() - lastUpdate > 1000) {
    uint16_t responseCode = client.update(&data, &auth);
    lastUpdate = millis();
    Serial.printf("--------Response Code: %d\n", responseCode);
    Serial.printf("--------Free mem: %d\n", ESP.getFreeHeap());
    if (responseCode == 401) {
      client.getToken(&auth, "refresh_token", auth.refreshToken);
      if (auth.refreshToken != "") {
        saveRefreshToken(auth.refreshToken);
      }
    }
    if (responseCode == 400) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_5x8_tf);
      u8g2.drawStr(0, 20, "Please define");
      u8g2.drawStr(0, 32, "clientId and clientSecret");
      u8g2.sendBuffer();
      setStatusColor(COLOR_ERROR);
    } else if (data.isPlayerActive) {
      setStatusColor(data.isPlaying ? COLOR_PLAYING : COLOR_PAUSED);
    } else {
      setStatusColor(COLOR_IDLE);
      if (!isIdle) {
        isIdle = true;
        idleSince = millis();
      } else if (millis() - idleSince > IDLE_SLEEP_TIMEOUT_MS) {
        enterSleep();
      }
    }
    if (data.isPlayerActive) {
      isIdle = false;
    }
  }
  drawSongInfo();

  bool selectDown = digitalRead(PIN_BTN_SELECT) == LOW;
  if (selectDown && selectPressStartMillis == 0) {
    selectPressStartMillis = millis();
  } else if (selectDown && millis() - selectPressStartMillis >= SLEEP_BUTTON_HOLD_MS) {
    enterSleep();
  } else if (!selectDown && selectPressStartMillis != 0) {
    if (millis() - selectPressStartMillis < SLEEP_BUTTON_HOLD_MS) {
      String command = data.isPlaying ? "pause" : "play";
      data.isPlaying = !data.isPlaying;
      uint16_t responseCode = client.playerCommand(&auth, "PUT", command);
      Serial.print("playerCommand response =");
      Serial.println(responseCode);
    }
    selectPressStartMillis = 0;
  }
  if (buttonPressed(PIN_BTN_UP, lastUpMillis)) {
    uint16_t responseCode = client.playerCommand(&auth, "POST", "previous");
    Serial.print("playerCommand response =");
    Serial.println(responseCode);
  }
  if (buttonPressed(PIN_BTN_DOWN, lastDownMillis)) {
    uint16_t responseCode = client.playerCommand(&auth, "POST", "next");
    Serial.print("playerCommand response =");
    Serial.println(responseCode);
  }
}

// Set time via NTP, as required for x.509 validation
void setClock() {
  configTime(TIMEZONE, "pool.ntp.org");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time UTC: ");
  Serial.print(asctime(&timeinfo));
}

void drawSongInfo() {
  if (millis() - lastDrawingUpdate < 333) {
    return;
  }
  lastDrawingUpdate = millis();
  long timeSinceUpdate = 0;
  if (data.isPlaying) {
    timeSinceUpdate = millis() - lastUpdate;
  }
  if (data.isPlayerActive && !playerWasActive) {
    showingSongTitle = true;
    firstLineShownSince = millis();
  }
  playerWasActive = data.isPlayerActive;
  unsigned long now = millis();
  unsigned long displayDuration = showingSongTitle ? SONG_TITLE_DURATION_MS : ALBUM_NAME_DURATION_MS;
  if (now - firstLineShownSince >= displayDuration) {
    showingSongTitle = !showingSongTitle;
    firstLineShownSince = now;
  }
  const String &firstLine = showingSongTitle || data.albumName == "" ? data.title : data.albumName;
  drawProgress(_min(data.progressMs + timeSinceUpdate, data.durationMs), data.durationMs, firstLine, data.artistName, data.isPlaying, data.isPlayerActive);
}

void drawProgress(uint64_t progressMs, uint64_t durationMs, const String &firstLine, const String &artistName, boolean isPlaying, boolean isPlayerActive) {

  if (!isPlayerActive) {
    displayLogo();
    return;
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  drawScrollingLine(0, 128, 9, 0, 10, firstLine, titleLineScrollStart, titleLineLastText);

  u8g2.setFont(u8g2_font_5x8_tf);
  drawScrollingLine(0, 128, 20, 11, 23, artistName, artistLineScrollStart, artistLineLastText);

  uint8_t percentage = 100.0 * progressMs / durationMs;
  uint16_t barX = 4, barW = 120, barY = 34;
  u8g2.drawFrame(barX, barY, barW, 4);
  u8g2.drawBox(barX, barY, barW * percentage / 100, 4);
  u8g2.drawStr(0, 46, formatTime(progressMs).c_str());
  String total = formatTime(durationMs);
  u8g2.drawStr(128 - u8g2.getStrWidth(total.c_str()), 46, total.c_str());

  u8g2.drawTriangle(16, 52, 16, 64, 8, 58);
  u8g2.drawTriangle(112, 52, 112, 64, 120, 58);
  if (isPlaying) {
    u8g2.drawBox(60, 52, 3, 12);
    u8g2.drawBox(66, 52, 3, 12);
  } else {
    u8g2.drawTriangle(58, 52, 58, 64, 70, 58);
  }

  u8g2.sendBuffer();
}

// Draws `text` centered within [x0, x1) at baseline y. If it's too wide to fit,
// scrolls it left instead, pausing briefly at the start and end of each pass.
// Scroll position is time-based (not frame-based) and resets whenever the text changes.
void drawScrollingLine(uint8_t x0, uint8_t x1, uint8_t y, uint8_t clipYTop, uint8_t clipYBottom, const String &text, unsigned long &scrollStartMillis, String &lastText) {
  uint16_t boxWidth = x1 - x0;
  uint16_t textWidth = u8g2.getStrWidth(text.c_str());

  if (text != lastText) {
    lastText = text;
    scrollStartMillis = millis();
  }

  if (textWidth <= boxWidth) {
    u8g2.drawStr(x0 + (boxWidth - textWidth) / 2, y, text.c_str());
    return;
  }

  uint16_t scrollDistance = textWidth - boxWidth;
  unsigned long scrollDurationMs = scrollDistance / SCROLL_SPEED_PX_PER_SEC * 1000UL;
  unsigned long cycleLength = 2 * SCROLL_PAUSE_MS + scrollDurationMs;
  unsigned long elapsed = (millis() - scrollStartMillis) % cycleLength;

  int16_t offset;
  if (elapsed < SCROLL_PAUSE_MS) {
    offset = 0;
  } else if (elapsed < SCROLL_PAUSE_MS + scrollDurationMs) {
    offset = (elapsed - SCROLL_PAUSE_MS) * SCROLL_SPEED_PX_PER_SEC / 1000UL;
  } else {
    offset = scrollDistance;
  }

  u8g2.setClipWindow(x0, clipYTop, x1, clipYBottom);
  u8g2.drawStr(x0 - offset, y, text.c_str());
  u8g2.setMaxClipWindow();
}

void displayLogo() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(28, 36, "ESPotify-Remote");
  u8g2.sendBuffer();
}

String formatTime(uint32_t time) {
  char time_str[10];
  uint8_t minutes = time / (1000 * 60);
  uint8_t seconds = (time / 1000) % 60;
  sprintf(time_str, "%2d:%02d", minutes, seconds);
  return String(time_str);
}

void saveRefreshToken(String refreshToken) {
  File f = LittleFS.open("/refreshToken.txt", "w+");
  if (!f) {
    Serial.println("Failed to open config file");
    return;
  }
  f.println(refreshToken);
  f.close();
}

String loadRefreshToken() {
  Serial.println("Loading config");
  File f = LittleFS.open("/refreshToken.txt", "r");
  if (!f) {
    Serial.println("Failed to open config file");
    return "";
  }
  while (f.available()) {
    // Lets read line by line from the file
    String token = f.readStringUntil('\r');
    Serial.printf("Refresh Token: %s\n", token.c_str());
    f.close();
    return token;
  }
  return "";
}
