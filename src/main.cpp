// ESP32-WROOM-32E + KY-040 rotary encoder + 3.2" SPI TFT 240x320 (ILI9341)
// Pomodoro Timer - "Zero Timer"
// Encoder: CW -> Break mode, CCW -> Focus mode, Click -> Pause, Hold -> Settings

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ESP32Encoder.h>
#include <Preferences.h>

// --- Preferences (NVS storage) ---
Preferences prefs;

// --- TFT pins ---
static constexpr int TFT_CS = 5;
static constexpr int TFT_DC = 16;
static constexpr int TFT_RST = 27;
static constexpr int TFT_MOSI = 23;
static constexpr int TFT_SCK = 18;
static constexpr int TFT_MISO = 19;

// --- Encoder pins ---
static constexpr int ENC_CLK = 32;
static constexpr int ENC_DT = 33;
static constexpr int ENC_SW = 25;

// --- Buzzer pin ---
static constexpr int BUZZER_PIN = 26;
static constexpr int BUZZER_CHANNEL = 0;

// --- Backlight pin ---
static constexpr int TFT_LED = 4;

// --- Display & Encoder objects ---
Adafruit_ILI9341 tft(&SPI, TFT_DC, TFT_CS, TFT_RST);
ESP32Encoder encoder;

// --- Apple-inspired Colors (Dark Mode) ---
static constexpr uint16_t COLOR_BG = 0x0841;           // #0C0C0C - near black
static constexpr uint16_t COLOR_SURFACE = 0x18E3;     // #1C1C1E - dark gray card
static constexpr uint16_t COLOR_TEXT = 0xFFFF;        // White
static constexpr uint16_t COLOR_TEXT_DIM = 0x7BEF;    // #7A7A7A - gray
static constexpr uint16_t COLOR_ACCENT_FOCUS = 0x2D7F; // Soft blue
static constexpr uint16_t COLOR_ACCENT_BREAK = 0x2E8B; // Soft green
static constexpr uint16_t COLOR_ACCENT_PAUSE = 0xFB80; // Soft orange
static constexpr uint16_t COLOR_RED = 0xF800;          // Red for Turn Off

// --- State Machine ---
enum class TimerState { FOCUS, BREAK };
enum class AppState { OFF, RUNNING, PAUSED, MENU };
enum class MenuItem { FOCUS_TIME, BREAK_TIME, AUTO_CONTINUE, VOLUME, STATS, TURN_OFF, EXIT };
enum class VolumeLevel { VOL_MUTE, VOL_LOW, VOL_MED, VOL_HIGH };

// --- Timer Settings (in seconds) ---
static int32_t focusDurationSec = 50 * 60;
static int32_t breakDurationSec = 25 * 60;
static bool autoContinue = true;  // Auto-start next session when timer ends
static VolumeLevel volumeLevel = VolumeLevel::VOL_HIGH;  // Default volume

// --- Stats Tracking ---
static uint32_t totalFocusSeconds = 0;  // Total time spent in focus mode

// --- Runtime State ---
static TimerState timerState = TimerState::FOCUS;
static AppState appState = AppState::OFF;  // Start in OFF state
static MenuItem currentMenuItem = MenuItem::FOCUS_TIME;

static int32_t remainingTimeSec = focusDurationSec;
static uint32_t lastTickMs = 0;

// --- Encoder State ---
static int64_t lastEncoderPos = 0;
static constexpr int32_t COUNTS_PER_DETENT = 2;

// --- Button State ---
static bool buttonPressed = false;
static uint32_t buttonPressStartMs = 0;
static constexpr uint32_t LONG_PRESS_MS = 800;
static bool longPressHandled = false;

// --- Display State ---
static bool needsFullRedraw = true;
static int32_t lastDisplayedTime = -1;
static int32_t lastDisplayedMins = -1;
static int32_t lastDisplayedSecs = -1;
static AppState lastDisplayedAppState = AppState::RUNNING;
static TimerState lastDisplayedTimerState = TimerState::FOCUS;
static MenuItem lastDisplayedMenuItem = MenuItem::FOCUS_TIME;

// --- Menu Scroll State ---
static constexpr int MENU_ITEM_COUNT = 7;
static constexpr int MENU_VISIBLE_ITEMS = 3;
static int menuScrollOffset = 0;
static int lastMenuScrollOffset = -1;

