#pragma once
#include <Arduino.h>

// ======================= MiniMelody configuration =======================
// !! Check EVERY pin below against your KiCad schematic before flashing !!
// All hardware-specific numbers live here, so a rewired pin is a one-line fix.

// ---- Audio ----
constexpr uint32_t SAMPLE_RATE = 32000;   // samples per second
constexpr uint8_t  NUM_KEYS    = 8;       // C D E F G A B and high C

// I2S amplifier (MAX98357A). LRCLK is automatically BCLK + 1 (GP15 here).
constexpr uint8_t PIN_I2S_DIN  = 13;
constexpr uint8_t PIN_I2S_BCLK = 14;

// ---- OLED display (I2C0, SSD1306 128x64) ----
constexpr uint8_t PIN_I2C_SDA = 0;
constexpr uint8_t PIN_I2C_SCL = 1;
constexpr uint8_t SCREEN_W    = 128;
constexpr uint8_t SCREEN_H    = 64;
constexpr uint8_t OLED_ADDR   = 0x3C;     // some modules use 0x3D

// ---- Inputs (buttons connect the pin to GND, internal pull-ups are used) ----
static const uint8_t KEY_PINS[NUM_KEYS] = { 2, 3, 4, 5, 6, 7, 8, 9 };  // C..high C
constexpr uint8_t PIN_BTN_MODE = 10;
constexpr uint8_t PIN_BTN_OCT  = 11;
constexpr uint8_t PIN_ENC_A    = 16;      // rotary encoder (volume)
constexpr uint8_t PIN_ENC_B    = 17;      // encoder common pin goes to GND

constexpr bool    ENC_REVERSE             = false; // flip if volume turns the wrong way
constexpr int8_t  ENC_TRANSITIONS_PER_STEP = 4;    // use 2 if one click skips two steps
constexpr uint8_t DEBOUNCE_MS             = 5;

// ---- Volume ----
constexpr uint8_t VOLUME_MAX   = 10;
constexpr uint8_t VOLUME_START = 6;
