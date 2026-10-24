#pragma once
#include <Arduino.h>
#include "config.h"

// Sound types the MODE button cycles through.
enum Waveform : uint8_t { WAVE_SINE, WAVE_SQUARE, WAVE_SAW, WAVE_TRIANGLE, WAVE_COUNT };

inline const char* waveformName(uint8_t w) {
  switch (w) {
    case WAVE_SINE:     return "SINE";
    case WAVE_SQUARE:   return "SQUARE";
    case WAVE_SAW:      return "SAW";
    default:            return "TRIANGLE";
  }
}

// State shared between the two cores.
//  Core 0 (input + display) WRITES it, core 1 (audio) READS it.
//  Every field is a single byte, so reads and writes are atomic on the RP2040.
struct SharedState {
  volatile uint8_t heldMask = 0;              // bit k = key k is held down
  volatile uint8_t waveform = WAVE_SINE;
  volatile int8_t  octave   = 0;              // -1 .. +2
  volatile uint8_t volume   = VOLUME_START;   // 0 .. VOLUME_MAX
};

extern SharedState gState;