// -----------------------------------------------------------------------------
// Storage Functions (NVS)
// -----------------------------------------------------------------------------

static void saveStats() {
  prefs.begin("zerotimer", false);
  prefs.putUInt("focusTime", totalFocusSeconds);
  prefs.end();
}

static void loadStats() {
  prefs.begin("zerotimer", true);  // read-only
  totalFocusSeconds = prefs.getUInt("focusTime", 0);
  prefs.end();
}

// -----------------------------------------------------------------------------
// Power On/Off Functions
// -----------------------------------------------------------------------------

// Forward declaration
static void playStartupJingle();

static void displaySleep(bool sleep) {
  // ILI9341 sleep commands
  tft.startWrite();
  tft.writeCommand(sleep ? 0x10 : 0x11);  // 0x10 = Sleep In, 0x11 = Sleep Out
  tft.endWrite();
  if (!sleep) delay(120);  // Need 120ms after wake
}

static void turnOff() {
  // Save stats before turning off
  saveStats();
  
  // Turn off display and backlight
  tft.fillScreen(0x0000);  // Black screen
  displaySleep(true);      // Put display in sleep mode
  digitalWrite(TFT_LED, LOW);  // Turn off backlight
  
  appState = AppState::OFF;
  Serial.println("Device OFF - press button to wake");
}

static void turnOn() {
  // Wake up display and backlight
  digitalWrite(TFT_LED, HIGH);  // Turn on backlight
  displaySleep(false);
  
  appState = AppState::PAUSED;
  needsFullRedraw = true;
  lastDisplayedAppState = AppState::OFF;  // Force redraw
  Serial.println("Device ON");
  
  // Play startup jingle
  if (volumeLevel != VolumeLevel::VOL_MUTE) {
    playStartupJingle();
  }
}

// -----------------------------------------------------------------------------
// Icon Drawing (Simple geometric icons)
// -----------------------------------------------------------------------------

// Laptop icon for Focus mode (work)
static void drawFocusIcon(int16_t x, int16_t y, uint16_t color) {
  // Screen
  tft.fillRoundRect(x, y, 32, 20, 2, color);
  tft.fillRect(x + 2, y + 2, 28, 16, COLOR_BG);
  // Base
  tft.fillRoundRect(x - 2, y + 22, 36, 4, 1, color);
}

// Coffee cup icon for Break mode
static void drawBreakIcon(int16_t x, int16_t y, uint16_t color) {
  // Cup body
  tft.fillRoundRect(x, y + 4, 24, 22, 3, color);
  tft.fillRoundRect(x + 3, y + 7, 18, 16, 2, COLOR_BG);
  // Handle (simple arc using circles)
  tft.fillCircle(x + 28, y + 14, 6, color);
  tft.fillCircle(x + 28, y + 14, 3, COLOR_BG);
  tft.fillRect(x + 22, y + 8, 6, 12, COLOR_BG);
  // Steam (3 wavy lines)
  for (int i = 0; i < 3; i++) {
    int16_t sx = x + 6 + i * 6;
    tft.drawPixel(sx, y, color);
    tft.drawPixel(sx + 1, y - 1, color);
    tft.drawPixel(sx, y - 2, color);
  }
}

// Pause indicator (two vertical bars)
static void drawPauseIndicator(int16_t x, int16_t y, uint16_t color) {
  tft.fillRoundRect(x, y, 6, 20, 2, color);
  tft.fillRoundRect(x + 10, y, 6, 20, 2, color);
}

// Play indicator (triangle)
static void drawPlayIndicator(int16_t x, int16_t y, uint16_t color) {
  tft.fillTriangle(x, y, x, y + 20, x + 16, y + 10, color);
}

// -----------------------------------------------------------------------------
// Sound/Buzzer (Passive buzzer melodies)
// -----------------------------------------------------------------------------

// Musical note frequencies (Hz)
static constexpr int NOTE_C4 = 262;
static constexpr int NOTE_D4 = 294;
static constexpr int NOTE_E4 = 330;
static constexpr int NOTE_F4 = 349;
static constexpr int NOTE_G4 = 392;
static constexpr int NOTE_A4 = 440;
static constexpr int NOTE_B4 = 494;
static constexpr int NOTE_C5 = 523;
static constexpr int NOTE_D5 = 587;
static constexpr int NOTE_E5 = 659;
static constexpr int NOTE_G5 = 784;

