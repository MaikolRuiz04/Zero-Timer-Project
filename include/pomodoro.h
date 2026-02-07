#pragma once
#include "common.h"

// =============================================================================
// Pomodoro Timer App
// =============================================================================

enum class PomState { RUNNING, PAUSED, MENU };
enum class PomTimerMode { FOCUS, BREAK };
enum class PomMenuItem { FOCUS_TIME, BREAK_TIME, AUTO_CONTINUE, VOLUME, STATS, SWITCH_APP, TURN_OFF };

static constexpr int POM_MENU_COUNT   = 7;
static constexpr int POM_MENU_VISIBLE = 3;

// --- State ---
static PomState     pomState   = PomState::PAUSED;
static PomTimerMode pomMode    = PomTimerMode::FOCUS;
static PomMenuItem  pomMenuSel = PomMenuItem::FOCUS_TIME;
static int          pomMenuScroll = 0;

static int32_t  pomFocusSec     = 50 * 60;
static int32_t  pomBreakSec     = 25 * 60;
static bool     pomAutoContinue = true;
static int32_t  pomRemaining    = 50 * 60;
static uint32_t pomLastTick     = 0;
static uint32_t pomTotalFocus   = 0;

// --- Display cache ---
static PomState     lastPomState = PomState::PAUSED;
static PomTimerMode lastPomMode  = PomTimerMode::FOCUS;

// --- Helpers ---

static uint16_t pomAccentColor() {
  if (pomState == PomState::PAUSED) return COLOR_ACCENT_PAUSE;
  return (pomMode == PomTimerMode::FOCUS) ? COLOR_ACCENT_FOCUS : COLOR_ACCENT_BREAK;
}

static void pomSwitchToFocus(bool sound = true) {
  if (pomMode != PomTimerMode::FOCUS) {
    pomMode = PomTimerMode::FOCUS;
    pomRemaining = pomFocusSec;
    needsFullRedraw = true;
    if (sound) playFocusMelody();
  }
}

static void pomSwitchToBreak(bool sound = true) {
  if (pomMode != PomTimerMode::BREAK) {
    pomMode = PomTimerMode::BREAK;
    pomRemaining = pomBreakSec;
    needsFullRedraw = true;
    if (sound) playBreakMelody();
  }
}

static void pomTogglePause() {
  if (pomState == PomState::RUNNING) {
    pomState = PomState::PAUSED;
    playClickSound();
  } else if (pomState == PomState::PAUSED) {
    pomState = PomState::RUNNING;
    pomLastTick = millis();
    playClickSound();
  }
  needsFullRedraw = true;
}

static void pomEnterMenu() {
  pomState = PomState::MENU;
  pomMenuSel = PomMenuItem::FOCUS_TIME;
  pomMenuScroll = 0;
  dispMenuScroll = -1;
  needsFullRedraw = true;
  playMenuSound();
}

static void pomExitMenu() {
  pomState = PomState::PAUSED;
  needsFullRedraw = true;
}

// --- Timer Tick ---

static void pomTick() {
  if (pomState != PomState::RUNNING) return;
  uint32_t now = millis();
  if (now - pomLastTick >= 1000) {
    pomLastTick += 1000;
    if (pomRemaining > 0) {
      pomRemaining--;
      if (pomMode == PomTimerMode::FOCUS) {
        pomTotalFocus++;
        if (pomTotalFocus % 60 == 0) saveSettings();
      }
    }
    if (pomRemaining == 0) {
      if (pomMode == PomTimerMode::FOCUS) pomSwitchToBreak();
      else pomSwitchToFocus();
      if (!pomAutoContinue) pomState = PomState::PAUSED;
      pomLastTick = now;
      needsFullRedraw = true;
    }
  }
}

// --- Draw Timer Screen ---

