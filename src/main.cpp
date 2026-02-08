// ESP32-WROOM-32E + KY-040 rotary encoder + 3.2" SPI TFT 240x320 (ILI9341)
// Zero Timer - Multi-App Timer Platform
// Apps: Pomodoro Timer, Quick Timer, Stopwatch
// Input: Rotate -> Navigate/Adjust, Click -> Action, Hold -> Menu/Back

#include <Arduino.h>
#include <SPI.h>
#include "common.h"

// =============================================================================
// Global Hardware Objects
// =============================================================================

Adafruit_ILI9341 tft(&SPI, TFT_DC, TFT_CS, TFT_RST);
ESP32Encoder encoder;
Preferences prefs;

// =============================================================================
// Global State (declared extern in common.h)
// =============================================================================

ActiveApp   activeApp   = ActiveApp::LAUNCHER;
VolumeLevel volumeLevel = VolumeLevel::VOL_HIGH;
bool        deviceOn    = false;
bool        needsFullRedraw = true;

// Display cache
int32_t  dispTime       = -1;
int32_t  dispMins       = -1;
int32_t  dispSecs       = -1;
int32_t  dispTenths     = -1;
int      dispSel        = -1;
int      dispMenuSel    = -1;
int      dispMenuScroll = -1;
ActiveApp dispApp       = ActiveApp::LAUNCHER;

// Shared simple menu selection
SimpleMenuItem simpleMenuSel = SimpleMenuItem::VOLUME;

// =============================================================================
// Forward Declarations (needed by common.h / app headers)
// =============================================================================

void saveSettings();
void loadSettings();
void switchToLauncher();
void turnOff();
void turnOn();

// =============================================================================
// Include App Modules
// =============================================================================

#include "launcher.h"
#include "pomodoro.h"
#include "quicktimer.h"
#include "stopwatch.h"
#include "counter.h"
#include "dice.h"
#include "metronome.h"

// =============================================================================
// Storage (NVS)
// =============================================================================

void saveSettings() {
  prefs.begin("zerotimer", false);
  prefs.putUInt("focusTime", pomTotalFocus);
  prefs.putUChar("volume", (uint8_t)volumeLevel);
  prefs.putInt("focusDur", pomFocusSec);
  prefs.putInt("breakDur", pomBreakSec);
  prefs.putBool("autoCont", pomAutoContinue);
  prefs.end();
}

void loadSettings() {
  prefs.begin("zerotimer", true);
  pomTotalFocus   = prefs.getUInt("focusTime", 0);
  volumeLevel     = (VolumeLevel)prefs.getUChar("volume", (uint8_t)VolumeLevel::VOL_HIGH);
  pomFocusSec     = prefs.getInt("focusDur", 50 * 60);
  pomBreakSec     = prefs.getInt("breakDur", 25 * 60);
  pomAutoContinue = prefs.getBool("autoCont", true);
  prefs.end();
  pomRemaining = pomFocusSec;
}

// =============================================================================
// Power Management & App Switching
// =============================================================================

void turnOff() {
  saveSettings();
  tft.fillScreen(0x0000);
  displaySleep(true);
  digitalWrite(TFT_LED, LOW);
  deviceOn = false;
  Serial.println("Device OFF");
}

void turnOn() {
  digitalWrite(TFT_LED, HIGH);
  displaySleep(false);
  deviceOn = true;
  activeApp = ActiveApp::LAUNCHER;
  needsFullRedraw = true;
  Serial.println("Device ON");
  playBootSplash();
  if (volumeLevel != VolumeLevel::VOL_MUTE) playStartupJingle();
}

void switchToLauncher() {
  // Pause running apps before switching
  if (activeApp == ActiveApp::POMODORO && pomState == PomState::RUNNING) {
    pomState = PomState::PAUSED;
  }
  if (activeApp == ActiveApp::QUICK_TIMER && qtState == QTState::RUNNING) {
    qtState = QTState::PAUSED;
  }
  if (activeApp == ActiveApp::STOPWATCH && swState == SWState::RUNNING) {
    swAccumMs += millis() - swStartMs;
    swState = SWState::STOPPED;
  }
  if (activeApp == ActiveApp::METRONOME && metroState == MetroState::PLAYING) {
    metroState = MetroState::IDLE;
    ledcWriteTone(BUZZER_CHANNEL, 0);
    metroBuzzing = false;
  }

  activeApp = ActiveApp::LAUNCHER;
  needsFullRedraw = true;
  playMenuSound();
}

// =============================================================================
// Encoder Reading
// =============================================================================

static int64_t lastEncPos = 0;

static int32_t readEncoderDelta() {
  int64_t rawPos = encoder.getCount();
  int32_t det    = (int32_t)(rawPos / COUNTS_PER_DETENT);
  int32_t lastDet = (int32_t)(lastEncPos / COUNTS_PER_DETENT);
  int32_t delta  = det - lastDet;
  if (delta != 0) lastEncPos = rawPos;
  return delta;
}

// =============================================================================
// Button Reading
// =============================================================================

static bool     btnDown          = false;
static uint32_t btnDownMs        = 0;
static bool     longPressHandled = false;
static constexpr uint32_t DEBOUNCE_MS = 30;  // Minimum press duration to count

