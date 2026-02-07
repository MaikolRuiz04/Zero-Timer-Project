#pragma once
#include "common.h"

// =============================================================================
// Quick Timer App - 1-60 Minute Countdown
// =============================================================================

enum class QTState { SETTING, RUNNING, PAUSED, DONE, MENU };

// --- State ---
static QTState  qtState     = QTState::SETTING;
static int32_t  qtSetMin    = 5;
static int32_t  qtRemaining = 0;
static int32_t  qtTotalSec  = 0;
static uint32_t qtLastTick  = 0;

// --- Display cache ---
static QTState lastQTState = QTState::SETTING;

// --- Timer Tick ---

static void qtTick() {
  if (qtState != QTState::RUNNING) return;
  uint32_t now = millis();
  if (now - qtLastTick >= 1000) {
    qtLastTick += 1000;
    if (qtRemaining > 0) qtRemaining--;
    if (qtRemaining == 0) {
      qtState = QTState::DONE;
      needsFullRedraw = true;
      playAlarmMelody();
    }
  }
}

// --- Draw Timer Screen ---

static void drawQTTimerScreen() {
  uint16_t accent = COLOR_ACCENT_QTIMER;
  if (qtState == QTState::PAUSED) accent = COLOR_ACCENT_PAUSE;
  if (qtState == QTState::DONE) accent = COLOR_ACCENT_BREAK;

  if (needsFullRedraw || lastQTState != qtState || dispApp != ActiveApp::QUICK_TIMER) {
    tft.fillScreen(COLOR_BG);

    // Title
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(12, 12);
    tft.print("QUICK TIMER");

    // Mode card
    tft.fillRoundRect(10, 35, 300, 50, 8, COLOR_SURFACE);

    if (qtState == QTState::DONE) {
      drawCheckIcon(24, 45, accent);
    } else {
      drawHourglassIcon(24, 45, accent);
    }

    tft.setTextColor(COLOR_TEXT, COLOR_SURFACE);
    tft.setTextSize(2);
    tft.setCursor(70, 43);
    switch (qtState) {
      case QTState::SETTING: tft.print("Set Timer");      break;
      case QTState::RUNNING: tft.print("Counting Down");  break;
      case QTState::PAUSED:  tft.print("Paused");         break;
      case QTState::DONE:    tft.print("Time's Up!");      break;
      default: break;
    }

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_SURFACE);
    tft.setTextSize(1);
    tft.setCursor(70, 65);
    if (qtState == QTState::SETTING) {
      tft.print("Rotate to set time");
    } else {
      tft.print(qtSetMin);
      tft.print(" min timer");
    }

    // Status icon (right side)
    if (qtState == QTState::RUNNING)  drawPauseIcon(270, 50, accent);
    else if (qtState == QTState::PAUSED) drawPlayIcon(270, 50, accent);
    else if (qtState == QTState::DONE)   drawCheckIcon(270, 45, accent);

    // Encoder arrows in setting mode
    if (qtState == QTState::SETTING) {
      tft.fillTriangle(30, 140, 38, 130, 38, 150, COLOR_TEXT_DIM);
      tft.fillTriangle(290, 140, 282, 130, 282, 150, COLOR_TEXT_DIM);
    }

    // Hints
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(65, 215);
    switch (qtState) {
      case QTState::SETTING: tft.print("click: start timer"); break;
      case QTState::RUNNING: tft.print("click: pause");       break;
      case QTState::PAUSED:  tft.print("click: resume  rotate: +/- 1m"); break;
      case QTState::DONE:    tft.print("click: reset");       break;
      default: break;
    }
    tft.setCursor(65, 227);
    tft.print("hold: settings");

    lastQTState = qtState;
    dispApp = ActiveApp::QUICK_TIMER;
    needsFullRedraw = false;
    resetDispCache();
  }

  // Time display
  int32_t displaySec = (qtState == QTState::SETTING) ? qtSetMin * 60 : qtRemaining;

  if (displaySec != dispTime) {
    int32_t mins = displaySec / 60;
    int32_t secs = displaySec % 60;
    int16_t bx = 50, by = 115, cw = 42;

    tft.setTextSize(7);

    // In setting mode: minutes bright, seconds dim
    // In other modes: all white (or accent for DONE)
    uint16_t minColor = COLOR_TEXT;
    uint16_t secColor = COLOR_TEXT;
    uint16_t colonColor = COLOR_TEXT;

    if (qtState == QTState::SETTING) {
      secColor = COLOR_TEXT_DIM;
      colonColor = COLOR_TEXT_DIM;
    } else if (qtState == QTState::DONE) {
      minColor = accent;
      secColor = accent;
      colonColor = accent;
    }

    if (mins != dispMins || dispTime == -1) {
      tft.fillRect(bx, by, cw * 2, 50, COLOR_BG);
      tft.setTextColor(minColor, COLOR_BG);
      tft.setCursor(bx, by);
      if (mins < 10) tft.print('0');
      tft.print(mins);
      dispMins = mins;
    }

    if (dispTime == -1) {
      tft.setTextColor(colonColor, COLOR_BG);
      tft.setCursor(bx + cw * 2, by);
      tft.print(':');
    }

    if (secs != dispSecs || dispTime == -1) {
      tft.fillRect(bx + cw * 3, by, cw * 2, 50, COLOR_BG);
      tft.setTextColor(secColor, COLOR_BG);
      tft.setCursor(bx + cw * 3, by);
      if (secs < 10) tft.print('0');
      tft.print(secs);
      dispSecs = secs;
    }

    // Progress bar (only when not in setting mode)
    if (qtState != QTState::SETTING && qtTotalSec > 0) {
      int32_t elapsed = qtTotalSec - qtRemaining;
      int16_t pw = (int16_t)((elapsed * 280L) / qtTotalSec);
      tft.fillRoundRect(20, 190, 280, 4, 2, COLOR_SURFACE);
      if (pw > 0) tft.fillRoundRect(20, 190, pw, 4, 2, accent);
    }

    dispTime = displaySec;
  }
}