static void drawPomTimerScreen() {
  uint16_t accent = pomAccentColor();

  if (needsFullRedraw || lastPomState != pomState || lastPomMode != pomMode
      || dispApp != ActiveApp::POMODORO) {
    tft.fillScreen(COLOR_BG);

    // Title
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(12, 12);
    tft.print("POMODORO");

    // Mode card
    tft.fillRoundRect(10, 35, 300, 50, 8, COLOR_SURFACE);

    if (pomMode == PomTimerMode::FOCUS) drawLaptopIcon(24, 48, accent);
    else drawCoffeeIcon(24, 45, accent);

    tft.setTextColor(COLOR_TEXT, COLOR_SURFACE);
    tft.setTextSize(2);
    tft.setCursor(70, 43);
    tft.print(pomMode == PomTimerMode::FOCUS ? "Focus" : "Break");

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_SURFACE);
    tft.setTextSize(1);
    tft.setCursor(70, 65);
    tft.print((pomMode == PomTimerMode::FOCUS ? pomFocusSec : pomBreakSec) / 60);
    tft.print(" min session");

    if (pomState == PomState::PAUSED) drawPlayIcon(270, 50, accent);
    else drawPauseIcon(270, 50, accent);

    // Hints
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(65, 215);
    tft.print("click: play/pause");
    tft.setCursor(65, 227);
    tft.print("hold: settings");

    lastPomState = pomState;
    lastPomMode = pomMode;
    dispApp = ActiveApp::POMODORO;
    needsFullRedraw = false;
    resetDispCache();
  }

  // Timer digits (partial redraw)
  if (pomRemaining != dispTime) {
    int32_t mins = pomRemaining / 60;
    int32_t secs = pomRemaining % 60;
    int16_t bx = 50, by = 115, cw = 42;

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
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

    // Progress bar
    int32_t total = (pomMode == PomTimerMode::FOCUS) ? pomFocusSec : pomBreakSec;
    int32_t elapsed = total - pomRemaining;
    int16_t pw = (int16_t)((elapsed * 280L) / total);
    tft.fillRoundRect(20, 190, 280, 4, 2, COLOR_SURFACE);
    if (pw > 0) tft.fillRoundRect(20, 190, pw, 4, 2, accent);

    dispTime = pomRemaining;
  }
}

// --- Draw Menu ---

static void drawPomMenuItemAt(int slot, int itemIdx, bool selected) {
  const int IH = 50, SY = 60;
  int16_t y = SY + slot * IH;
  uint16_t bg = selected ? COLOR_SURFACE : COLOR_BG;

  tft.fillRoundRect(15, y, 290, 44, 8, bg);

  switch (itemIdx) {
    case 0: // Focus Duration
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 14); tft.print("Focus Duration");
      tft.setTextColor(selected ? COLOR_ACCENT_FOCUS : COLOR_TEXT_DIM, bg);
      tft.setCursor(240, y + 14); tft.print(pomFocusSec / 60);
      tft.setTextSize(1); tft.setCursor(270, y + 19); tft.print("min");
      break;
    case 1: // Break Duration
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 14); tft.print("Break Duration");
      tft.setTextColor(selected ? COLOR_ACCENT_BREAK : COLOR_TEXT_DIM, bg);
      tft.setCursor(240, y + 14); tft.print(pomBreakSec / 60);
      tft.setTextSize(1); tft.setCursor(270, y + 19); tft.print("min");
      break;
    case 2: // Auto Continue
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 14); tft.print("Auto Continue");
      tft.setTextColor(pomAutoContinue ? COLOR_ACCENT_BREAK : COLOR_TEXT_DIM, bg);
      tft.setCursor(240, y + 14); tft.print(pomAutoContinue ? "On" : "Off");
      break;
    case 3: // Volume
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 14); tft.print("Volume");
      tft.setTextColor(selected ? COLOR_ACCENT_FOCUS : COLOR_TEXT_DIM, bg);
      tft.setCursor(220, y + 14);
      switch (volumeLevel) {
        case VolumeLevel::VOL_MUTE: tft.print("Mute"); break;
        case VolumeLevel::VOL_LOW:  tft.print("Low");  break;
        case VolumeLevel::VOL_MED:  tft.print("Med");  break;
        case VolumeLevel::VOL_HIGH: tft.print("High"); break;
      }
      break;
    case 4: { // Stats
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(25, y + 14); tft.print("Focus Time");
      tft.setTextColor(selected ? COLOR_ACCENT_FOCUS : COLOR_TEXT_DIM, bg);
      tft.setCursor(200, y + 14);
      tft.print(pomTotalFocus / 3600); tft.print("h ");
      tft.print((pomTotalFocus % 3600) / 60); tft.print("m");
      break;
    }
    case 5: // Switch App
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
      tft.setTextSize(2); tft.setCursor(100, y + 14); tft.print("Switch App");
      break;
    case 6: // Turn Off
      tft.setTextColor(COLOR_RED, bg);
      tft.setTextSize(2); tft.setCursor(115, y + 14); tft.print("Turn Off");
      break;
  }
}

