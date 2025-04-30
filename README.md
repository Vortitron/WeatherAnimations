# WeatherAnimations Library

A versatile Arduino library for displaying weather animations on OLED and TFT displays. This library fetches weather data from Home Assistant and displays appropriate animations based on current conditions.

## Features

- Supports both OLED (SSD1306) and TFT displays
- Connects to Home Assistant to fetch weather data
- Multiple animation modes:
  - **Static Mode**: Basic static weather icons
  - **Embedded Mode**: Pre-loaded animations stored in program memory
  - **Online Mode**: Dynamically fetches animation data from online sources
- Smooth transitions between weather states
- Handles various weather conditions: clear, cloudy, rain, snow, and storms
- Low memory footprint with fallback capabilities when network is unavailable

## Animation Modes

The library supports three animation modes:

1. **ANIMATION_STATIC**: Uses simple static images for each weather condition, consuming minimal memory.
2. **ANIMATION_EMBEDDED**: Uses animations embedded in the program memory, providing a balance between visual appeal and memory usage.
3. **ANIMATION_ONLINE**: Fetches animation data from online sources, providing the richest visuals but requiring an active internet connection.

## Online Animation Support

The online animation feature allows the library to fetch animation frames from web URLs. If the online resources are unavailable, the library gracefully falls back to locally generated animations.

### Animation Sources

The library supports fetching animation frames from our GitHub repository. These are stored as PNG files with a naming convention of `[weather_condition]_frame_[frame_number].png`. For example:

- sunny-day_frame_000.png
- sunny-day_frame_001.png
- etc.

### Setting Up Online Animations

To use online animations, set the animation mode to `ANIMATION_ONLINE` and provide base URLs for each weather condition:

```cpp
// Set animation mode to online
weatherAnim.setAnimationMode(ANIMATION_ONLINE);

// Set online animation sources (base URLs)
weatherAnim.setOnlineAnimationSource(WEATHER_CLEAR, 
    "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/sunny-day_frame_");
weatherAnim.setOnlineAnimationSource(WEATHER_CLOUDY, 
    "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/cloudy_frame_");
// Add other weather conditions as needed
```

The library will append the frame number and file extension (e.g., "000.png", "001.png") to fetch each animation frame.

## Examples

The library includes several examples:

1. **BasicUsage**: A simple demonstration with button control to switch between animation modes.
2. **MinimalWeatherStation**: A minimal implementation with online animation support.
3. **FullWeatherStation**: A comprehensive example showing advanced features and multiple display support.

## Getting Started

1. Install the library via the Arduino Library Manager or download from GitHub.
2. Connect your display (OLED or TFT) to your Arduino/ESP board.
3. Set up your Home Assistant connection in the example sketch.
4. Upload one of the example sketches to your board.

## Configuration

For each example, copy the `config_example.h` file to the example directory and rename it to `config.h`. Then modify it with your WiFi and Home Assistant credentials:

```cpp
// WiFi Settings
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"

// Home Assistant Settings
#define HA_IP "your-home-assistant-ip"
#define HA_TOKEN "your-long-lived-access-token"
#define HA_WEATHER_ENTITY "weather.your_weather_entity"

// OLED Settings
#define OLED_ADDRESS 0x3C
```

## Button Controls

The examples use buttons to provide user control:

- **Encoder Push Button**: Cycle through weather animations in manual mode
- **Back Button**: Return to live weather data mode
- **Left Button / Mode Button**: Toggle between static and online animation modes

## Contributing

Contributions to improve the library are welcome. Please feel free to submit issues and pull requests on GitHub.

## License

This library is released under the MIT License.

## Required Libraries

- Adafruit SSD1306
- Adafruit GFX Library
- TFT_eSPI (for TFT displays)
- ESP8266WiFi or WiFi (depending on platform)
- ESP8266HTTPClient or HTTPClient (depending on platform)
- ArduinoJson

## Credits

Weather icons based on designs from https://github.com/basmilius/weather-icons
