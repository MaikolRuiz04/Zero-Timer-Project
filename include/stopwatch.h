#pragma once
#include "common.h"

// =============================================================================
// Stopwatch App - Count Up with Laps
// =============================================================================

enum class SWState { RESET, RUNNING, STOPPED, MENU };

// --- State ---
static SWState  swState      = SWState::RESET;
static uint32_t swStartMs    = 0;
static uint32_t swAccumMs    = 0;
static uint32_t swLapStartMs = 0;
static uint32_t swLastLapMs  = 0;
static int      swLapCount   = 0;

// --- Display cache ---
static SWState lastSWState = SWState::RESET;

// --- Helpers ---

static uint32_t swGetElapsed() {
  uint32_t total = swAccumMs;
  if (swState == SWState::RUNNING) total += millis() - swStartMs;
  return total;
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

    // Lap info in card subtitle
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_SURFACE);
    tft.setTextSize(1);
    tft.setCursor(70, 65);
    if (swLapCount > 0) {
      tft.print("Lap ");
      tft.print(swLapCount);
      tft.print(": ");
      uint32_t lapMs = swLastLapMs;
      int32_t lapSec = lapMs / 1000;
      int32_t lapTen = (lapMs / 100) % 10;
      tft.print(lapSec / 60);
      tft.print(":");
      if (lapSec % 60 < 10) tft.print("0");
      tft.print(lapSec % 60);
      tft.print(".");
      tft.print(lapTen);
    } else if (swState == SWState::RUNNING) {
      tft.print("Rotate for laps");
    }

    // Status icon
    if (swState == SWState::RUNNING) drawPauseIcon(270, 50, accent);
    else drawPlayIcon(270, 50, accent);

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

  // Time display
  uint32_t elapsed = swGetElapsed();
  int32_t totalSec = elapsed / 1000;
  int32_t tenths = (elapsed / 100) % 10;
  int32_t secs = totalSec % 60;
  int32_t mins = totalSec / 60;
  if (mins > 99) mins = 99;

  // Main MM:SS digits
  if (mins != dispMins || secs != dispSecs || dispTime == -1) {
    int16_t bx = 50, by = 115, cw = 42;
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
    int16_t tx = 262, ty = 143;
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

// --- Input ---

static void swEnterMenu() {
  // Pause if running
  if (swState == SWState::RUNNING) {
    swAccumMs += millis() - swStartMs;
    swState = SWState::STOPPED;
  }
  swState = SWState::MENU;
  simpleMenuSel = SimpleMenuItem::VOLUME;
  needsFullRedraw = true;
  playMenuSound();
}

static void swExitMenu() {
  if (swAccumMs > 0) swState = SWState::STOPPED;
  else swState = SWState::RESET;
  needsFullRedraw = true;
}

static void swHandleEncoder(int32_t delta) {
  if (swState == SWState::RUNNING) {
    // Record lap
    uint32_t elapsed = swGetElapsed();
    swLastLapMs = elapsed - swLapStartMs;
    swLapStartMs = elapsed;
    swLapCount++;
    needsFullRedraw = true;
    playLapSound();
  } else if (swState == SWState::STOPPED) {
    // Reset
    swAccumMs = 0;
    swLapStartMs = 0;
    swLastLapMs = 0;
    swLapCount = 0;
    swState = SWState::RESET;
    needsFullRedraw = true;
    playClickSound();
  } else if (swState == SWState::MENU) {
    handleSimpleMenuEncoder(delta);
  }
}

static void swHandleClick() {
  if (swState == SWState::MENU) {
    handleSimpleMenuClick();
    return;
  }

  switch (swState) {
    case SWState::RESET:
      swStartMs = millis();
      swAccumMs = 0;
      swLapStartMs = 0;
      swLastLapMs = 0;
      swLapCount = 0;
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
  if (swState == SWState::MENU) drawSimpleMenu("Settings");
  else drawSWTimerScreen();
}