static void drawPomMenuScreen() {
  int sel = (int)pomMenuSel;

  if (sel < pomMenuScroll) pomMenuScroll = sel;
  else if (sel >= pomMenuScroll + POM_MENU_VISIBLE) pomMenuScroll = sel - POM_MENU_VISIBLE + 1;

  bool scrollChanged = (pomMenuScroll != dispMenuScroll);

  if (needsFullRedraw || dispApp != ActiveApp::POMODORO || scrollChanged) {
    tft.fillScreen(COLOR_BG);

    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextSize(2); tft.setCursor(110, 20); tft.print("Settings");
    tft.fillRect(20, 50, 280, 1, COLOR_SURFACE);

    if (pomMenuScroll > 0)
      tft.fillTriangle(160, 55, 155, 58, 165, 58, COLOR_TEXT_DIM);
    if (pomMenuScroll + POM_MENU_VISIBLE < POM_MENU_COUNT)
      tft.fillTriangle(160, 220, 155, 217, 165, 217, COLOR_TEXT_DIM);

    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1); tft.setCursor(80, 228);
    tft.print("click: change  hold: back");

    for (int s = 0; s < POM_MENU_VISIBLE; s++) {
      int idx = pomMenuScroll + s;
      if (idx < POM_MENU_COUNT) drawPomMenuItemAt(s, idx, idx == sel);
    }

    dispApp = ActiveApp::POMODORO;
    dispMenuScroll = pomMenuScroll;
    needsFullRedraw = false;
    dispMenuSel = sel;
  }
  else if (sel != dispMenuSel) {
    int prev = dispMenuSel;
    for (int s = 0; s < POM_MENU_VISIBLE; s++) {
      int idx = pomMenuScroll + s;
      if (idx < POM_MENU_COUNT && (idx == sel || idx == prev))
        drawPomMenuItemAt(s, idx, idx == sel);
    }
    dispMenuSel = sel;
  }
}

// --- Input ---

static void pomMenuClick() {
  switch (pomMenuSel) {
    case PomMenuItem::FOCUS_TIME:
      if      (pomFocusSec < 30*60) pomFocusSec = 30*60;
      else if (pomFocusSec < 45*60) pomFocusSec = 45*60;
      else if (pomFocusSec < 50*60) pomFocusSec = 50*60;
      else if (pomFocusSec < 60*60) pomFocusSec = 60*60;
      else pomFocusSec = 25*60;
      if (pomMode == PomTimerMode::FOCUS) pomRemaining = pomFocusSec;
      needsFullRedraw = true;
      break;
    case PomMenuItem::BREAK_TIME:
      if      (pomBreakSec < 10*60) pomBreakSec = 10*60;
      else if (pomBreakSec < 15*60) pomBreakSec = 15*60;
      else if (pomBreakSec < 20*60) pomBreakSec = 20*60;
      else if (pomBreakSec < 25*60) pomBreakSec = 25*60;
      else pomBreakSec = 5*60;
      if (pomMode == PomTimerMode::BREAK) pomRemaining = pomBreakSec;
      needsFullRedraw = true;
      break;
    case PomMenuItem::AUTO_CONTINUE:
      pomAutoContinue = !pomAutoContinue;
      needsFullRedraw = true;
      break;
    case PomMenuItem::VOLUME:
      cycleVolume();
      needsFullRedraw = true;
      break;
    case PomMenuItem::STATS:
      pomTotalFocus = 0;
      saveSettings();
      needsFullRedraw = true;
      break;
    case PomMenuItem::SWITCH_APP:
      switchToLauncher();
      break;
    case PomMenuItem::TURN_OFF:
      turnOff();
      break;
  }
}

static void pomHandleEncoder(int32_t delta) {
  if (pomState == PomState::MENU) {
    int idx = (int)pomMenuSel;
    if (delta > 0) { idx++; if (idx >= POM_MENU_COUNT) idx = 0; }
    else            { idx--; if (idx < 0) idx = POM_MENU_COUNT - 1; }
    pomMenuSel = (PomMenuItem)idx;
  } else {
    if (delta > 0) pomSwitchToBreak(false);
    else if (delta < 0) pomSwitchToFocus(false);
  }
}

static void pomHandleClick() {
  if (pomState == PomState::MENU) pomMenuClick();
  else pomTogglePause();
}

static void pomHandleLongPress() {
  if (pomState == PomState::MENU) pomExitMenu();
  else pomEnterMenu();
}

static void pomDraw() {
  if (pomState == PomState::MENU) drawPomMenuScreen();
  else drawPomTimerScreen();
}
