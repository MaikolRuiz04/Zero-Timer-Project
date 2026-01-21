# Zero Timer

A minimalist Pomodoro-style focus timer built with ESP32 and ILI9341 TFT display.

![Zero Timer](https://img.shields.io/badge/Platform-ESP32-blue) ![License](https://img.shields.io/badge/License-MIT-green)

## Features

- **Focus/Break Modes** - 50 min focus, 25 min break (customizable)
- **Apple-style Dark UI** - Clean minimalist design with icons
- **Rotary Encoder Control** - CW for Break, CCW for Focus, Click to pause
- **Auto-Continue** - Automatically cycles between Focus and Break
- **Sound Feedback** - Passive buzzer melodies for state changes
- **Volume Control** - Mute, Low, Medium, High
- **Stats Tracking** - Track total focus time in hours/minutes

## Hardware

- ESP32-WROOM-32E (NodeMCU devkit)
- 3.2" ILI9341 SPI TFT Display (320x240)
- KY-040 Rotary Encoder
- Passive Buzzer

## Wiring

### Display (ILI9341)
| Signal | ESP32 GPIO |
|--------|------------|
| VCC | 5V |
| GND | GND |
| CS | GPIO 5 |
| DC | GPIO 16 |
| RST | GPIO 27 |
| MOSI | GPIO 23 |
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| LED | 3.3V/5V |

### Encoder (KY-040)
| Signal | ESP32 GPIO |
|--------|------------|
| + | 3.3V |
| GND | GND |
| CLK | GPIO 32 |
| DT | GPIO 33 |
| SW | GPIO 25 |

### Buzzer
| Pin | ESP32 GPIO |
|-----|------------|
| + | GPIO 26 |
| - | GND |

## Build & Upload

This project uses [PlatformIO](https://platformio.org/). 

```bash
# Build
pio run

# Upload
pio run --target upload
```

## Controls

- **Rotate CW** → Switch to Break mode
- **Rotate CCW** → Switch to Focus mode  
- **Click** → Pause/Resume timer
- **Long Press** → Open Settings menu

## Settings Menu

- Focus Duration (25-60 min)
- Break Duration (5-25 min)
- Auto Continue (On/Off)
- Volume (High/Med/Low/Mute)
- Focus Time (stats, click to reset)

## License

MIT License