static void playTone(int frequency, int durationMs) {
  if (volumeLevel == VolumeLevel::VOL_MUTE) {
    delay(durationMs);  // Silent but maintain timing
    return;
  }
  
  if (frequency > 0) {
    // Volume control via duty cycle (louder base values)
    int duty = 0;
    switch (volumeLevel) {
      case VolumeLevel::VOL_LOW:  duty = 32;  break;  // 12.5%
      case VolumeLevel::VOL_MED:  duty = 64;  break;  // 25%
      case VolumeLevel::VOL_HIGH: duty = 128; break;  // 50%
      default: break;
    }
    ledcWriteTone(BUZZER_CHANNEL, frequency);
    ledcWrite(BUZZER_CHANNEL, duty);
    delay(durationMs);
  } else {
    delay(durationMs);  // Rest
  }
  ledcWriteTone(BUZZER_CHANNEL, 0);  // Stop tone
}

// Focus start: Uplifting ascending chime (let's get to work!)
static void playFocusMelody() {
  playTone(NOTE_E4, 100);
  delay(30);
  playTone(NOTE_G4, 100);
  delay(30);
  playTone(NOTE_C5, 150);
}

// Break start: Gentle descending relaxing tone (time to rest)
static void playBreakMelody() {
  playTone(NOTE_G4, 120);
  delay(40);
  playTone(NOTE_E4, 120);
  delay(40);
  playTone(NOTE_C4, 180);
}

// Click confirmation: Short subtle beep
static void playClickSound() {
  playTone(NOTE_E5, 50);
}

// Menu enter sound
static void playMenuSound() {
  playTone(NOTE_C5, 60);
  delay(20);
  playTone(NOTE_E5, 60);
}

// Startup jingle: Cheerful ascending arpeggio
static void playStartupJingle() {
  playTone(NOTE_C4, 80);
  delay(30);
  playTone(NOTE_E4, 80);
  delay(30);
  playTone(NOTE_G4, 80);
  delay(30);
  playTone(NOTE_C5, 120);
  delay(50);
  playTone(NOTE_E5, 150);
}

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

static void formatTime(int32_t totalSec, char* buf, size_t bufSize) {
  int32_t mins = totalSec / 60;
  int32_t secs = totalSec % 60;
  snprintf(buf, bufSize, "%02d:%02d", (int)mins, (int)secs);
}

static uint16_t getAccentColor() {
  if (appState == AppState::PAUSED) return COLOR_ACCENT_PAUSE;
  return (timerState == TimerState::FOCUS) ? COLOR_ACCENT_FOCUS : COLOR_ACCENT_BREAK;
}

// -----------------------------------------------------------------------------
// Display Functions
// -----------------------------------------------------------------------------

