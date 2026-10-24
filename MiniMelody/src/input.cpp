#include "input.h"
#include "config.h"
#include "state.h"

// ---------- debounced button ----------
struct Button {
  uint8_t  pin;
  bool     pressed;     // debounced state
  bool     lastRaw;     // last raw reading
  uint32_t changedAt;   // when the raw reading last changed
};

static Button keys[NUM_KEYS];
static Button btnMode, btnOct;

static void initButton(Button& b, uint8_t pin) {
  b.pin = pin;
  b.pressed = false;
  b.lastRaw = false;
  b.changedAt = 0;
  pinMode(pin, INPUT_PULLUP);
}

// Returns true when the debounced state just changed.
static bool updateButton(Button& b, uint32_t now) {
  bool raw = (digitalRead(b.pin) == LOW);       // pressed = pin pulled to GND
  if (raw != b.lastRaw) {
    b.lastRaw = raw;
    b.changedAt = now;
  }
  if (raw != b.pressed && (now - b.changedAt) >= DEBOUNCE_MS) {
    b.pressed = raw;
    return true;
  }
  return false;
}

// ---------- rotary encoder (interrupt driven, so the slow OLED never misses a turn) ----------
static volatile uint8_t encPrev  = 0;
static volatile int8_t  encAccum = 0;

static void changeVolume(int8_t delta) {
  int16_t v = (int16_t)gState.volume + delta;
  if (v < 0) v = 0;
  if (v > VOLUME_MAX) v = VOLUME_MAX;
  gState.volume = (uint8_t)v;
}

static void encoderISR() {
  // Quadrature decoding table: index = (previous AB << 2) | current AB
  static const int8_t TABLE[16] = { 0, -1, 1, 0,  1, 0, 0, -1,  -1, 0, 0, 1,  0, 1, -1, 0 };
  uint8_t cur = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
  encAccum += TABLE[(encPrev << 2) | cur];
  encPrev = cur;

  if (encAccum >= ENC_TRANSITIONS_PER_STEP) {
    encAccum = 0;
    changeVolume(ENC_REVERSE ? -1 : +1);
  } else if (encAccum <= -ENC_TRANSITIONS_PER_STEP) {
    encAccum = 0;
    changeVolume(ENC_REVERSE ? +1 : -1);
  }
}

// ---------- public API ----------
void inputBegin() {
  for (uint8_t k = 0; k < NUM_KEYS; k++) initButton(keys[k], KEY_PINS[k]);
  initButton(btnMode, PIN_BTN_MODE);
  initButton(btnOct,  PIN_BTN_OCT);

  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  encPrev = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), encoderISR, CHANGE);
}

void inputUpdate() {
  uint32_t now = millis();

  // Note keys -> bit mask of held keys (any combination = chords)
  uint8_t mask = 0;
  for (uint8_t k = 0; k < NUM_KEYS; k++) {
    updateButton(keys[k], now);
    if (keys[k].pressed) mask |= (1 << k);
  }
  gState.heldMask = mask;

  // MODE: next sound
  if (updateButton(btnMode, now) && btnMode.pressed) {
    gState.waveform = (uint8_t)((gState.waveform + 1) % WAVE_COUNT);
  }

  // OCT: -1 -> 0 -> +1 -> +2 -> back to -1
  if (updateButton(btnOct, now) && btnOct.pressed) {
    int8_t o = (int8_t)(gState.octave + 1);
    if (o > 2) o = -1;
    gState.octave = o;
  }
}
