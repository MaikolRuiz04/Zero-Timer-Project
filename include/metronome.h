#pragma once
#include "common.h"

// =============================================================================
// Metronome App - BPM Beat Keeper with Visual & Audio
// =============================================================================

enum class MetroState { IDLE, PLAYING, MENU };

// --- State ---
static MetroState metroState   = MetroState::IDLE;
static int32_t    metroBPM     = 120;
static int        metroBeat    = 0;       // Current beat (0-3, 4/4 time)
static uint32_t   metroLastBeat = 0;
static bool       metroBeatFlash = false;
static uint32_t   metroBeatFlashMs = 0;

// Buzzer management (non-blocking)
static bool       metroBuzzing    = false;
static uint32_t   metroBuzzStart  = 0;
static constexpr uint32_t METRO_TICK_MS = 12;  // Buzz duration

// --- Menu ---
static int metroMenuSel = 0;
static constexpr int METRO_MENU_COUNT = 3;

// --- Display cache ---
static int32_t lastMetroBPM  = -1;
static int     lastMetroBeat = -1;
static bool    lastMetroFlash = false;

// --- Beat Engine (non-blocking, called from loop) ---

static void metroTick() {
  uint32_t now = millis();

  // Stop buzzer after tick duration
  if (metroBuzzing && now - metroBuzzStart >= METRO_TICK_MS) {
    ledcWriteTone(BUZZER_CHANNEL, 0);
    metroBuzzing = false;
  }

  if (metroState != MetroState::PLAYING) return;

  uint32_t interval = 60000 / metroBPM;

  if (now - metroLastBeat >= interval) {
    metroLastBeat += interval;

    // Advance beat
    metroBeat = (metroBeat + 1) % 4;
    metroBeatFlash = true;
    metroBeatFlashMs = now;

    // Play tick (non-blocking) — accent on beat 1
    if (volumeLevel != VolumeLevel::VOL_MUTE) {
      int freq = (metroBeat == 0) ? 1200 : 800;
      uint8_t duty = 0;
      switch (volumeLevel) {
        case VolumeLevel::VOL_LOW:  duty = 32;  break;
        case VolumeLevel::VOL_MED:  duty = 64;  break;
        case VolumeLevel::VOL_HIGH: duty = 128; break;
        default: break;
      }
      ledcWriteTone(BUZZER_CHANNEL, freq);
      ledcWrite(BUZZER_CHANNEL, duty);
      metroBuzzing = true;
      metroBuzzStart = now;
    }
  }

  // Clear flash after 100ms
  if (metroBeatFlash && now - metroBeatFlashMs >= 100) {
    metroBeatFlash = false;
  }
}

// --- Draw ---

static void drawMetroBeatIndicators() {
  const int16_t cy = 180, spacing = 36;
  int16_t startX = 160 - (spacing * 3) / 2;

  for (int i = 0; i < 4; i++) {
    int16_t cx = startX + i * spacing;
    bool active = (metroState == MetroState::PLAYING && metroBeatFlash && i == metroBeat);
    bool current = (metroState == MetroState::PLAYING && i == metroBeat && !metroBeatFlash);

    if (active) {
      // Big filled circle on beat
      tft.fillCircle(cx, cy, 10, COLOR_ACCENT_METRO);
    } else if (i == 0) {
      // Beat 1 accent marker (filled small)
      tft.fillCircle(cx, cy, 5, current ? COLOR_TEXT_DIM : COLOR_SURFACE);
      tft.drawCircle(cx, cy, 10, COLOR_SURFACE);
    } else {
      // Regular beat marker
      tft.fillCircle(cx, cy, 4, current ? COLOR_TEXT_DIM : COLOR_SURFACE);
      tft.drawCircle(cx, cy, 10, COLOR_SURFACE);
    }
  }
}

static void drawMetroScreen() {
  bool stateChanged = needsFullRedraw || dispApp != ActiveApp::METRONOME;

  if (stateChanged) {
    tft.fillScreen(COLOR_BG);

    // Title
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(12, 12);
    tft.print("METRONOME");

    // Mode card
    tft.fillRoundRect(10, 35, 300, 50, 8, COLOR_SURFACE);
    drawMetronomeIcon(24, 45, COLOR_ACCENT_METRO);

    tft.setTextColor(COLOR_TEXT, COLOR_SURFACE);
    tft.setTextSize(2);
    tft.setCursor(70, 43);
    tft.print(metroState == MetroState::PLAYING ? "Playing" : "Stopped");

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_SURFACE);
    tft.setTextSize(1);
    tft.setCursor(70, 65);
    tft.print("4/4 time");

    // Status icon
    if (metroState == MetroState::PLAYING) drawPauseIcon(270, 50, COLOR_ACCENT_METRO);
    else drawPlayIcon(270, 50, COLOR_ACCENT_METRO);

    // Encoder arrows
    tft.fillTriangle(30, 125, 38, 115, 38, 135, COLOR_TEXT_DIM);
    tft.fillTriangle(290, 125, 282, 115, 282, 135, COLOR_TEXT_DIM);

    // Hints
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(65, 215);
    tft.print("rotate: BPM  click: start/stop");
    tft.setCursor(65, 227);
    tft.print("hold: settings");

    dispApp = ActiveApp::METRONOME;
    needsFullRedraw = false;
    lastMetroBPM = -1;
    lastMetroBeat = -1;
    lastMetroFlash = false;
  }

  // BPM display (big number, centered)
  if (metroBPM != lastMetroBPM) {
    tft.fillRect(20, 95, 280, 55, COLOR_BG);

    // Calculate centering
    int digits = 1;
    int32_t temp = metroBPM;
    while (temp >= 10) { digits++; temp /= 10; }

    // BPM number + "BPM" label
    int16_t numW = digits * 42;
    int16_t labelW = 3 * 18;  // "BPM" at size 3 = ~54px
    int16_t totalW = numW + 10 + labelW;
    int16_t bx = (320 - totalW) / 2;

    tft.setTextColor(COLOR_ACCENT_METRO, COLOR_BG);
    tft.setTextSize(7);
    tft.setCursor(bx, 100);
    tft.print(metroBPM);

    // "BPM" label (smaller, right of number)
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(2);
    tft.setCursor(bx + numW + 8, 120);
    tft.print("bpm");

    lastMetroBPM = metroBPM;
  }

  // Beat indicators
  if (metroBeat != lastMetroBeat || metroBeatFlash != lastMetroFlash || stateChanged) {
    // Clear indicator area
    tft.fillRect(20, 165, 280, 35, COLOR_BG);
    drawMetroBeatIndicators();
    lastMetroBeat = metroBeat;
    lastMetroFlash = metroBeatFlash;
  }
}