// Button events: 0=none, 1=click, 2=long press
static int readButton() {
  bool pressed = (digitalRead(ENC_SW) == LOW);
  uint32_t now = millis();
  int event = 0;

  // Button just pressed
  if (pressed && !btnDown) {
    btnDown = true;
    btnDownMs = now;
    longPressHandled = false;
  }

  // Only consider presses valid after debounce period
  bool validPress = btnDown && (now - btnDownMs >= DEBOUNCE_MS);

  // Long press detection (only after debounce)
  if (validPress && pressed && !longPressHandled && (now - btnDownMs >= LONG_PRESS_MS)) {
    longPressHandled = true;
    event = 2;  // long press
  }

  // Button released
  if (!pressed && btnDown) {
    if (!longPressHandled && validPress) event = 1;  // click (only if held > debounce)
    btnDown = false;
  }

  return event;
}

// =============================================================================
// Input Dispatch
// =============================================================================

static void dispatchEncoder(int32_t delta) {
  switch (activeApp) {
    case ActiveApp::LAUNCHER:    launcherHandleEncoder(delta); break;
    case ActiveApp::POMODORO:    pomHandleEncoder(delta);      break;
    case ActiveApp::QUICK_TIMER: qtHandleEncoder(delta);       break;
    case ActiveApp::STOPWATCH:   swHandleEncoder(delta);       break;
    case ActiveApp::COUNTER:     ctHandleEncoder(delta);       break;
    case ActiveApp::DICE:        diceHandleEncoder(delta);     break;
    case ActiveApp::METRONOME:   metroHandleEncoder(delta);    break;
  }
}

static void dispatchClick() {
  switch (activeApp) {
    case ActiveApp::LAUNCHER:    launcherClick();    break;
    case ActiveApp::POMODORO:    pomHandleClick();   break;
    case ActiveApp::QUICK_TIMER: qtHandleClick();    break;
    case ActiveApp::STOPWATCH:   swHandleClick();    break;
    case ActiveApp::COUNTER:     ctHandleClick();    break;
    case ActiveApp::DICE:        diceHandleClick();  break;
    case ActiveApp::METRONOME:   metroHandleClick(); break;
  }
}

static void dispatchLongPress() {
  switch (activeApp) {
    case ActiveApp::LAUNCHER:    break;  // No long press on launcher
    case ActiveApp::POMODORO:    pomHandleLongPress();  break;
    case ActiveApp::QUICK_TIMER: qtHandleLongPress();   break;
    case ActiveApp::STOPWATCH:   swHandleLongPress();   break;
    case ActiveApp::COUNTER:     ctHandleLongPress();   break;
    case ActiveApp::DICE:        diceHandleLongPress(); break;
    case ActiveApp::METRONOME:   metroHandleLongPress(); break;
  }
}

static void dispatchTick() {
  switch (activeApp) {
    case ActiveApp::POMODORO:    pomTick(); break;
    case ActiveApp::QUICK_TIMER: qtTick();     break;
    case ActiveApp::METRONOME:   metroTick(); break;
    default: break;
  }
}

static void dispatchDraw() {
  switch (activeApp) {
    case ActiveApp::LAUNCHER:    drawLauncherScreen(); break;
    case ActiveApp::POMODORO:    pomDraw();             break;
    case ActiveApp::QUICK_TIMER: qtDraw();              break;
    case ActiveApp::STOPWATCH:   swDraw();              break;
    case ActiveApp::COUNTER:     ctDraw();              break;
    case ActiveApp::DICE:        diceDraw();            break;
    case ActiveApp::METRONOME:   metroDraw();           break;
  }
}

// =============================================================================
// Arduino Setup & Loop
// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nZero Timer - Multi-App Platform");

  pinMode(ENC_SW, INPUT_PULLUP);

  // Backlight (start off)
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, LOW);

  // Buzzer
  ledcSetup(BUZZER_CHANNEL, 2000, 8);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  // Encoder
  ESP32Encoder::useInternalWeakPullResistors = puType::up;
  encoder.attachHalfQuad(ENC_CLK, ENC_DT);
  encoder.setFilter(1023);
  encoder.clearCount();

  // TFT
  pinMode(TFT_CS, OUTPUT);  digitalWrite(TFT_CS, HIGH);
  pinMode(TFT_DC, OUTPUT);  digitalWrite(TFT_DC, HIGH);
  pinMode(TFT_RST, OUTPUT); digitalWrite(TFT_RST, HIGH);

  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin(8000000);
  tft.setRotation(3);
  tft.invertDisplay(false);

  // Load saved settings
  loadSettings();
  Serial.print("Loaded focus time: ");
  Serial.print(pomTotalFocus / 3600);
  Serial.print("h ");
  Serial.print((pomTotalFocus % 3600) / 60);
  Serial.println("m");

  // Start with display off, wait for button press
  tft.fillScreen(0x0000);
  displaySleep(true);
  deviceOn = false;

  Serial.println("Press button to start...");

  // Wait for button press to turn on
  while (digitalRead(ENC_SW) == HIGH) { delay(10); }
  while (digitalRead(ENC_SW) == LOW)  { delay(10); }

  turnOn();
}

void loop() {
  // Handle OFF state
  if (!deviceOn) {
    if (digitalRead(ENC_SW) == LOW) {
      delay(50);
      while (digitalRead(ENC_SW) == LOW) { delay(10); }
      turnOn();
    }
    return;
  }

  // Timer ticks
  dispatchTick();

  // Encoder input
  int32_t delta = readEncoderDelta();
  if (delta != 0) {
    Serial.print("ENC delta="); Serial.println(delta);
    dispatchEncoder(delta);
  }

  // Button input
  int btnEvent = readButton();
  if (btnEvent == 1) {
    Serial.println("BTN click");
    dispatchClick();
  }
  if (btnEvent == 2) {
    Serial.println("BTN long");
    dispatchLongPress();
  }

  // Display
  dispatchDraw();
}
