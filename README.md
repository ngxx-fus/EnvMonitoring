# EnvMonitoring_New

EnvMonitoring_New is a PlatformIO project for ESP32 that monitors temperature and humidity with a DHT11 sensor and shows the result on a 128x64 OLED display. The project also connects to Wi-Fi, synchronizes time from NTP servers, and switches the OLED between environment and date/time views automatically.

## Hardware

- MCU: ESP32 DevKit
- Sensor: DHT11 on GPIO 19
- Display: OLED 128x64 over I2C
- OLED SCL: default ESP32 I2C SCL pin
- OLED SDA: default ESP32 I2C SDA pin

The implementation uses the common ESP32 default I2C pins:

- SDA = GPIO 21
- SCL = GPIO 22

## Features

- Reads DHT11 temperature and humidity values
- Stores a rolling raw sample buffer and exposes averaged values
- Connects to Wi-Fi and synchronizes date/time from NTP servers
- Shows two OLED screens and switches them automatically by timer
- Falls back to an internal software clock when NTP is not yet available
- Displays connection status text in the time screen while waiting for network or NTP data

Additional behaviors implemented:

- Multi-WiFi support: the firmware contains a prioritized `WiFiList` array. When disconnected it will try networks from top to bottom until one connects. Configure entries in `src/Config/Config.cpp`.
- BOOT0 display behavior toggle: the device uses the physical BOOT0 (IO0) button to change OLED behavior. Press the button to cycle the display mode:
  1. `SWAP` (default): automatic swap between ENV and TIME screens
  2. `FIX_ENV`: fixed on the ENV screen
  3. `FIX_TIME`: fixed on the TIME screen
  Pressing again cycles back to `SWAP`.

## Project Structure

The code is organized under `src` with module-style folders:

- `src/main.cpp` - application entry point
- `src/Config/` - system configuration values and shared return codes
- `src/Log/` - lightweight printf-style log wrapper
- `src/Task0/` - DHT11 acquisition task
- `src/Task1/` - Wi-Fi and NTP synchronization task
- `src/Task2/` - OLED rendering task

Note: the implementation uses `.cpp` files because the Arduino framework and the display/Wi-Fi/DHT libraries are C++ based.

## Module Overview

### Config

`Config` contains the main system settings:

- `DHT11_DATA_PIN` = 19
- `DHT11_MEAS_AVG_NUM` = number of raw samples used to create one averaged value
- `DHT11_UPDATE_INTERVAL_MS` = DHT11 polling interval
- `OLED128x64_SCL_PIN` and `OLED128x64_SDA_PIN` = I2C pins for the display
- `OLED128x64_ORIENTAL` = display rotation
- `OLED128x64_BACKGROUND_COLOR` = display color theme flag
- `DISP_ENV_INFO_SHOW_MS` and `DISP_TIME_INFO_SHOW_MS` = screen switch durations
- `TIME_NTP_SERVER[]` = list of NTP servers
- `TIME_UPDATE_INTERVAL_MS` = time update interval
- `WIFI_SSID[]` and `WIFI_PASSWORD[]` = Wi-Fi credentials placeholders

### Log

`Log` wraps Serial logging with variadic functions:

- `SystemLog(...)`
- `SystemWarn(...)`
- `SystemErr(...)`

### Task0

`Task0` handles DHT11 sampling:

- Reads temperature and humidity from the sensor
- Stores raw values in a rolling buffer
- Provides `DHT11_GetHumidity(index)` and `DHT11_GetTemperature(index)`
- `index = 0` returns the newest averaged slot

### Task1

`Task1` handles Wi-Fi and NTP:

- Connects the ESP32 to Wi-Fi in station mode
- Configures NTP using the configured server list
- Updates the shared time variables:
  - `g_Hour`
  - `g_Min`
  - `g_Sec`
  - `g_Year`
  - `g_Mon`
  - `g_Day`

### Task2

`Task2` controls the OLED UI:

- Environment screen:
  - `HH:MM:SS`
  - `HUMID / TEMP`
- Time screen:
  - `DD/MM/YYYY`
  - `HH:MM:SS`
- Automatically switches between the two screens using configured durations
- Shows connection status text while waiting for Wi-Fi, internet reachability, or NTP server data
- Uses an internal clock when NTP data is not ready yet

## Runtime Behavior

The application starts all tasks from `main.cpp` and then runs them continuously in the Arduino loop:

1. `Task0_Runtime()` samples the DHT11 at a fixed interval
2. `Task1_Runtime()` keeps Wi-Fi/NTP state updated
3. `Task2_Runtime()` refreshes the OLED and handles screen switching

When valid NTP time is available, the time screen shows the real date/time. If NTP is not ready yet, the display uses the internal fallback clock so the screen still advances normally.

## Configuration Notes

Before uploading, update these values in `src/Config/Config.cpp`:

- `WIFI_SSID`
- `WIFI_PASSWORD`

For multiple networks, edit the `WiFiList` array in `src/Config/Config.cpp`. The code will attempt to connect to each entry in order if disconnected.

The BOOT0 button (IO0) is used to change the OLED display behavior at runtime. It is configured as `BOOT0_PIN` in `src/Config/Config.cpp`.

If your OLED uses a different I2C address or your board uses non-default I2C pins, update the corresponding values in `Config.cpp`.

## Dependencies

The PlatformIO configuration includes:

- `adafruit/DHT sensor library`
- `adafruit/Adafruit Unified Sensor`
- `adafruit/Adafruit GFX Library`
- `adafruit/Adafruit SSD1306`
- `SPI`
- `olikraus/U8g2`

## Build and Upload

From the project root:

```bash
platformio run
platformio run --target upload
```

Serial monitor speed:

- `115200`

## GitHub

This project is already initialized as a Git repository and pushed to GitHub.
