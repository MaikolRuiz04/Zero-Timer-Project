#pragma once
#include "common.h"

// =============================================================================
// Stopwatch App - Count Up with Lap History
// =============================================================================

enum class SWState { RESET, RUNNING, STOPPED, MENU };
enum class SWMenuItem { VOLUME, LAP_RESET, CLEAR_LAPS, SWITCH_APP, TURN_OFF };

static constexpr int SW_MAX_LAPS  = 20;
static constexpr int SW_MENU_COUNT = 5;

// --- State ---
static SWState  swState      = SWState::RESET;
static uint32_t swStartMs    = 0;
static uint32_t swAccumMs    = 0;
static uint32_t swLapStartMs = 0;
static int      swLapCount   = 0;
static uint32_t swLapTimes[SW_MAX_LAPS] = {0};
static bool     swLapReset   = true;  // Reset timer on lap (default: on)

// --- Menu ---
static SWMenuItem swMenuSel = SWMenuItem::VOLUME;

// --- Display cache ---
static SWState lastSWState = SWState::RESET;

// --- Helpers ---

static uint32_t swGetElapsed() {
  uint32_t total = swAccumMs;
  if (swState == SWState::RUNNING) total += millis() - swStartMs;
  return total;
}

static void swPrintLapTime(uint32_t ms) {
  int32_t s = ms / 1000;
  int32_t t = (ms / 100) % 10;
  tft.print(s / 60);
  tft.print(":");
  if (s % 60 < 10) tft.print("0");
  tft.print(s % 60);
  tft.print(".");
  tft.print(t);
}

// --- Draw Timer Screen ---

static void drawSWTimerScreen() {
  uint16_t accent = COLOR_ACCENT_STOPWATCH;
  if (swState == SWState::STOPPED) accent = COLOR_ACCENT_PAUSE;

  if (needsFullRedraw || lastSWState != swState || dispApp != ActiveApp::STOPWATCH) {
    tft.fillScreen(COLOR_BG);

    // Title
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(12, 12);
    tft.print("STOPWATCH");

    // Mode card
    tft.fillRoundRect(10, 35, 300, 50, 8, COLOR_SURFACE);
    drawStopwatchIcon(24, 45, accent);

    tft.setTextColor(COLOR_TEXT, COLOR_SURFACE);
    tft.setTextSize(2);
    tft.setCursor(70, 43);
    switch (swState) {
      case SWState::RESET:   tft.print("Ready");   break;
      case SWState::RUNNING: tft.print("Running"); break;
      case SWState::STOPPED: tft.print("Stopped"); break;
      default: break;
    }

    // Lap count in card subtitle
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_SURFACE);
    tft.setTextSize(1);
    tft.setCursor(70, 65);
    if (swLapCount > 0) {
      tft.print(swLapCount);
      tft.print(swLapCount == 1 ? " lap" : " laps");
    } else if (swState == SWState::RUNNING) {
      tft.print("Rotate for laps");
    }

    // Status icon
    if (swState == SWState::RUNNING) drawPauseIcon(270, 50, accent);
    else drawPlayIcon(270, 50, accent);

    // Lap history list (show last 3, between time and hints)
    if (swLapCount > 0) {
      int showCount = (swLapCount < 3) ? swLapCount : 3;
      for (int i = 0; i < showCount; i++) {
        int lapIdx = swLapCount - 1 - i;  // most recent first
        int16_t ly = 174 + i * 12;
        tft.setTextColor(i == 0 ? accent : COLOR_TEXT_DIM, COLOR_BG);
        tft.setTextSize(1);
        tft.setCursor(85, ly);
        tft.print("L");
        if (lapIdx + 1 < 10) tft.print(" ");
        tft.print(lapIdx + 1);
        tft.print("  ");
        swPrintLapTime(swLapTimes[lapIdx]);
      }
    }

    // Hints
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(65, 215);
    switch (swState) {
      case SWState::RESET:   tft.print("click: start");                  break;
      case SWState::RUNNING: tft.print("click: stop   rotate: lap");     break;
      case SWState::STOPPED: tft.print("click: resume  rotate: reset");  break;
      default: break;
    }
    tft.setCursor(65, 227);
    tft.print("hold: settings");

    lastSWState = swState;
    dispApp = ActiveApp::STOPWATCH;
    needsFullRedraw = false;
    resetDispCache();
  }

  // Time display — show current lap time or total based on setting
  uint32_t elapsed = swGetElapsed();
  uint32_t displayMs = swLapReset ? (elapsed - swLapStartMs) : elapsed;
  int32_t totalSec = displayMs / 1000;
  int32_t tenths = (displayMs / 100) % 10;
  int32_t secs = totalSec % 60;
  int32_t mins = totalSec / 60;
  if (mins > 99) mins = 99;

  // Main MM:SS digits
  if (mins != dispMins || secs != dispSecs || dispTime == -1) {
    int16_t bx = 50, by = 105, cw = 42;
    uint16_t timeColor = (swState == SWState::STOPPED) ? COLOR_ACCENT_PAUSE : COLOR_TEXT;

    tft.setTextColor(timeColor, COLOR_BG);
    tft.setTextSize(7);

    if (mins != dispMins || dispTime == -1) {
      tft.fillRect(bx, by, cw * 2, 50, COLOR_BG);
      tft.setCursor(bx, by);
      if (mins < 10) tft.print('0');
      tft.print(mins);
      dispMins = mins;
    }

    if (dispTime == -1) {
      tft.setCursor(bx + cw * 2, by);
      tft.print(':');
    }

    if (secs != dispSecs || dispTime == -1) {
      tft.fillRect(bx + cw * 3, by, cw * 2, 50, COLOR_BG);
      tft.setCursor(bx + cw * 3, by);
      if (secs < 10) tft.print('0');
      tft.print(secs);
      dispSecs = secs;
    }

    dispTime = totalSec;
  }

  // Tenths digit (smaller, to the right of main time)
  if (tenths != dispTenths || dispTenths == -1) {
    int16_t tx = 262, ty = 133;
    tft.fillRect(tx, ty, 40, 24, COLOR_BG);
    uint16_t tenColor = (swState == SWState::STOPPED) ? COLOR_ACCENT_PAUSE : COLOR_TEXT_DIM;
    tft.setTextColor(tenColor, COLOR_BG);
    tft.setTextSize(3);
    tft.setCursor(tx, ty);
    tft.print('.');
    tft.print(tenths);
    dispTenths = tenths;
  }
}