// --- Input ---

static void qtEnterMenu() {
  qtState = QTState::MENU;
  simpleMenuSel = SimpleMenuItem::VOLUME;
  needsFullRedraw = true;
  playMenuSound();
}

static void qtExitMenu() {
  if (qtRemaining > 0) qtState = QTState::PAUSED;
  else qtState = QTState::SETTING;
  needsFullRedraw = true;
}

static void qtHandleEncoder(int32_t delta) {
  if (qtState == QTState::SETTING) {
    qtSetMin += delta;
    if (qtSetMin < 1)  qtSetMin = 1;
    if (qtSetMin > 60) qtSetMin = 60;
    dispTime = -1;  // Force time redraw
  } else if (qtState == QTState::PAUSED) {
    qtRemaining += delta * 60;
    if (qtRemaining < 1) qtRemaining = 1;
    if (qtRemaining > 3600) qtRemaining = 3600;
    // Adjust total for progress bar
    if (qtTotalSec < qtRemaining) qtTotalSec = qtRemaining;
    dispTime = -1;
    needsFullRedraw = true;
  } else if (qtState == QTState::MENU) {
    handleSimpleMenuEncoder(delta);
  }
}

static void qtHandleClick() {
  if (qtState == QTState::MENU) {
    handleSimpleMenuClick();
    return;
  }

  switch (qtState) {
    case QTState::SETTING:
      qtRemaining = qtSetMin * 60;
      qtTotalSec = qtRemaining;
      qtState = QTState::RUNNING;
      qtLastTick = millis();
      playClickSound();
      needsFullRedraw = true;
      break;
    case QTState::RUNNING:
      qtState = QTState::PAUSED;
      playClickSound();
      needsFullRedraw = true;
      break;
    case QTState::PAUSED:
      qtState = QTState::RUNNING;
      qtLastTick = millis();
      playClickSound();
      needsFullRedraw = true;
      break;
    case QTState::DONE:
      qtState = QTState::SETTING;
      qtRemaining = 0;
      playClickSound();
      needsFullRedraw = true;
      break;
    default: break;
  }
}

static void qtHandleLongPress() {
  if (qtState == QTState::MENU) qtExitMenu();
  else qtEnterMenu();
}

static void qtDraw() {
  if (qtState == QTState::MENU) drawSimpleMenu("Settings");
  else drawQTTimerScreen();
}
