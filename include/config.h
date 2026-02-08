#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <ESP32Encoder.h>
#include <Preferences.h>

// =============================================================================
// Hardware Pin Definitions
// =============================================================================

static constexpr int TFT_CS   = 5;
static constexpr int TFT_DC   = 16;
static constexpr int TFT_RST  = 27;
static constexpr int TFT_MOSI = 23;
static constexpr int TFT_SCK  = 18;
static constexpr int TFT_MISO = 19;
static constexpr int TFT_LED  = 4;

static constexpr int ENC_CLK  = 32;
static constexpr int ENC_DT   = 33;
static constexpr int ENC_SW   = 25;

static constexpr int BUZZER_PIN     = 26;
static constexpr int BUZZER_CHANNEL = 0;

// =============================================================================
// Colors (Apple-inspired Dark Mode)
// =============================================================================

static constexpr uint16_t COLOR_BG              = 0x0841;  // Near black
static constexpr uint16_t COLOR_SURFACE         = 0x18E3;  // Dark gray card
static constexpr uint16_t COLOR_TEXT            = 0xFFFF;  // White
static constexpr uint16_t COLOR_TEXT_DIM        = 0x7BEF;  // Gray
static constexpr uint16_t COLOR_ACCENT_FOCUS    = 0x2D7F;  // Soft blue
static constexpr uint16_t COLOR_ACCENT_BREAK    = 0x2E8B;  // Soft green
static constexpr uint16_t COLOR_ACCENT_PAUSE    = 0xFB80;  // Soft orange
static constexpr uint16_t COLOR_ACCENT_QTIMER   = 0xFD20;  // Warm amber
static constexpr uint16_t COLOR_ACCENT_STOPWATCH = 0x07FF; // Cyan
static constexpr uint16_t COLOR_RED              = 0xF800;  // Red
static constexpr uint16_t COLOR_ACCENT_COUNTER  = 0x07C0;  // Green
static constexpr uint16_t COLOR_ACCENT_DICE     = 0xB81F;  // Purple
static constexpr uint16_t COLOR_ACCENT_METRO    = 0xFC10;  // Salmon pink

// =============================================================================
// Timing Constants
// =============================================================================

static constexpr uint32_t LONG_PRESS_MS    = 800;
static constexpr int32_t  COUNTS_PER_DETENT = 2;
