#pragma once
#include "common.h"

// =============================================================================
// Launcher - App Selection Screen
// =============================================================================

static constexpr int LAUNCHER_COUNT = 3;
static int launcherSel = 0;

static void drawLauncherScreen() {
  if (!needsFullRedraw && launcherSel == dispSel && dispApp == ActiveApp::LAUNCHER) return;

  bool fullRedraw = needsFullRedraw || (dispApp != ActiveApp::LAUNCHER);

  if (fullRedraw) {
    tft.fillScreen(COLOR_BG);

    // Title
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextSize(2);
    tft.setCursor(100, 12);
    tft.print("Zero Timer");

    // Accent line under title
    tft.fillRect(80, 36, 160, 2, COLOR_ACCENT_FOCUS);

    // Hints
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(75, 222);
    tft.print("rotate: browse  click: open");
  }

  // App cards
  const int CARD_Y[] = {48, 104, 160};
  const char* names[] = {"Pomodoro", "Quick Timer", "Stopwatch"};
  const char* descs[] = {"Focus & break cycles", "Countdown 1-60 min", "Count up with laps"};
  const uint16_t colors[] = {COLOR_RED, COLOR_ACCENT_QTIMER, COLOR_ACCENT_STOPWATCH};

  for (int i = 0; i < LAUNCHER_COUNT; i++) {
    if (!fullRedraw && i != launcherSel && i != dispSel) continue;

    bool sel = (i == launcherSel);
    uint16_t bg = sel ? COLOR_SURFACE : COLOR_BG;

    // Card background
    tft.fillRoundRect(15, CARD_Y[i], 290, 48, 8, bg);

    // Selection indicator (left accent bar)
    if (sel) {
      tft.fillRoundRect(15, CARD_Y[i], 4, 48, 2, colors[i]);
    }

    // Icon
    int16_t iconX = 30;
    int16_t iconY = CARD_Y[i] + 10;
    uint16_t iconColor = sel ? colors[i] : COLOR_TEXT_DIM;

    switch (i) {
      case 0: drawTomatoIcon(iconX, iconY, iconColor); break;
      case 1: drawHourglassIcon(iconX, iconY, iconColor); break;
      case 2: drawStopwatchIcon(iconX, iconY, iconColor); break;
    }

    // App name
    tft.setTextColor(sel ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
    tft.setTextSize(2);
    tft.setCursor(70, CARD_Y[i] + 8);
    tft.print(names[i]);

    // Description
    tft.setTextColor(sel ? colors[i] : COLOR_TEXT_DIM, bg);
    tft.setTextSize(1);
    tft.setCursor(70, CARD_Y[i] + 30);
    tft.print(descs[i]);
  }

  dispSel = launcherSel;
  dispApp = ActiveApp::LAUNCHER;
  needsFullRedraw = false;
}

static void launcherHandleEncoder(int32_t delta) {
  if (delta > 0)      launcherSel = (launcherSel + 1) % LAUNCHER_COUNT;
  else if (delta < 0) launcherSel = (launcherSel - 1 + LAUNCHER_COUNT) % LAUNCHER_COUNT;
}

static void launcherClick() {
  playSelectSound();
  switch (launcherSel) {
    case 0: activeApp = ActiveApp::POMODORO;    break;
    case 1: activeApp = ActiveApp::QUICK_TIMER; break;
    case 2: activeApp = ActiveApp::STOPWATCH;   break;
  }
  needsFullRedraw = true;
}