// --- Metronome Menu ---

static void drawMetroMenuItemAt(int idx, bool selected) {
  const int IH = 50, SY = 60;
  int16_t y = SY + idx * IH;
  uint16_t bg = selected ? COLOR_SURFACE : COLOR_BG;
  tft.fillRoundRect(15, y, 290, 44, 8, bg);

  switch (idx) {
    case 0: // Volume
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 14); tft.print("Volume");
      tft.setTextColor(selected ? COLOR_ACCENT_METRO : COLOR_TEXT_DIM, bg);
      tft.setCursor(220, y + 14);
      switch (volumeLevel) {
        case VolumeLevel::VOL_MUTE: tft.print("Mute"); break;
        case VolumeLevel::VOL_LOW:  tft.print("Low");  break;
        case VolumeLevel::VOL_MED:  tft.print("Med");  break;
        case VolumeLevel::VOL_HIGH: tft.print("High"); break;
      }
      break;
    case 1: // Switch App
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(100, y + 14); tft.print("Switch App");
      break;
    case 2: // Turn Off
      tft.setTextColor(COLOR_RED, bg);
      tft.setTextSize(2); tft.setCursor(115, y + 14); tft.print("Turn Off");
      break;
  }
}

static void drawMetroMenuScreen() {
  int sel = metroMenuSel;

  if (needsFullRedraw || dispApp != ActiveApp::METRONOME) {
    tft.fillScreen(COLOR_BG);

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextSize(2); tft.setCursor(110, 20); tft.print("Settings");
    tft.fillRect(20, 50, 280, 1, COLOR_SURFACE);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1); tft.setCursor(80, 228);
    tft.print("click: change  hold: back");

    for (int i = 0; i < METRO_MENU_COUNT; i++) {
      drawMetroMenuItemAt(i, i == sel);
    }

    dispApp = ActiveApp::METRONOME;
    needsFullRedraw = false;
    dispMenuSel = sel;
  }
  else if (sel != dispMenuSel) {
    for (int i = 0; i < METRO_MENU_COUNT; i++) {
      if (i == sel || i == dispMenuSel)
        drawMetroMenuItemAt(i, i == sel);
    }
    dispMenuSel = sel;
  }
}

// --- Input ---

static void metroEnterMenu() {
  // Stop playing when entering menu
  if (metroState == MetroState::PLAYING) {
    ledcWriteTone(BUZZER_CHANNEL, 0);
    metroBuzzing = false;
  }
  metroState = MetroState::MENU;
  metroMenuSel = 0;
  needsFullRedraw = true;
  playMenuSound();
}

static void metroExitMenu() {
  metroState = MetroState::IDLE;
  needsFullRedraw = true;
}

static void metroMenuClick() {
  switch (metroMenuSel) {
    case 0: // Volume
      cycleVolume();
      needsFullRedraw = true;
      break;
    case 1: // Switch App
      if (metroState == MetroState::PLAYING) {
        ledcWriteTone(BUZZER_CHANNEL, 0);
        metroBuzzing = false;
      }
      switchToLauncher();
      break;
    case 2: // Turn Off
      if (metroState == MetroState::PLAYING) {
        ledcWriteTone(BUZZER_CHANNEL, 0);
        metroBuzzing = false;
      }
      turnOff();
      break;
  }
}

static void metroHandleEncoder(int32_t delta) {
  if (metroState == MetroState::MENU) {
    metroMenuSel += (delta > 0) ? 1 : -1;
    if (metroMenuSel >= METRO_MENU_COUNT) metroMenuSel = 0;
    if (metroMenuSel < 0) metroMenuSel = METRO_MENU_COUNT - 1;
  } else {
    metroBPM -= delta;
    if (metroBPM < 20)  metroBPM = 20;
    if (metroBPM > 300) metroBPM = 300;
  }
}

static void metroHandleClick() {
  if (metroState == MetroState::MENU) {
    metroMenuClick();
    return;
  }

  if (metroState == MetroState::PLAYING) {
    // Stop
    metroState = MetroState::IDLE;
    ledcWriteTone(BUZZER_CHANNEL, 0);
    metroBuzzing = false;
    metroBeatFlash = false;
    playClickSound();
    needsFullRedraw = true;
  } else {
    // Start
    metroState = MetroState::PLAYING;
    metroBeat = -1;  // Will advance to 0 on first tick
    metroLastBeat = millis();
    metroBeatFlash = false;
    playClickSound();
    needsFullRedraw = true;
  }
}

static void metroHandleLongPress() {
  if (metroState == MetroState::MENU) metroExitMenu();
  else metroEnterMenu();
}

static void metroDraw() {
  if (metroState == MetroState::MENU) drawMetroMenuScreen();
  else drawMetroScreen();
}
