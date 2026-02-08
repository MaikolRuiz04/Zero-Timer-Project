#pragma once
#include "common.h"

// =============================================================================
// Launcher - App Selection Screen (scrollable)
// =============================================================================

static constexpr int LAUNCHER_COUNT   = 6;
static constexpr int LAUNCHER_VISIBLE = 3;
static int launcherSel    = 0;
static int launcherScroll = 0;

static void drawLauncherScreen() {
  // Update scroll position
  if (launcherSel < launcherScroll) launcherScroll = launcherSel;
  else if (launcherSel >= launcherScroll + LAUNCHER_VISIBLE)
    launcherScroll = launcherSel - LAUNCHER_VISIBLE + 1;

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

    // Scroll indicators
    if (launcherScroll > 0)
      tft.fillTriangle(160, 43, 155, 47, 165, 47, COLOR_TEXT_DIM);
    if (launcherScroll + LAUNCHER_VISIBLE < LAUNCHER_COUNT)
      tft.fillTriangle(160, 213, 155, 209, 165, 209, COLOR_TEXT_DIM);

    // Hints
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(75, 222);
    tft.print("rotate: browse  click: open");
  }

  // App cards
  const int CARD_Y[] = {48, 104, 160};
  const char* names[] = {"Pomodoro", "Quick Timer", "Stopwatch", "Counter", "Dice", "Metronome"};
  const char* descs[] = {"Focus & break cycles", "Countdown timer", "Count up with laps", "Tally counter", "Roll the dice", "Beat keeper"};
  const uint16_t colors[] = {COLOR_RED, COLOR_ACCENT_QTIMER, COLOR_ACCENT_STOPWATCH, COLOR_ACCENT_COUNTER, COLOR_ACCENT_DICE, COLOR_ACCENT_METRO};

  for (int s = 0; s < LAUNCHER_VISIBLE; s++) {
    int i = launcherScroll + s;
    if (i >= LAUNCHER_COUNT) continue;

    // In partial mode, only redraw changed cards
    if (!fullRedraw && i != launcherSel && i != dispSel) continue;

    bool sel = (i == launcherSel);
    uint16_t bg = sel ? COLOR_SURFACE : COLOR_BG;

    // Card background
    tft.fillRoundRect(15, CARD_Y[s], 290, 48, 8, bg);

    // Selection indicator (left accent bar)
    if (sel) {
      tft.fillRoundRect(15, CARD_Y[s], 4, 48, 2, colors[i]);
    }

    // Icon
    int16_t iconX = 30;
    int16_t iconY = CARD_Y[s] + 10;
    uint16_t iconColor = sel ? colors[i] : COLOR_TEXT_DIM;

    switch (i) {
      case 0: drawTomatoIcon(iconX, iconY, iconColor);    break;
      case 1: drawHourglassIcon(iconX, iconY, iconColor);  break;
      case 2: drawStopwatchIcon(iconX, iconY, iconColor);  break;
      case 3: drawCounterIcon(iconX, iconY, iconColor);    break;
      case 4: drawDiceIcon(iconX, iconY, iconColor);       break;
      case 5: drawMetronomeIcon(iconX, iconY, iconColor);  break;
    }

    // App name
    tft.setTextColor(sel ? COLOR_TEXT : COLOR_TEXT_DIM, bg);
    tft.setTextSize(2);
    tft.setCursor(70, CARD_Y[s] + 8);
    tft.print(names[i]);

    // Description
    tft.setTextColor(sel ? colors[i] : COLOR_TEXT_DIM, bg);
    tft.setTextSize(1);
    tft.setCursor(70, CARD_Y[s] + 30);
    tft.print(descs[i]);
  }

  dispSel = launcherSel;
  dispApp = ActiveApp::LAUNCHER;
  needsFullRedraw = false;
}

static void launcherHandleEncoder(int32_t delta) {
  if (delta > 0)      launcherSel = (launcherSel + 1) % LAUNCHER_COUNT;
  else if (delta < 0) launcherSel = (launcherSel - 1 + LAUNCHER_COUNT) % LAUNCHER_COUNT;

  // Check if scroll needs to change
  int newScroll = launcherScroll;
  if (launcherSel < newScroll) newScroll = launcherSel;
  else if (launcherSel >= newScroll + LAUNCHER_VISIBLE) newScroll = launcherSel - LAUNCHER_VISIBLE + 1;

  if (newScroll != launcherScroll) {
    launcherScroll = newScroll;
    needsFullRedraw = true;  // Scroll changed, full redraw needed
  }
}

static void launcherClick() {
  playSelectSound();
  switch (launcherSel) {
    case 0: activeApp = ActiveApp::POMODORO;    break;
    case 1: activeApp = ActiveApp::QUICK_TIMER; break;
    case 2: activeApp = ActiveApp::STOPWATCH;   break;
    case 3: activeApp = ActiveApp::COUNTER;     break;
    case 4: activeApp = ActiveApp::DICE;        break;
    case 5: activeApp = ActiveApp::METRONOME;   break;
  }
  needsFullRedraw = true;
}
