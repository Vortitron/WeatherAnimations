# WeatherAnimations

## Overview

`WeatherAnimations` is an Arduino library designed to create transition animations on small OLED or TFT screens based on weather data retrieved from Home Assistant. The library supports two modes:
- **Simple Transition Mode**: Displays a brief animation transitioning between weather states.
- **Continuous Weather Display Mode**: Shows the current weather until interrupted by another event or command.

This library connects to Home Assistant via Wi-Fi to fetch current weather conditions and predictions, then uses this data to trigger appropriate animations on the connected display.

## Features

- Connects to Home Assistant to retrieve real-time weather data.
- Supports OLED and TFT displays.
- Two operational modes for flexible display options.
- Customizable animations for different weather conditions.

## Installation

1. Download the library as a ZIP file from the GitHub repository.
2. In the Arduino IDE, go to `Sketch` > `Include Library` > `Add .ZIP Library...` and select the downloaded file.
3. The library will now be available under `Sketch` > `Include Library` > `WeatherAnimations`.

## Usage

```cpp
#include <WeatherAnimations.h>

// Initialize with your Wi-Fi and Home Assistant details
WeatherAnimations weatherAnim("YourWiFiSSID", "YourWiFiPassword", "YourHomeAssistantIP", "YourHomeAssistantToken");

void setup() {
  // Start the connection and display
  weatherAnim.begin();
  // Set mode to continuous display
  weatherAnim.setMode(CONTINUOUS_WEATHER);
}

void loop() {
  // Update weather data and display animations
  weatherAnim.update();
}
```

## Requirements

- Arduino IDE
- ESP8266 or ESP32 board (for Wi-Fi connectivity)
- OLED or TFT display compatible with Adafruit_GFX library
- Libraries: `Adafruit_GFX`, `Adafruit_SSD1306` (for OLED), or `TFT_eSPI` (for TFT), `WiFi`, `HTTPClient`

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request or open an Issue on GitHub.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