static void drawTimerScreen() {
  uint16_t accent = getAccentColor();
  
  if (needsFullRedraw || lastDisplayedAppState != appState || lastDisplayedTimerState != timerState) {
    tft.fillScreen(COLOR_BG);
    
    // Title - subtle, top left
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(12, 12);
    tft.print("ZERO TIMER");
    
    // Mode card background
    tft.fillRoundRect(10, 35, 300, 50, 8, COLOR_SURFACE);
    
    // Icon (left side of card)
    if (timerState == TimerState::FOCUS) {
      drawFocusIcon(24, 48, accent);
    } else {
      drawBreakIcon(24, 45, accent);
    }
    
    // Mode label (right of icon)
    tft.setTextColor(COLOR_TEXT, COLOR_SURFACE);
    tft.setTextSize(2);
    tft.setCursor(70, 43);
    tft.print(timerState == TimerState::FOCUS ? "Focus" : "Break");
    
    // Duration info
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_SURFACE);
    tft.setTextSize(1);
    tft.setCursor(70, 65);
    int32_t totalMin = (timerState == TimerState::FOCUS ? focusDurationSec : breakDurationSec) / 60;
    tft.print(totalMin);
    tft.print(" min session");
    
    // Play/Pause indicator (right side of card)
    if (appState == AppState::PAUSED) {
      drawPlayIndicator(270, 50, accent);
    } else {
      drawPauseIndicator(270, 50, accent);
    }
    
    // Hint text at bottom
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(65, 215);
    tft.print("click: play/pause");
    tft.setCursor(75, 227);
    tft.print("hold: settings");
    
    lastDisplayedAppState = appState;
    lastDisplayedTimerState = timerState;
    needsFullRedraw = false;
    lastDisplayedTime = -1;
    lastDisplayedMins = -1;
    lastDisplayedSecs = -1;
  }
  
  // Timer display (large, centered) - only redraw digits that changed
  if (remainingTimeSec != lastDisplayedTime) {
    int32_t mins = remainingTimeSec / 60;
    int32_t secs = remainingTimeSec % 60;
    
    // Text size 7: each char is ~42px wide (6*7), spacing included
    // Format: "MM:SS" - positions approximate
    int16_t baseX = 50;  // Starting X for first digit
    int16_t baseY = 115;
    int16_t charW = 42;  // Width per character at size 7
    
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextSize(7);
    
    // Only redraw minutes if they changed
    if (mins != lastDisplayedMins || lastDisplayedTime == -1) {
      // Clear minutes area (2 digits)
      tft.fillRect(baseX, baseY, charW * 2, 50, COLOR_BG);
      tft.setCursor(baseX, baseY);
      if (mins < 10) tft.print('0');
      tft.print(mins);
      lastDisplayedMins = mins;
    }
    
    // Colon (only draw on full redraw)
    if (lastDisplayedTime == -1) {
      tft.setCursor(baseX + charW * 2, baseY);
      tft.print(':');
    }
    
    // Only redraw seconds if they changed
    if (secs != lastDisplayedSecs || lastDisplayedTime == -1) {
      // Clear seconds area (2 digits)
      tft.fillRect(baseX + charW * 3, baseY, charW * 2, 50, COLOR_BG);
      tft.setCursor(baseX + charW * 3, baseY);
      if (secs < 10) tft.print('0');
      tft.print(secs);
      lastDisplayedSecs = secs;
    }
    
    // Progress indicator line
    int32_t totalSec = (timerState == TimerState::FOCUS) ? focusDurationSec : breakDurationSec;
    int32_t elapsed = totalSec - remainingTimeSec;
    int16_t progressWidth = (int16_t)((elapsed * 280L) / totalSec);
    
    tft.fillRoundRect(20, 190, 280, 4, 2, COLOR_SURFACE);
    if (progressWidth > 0) {
      tft.fillRoundRect(20, 190, progressWidth, 4, 2, accent);
    }
    
    lastDisplayedTime = remainingTimeSec;
  }
}

static void drawMenuItem(int slot, int itemIndex, bool selected) {
  const int ITEM_HEIGHT = 50;
  const int ITEM_START_Y = 60;
  int16_t y = ITEM_START_Y + slot * ITEM_HEIGHT;
  uint16_t bgColor = selected ? COLOR_SURFACE : COLOR_BG;
  
  tft.fillRoundRect(15, y, 290, 44, 8, bgColor);
  
  switch (itemIndex) {
    case 0: // Focus Duration
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bgColor);
      tft.setTextSize(2);
      tft.setCursor(25, y + 14);
      tft.print("Focus Duration");
      tft.setTextColor(selected ? COLOR_ACCENT_FOCUS : COLOR_TEXT_DIM, bgColor);
      tft.setCursor(240, y + 14);
      tft.print(focusDurationSec / 60);
      tft.setTextSize(1);
      tft.setCursor(270, y + 19);
      tft.print("min");
      break;
      
    case 1: // Break Duration
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bgColor);
      tft.setTextSize(2);
      tft.setCursor(25, y + 14);
      tft.print("Break Duration");
      tft.setTextColor(selected ? COLOR_ACCENT_BREAK : COLOR_TEXT_DIM, bgColor);
      tft.setCursor(240, y + 14);
      tft.print(breakDurationSec / 60);
      tft.setTextSize(1);
      tft.setCursor(270, y + 19);
      tft.print("min");
      break;
      
    case 2: // Auto Continue
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bgColor);
      tft.setTextSize(2);
      tft.setCursor(25, y + 14);
      tft.print("Auto Continue");
      tft.setTextColor(autoContinue ? COLOR_ACCENT_BREAK : COLOR_TEXT_DIM, bgColor);
      tft.setCursor(240, y + 14);
      tft.print(autoContinue ? "On" : "Off");
      break;
      
    case 3: // Volume
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bgColor);
      tft.setTextSize(2);
      tft.setCursor(25, y + 14);
      tft.print("Volume");
      tft.setTextColor(selected ? COLOR_ACCENT_FOCUS : COLOR_TEXT_DIM, bgColor);
      tft.setCursor(220, y + 14);
      switch (volumeLevel) {
        case VolumeLevel::VOL_MUTE: tft.print("Mute"); break;
        case VolumeLevel::VOL_LOW:  tft.print("Low"); break;
        case VolumeLevel::VOL_MED:  tft.print("Med"); break;
        case VolumeLevel::VOL_HIGH: tft.print("High"); break;
      }
      break;
      
    case 4: { // Stats
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bgColor);
      tft.setTextSize(2);
      tft.setCursor(25, y + 14);
      tft.print("Focus Time");
      tft.setTextColor(selected ? COLOR_ACCENT_FOCUS : COLOR_TEXT_DIM, bgColor);
      uint32_t hours = totalFocusSeconds / 3600;
      uint32_t mins = (totalFocusSeconds % 3600) / 60;
      tft.setCursor(200, y + 14);
      tft.print(hours);
      tft.print("h ");
      tft.print(mins);
      tft.print("m");
      break;
    }
      
    case 5: // Turn Off
      tft.setTextColor(COLOR_RED, bgColor);  // Always red
      tft.setTextSize(2);
      tft.setCursor(115, y + 14);
      tft.print("Turn Off");
      break;
      
    case 6: // Done
      tft.setTextColor(selected ? COLOR_TEXT : COLOR_TEXT_DIM, bgColor);
      tft.setTextSize(2);
      tft.setCursor(130, y + 14);
      tft.print("Done");
      break;
  }
}

