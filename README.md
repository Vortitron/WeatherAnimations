# Weather Animations Library

A library for displaying animated weather icons on OLED and TFT displays for Arduino and ESP32/ESP8266 projects. This library connects to Home Assistant to fetch weather data and display appropriate animations.

## Features

- Support for both OLED (monochrome) and TFT (color) displays
- Animated weather icons for various conditions
- Home Assistant integration for real-time weather data
- Configurable refresh intervals and display modes
- Transition effects between animations
- Day/Night variants for weather conditions
- Customizable and easily extendable

## Installation

1. Download the latest release as a ZIP file
2. In the Arduino IDE, go to Sketch > Include Library > Add .ZIP Library
3. Select the downloaded ZIP file
4. The library will be installed and available in the Arduino IDE

## Configuration

For security reasons, sensitive configuration details like WiFi credentials and API keys should be kept separate from your main sketch. The library provides a configuration approach to help with this:

1. Copy the `examples/config_example.h` file to the same directory as your sketch and rename it to `config.h`
2. Edit `config.h` with your personal settings (WiFi credentials, Home Assistant IP and token)
3. Add `#include "config.h"` to your sketch
4. Add `config.h` to your `.gitignore` file to keep credentials private

Example configuration file:

```cpp
// WiFi credentials
#define WIFI_SSID       "YourWiFiSSID"
#define WIFI_PASSWORD   "YourWiFiPassword"

// Home Assistant settings
#define HA_IP           "192.168.1.100"
#define HA_PORT         8123
#define HA_TOKEN        "YourHomeAssistantLongLivedToken"
#define HA_WEATHER_ENTITY "weather.forecast_home"
```

## Usage

The library includes several examples to help you get started:

- **BasicUsage**: Simple example for OLED displays
- **TFTUsage**: Example for TFT displays with online animations
- **FullWeatherStation**: Advanced example with both display types and more features

Basic usage:

```cpp
#include <WeatherAnimations.h>
#include "config.h"  // Your configuration file

// Create WeatherAnimations instance with credentials from config.h
WeatherAnimations weatherAnim(WIFI_SSID, WIFI_PASSWORD, HA_IP, HA_TOKEN);

void setup() {
  // Initialize with OLED display
  weatherAnim.begin(OLED_DISPLAY, OLED_ADDRESS, true);
  
  // Set the weather entity ID from Home Assistant
  weatherAnim.setWeatherEntity(HA_WEATHER_ENTITY);
  
  // Set mode to continuous weather display
  weatherAnim.setMode(CONTINUOUS_WEATHER);
}

void loop() {
  // Update the display with current weather
  weatherAnim.update();
  
  // Small delay to prevent excessive updates
  delay(100);
}
```

## Requirements

- Arduino IDE 1.8.0 or later
- ESP8266 or ESP32 board (recommended) or Arduino board with sufficient memory
- OLED display (SSD1306 or similar) or TFT display (ILI9341, ST7735, ST7789, etc.)
- Stable WiFi connection for Home Assistant integration

## License

This library is released under the MIT License. See LICENSE file for details.

## Credits

- Weather icons based on [weather-icons by Bas Milius](https://github.com/basmilius/weather-icons)
- Animation conversion script developed specifically for this project
