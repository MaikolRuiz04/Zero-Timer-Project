#pragma once
#include "config.h"

// =============================================================================
// Global Hardware Objects (defined in main.cpp)
// =============================================================================

extern Adafruit_ILI9341 tft;
extern ESP32Encoder encoder;
extern Preferences prefs;

// =============================================================================
// Enums
// =============================================================================

enum class ActiveApp { LAUNCHER, POMODORO, QUICK_TIMER, STOPWATCH };
enum class VolumeLevel : uint8_t { VOL_MUTE, VOL_LOW, VOL_MED, VOL_HIGH };

// =============================================================================
// Shared Global State (defined in main.cpp)
// =============================================================================

extern ActiveApp  activeApp;
extern VolumeLevel volumeLevel;
extern bool        deviceOn;
extern bool        needsFullRedraw;

// =============================================================================
// Display Cache (shared across apps to avoid stale redraws)
// =============================================================================

extern int32_t  dispTime;
extern int32_t  dispMins;
extern int32_t  dispSecs;
extern int32_t  dispTenths;
extern int      dispSel;
extern int      dispMenuSel;
extern int      dispMenuScroll;
extern ActiveApp dispApp;

inline void resetDispCache() {
  dispTime = -1; dispMins = -1; dispSecs = -1; dispTenths = -1;
  dispSel = -1; dispMenuSel = -1; dispMenuScroll = -1;
}

// =============================================================================
// Sound System
// =============================================================================

static constexpr int NOTE_C4 = 262, NOTE_D4 = 294, NOTE_E4 = 330, NOTE_F4 = 349;
static constexpr int NOTE_G4 = 392, NOTE_A4 = 440, NOTE_B4 = 494;
static constexpr int NOTE_C5 = 523, NOTE_D5 = 587, NOTE_E5 = 659, NOTE_G5 = 784;