static void drawMenuScreen() {
  int selectedIndex = static_cast<int>(currentMenuItem);
  
  // Calculate scroll offset to keep selection visible
  if (selectedIndex < menuScrollOffset) {
    menuScrollOffset = selectedIndex;
  } else if (selectedIndex >= menuScrollOffset + MENU_VISIBLE_ITEMS) {
    menuScrollOffset = selectedIndex - MENU_VISIBLE_ITEMS + 1;
  }
  
  bool scrollChanged = (menuScrollOffset != lastMenuScrollOffset);
  
  if (needsFullRedraw || lastDisplayedAppState != appState || scrollChanged) {
    tft.fillScreen(COLOR_BG);
    
    // Title
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    tft.setTextSize(2);
    tft.setCursor(110, 20);
    tft.print("Settings");
    
    // Divider line
    tft.fillRect(20, 50, 280, 1, COLOR_SURFACE);
    
    // Scroll indicators
    if (menuScrollOffset > 0) {
      // Up arrow indicator
      tft.fillTriangle(160, 55, 155, 58, 165, 58, COLOR_TEXT_DIM);
    }
    if (menuScrollOffset + MENU_VISIBLE_ITEMS < MENU_ITEM_COUNT) {
      // Down arrow indicator
      tft.fillTriangle(160, 220, 155, 217, 165, 217, COLOR_TEXT_DIM);
    }
    
    // Hint at bottom
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(80, 228);
    tft.print("click: change  hold: back");
    
    // Draw visible menu items
    for (int slot = 0; slot < MENU_VISIBLE_ITEMS; slot++) {
      int itemIndex = menuScrollOffset + slot;
      if (itemIndex < MENU_ITEM_COUNT) {
        bool selected = (itemIndex == selectedIndex);
        drawMenuItem(slot, itemIndex, selected);
      }
    }
    
    lastDisplayedAppState = appState;
    lastMenuScrollOffset = menuScrollOffset;
    needsFullRedraw = false;
    lastDisplayedMenuItem = currentMenuItem;
  }
  else if (currentMenuItem != lastDisplayedMenuItem) {
    // Only selection changed, redraw affected items
    int prevIndex = static_cast<int>(lastDisplayedMenuItem);
    
    for (int slot = 0; slot < MENU_VISIBLE_ITEMS; slot++) {
      int itemIndex = menuScrollOffset + slot;
      if (itemIndex < MENU_ITEM_COUNT) {
        if (itemIndex == selectedIndex || itemIndex == prevIndex) {
          drawMenuItem(slot, itemIndex, itemIndex == selectedIndex);
        }
      }
    }
    
    lastDisplayedMenuItem = currentMenuItem;
  }
}

