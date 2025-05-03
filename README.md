# Weather Animations Library for ESP32

A library for displaying animated weather icons on a SH1106 OLED display connected to an ESP32, with Home Assistant integration.

## Features

- Display different weather animations on SH1106 OLED display
- Connect to Home Assistant to fetch real-time weather data
- Multiple transition types for smooth weather changes
- Support for both static and animated icons
- Simple button interface for demonstration
- Idle timeout functionality to display weather after inactivity
- Wake from weather display with any button press

## Hardware Requirements

- ESP32 development board
- SH1106 OLED display (I2C, typically at address 0x3C)
- Push buttons for demo interface (optional)

## Software Dependencies

This library requires the following Arduino libraries:

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit SH110X Library](https://github.com/adafruit/Adafruit_SH110x) or [Adafruit SSD1306 Library](https://github.com/adafruit/Adafruit_SSD1306)
- [U8g2lib](https://github.com/olikraus/u8g2) (if using the original WeatherAnimations functions)
- [ArduinoJson](https://arduinojson.org/) (for Home Assistant API integration)
- [PNGdec](https://github.com/bitbank2/PNGdec) (for decoding PNG images from online sources)
- Arduino ESP32 core

### PNG Decoding

The library now uses the PNGdec library to properly decode PNG images from online sources. This provides several benefits:

- Proper decoding of all standard PNG formats
- Support for transparency
- Conversion of colour images to appropriate monochrome format for OLED displays
- Better image quality than the previous placeholder implementation
- Improved error handling with detailed diagnostic messages

The PNG decoder converts downloaded images to the appropriate format for display on monochrome OLED screens, using a luminance calculation and threshold to determine whether each pixel should be on or off.

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
     - "Adafruit SSD1306" (recommended for SH1106 displays on ESP32)
     - "ArduinoJson"
     - "PNGdec" (for proper PNG image decoding)

### Configuration

1. Copy `examples/config_example.h` to `examples/BasicUsage/config.h`
2. Edit `config.h` with your Wi-Fi and Home Assistant credentials:
   - Wi-Fi SSID and password
   - Home Assistant IP address
   - Home Assistant long-lived access token
   - Weather entity ID to use

## Usage

### Examples

There are several examples provided:

#### 1. BasicDisplay Example

A simple example using just the Adafruit_SSD1306 library to display static and animated weather icons:

```arduino
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Create display
Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  // Initialize display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  
  // Display weather
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.println("SUNNY");
  display.display();
}
```

#### 2. SimpleWeatherDisplay Example

A minimalist example that shows how to use the library to display weather animations after an idle timeout and dismiss with a button press:

```arduino
// Create WeatherAnimations instance with your credentials
WeatherAnimations weatherAnim(ssid, password, haIP, haToken);

// In setup()
weatherAnim.begin(OLED_SH1106, OLED_ADDR, true);
weatherAnim.setAnimationMode(ANIMATION_EMBEDDED);
weatherAnim.setWeatherEntity("weather.forecast_home");

// In loop()
// Check for idle timeout
if (!isDisplayingIdle && (millis() - lastUserActivityTime >= idleTimeout)) {
  // Show weather animation
  weatherAnim.setMode(CONTINUOUS_WEATHER);
  weatherAnim.updateWeatherData();
  isDisplayingIdle = true;
}

// Check for button press to wake
if (buttonPressed && isDisplayingIdle) {
  // Hide weather animation
  display.clearDisplay();
  // Your wake screen code here
  lastUserActivityTime = millis();
  isDisplayingIdle = false;
}

// Update animation if displaying weather
if (isDisplayingIdle) {
  weatherAnim.update();
}
```

#### 3. Full WeatherAnimations Library Usage

For more complete functionality:

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

The demo examples use three buttons:
- Button 1 (Encoder push): Cycle through different weather types
- Button 2 (Back): Return to live weather data from Home Assistant
- Button 3 (Left): Toggle between static and animated icons

### Idle Timeout Feature

The library can be used to show weather animations after a period of inactivity:

1. **Track user activity:** Set a timestamp whenever user input is detected
2. **Check idle timeout:** Compare the current time with the last activity timestamp
3. **Display weather:** When the idle timeout is reached, show weather animations
4. **Wake on button press:** Return to normal operation when any button is pressed

This is ideal for ambient displays that show weather conditions when not otherwise in use.

## SSD1306 Library for SH1106 Displays

**Important Note**: While the SH1106 and SSD1306 are different OLED controllers, the SSD1306 library often works better with SH1106 displays on ESP32. If you're experiencing crashes or display issues with the SH110X library, we recommend using the Adafruit_SSD1306 library instead.

### Benefits of using SSD1306 library:

1. Better compatibility with ESP32
2. More stable operation
3. Well-tested codebase
4. Includes all necessary graphics functions

### How to switch from SH110X to SSD1306:

1. Replace library includes:
   ```arduino
   // Instead of:
   // #include <Adafruit_SH110X.h>
   // Adafruit_SH1106G display(128, 64, &Wire, -1);
   
   // Use:
   #include <Adafruit_SSD1306.h>
   Adafruit_SSD1306 display(128, 64, &Wire, -1);
   ```

2. Change initialization:
   ```arduino
   // Instead of:
   // display.begin(0x3C, true);
   
   // Use:
   display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
   ```

3. Update color constants:
   ```arduino
   // Instead of:
   // display.setTextColor(SH110X_WHITE);
   
   // Use:
   display.setTextColor(SSD1306_WHITE);
   ```

The examples are set up to use the SSD1306 library with the SH1106 display, which provides better reliability.

## Troubleshooting

### SH1106 Display Issues

If you're having trouble with the SH1106 display:
- Verify the I2C address (typically 0x3C or 0x3D)
- Check the I2C connections (SDA and SCL)
- Try a lower I2C speed if you experience issues
- Try using the Adafruit SSD1306 library instead of the SH110X library

### PNG Decoder Issues

If you're experiencing problems with PNG images:
- Ensure the PNGdec library is installed correctly
- Verify PNG files are valid and not corrupted (test in an image viewer first)
- Check that the PNG dimensions are appropriate for your display (ideally 128x64 for most OLED displays)
- The PNG decoder requires at least 48KB of RAM, so memory issues may occur on devices with limited RAM
- Enable Serial output to see detailed error messages from the PNG decoder
- For persistent issues, try using the embedded fallback animations instead

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
   - Try the SimpleWeatherDisplay example that uses simpler direct drawing
   - Create your own simple weather display using just the SSD1306 library

## Contributing

Contributions to improve the library are welcome. Please submit pull requests with clear descriptions of the changes and benefits.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
