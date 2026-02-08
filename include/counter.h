#pragma once
#include "common.h"

// =============================================================================
// Counter / Tally App
// =============================================================================

enum class CTState { COUNTING, MENU };
enum class CTMenuItem { VOLUME, RESET_COUNT, SWITCH_APP, TURN_OFF };

static constexpr int CT_MENU_COUNT = 4;

// --- State ---
static CTState    ctState   = CTState::COUNTING;
static int32_t    ctCount   = 0;
static CTMenuItem ctMenuSel = CTMenuItem::VOLUME;

// --- Display cache ---
static int32_t lastCtCount = -1;

// --- Draw Counter Screen ---

static void drawCTScreen() {
  if (needsFullRedraw || dispApp != ActiveApp::COUNTER) {
    tft.fillScreen(COLOR_BG);

    // Title
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(12, 12);
    tft.print("COUNTER");

    // Mode card
    tft.fillRoundRect(10, 35, 300, 50, 8, COLOR_SURFACE);
    drawCounterIcon(24, 45, COLOR_ACCENT_COUNTER);

    tft.setTextColor(COLOR_TEXT, COLOR_SURFACE);
    tft.setTextSize(2);
    tft.setCursor(70, 43);
    tft.print("Tally Counter");

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_SURFACE);
    tft.setTextSize(1);
    tft.setCursor(70, 65);
    tft.print("Click or rotate to count");

    // +/- arrows
    tft.fillTriangle(30, 135, 38, 125, 38, 145, COLOR_TEXT_DIM);
    tft.fillTriangle(290, 135, 282, 125, 282, 145, COLOR_TEXT_DIM);

    // Hints
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(65, 215);
    tft.print("rotate: +/- 1  click: +1");
    tft.setCursor(65, 227);
    tft.print("hold: settings");

    dispApp = ActiveApp::COUNTER;
    needsFullRedraw = false;
    lastCtCount = -1;
  }

  // Counter display (big number, centered)
  if (ctCount != lastCtCount) {
    // Clear number area
    tft.fillRect(20, 100, 280, 55, COLOR_BG);

    // Calculate digit count for centering
    int digits = 1;
    int32_t temp = ctCount;
    while (temp >= 10) { digits++; temp /= 10; }

    int16_t cw = 42;  // char width at size 7
    int16_t bx = (320 - digits * cw) / 2;

    tft.setTextColor(COLOR_ACCENT_COUNTER, COLOR_BG);
    tft.setTextSize(7);
    tft.setCursor(bx, 105);
    tft.print(ctCount);

    lastCtCount = ctCount;
  }
}

// --- Counter Menu ---

static void drawCTMenuItemAt(int idx, bool selected) {
  const int IH = 44, SY = 60;
  int16_t y = SY + idx * IH;
  uint16_t bg = selected ? COLOR_SURFACE : COLOR_BG;
  tft.fillRoundRect(15, y, 290, 40, 8, bg);

  switch (idx) {
    case 0: // Volume
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 12); tft.print("Volume");
      tft.setTextColor(selected ? COLOR_ACCENT_COUNTER : COLOR_TEXT_DIM, bg);
      tft.setCursor(220, y + 12);
      switch (volumeLevel) {
        case VolumeLevel::VOL_MUTE: tft.print("Mute"); break;
        case VolumeLevel::VOL_LOW:  tft.print("Low");  break;
        case VolumeLevel::VOL_MED:  tft.print("Med");  break;
        case VolumeLevel::VOL_HIGH: tft.print("High"); break;
      }
      break;
    case 1: // Reset Counter
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 12); tft.print("Reset Count");
      tft.setTextColor(selected ? COLOR_ACCENT_COUNTER : COLOR_TEXT_DIM, bg);
      tft.setCursor(250, y + 12); tft.print(ctCount);
      break;
    case 2: // Switch App
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(100, y + 12); tft.print("Switch App");
      break;
    case 3: // Turn Off
      tft.setTextColor(COLOR_RED, bg);
      tft.setTextSize(2); tft.setCursor(115, y + 12); tft.print("Turn Off");
      break;
  }
}

static void drawCTMenuScreen() {
  int sel = (int)ctMenuSel;

  if (needsFullRedraw || dispApp != ActiveApp::COUNTER) {
    tft.fillScreen(COLOR_BG);

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextSize(2); tft.setCursor(110, 20); tft.print("Settings");
    tft.fillRect(20, 50, 280, 1, COLOR_SURFACE);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1); tft.setCursor(80, 228);
    tft.print("click: change  hold: back");

    for (int i = 0; i < CT_MENU_COUNT; i++) {
      drawCTMenuItemAt(i, i == sel);
    }

    dispApp = ActiveApp::COUNTER;
    needsFullRedraw = false;
    dispMenuSel = sel;
  }
  else if (sel != dispMenuSel) {
    for (int i = 0; i < CT_MENU_COUNT; i++) {
      if (i == sel || i == dispMenuSel)
        drawCTMenuItemAt(i, i == sel);
    }
    dispMenuSel = sel;
  }
}

// --- Input ---

static void ctEnterMenu() {
  ctState = CTState::MENU;
  ctMenuSel = CTMenuItem::VOLUME;
  needsFullRedraw = true;
  playMenuSound();
}

static void ctExitMenu() {
  ctState = CTState::COUNTING;
  needsFullRedraw = true;
}

static void ctMenuClick() {
  switch (ctMenuSel) {
    case CTMenuItem::VOLUME:
      cycleVolume();
      needsFullRedraw = true;
      break;
    case CTMenuItem::RESET_COUNT:
      ctCount = 0;
      playClickSound();
      needsFullRedraw = true;
      break;
    case CTMenuItem::SWITCH_APP:
      switchToLauncher();
      break;
    case CTMenuItem::TURN_OFF:
      turnOff();
      break;
  }
}

static void ctHandleEncoder(int32_t delta) {
  if (ctState == CTState::MENU) {
    int idx = (int)ctMenuSel;
    if (delta > 0) { idx++; if (idx >= CT_MENU_COUNT) idx = 0; }
    else            { idx--; if (idx < 0) idx = CT_MENU_COUNT - 1; }
    ctMenuSel = (CTMenuItem)idx;
  } else {
    ctCount -= delta;
    if (ctCount < 0) ctCount = 0;
    if (ctCount > 9999) ctCount = 9999;
  }
}

static void ctHandleClick() {
  if (ctState == CTState::MENU) {
    ctMenuClick();
    return;
  }
  if (ctCount < 9999) ctCount++;
  playClickSound();
}

static void ctHandleLongPress() {
  if (ctState == CTState::MENU) ctExitMenu();
  else ctEnterMenu();
}

static void ctDraw() {
  if (ctState == CTState::MENU) drawCTMenuScreen();
  else drawCTScreen();
}
