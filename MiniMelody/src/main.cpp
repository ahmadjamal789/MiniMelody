// MiniMelody - pocket synth keyboard for the Raspberry Pi Pico
//
// Two cores, two jobs:
//   Core 0: setup() / loop()   -> buttons, rotary encoder, OLED display
//   Core 1: setup1() / loop1() -> audio engine (never interrupted, so no clicks)

#include <Arduino.h>
#include "config.h"
#include "state.h"
#include "input.h"
#include "audio.h"
#include "display.h"

SharedState gState;

// ---------- Core 0 ----------
void setup() {
  Serial.begin(115200);
  inputBegin();
  if (!displayBegin()) {
    Serial.println("OLED not found - check wiring and address. Audio still works.");
  }
}

void loop() {
  inputUpdate();
  displayUpdate();
}

// ---------- Core 1 ----------
void setup1() {
  audioBegin();
}

void loop1() {
  audioLoop();
}