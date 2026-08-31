#pragma once

// OLED (SH1106, I2C) — pins swapped vs. Wire's ESP8266 defaults
#define PIN_OLED_SDA 5
#define PIN_OLED_SCL 4

// WS2812B status LED
#define PIN_WS2812 15

// Buttons (active-low, INPUT_PULLUP)
#define PIN_BTN_SELECT 14
#define PIN_BTN_UP 12
#define PIN_BTN_DOWN 13