// -----------------------------------------------------------------------------
// State Transitions
// -----------------------------------------------------------------------------

static void switchToFocus(bool playSound = true) {
  if (timerState != TimerState::FOCUS) {
    timerState = TimerState::FOCUS;
    remainingTimeSec = focusDurationSec;
    needsFullRedraw = true;
    if (playSound) playFocusMelody();
    Serial.println("Switched to FOCUS");
  }
}

static void switchToBreak(bool playSound = true) {
  if (timerState != TimerState::BREAK) {
    timerState = TimerState::BREAK;
    remainingTimeSec = breakDurationSec;
    needsFullRedraw = true;
    if (playSound) playBreakMelody();
    Serial.println("Switched to BREAK");
  }
}

static void togglePause() {
  if (appState == AppState::RUNNING) {
    appState = AppState::PAUSED;
    playClickSound();
    Serial.println("PAUSED");
  } else if (appState == AppState::PAUSED) {
    appState = AppState::RUNNING;
    lastTickMs = millis();
    playClickSound();
    Serial.println("RUNNING");
  }
  needsFullRedraw = true;
}

static void enterMenu() {
  appState = AppState::MENU;
  currentMenuItem = MenuItem::FOCUS_TIME;
  menuScrollOffset = 0;
  lastMenuScrollOffset = -1;
  needsFullRedraw = true;
  playMenuSound();
}

static void exitMenu() {
  appState = AppState::PAUSED;
  needsFullRedraw = true;
}

// -----------------------------------------------------------------------------
// Input Handling
// -----------------------------------------------------------------------------

static void handleEncoderInTimer(int32_t delta) {
  if (delta > 0) switchToBreak(false);  // No sound for manual switch
  else if (delta < 0) switchToFocus(false);
}

static void handleEncoderInMenu(int32_t delta) {
  int idx = (int)currentMenuItem;
  int maxIdx = (int)MenuItem::EXIT;
  if (delta > 0) {
    idx++;
    if (idx > maxIdx) idx = 0;
  } else {
    idx--;
    if (idx < 0) idx = maxIdx;
  }
  currentMenuItem = (MenuItem)idx;
}

static void handleMenuClick() {
  switch (currentMenuItem) {
    case MenuItem::FOCUS_TIME:
      if (focusDurationSec < 30*60) focusDurationSec = 30*60;
      else if (focusDurationSec < 45*60) focusDurationSec = 45*60;
      else if (focusDurationSec < 50*60) focusDurationSec = 50*60;
      else if (focusDurationSec < 60*60) focusDurationSec = 60*60;
      else focusDurationSec = 25*60;
      if (timerState == TimerState::FOCUS) remainingTimeSec = focusDurationSec;
      needsFullRedraw = true;
      break;
    case MenuItem::BREAK_TIME:
      if (breakDurationSec < 10*60) breakDurationSec = 10*60;
      else if (breakDurationSec < 15*60) breakDurationSec = 15*60;
      else if (breakDurationSec < 20*60) breakDurationSec = 20*60;
      else if (breakDurationSec < 25*60) breakDurationSec = 25*60;
      else breakDurationSec = 5*60;
      if (timerState == TimerState::BREAK) remainingTimeSec = breakDurationSec;
      needsFullRedraw = true;
      break;
    case MenuItem::AUTO_CONTINUE:
      autoContinue = !autoContinue;
      needsFullRedraw = true;
      break;
    case MenuItem::VOLUME:
      // Cycle through volume levels: High -> Medium -> Low -> Mute -> High
      switch (volumeLevel) {
        case VolumeLevel::VOL_HIGH: volumeLevel = VolumeLevel::VOL_MED; break;
        case VolumeLevel::VOL_MED:  volumeLevel = VolumeLevel::VOL_LOW; break;
        case VolumeLevel::VOL_LOW:  volumeLevel = VolumeLevel::VOL_MUTE; break;
        case VolumeLevel::VOL_MUTE: volumeLevel = VolumeLevel::VOL_HIGH; break;
      }
      playClickSound();  // Preview the volume
      needsFullRedraw = true;
      break;
    case MenuItem::STATS:
      // Stats is display only, clicking resets the counter
      totalFocusSeconds = 0;
      saveStats();  // Save the reset
      needsFullRedraw = true;
      break;
    case MenuItem::TURN_OFF:
      turnOff();
      break;
    case MenuItem::EXIT:
      exitMenu();
      break;
  }
}

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nZero Timer");

  pinMode(ENC_SW, INPUT_PULLUP);

  // Initialize backlight control
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LOW);  // Start with backlight off

  // Initialize buzzer
  ledcSetup(BUZZER_CHANNEL, 2000, 8);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  encoder.attachHalfQuad(ENC_DT, ENC_CLK);
  encoder.setFilter(1023);
  encoder.clearCount();

  pinMode(TFT_CS, OUTPUT);  digitalWrite(TFT_CS, HIGH);
  pinMode(TFT_DC, OUTPUT);  digitalWrite(TFT_DC, HIGH);
  pinMode(TFT_RST, OUTPUT); digitalWrite(TFT_RST, HIGH);

  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin(8000000);
  tft.setRotation(3);
  tft.invertDisplay(false);  // Ensure normal color mode

  // Load saved stats
  loadStats();
  Serial.print("Loaded focus time: ");
  Serial.print(totalFocusSeconds / 3600);
  Serial.print("h ");
  Serial.print((totalFocusSeconds % 3600) / 60);
  Serial.println("m");

  // Start with display off, wait for button press
  tft.fillScreen(0x0000);
  displaySleep(true);
  appState = AppState::OFF;
  
  Serial.println("Press button to start...");
  
  // Wait for button press to turn on
  while (digitalRead(ENC_SW) == HIGH) {
    delay(10);
  }
  // Wait for release
  while (digitalRead(ENC_SW) == LOW) {
    delay(10);
  }
  
  // Turn on
  turnOn();
  lastTickMs = millis();
}