// --- Stopwatch Menu ---

static void drawSWMenuItemAt(int slot, int itemIdx, bool selected) {
  const int IH = 44, SY = 60;
  int16_t y = SY + slot * IH;
  uint16_t bg = selected ? COLOR_SURFACE : COLOR_BG;
  tft.fillRoundRect(15, y, 290, 40, 8, bg);

  switch (itemIdx) {
    case 0: // Volume
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 12); tft.print("Volume");
      tft.setTextColor(selected ? COLOR_ACCENT_STOPWATCH : COLOR_TEXT_DIM, bg);
      tft.setCursor(220, y + 12);
      switch (volumeLevel) {
        case VolumeLevel::VOL_MUTE: tft.print("Mute"); break;
        case VolumeLevel::VOL_LOW:  tft.print("Low");  break;
        case VolumeLevel::VOL_MED:  tft.print("Med");  break;
        case VolumeLevel::VOL_HIGH: tft.print("High"); break;
      }
      break;
    case 1: // Lap Reset
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 12); tft.print("Lap Reset");
      tft.setTextColor(swLapReset ? COLOR_ACCENT_STOPWATCH : COLOR_TEXT_DIM, bg);
      tft.setCursor(240, y + 12); tft.print(swLapReset ? "On" : "Off");
      break;
    case 2: // Clear Laps
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 12); tft.print("Clear Laps");
      tft.setTextColor(selected ? COLOR_ACCENT_STOPWATCH : COLOR_TEXT_DIM, bg);
      tft.setCursor(250, y + 12); tft.print(swLapCount);
      break;
    case 3: // Switch App
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(100, y + 12); tft.print("Switch App");
      break;
    case 4: // Turn Off
      tft.setTextColor(COLOR_RED, bg);
      tft.setTextSize(2); tft.setCursor(115, y + 12); tft.print("Turn Off");
      break;
  }
}

