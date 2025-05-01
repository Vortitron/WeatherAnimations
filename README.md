# Weather Animations Library for ESP32

A library for displaying animated weather icons on a SH1106 OLED display connected to an ESP32, with Home Assistant integration.

## Features

- Display different weather animations on SH1106 OLED display
- Connect to Home Assistant to fetch real-time weather data
- Multiple transition types for smooth weather changes
- Support for both static and animated icons
- Simple button interface for demonstration

## Hardware Requirements

- ESP32 development board
- SH1106 OLED display (I2C, typically at address 0x3C)
- Push buttons for demo interface (optional)

## Software Dependencies

This library requires the following Arduino libraries:

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit SH110X Library](https://github.com/adafruit/Adafruit_SH110x)
- [U8g2lib](https://github.com/olikraus/u8g2)
- Arduino ESP32 core

## Installation

### Libraries Installation

1. Install the Arduino IDE
2. Install the ESP32 board support package:
   - In Arduino IDE: Tools > Board > Boards Manager
   - Search for "esp32" and install the latest version
3. Install required libraries:
   - In Arduino IDE: Sketch > Include Library > Manage Libraries
   - Search and install:
     - "Adafruit GFX Library"
     - "Adafruit SH110X"
     - "U8g2"

### Configuration

1. Copy `examples/config_example.h` to `examples/BasicUsage/config.h`
2. Edit `config.h` with your Wi-Fi and Home Assistant credentials:
   - Wi-Fi SSID and password
   - Home Assistant IP address
   - Home Assistant long-lived access token
   - Weather entity ID to use

## Usage

### Basic Example

Load the `BasicUsage` example to display weather animations with manual control:

```arduino
// Initialize the library
weatherAnim.begin(OLED_SH1106, oledAddress, true);

// Set weather entity
weatherAnim.setWeatherEntity("weather.forecast_home");

// Set mode
weatherAnim.setMode(CONTINUOUS_WEATHER);

// Choose animation mode (ANIMATION_STATIC, ANIMATION_EMBEDDED, or ANIMATION_ONLINE)
weatherAnim.setAnimationMode(ANIMATION_EMBEDDED);

// Update in the main loop
weatherAnim.update();
```

### Buttons in Demo

The demo example uses three buttons:
- Button 1 (Encoder push): Cycle through different weather types
- Button 2 (Back): Return to live weather data from Home Assistant
- Button 3 (Left): Toggle between static and animated icons

### Display Transitions

You can manually trigger transitions between weather types:

```arduino
// Transition to a specific weather with right-to-left animation
weatherAnim.runTransition(WEATHER_CLEAR, TRANSITION_RIGHT_TO_LEFT, 500);
```

Available transition types:
- `TRANSITION_LEFT_TO_RIGHT`
- `TRANSITION_RIGHT_TO_LEFT`
- `TRANSITION_TOP_TO_BOTTOM`
- `TRANSITION_BOTTOM_TO_TOP`
- `TRANSITION_FADE`

## Troubleshooting

### SH1106 Display Issues

If you're having trouble with the SH1106 display:
- Verify the I2C address (typically 0x3C or 0x3D)
- Check the I2C connections (SDA and SCL)
- Try a lower I2C speed if you experience issues
- Make sure you've installed the Adafruit SH110X library

### Home Assistant Connection

If you can't connect to Home Assistant:
- Verify Wi-Fi credentials
- Check that your Home Assistant token has the necessary permissions
- Make sure the weather entity exists and is accessible

### Crash During Animation

If your ESP32 crashes or freezes during animation display:

1. **Library Conflicts**: There might be conflicts between U8g2 and Adafruit libraries. Try the `BasicDisplay` example first to verify your display is working correctly.

2. **Memory Issues**: The WeatherAnimations library uses both U8g2 and the Adafruit libraries which can consume significant memory. Solutions:
   - Use `ANIMATION_STATIC` mode instead of animated modes
   - Don't include implementation files directly in your sketch. Instead, install the library properly through the Arduino Library Manager
   - Add error handling with try/catch blocks around animation calls

3. **Display Initialization**: If you get a blank screen or noise:
   - Make sure to initialize the display with the correct address
   - Add a delay (200-300ms) after display initialization
   - Try different I2C speed settings

4. **Alternative Approach**: If animations still don't work:
   - Use the `BasicDisplay` example which doesn't use the animation features
   - Create your own simple weather display using just the Adafruit SH110X library
   - Consider modifying the library to use only one display driver (either U8g2 or Adafruit)

### Using Only Adafruit Libraries

For a more reliable setup, you can create a simpler weather display using just the Adafruit libraries:

```arduino
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Create display
Adafruit_SH1106G display(128, 64, &Wire, -1);

void setup() {
  // Initialize display
  display.begin(0x3C, true);
  display.clearDisplay();
  
  // Display weather
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.println("SUNNY");
  display.display();
}
```

## Contributing

Contributions to improve the library are welcome. Please submit pull requests with clear descriptions of the changes and benefits.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
