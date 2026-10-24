#include "audio.h"
#include "config.h"
#include "state.h"
#include <I2S.h>
#include <math.h>

// The synth engine. One oscillator per key, all mixed together.
// Everything is integer maths so it is fast on the RP2040 (no FPU).
// Each voice peaks at about 1/4 of full scale, so 4 notes together stay clean.

static I2S i2s(OUTPUT);

static int16_t  sineTable[256];
static uint32_t baseInc[NUM_KEYS];   // phase step per sample at octave 0
static uint32_t phase[NUM_KEYS];     // 32-bit phase accumulators
static uint16_t env[NUM_KEYS];       // envelope gain 0..65535

// Note frequencies, C4 to C5
static const float NOTE_HZ[NUM_KEYS] = {
  261.63f, 293.66f, 329.63f, 349.23f, 392.00f, 440.00f, 493.88f, 523.25f
};

// Envelope speed (change these to make notes snappier or softer)
constexpr uint32_t ATTACK_STEP  = 65535UL / (SAMPLE_RATE * 5UL   / 1000UL);  // ~5 ms fade in
constexpr uint32_t RELEASE_STEP = 65535UL / (SAMPLE_RATE * 120UL / 1000UL);  // ~120 ms fade out

static inline uint32_t shiftOctave(uint32_t inc, int8_t oct) {
  return (oct >= 0) ? (inc << oct) : (inc >> (-oct));
}

// One sample of the chosen waveform. p is a 32-bit phase (full range = one cycle).
static inline int32_t oscillator(uint8_t wave, uint32_t p) {
  switch (wave) {
    case WAVE_SINE:
      return sineTable[p >> 24];
    case WAVE_SQUARE:
      return (p & 0x80000000u) ? 4500 : -4500;              // lower level: squares sound louder
    case WAVE_SAW:
      return ((int32_t)(p >> 16) - 32768) >> 2;
    default: {                                              // triangle
      int32_t x = (int32_t)(p >> 16);
      int32_t y = (x < 32768) ? (x * 2 - 32768) : (32767 - (x - 32768) * 2);
      return y >> 2;
    }
  }
}

static inline int16_t renderSample() {
  const uint8_t held = gState.heldMask;
  const uint8_t wave = gState.waveform;
  const int8_t  oct  = gState.octave;
  const uint8_t vol  = gState.volume;

  int32_t mix = 0;
  for (uint8_t k = 0; k < NUM_KEYS; k++) {
    // Envelope: fade in while held, fade out when released (no clicks)
    if (held & (1 << k)) {
      uint32_t e = env[k] + ATTACK_STEP;
      env[k] = (e > 65535) ? 65535 : (uint16_t)e;
    } else if (env[k]) {
      env[k] = (env[k] > RELEASE_STEP) ? (uint16_t)(env[k] - RELEASE_STEP) : 0;
    }
    if (env[k] == 0) continue;                              // silent voice: skip the work

    phase[k] += shiftOctave(baseInc[k], oct);
    int32_t s = oscillator(wave, phase[k]);
    mix += (s * (int32_t)env[k]) >> 16;
  }

  mix = (mix * vol) / VOLUME_MAX;                           // master volume
  if (mix > 32767)  mix = 32767;                            // hard limit if more than ~4 loud notes pile up
  if (mix < -32768) mix = -32768;
  return (int16_t)mix;
}

void audioBegin() {
  for (int i = 0; i < 256; i++) {
    sineTable[i] = (int16_t)(sinf(2.0f * (float)M_PI * i / 256.0f) * 8000.0f);
  }
  for (uint8_t k = 0; k < NUM_KEYS; k++) {
    baseInc[k] = (uint32_t)((double)NOTE_HZ[k] * 4294967296.0 / (double)SAMPLE_RATE);
    phase[k] = 0;
    env[k] = 0;
  }
  i2s.setBCLK(PIN_I2S_BCLK);      // LRCLK = BCLK + 1 automatically
  i2s.setDATA(PIN_I2S_DIN);
  i2s.setBitsPerSample(16);
  i2s.begin(SAMPLE_RATE);
}

void audioLoop() {
  for (int n = 0; n < 64; n++) {
    int16_t s = renderSample();
    i2s.write(s);                 // left
    i2s.write(s);                 // right (the amp mixes both to mono)
  }
}
