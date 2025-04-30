# WeatherAnimations Library

A library for displaying animated weather icons and forecasts on OLED and TFT displays.

## Overview

This library allows you to display weather animations from various sources:
- Embedded animations (stored in program memory)
- Online animations (downloaded and cached)
- Static icons

It supports both OLED displays (via Adafruit_SSD1306) and TFT displays (via TFT_eSPI), and can connect to Home Assistant for weather data.

## Installation

### Using PlatformIO

1. Add the library to your `platformio.ini` file:

```ini
lib_deps =
    https://github.com/vortitron/WeatherAnimations.git
```

2. The necessary dependencies will be installed automatically.

### Using Arduino IDE

1. Install the following libraries through the Arduino Library Manager:
   - Adafruit SSD1306
   - Adafruit GFX Library
   - TFT_eSPI
   - ArduinoJson

2. Download this library as a ZIP file and add it through Sketch > Include Library > Add .ZIP Library...

## Required Libraries

- Adafruit SSD1306
- Adafruit GFX Library
- TFT_eSPI (for TFT displays)
- ESP8266WiFi or WiFi (depending on platform)
- ESP8266HTTPClient or HTTPClient (depending on platform)
- ArduinoJson

## Examples

See the `examples` directory for examples, including:
- Basic OLED weather display
- TFT weather animations
- Full weather station with Home Assistant integration

## Configuration

The library can be configured for different displays and data sources. See the example configurations in the `examples` directory.

## Troubleshooting

### Serial Undefined Error
If you encounter `identifier "Serial" is undefined` errors when compiling:

1. Make sure to initialize Serial in your sketch before using the library:
   ```cpp
   void setup() {
     Serial.begin(115200);
     // Rest of your setup code...
   }
   ```

2. If you don't need Serial debugging, you can modify the library to disable Serial calls by defining:
   ```cpp
   #define WA_DISABLE_SERIAL
   ```
   before including the library.

## License

MIT License

## Credits

Weather icons based on designs from https://github.com/basmilius/weather-icons