inline void playTone(int freq, int durMs) {
  if (volumeLevel == VolumeLevel::VOL_MUTE) { delay(durMs); return; }
  if (freq > 0) {
    int duty = 0;
    switch (volumeLevel) {
      case VolumeLevel::VOL_LOW:  duty = 32;  break;
      case VolumeLevel::VOL_MED:  duty = 64;  break;
      case VolumeLevel::VOL_HIGH: duty = 128; break;
      default: break;
    }
    ledcWriteTone(BUZZER_CHANNEL, freq);
    ledcWrite(BUZZER_CHANNEL, duty);
    delay(durMs);
  } else {
    delay(durMs);
  }
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

inline void playClickSound()   { playTone(NOTE_E5, 50); }
inline void playMenuSound()    { playTone(NOTE_C5, 60); delay(20); playTone(NOTE_E5, 60); }
inline void playSelectSound()  { playTone(NOTE_E4, 60); delay(20); playTone(NOTE_G4, 80); }
inline void playLapSound()     { playTone(NOTE_G5, 40); }

inline void playFocusMelody() {
  playTone(NOTE_E4, 100); delay(30);
  playTone(NOTE_G4, 100); delay(30);
  playTone(NOTE_C5, 150);
}

inline void playBreakMelody() {
  playTone(NOTE_G4, 120); delay(40);
  playTone(NOTE_E4, 120); delay(40);
  playTone(NOTE_C4, 180);
}

inline void playStartupJingle() {
  playTone(NOTE_C4, 80); delay(30);
  playTone(NOTE_E4, 80); delay(30);
  playTone(NOTE_G4, 80); delay(30);
  playTone(NOTE_C5, 120); delay(50);
  playTone(NOTE_E5, 150);
}

inline void playAlarmMelody() {
  for (int i = 0; i < 3; i++) {
    playTone(NOTE_E5, 150); delay(50);
    playTone(NOTE_C5, 150); delay(80);
  }
}

// =============================================================================
// Common Icon Drawing
// =============================================================================

// Pause icon (two bars)
inline void drawPauseIcon(int16_t x, int16_t y, uint16_t color) {
  tft.fillRoundRect(x, y, 6, 20, 2, color);
  tft.fillRoundRect(x + 10, y, 6, 20, 2, color);
}

// Play icon (triangle)
inline void drawPlayIcon(int16_t x, int16_t y, uint16_t color) {
  tft.fillTriangle(x, y, x, y + 20, x + 16, y + 10, color);
}

// Laptop icon (Pomodoro focus)
inline void drawLaptopIcon(int16_t x, int16_t y, uint16_t color) {
  tft.fillRoundRect(x, y, 32, 20, 2, color);
  tft.fillRect(x + 2, y + 2, 28, 16, COLOR_BG);
  tft.fillRoundRect(x - 2, y + 22, 36, 4, 1, color);
}

// Coffee cup icon (Pomodoro break)
inline void drawCoffeeIcon(int16_t x, int16_t y, uint16_t color) {
  tft.fillRoundRect(x, y + 4, 24, 22, 3, color);
  tft.fillRoundRect(x + 3, y + 7, 18, 16, 2, COLOR_BG);
  tft.fillCircle(x + 28, y + 14, 6, color);
  tft.fillCircle(x + 28, y + 14, 3, COLOR_BG);
  tft.fillRect(x + 22, y + 8, 6, 12, COLOR_BG);
  for (int i = 0; i < 3; i++) {
    int16_t sx = x + 6 + i * 6;
    tft.drawPixel(sx, y, color);
    tft.drawPixel(sx + 1, y - 1, color);
    tft.drawPixel(sx, y - 2, color);
  }
}

// Tomato icon (Pomodoro on launcher)
inline void drawTomatoIcon(int16_t x, int16_t y, uint16_t color) {
  tft.fillCircle(x + 14, y + 16, 13, color);
  tft.fillRoundRect(x + 11, y, 6, 6, 2, COLOR_ACCENT_BREAK);
  tft.fillTriangle(x + 17, y + 2, x + 24, y - 2, x + 22, y + 5, COLOR_ACCENT_BREAK);
}

// Hourglass icon (Quick Timer)
inline void drawHourglassIcon(int16_t x, int16_t y, uint16_t color) {
  tft.fillRect(x, y, 24, 3, color);
  tft.fillTriangle(x + 2, y + 3, x + 22, y + 3, x + 12, y + 14, color);
  tft.fillTriangle(x + 12, y + 14, x + 2, y + 25, x + 22, y + 25, color);
  tft.fillRect(x, y + 25, 24, 3, color);
}

// Stopwatch icon
inline void drawStopwatchIcon(int16_t x, int16_t y, uint16_t color) {
  tft.drawCircle(x + 12, y + 15, 12, color);
  tft.drawCircle(x + 12, y + 15, 11, color);
  tft.drawLine(x + 12, y + 15, x + 12, y + 6, color);
  tft.drawLine(x + 13, y + 15, x + 13, y + 6, color);
  tft.fillRect(x + 9, y, 6, 4, color);
  tft.fillCircle(x + 12, y + 15, 2, color);
}

// Checkmark icon
inline void drawCheckIcon(int16_t x, int16_t y, uint16_t color) {
  for (int t = 0; t < 3; t++) {
    tft.drawLine(x, y + 10 + t, x + 8, y + 18 + t, color);
    tft.drawLine(x + 8, y + 18 + t, x + 22, y + t, color);
  }
}

// =============================================================================
// Power Management
// =============================================================================

inline void displaySleep(bool sleep) {
  tft.startWrite();
  tft.writeCommand(sleep ? 0x10 : 0x11);
  tft.endWrite();
  if (!sleep) delay(120);
}

// =============================================================================
// Storage (NVS)
// =============================================================================

// Forward declarations - these reference pomodoro state so are defined in main.cpp
void saveSettings();
void loadSettings();

// =============================================================================
// App Switching
// =============================================================================

void switchToLauncher();
void turnOff();
void turnOn();

// =============================================================================
// Shared Simple Menu (used by Quick Timer & Stopwatch)
// =============================================================================

enum class SimpleMenuItem { VOLUME, SWITCH_APP, TURN_OFF };
static constexpr int SIMPLE_MENU_COUNT = 3;

extern SimpleMenuItem simpleMenuSel;

inline void drawSimpleMenu(const char* title) {
  int sel = (int)simpleMenuSel;

  if (needsFullRedraw) {
    tft.fillScreen(COLOR_BG);

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextSize(2);
    tft.setCursor(110, 20);
    tft.print(title);
    tft.fillRect(20, 50, 280, 1, COLOR_SURFACE);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(80, 228);
    tft.print("click: change  hold: back");

    const int IH = 50, SY = 60;
    for (int i = 0; i < SIMPLE_MENU_COUNT; i++) {
      bool s = (i == sel);
      int16_t y = SY + i * IH;
      uint16_t bg = s ? COLOR_SURFACE : COLOR_BG;
      tft.fillRoundRect(15, y, 290, 44, 8, bg);

      switch (i) {
        case 0: // Volume
          tft.setTextColor(s ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
          tft.setTextSize(2); tft.setCursor(25, y + 14); tft.print("Volume");
          tft.setTextColor(s ? COLOR_ACCENT_FOCUS : COLOR_TEXT_DIM, bg);
          tft.setCursor(220, y + 14);
          switch (volumeLevel) {
            case VolumeLevel::VOL_MUTE: tft.print("Mute"); break;
            case VolumeLevel::VOL_LOW:  tft.print("Low");  break;
            case VolumeLevel::VOL_MED:  tft.print("Med");  break;
            case VolumeLevel::VOL_HIGH: tft.print("High"); break;
          }
          break;
        case 1: // Switch App
          tft.setTextColor(s ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
          tft.setTextSize(2); tft.setCursor(100, y + 14); tft.print("Switch App");
          break;
        case 2: // Turn Off
          tft.setTextColor(COLOR_RED, bg);
          tft.setTextSize(2); tft.setCursor(115, y + 14); tft.print("Turn Off");
          break;
      }
    }

    needsFullRedraw = false;
    dispMenuSel = sel;
  }
  else if (sel != dispMenuSel) {
    const int IH = 50, SY = 60;
    for (int i = 0; i < SIMPLE_MENU_COUNT; i++) {
      if (i != sel && i != dispMenuSel) continue;
      bool s = (i == sel);
      int16_t y = SY + i * IH;
      uint16_t bg = s ? COLOR_SURFACE : COLOR_BG;
      tft.fillRoundRect(15, y, 290, 44, 8, bg);

      switch (i) {
        case 0:
          tft.setTextColor(s ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
          tft.setTextSize(2); tft.setCursor(25, y + 14); tft.print("Volume");
          tft.setTextColor(s ? COLOR_ACCENT_FOCUS : COLOR_TEXT_DIM, bg);
          tft.setCursor(220, y + 14);
          switch (volumeLevel) {
            case VolumeLevel::VOL_MUTE: tft.print("Mute"); break;
            case VolumeLevel::VOL_LOW:  tft.print("Low");  break;
            case VolumeLevel::VOL_MED:  tft.print("Med");  break;
            case VolumeLevel::VOL_HIGH: tft.print("High"); break;
          }
          break;
        case 1:
          tft.setTextColor(s ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
          tft.setTextSize(2); tft.setCursor(100, y + 14); tft.print("Switch App");
          break;
        case 2:
          tft.setTextColor(COLOR_RED, bg);
          tft.setTextSize(2); tft.setCursor(115, y + 14); tft.print("Turn Off");
          break;
      }
    }
    dispMenuSel = sel;
  }
}

inline void handleSimpleMenuEncoder(int32_t delta) {
  int idx = (int)simpleMenuSel;
  if (delta > 0) { idx++; if (idx >= SIMPLE_MENU_COUNT) idx = 0; }
  else            { idx--; if (idx < 0) idx = SIMPLE_MENU_COUNT - 1; }
  simpleMenuSel = (SimpleMenuItem)idx;
}

// Returns true if the click was handled by shared logic (volume/switch/off)
inline bool handleSimpleMenuClick() {
  switch (simpleMenuSel) {
    case SimpleMenuItem::VOLUME:
      switch (volumeLevel) {
        case VolumeLevel::VOL_HIGH: volumeLevel = VolumeLevel::VOL_MED;  break;
        case VolumeLevel::VOL_MED:  volumeLevel = VolumeLevel::VOL_LOW;  break;
        case VolumeLevel::VOL_LOW:  volumeLevel = VolumeLevel::VOL_MUTE; break;
        case VolumeLevel::VOL_MUTE: volumeLevel = VolumeLevel::VOL_HIGH; break;
      }
      playClickSound();
      needsFullRedraw = true;
      return true;
    case SimpleMenuItem::SWITCH_APP:
      switchToLauncher();
      return true;
    case SimpleMenuItem::TURN_OFF:
      turnOff();
      return true;
  }
  return false;
}

// Cycle volume (used by pomodoro menu too)
inline void cycleVolume() {
  switch (volumeLevel) {
    case VolumeLevel::VOL_HIGH: volumeLevel = VolumeLevel::VOL_MED;  break;
    case VolumeLevel::VOL_MED:  volumeLevel = VolumeLevel::VOL_LOW;  break;
    case VolumeLevel::VOL_LOW:  volumeLevel = VolumeLevel::VOL_MUTE; break;
    case VolumeLevel::VOL_MUTE: volumeLevel = VolumeLevel::VOL_HIGH; break;
  }
  playClickSound();
}