static void drawSWMenuScreen() {
  int sel = (int)swMenuSel;
  static int swMenuScroll = 0;

  // Scrolling: 3 visible items out of 5
  if (sel < swMenuScroll) swMenuScroll = sel;
  else if (sel >= swMenuScroll + 3) swMenuScroll = sel - 2;

  bool scrollChanged = (swMenuScroll != dispMenuScroll);

  if (needsFullRedraw || dispApp != ActiveApp::STOPWATCH || scrollChanged) {
    tft.fillScreen(COLOR_BG);

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextSize(2); tft.setCursor(110, 20); tft.print("Settings");
    tft.fillRect(20, 50, 280, 1, COLOR_SURFACE);

    if (swMenuScroll > 0)
      tft.fillTriangle(160, 55, 155, 58, 165, 58, COLOR_TEXT_DIM);
    if (swMenuScroll + 3 < SW_MENU_COUNT)
      tft.fillTriangle(160, 220, 155, 217, 165, 217, COLOR_TEXT_DIM);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1); tft.setCursor(80, 228);
    tft.print("click: change  hold: back");

    for (int s = 0; s < 3; s++) {
      int idx = swMenuScroll + s;
      if (idx < SW_MENU_COUNT) drawSWMenuItemAt(s, idx, idx == sel);
    }

    dispApp = ActiveApp::STOPWATCH;
    dispMenuScroll = swMenuScroll;
    needsFullRedraw = false;
    dispMenuSel = sel;
  }
  else if (sel != dispMenuSel) {
    int prev = dispMenuSel;
    for (int s = 0; s < 3; s++) {
      int idx = swMenuScroll + s;
      if (idx < SW_MENU_COUNT && (idx == sel || idx == prev))
        drawSWMenuItemAt(s, idx, idx == sel);
    }
    dispMenuSel = sel;
  }
}

// --- Input ---

static void swEnterMenu() {
  // Pause if running
  if (swState == SWState::RUNNING) {
    swAccumMs += millis() - swStartMs;
    swState = SWState::STOPPED;
  }
  swState = SWState::MENU;
  swMenuSel = SWMenuItem::VOLUME;
  needsFullRedraw = true;
  playMenuSound();
}

static void swExitMenu() {
  if (swAccumMs > 0) swState = SWState::STOPPED;
  else swState = SWState::RESET;
  needsFullRedraw = true;
}

static void swMenuClick() {
  switch (swMenuSel) {
    case SWMenuItem::VOLUME:
      cycleVolume();
      needsFullRedraw = true;
      break;
    case SWMenuItem::LAP_RESET:
      swLapReset = !swLapReset;
      playClickSound();
      needsFullRedraw = true;
      break;
    case SWMenuItem::CLEAR_LAPS:
      swLapCount = 0;
      swLapStartMs = 0;
      memset(swLapTimes, 0, sizeof(swLapTimes));
      playClickSound();
      needsFullRedraw = true;
      break;
    case SWMenuItem::SWITCH_APP:
      switchToLauncher();
      break;
    case SWMenuItem::TURN_OFF:
      turnOff();
      break;
  }
}

static void swHandleEncoder(int32_t delta) {
  if (swState == SWState::RUNNING) {
    // Record lap
    uint32_t elapsed = swGetElapsed();
    uint32_t lapTime = elapsed - swLapStartMs;
    if (swLapCount < SW_MAX_LAPS) {
      swLapTimes[swLapCount] = lapTime;
    }
    swLapStartMs = elapsed;
    swLapCount++;
    needsFullRedraw = true;
    playLapSound();
  } else if (swState == SWState::STOPPED) {
    // Reset
    swAccumMs = 0;
    swLapStartMs = 0;
    swLapCount = 0;
    memset(swLapTimes, 0, sizeof(swLapTimes));
    swState = SWState::RESET;
    needsFullRedraw = true;
    playClickSound();
  } else if (swState == SWState::MENU) {
    int idx = (int)swMenuSel;
    if (delta > 0) { idx++; if (idx >= SW_MENU_COUNT) idx = 0; }
    else            { idx--; if (idx < 0) idx = SW_MENU_COUNT - 1; }
    swMenuSel = (SWMenuItem)idx;
  }
}

static void swHandleClick() {
  if (swState == SWState::MENU) {
    swMenuClick();
    return;
  }

  switch (swState) {
    case SWState::RESET:
      swStartMs = millis();
      swAccumMs = 0;
      swLapStartMs = 0;
      swLapCount = 0;
      memset(swLapTimes, 0, sizeof(swLapTimes));
      swState = SWState::RUNNING;
      playClickSound();
      needsFullRedraw = true;
      break;
    case SWState::RUNNING:
      swAccumMs += millis() - swStartMs;
      swState = SWState::STOPPED;
      playClickSound();
      needsFullRedraw = true;
      break;
    case SWState::STOPPED:
      swStartMs = millis();
      swState = SWState::RUNNING;
      playClickSound();
      needsFullRedraw = true;
      break;
    default: break;
  }
}

static void swHandleLongPress() {
  if (swState == SWState::MENU) swExitMenu();
  else swEnterMenu();
}

static void swDraw() {
  if (swState == SWState::MENU) drawSWMenuScreen();
  else drawSWTimerScreen();
}