void loop() {
  uint32_t nowMs = millis();
  
  // Handle OFF state - wait for button press to wake
  if (appState == AppState::OFF) {
    if (digitalRead(ENC_SW) == LOW) {
      delay(50);  // Debounce
      while (digitalRead(ENC_SW) == LOW) {
        delay(10);  // Wait for release
      }
      turnOn();
      lastTickMs = millis();
    }
    return;  // Skip rest of loop when off
  }
  
  // Timer tick
  if (appState == AppState::RUNNING && nowMs - lastTickMs >= 1000) {
    lastTickMs += 1000;
    if (remainingTimeSec > 0) {
      remainingTimeSec--;
      // Track focus time for stats
      if (timerState == TimerState::FOCUS) {
        totalFocusSeconds++;
        // Auto-save every minute
        if (totalFocusSeconds % 60 == 0) {
          saveStats();
        }
      }
    }
    if (remainingTimeSec == 0) {
      if (timerState == TimerState::FOCUS) switchToBreak();
      else switchToFocus();
      if (!autoContinue) {
        appState = AppState::PAUSED;
      }
      lastTickMs = nowMs;  // Reset tick timer for new session
      needsFullRedraw = true;
    }
  }
  
  // Encoder
  int64_t rawPos = encoder.getCount();
  int32_t det = (int32_t)(rawPos / COUNTS_PER_DETENT);
  int32_t lastDet = (int32_t)(lastEncoderPos / COUNTS_PER_DETENT);
  int32_t delta = det - lastDet;
  
  if (delta != 0) {
    lastEncoderPos = rawPos;
    if (appState == AppState::MENU) handleEncoderInMenu(delta);
    else handleEncoderInTimer(delta);
  }
  
  // Button
  bool btn = (digitalRead(ENC_SW) == LOW);
  
  if (btn && !buttonPressed) {
    buttonPressed = true;
    buttonPressStartMs = nowMs;
    longPressHandled = false;
  }
  
  if (buttonPressed && btn && !longPressHandled && (nowMs - buttonPressStartMs >= LONG_PRESS_MS)) {
    longPressHandled = true;
    if (appState == AppState::MENU) exitMenu();
    else enterMenu();
  }
  
  if (!btn && buttonPressed) {
    if (!longPressHandled) {
      if (appState == AppState::MENU) handleMenuClick();
      else togglePause();
    }
    buttonPressed = false;
  }
  
  // Display
  if (appState == AppState::MENU) drawMenuScreen();
  else drawTimerScreen();
}
