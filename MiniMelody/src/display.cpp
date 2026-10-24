#include "display.h"
#include "config.h"
#include "state.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, -1);
static bool displayOk = false;

static const char* NOTE_NAMES[NUM_KEYS] = { "C", "D", "E", "F", "G", "A", "B", "C'" };

// Shape of the selected waveform, ph = 0..1 -> -1..+1
static float shape(uint8_t wave, float ph) {
  switch (wave) {
    case WAVE_SINE:   return sinf(2.0f * (float)M_PI * ph);
    case WAVE_SQUARE: return (ph < 0.5f) ? 1.0f : -1.0f;
    case WAVE_SAW:    return 2.0f * ph - 1.0f;
    default:          return (ph < 0.5f) ? (4.0f * ph - 1.0f) : (3.0f - 4.0f * ph);
  }
}

bool displayBegin() {
  Wire.setSDA(PIN_I2C_SDA);
  Wire.setSCL(PIN_I2C_SCL);
  Wire.begin();
  Wire.setClock(400000);

  displayOk = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (!displayOk) return false;

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(10, 14);
  oled.print("MiniMelody");
  oled.setTextSize(1);
  oled.setCursor(22, 42);
  oled.print("Make some noise!");
  oled.display();
  delay(900);
  return true;
}

void displayUpdate() {
  if (!displayOk) return;

  static uint32_t lastDraw = 0;
  static float anim = 0.0f;
  uint32_t now = millis();
  if (now - lastDraw < 100) return;     // ~10 frames per second keeps input responsive
  lastDraw = now;
  anim += 0.15f;
  if (anim > 1.0f) anim -= 1.0f;

  const uint8_t held = gState.heldMask;
  const uint8_t wave = gState.waveform;
  const int8_t  oct  = gState.octave;
  const uint8_t vol  = gState.volume;

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // --- Header: sound name (left) and octave (right) ---
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print(waveformName(wave));
  char buf[12];
  snprintf(buf, sizeof(buf), "OCT %+d", oct);
  oled.setCursor(SCREEN_W - 6 * strlen(buf), 0);
  oled.print(buf);

  // --- Notes currently being played ---
  int lowest = -1;
  oled.setTextSize(2);
  oled.setCursor(0, 13);
  for (uint8_t k = 0; k < NUM_KEYS; k++) {
    if (held & (1 << k)) {
      oled.print(NOTE_NAMES[k]);
      if (lowest < 0) lowest = k;
    }
  }
  if (lowest < 0) {
    oled.setTextSize(1);
    oled.setCursor(0, 18);
    oled.print("Press a key...");
  }

  // --- Live waveform: shows the sound shape, wiggles faster for higher notes ---
  const int top = 34, height = 21, mid = top + height / 2;
  if (lowest < 0) {
    oled.drawFastHLine(0, mid, SCREEN_W, SSD1306_WHITE);
  } else {
    float cycles = 2.0f + 0.6f * lowest;
    float amp = (height / 2 - 1) * (0.25f + 0.75f * vol / VOLUME_MAX);
    int prevY = mid;
    for (int x = 0; x < SCREEN_W; x++) {
      float ph = (float)x * cycles / SCREEN_W + anim;
      ph -= floorf(ph);
      int y = mid - (int)(shape(wave, ph) * amp);
      if (x > 0) oled.drawLine(x - 1, prevY, x, y, SSD1306_WHITE);
      prevY = y;
    }
  }

  // --- Volume bar ---
  oled.drawRect(0, 58, SCREEN_W, 6, SSD1306_WHITE);
  oled.fillRect(2, 60, ((SCREEN_W - 4) * vol) / VOLUME_MAX, 2, SSD1306_WHITE);

  oled.display();
}
