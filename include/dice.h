#pragma once
#include "common.h"

// =============================================================================
// Dice Roller App
// =============================================================================

enum class DiceAppState { IDLE, MENU };

static constexpr int DICE_TYPE_COUNT = 8;
static const int diceMax[] = {2, 4, 6, 8, 10, 12, 20, 100};
static const char* diceNames[] = {"d2", "d4", "d6", "d8", "d10", "d12", "d20", "d100"};
static const char* diceDescs[] = {"Coin flip", "4-sided", "6-sided", "8-sided", "10-sided", "12-sided", "20-sided", "100-sided"};

// --- State ---
static DiceAppState diceState  = DiceAppState::IDLE;
static int          diceType   = 2;      // Index into arrays (default d6)
static int32_t      diceResult = 0;      // 0 = no result yet

// --- Menu ---
static int diceMenuSel = 0;
static constexpr int DICE_MENU_COUNT = 3;

// --- Display cache ---
static int lastDiceType   = -1;
static int32_t lastDiceResult = -1;

// --- Rolling Animation ---

static void diceRollAnimation() {
  int max = diceMax[diceType];

  tft.setTextSize(7);
  for (int i = 0; i < 10; i++) {
    int fake = (esp_random() % max) + 1;

    // Clear and draw fake number
    tft.fillRect(20, 100, 280, 55, COLOR_BG);
    int digits = 1;
    int32_t temp = fake;
    while (temp >= 10) { digits++; temp /= 10; }
    int16_t bx = (320 - digits * 42) / 2;

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setCursor(bx, 105);
    tft.print(fake);

    delay(40 + i * 12);  // Gradually slow down
  }

  // Final result
  diceResult = (esp_random() % max) + 1;
  needsFullRedraw = true;
  playClickSound();
}

// --- Draw ---

static void drawDiceScreen() {
  if (needsFullRedraw || dispApp != ActiveApp::DICE || diceType != lastDiceType) {
    tft.fillScreen(COLOR_BG);

    // Title
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(12, 12);
    tft.print("DICE");

    // Mode card
    tft.fillRoundRect(10, 35, 300, 50, 8, COLOR_SURFACE);
    drawDiceIcon(24, 45, COLOR_ACCENT_DICE);

    tft.setTextColor(COLOR_TEXT, COLOR_SURFACE);
    tft.setTextSize(2);
    tft.setCursor(70, 43);
    tft.print(diceNames[diceType]);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_SURFACE);
    tft.setTextSize(1);
    tft.setCursor(70, 65);
    tft.print(diceDescs[diceType]);
    tft.print("  (1-");
    tft.print(diceMax[diceType]);
    tft.print(")");

    // Encoder arrows (change dice type)
    tft.fillTriangle(30, 135, 38, 125, 38, 145, COLOR_TEXT_DIM);
    tft.fillTriangle(290, 135, 282, 125, 282, 145, COLOR_TEXT_DIM);

    // Hints
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(65, 215);
    tft.print("rotate: dice type  click: roll");
    tft.setCursor(65, 227);
    tft.print("hold: settings");

    dispApp = ActiveApp::DICE;
    lastDiceType = diceType;
    needsFullRedraw = false;
    lastDiceResult = -1;
  }

  // Result display (big number, centered)
  if (diceResult != lastDiceResult) {
    tft.fillRect(20, 100, 280, 55, COLOR_BG);

    if (diceResult > 0) {
      int digits = 1;
      int32_t temp = diceResult;
      while (temp >= 10) { digits++; temp /= 10; }
      int16_t bx = (320 - digits * 42) / 2;

      tft.setTextColor(COLOR_ACCENT_DICE, COLOR_BG);
      tft.setTextSize(7);
      tft.setCursor(bx, 105);
      tft.print(diceResult);
    } else {
      // No result yet — show dash
      tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
      tft.setTextSize(7);
      tft.setCursor(139, 105);
      tft.print("-");
    }

    lastDiceResult = diceResult;
  }
}

// --- Dice Menu ---

static void drawDiceMenuItemAt(int idx, bool selected) {
  const int IH = 50, SY = 60;
  int16_t y = SY + idx * IH;
  uint16_t bg = selected ? COLOR_SURFACE : COLOR_BG;
  tft.fillRoundRect(15, y, 290, 44, 8, bg);

  switch (idx) {
    case 0: // Volume
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 14); tft.print("Volume");
      tft.setTextColor(selected ? COLOR_ACCENT_DICE : COLOR_TEXT_DIM, bg);
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

static void drawDiceMenuScreen() {
  int sel = diceMenuSel;

  if (needsFullRedraw || dispApp != ActiveApp::DICE) {
    tft.fillScreen(COLOR_BG);

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextSize(2); tft.setCursor(110, 20); tft.print("Settings");
    tft.fillRect(20, 50, 280, 1, COLOR_SURFACE);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1); tft.setCursor(80, 228);
    tft.print("click: change  hold: back");

    for (int i = 0; i < DICE_MENU_COUNT; i++) {
      drawDiceMenuItemAt(i, i == sel);
    }

    dispApp = ActiveApp::DICE;
    needsFullRedraw = false;
    dispMenuSel = sel;
  }
  else if (sel != dispMenuSel) {
    for (int i = 0; i < DICE_MENU_COUNT; i++) {
      if (i == sel || i == dispMenuSel)
        drawDiceMenuItemAt(i, i == sel);
    }
    dispMenuSel = sel;
  }
}

// --- Input ---

static void diceEnterMenu() {
  diceState = DiceAppState::MENU;
  diceMenuSel = 0;
  needsFullRedraw = true;
  playMenuSound();
}

static void diceExitMenu() {
  diceState = DiceAppState::IDLE;
  needsFullRedraw = true;
}

static void diceMenuClick() {
  switch (diceMenuSel) {
    case 0: // Volume
      cycleVolume();
      needsFullRedraw = true;
      break;
    case 1: // Switch App
      switchToLauncher();
      break;
    case 2: // Turn Off
      turnOff();
      break;
  }
}

static void diceHandleEncoder(int32_t delta) {
  if (diceState == DiceAppState::MENU) {
    diceMenuSel += (delta > 0) ? 1 : -1;
    if (diceMenuSel >= DICE_MENU_COUNT) diceMenuSel = 0;
    if (diceMenuSel < 0) diceMenuSel = DICE_MENU_COUNT - 1;
  } else {
    diceType -= (delta > 0) ? 1 : -1;
    if (diceType >= DICE_TYPE_COUNT) diceType = 0;
    if (diceType < 0) diceType = DICE_TYPE_COUNT - 1;
    diceResult = 0;  // Clear result when changing type
    needsFullRedraw = true;
  }
}

static void diceHandleClick() {
  if (diceState == DiceAppState::MENU) {
    diceMenuClick();
    return;
  }
  diceRollAnimation();
}

static void diceHandleLongPress() {
  if (diceState == DiceAppState::MENU) diceExitMenu();
  else diceEnterMenu();
}

static void diceDraw() {
  if (diceState == DiceAppState::MENU) drawDiceMenuScreen();
  else drawDiceScreen();
}
