# WeatherAnimations

## Overview

`WeatherAnimations` is an Arduino library designed to create transition animations on small OLED or TFT screens based on weather data retrieved from Home Assistant. The library supports two modes:
- **Simple Transition Mode**: Displays a brief animation transitioning between weather states.
- **Continuous Weather Display Mode**: Shows the current weather until interrupted by another event or command.

This library connects to Home Assistant via Wi-Fi to fetch current weather conditions and predictions, then uses this data to trigger appropriate animations on the connected display.

## Features

- Connects to Home Assistant to retrieve real-time weather data from services like Met.no.
- Supports both OLED (monochrome) and TFT (colour) displays.
- Two operational modes for flexible display options.
- Customizable animations for different weather conditions.
- Hybrid animation approach:
  - Built-in monochrome bitmap animations for OLED displays, stored in PROGMEM
  - Online fetch capability for colour animations on TFT displays
- Wi-Fi connection management with option to use a pre-existing connection.

## Installation

1. Download the library as a ZIP file from the GitHub repository.
2. In the Arduino IDE, go to `Sketch` > `Include Library` > `Add .ZIP Library...` and select the downloaded file.
3. The library will now be available under `Sketch` > `Include Library` > `WeatherAnimations`.

## Usage

### Basic Example

```cpp
#include <WeatherAnimations.h>

// Replace with your Wi-Fi and Home Assistant credentials
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* haIP = "YourHomeAssistantIP"; // e.g., "192.168.1.100"
const char* haToken = "YourHomeAssistantToken"; // Long-lived access token from Home Assistant

// Create WeatherAnimations instance
WeatherAnimations weatherAnim(ssid, password, haIP, haToken);

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  
  // Initialize with OLED display
  weatherAnim.begin(OLED_DISPLAY, 0x3C);
  
  // Set custom weather entity ID from Home Assistant
  weatherAnim.setWeatherEntity("weather.forecast_home");
  
  // Set mode to continuous weather display
  weatherAnim.setMode(CONTINUOUS_WEATHER);
}

void loop() {
  // Update weather data and display animations
  weatherAnim.update();
  
  // Add a small delay to prevent excessive updates
  delay(100);
}
```

### TFT Display with Online Animations

```cpp
#include <WeatherAnimations.h>

// Replace with your Wi-Fi and Home Assistant credentials
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* haIP = "YourHomeAssistantIP";
const char* haToken = "YourHomeAssistantToken";

void setup() {
  Serial.begin(115200);
  
  // Initialize with TFT display
  weatherAnim.begin(TFT_DISPLAY);
  
  // Set custom weather entity ID
  weatherAnim.setWeatherEntity("weather.forecast_home");
  
  // Set online animation sources for TFT display
  weatherAnim.setOnlineAnimationSource(WEATHER_CLEAR, "https://example.com/animations/clear_sky.jpg");
  weatherAnim.setOnlineAnimationSource(WEATHER_RAIN, "https://example.com/animations/rain.jpg");
  // Set sources for other weather conditions as needed
  
  // Set mode to simple transition
  weatherAnim.setMode(SIMPLE_TRANSITION);
}

void loop() {
  weatherAnim.update();
  delay(100);
}
```

## Customizing Animations

You can customize the animations for different weather conditions in several ways:

### 1. Custom Local Animations for OLED

```cpp
// Create bitmap arrays for your custom animations
const uint8_t myCustomFrame1[1024] PROGMEM = { /* bitmap data */ };
const uint8_t myCustomFrame2[1024] PROGMEM = { /* bitmap data */ };
const uint8_t* myCustomFrames[2] = {myCustomFrame1, myCustomFrame2};

// Set custom animation for clear sky condition
weatherAnim.setAnimation(WEATHER_CLEAR, myCustomFrames, 2, 300); // 2 frames, 300ms delay
```

### 2. Online Animations for TFT

```cpp
// Set URL to your hosted animation image for cloudy weather
weatherAnim.setOnlineAnimationSource(WEATHER_CLOUDY, "https://example.com/animations/cloudy.jpg");
```

## Using Hosted Weather Icons for TFT Displays

This project leverages a forked version of the [weather-icons](https://github.com/basmilius/weather-icons) repository by Bas Milius to source animated weather icons. These icons are converted to suitable formats and hosted on GitHub for on-the-fly fetching by ESP8266/ESP32 devices for TFT displays.

- **Forked Repository**: [https://github.com/vortitron/weather-icons](https://github.com/vortitron/weather-icons) (adjust if your username differs)
- **Hosted Files**: Images are stored in `production/tft` for colour TFT displays.
- **Integration**: Use `setOnlineAnimationSource()` to fetch these images based on weather conditions. Example:
  ```cpp
  weatherAnim.setOnlineAnimationSource(WEATHER_CLEAR, "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft/clear-day.png");
  weatherAnim.setOnlineAnimationSource(WEATHER_RAIN, "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft/rain.png");
  // Add more conditions as needed
  ```

Ensure your device has Wi-Fi connectivity to fetch these resources. For OLED displays, consider embedding monochrome bitmaps directly in the sketch due to memory constraints.

## Weather Icon Integration

This project now includes support for the beautiful animated weather icons from [Bas Milius's weather-icons repository](https://github.com/basmilius/weather-icons). Two Python scripts have been provided to convert these SVG icons to suitable formats for both OLED and TFT displays:

### Converting Icons to Arduino-Compatible Formats

1. **Fork the Repository**:
   - First, fork the [weather-icons repository](https://github.com/basmilius/weather-icons) to your GitHub account
   - Clone your forked repository to your local machine

2. **Convert the Icons**:
   - Use the provided `convert_weather_icons.py` script to convert the SVG icons to suitable formats:
     ```bash
     python3 convert_weather_icons.py /path/to/weather-icons
     ```
   - This will generate:
     - PNG files for TFT displays in `/path/to/weather-icons/production/tft/`
     - Monochrome PNGs for OLED displays in `/path/to/weather-icons/production/oled/`
     - A C++ header file (`WeatherAnimationsIcons.h`) with embedded bitmap data for OLED
     - A JSON mapping file (`weather_icon_urls.json`) for TFT display online fetching

3. **Integrate with WeatherAnimations**:
   - Use the provided `update_weatheranimations.py` script to integrate the converted icons:
     ```bash
     python3 update_weatheranimations.py /path/to/WeatherAnimations /path/to/weather_icon_urls.json
     ```
   - This will:
     - Copy the generated header file to your library's source directory
     - Update your library's code to use the new icons
     - Add a new method to handle weather conditions directly from Home Assistant

4. **Upload to GitHub**:
   - Push the generated PNGs to your forked repository
   - Ensure your repository is publicly accessible for TFT online fetching

### Using Weather Icons

The updated library seamlessly integrates with Home Assistant's weather states. It now supports:

1. **Direct Condition Handling**:
   - The library automatically maps Home Assistant weather conditions to appropriate icons
   - Day/night variants are selected based on time of day

2. **Display Type Detection**:
   - For OLED displays: embedded monochrome bitmaps are used (better performance)
   - For TFT displays: icons are fetched on-the-fly from your GitHub repository

3. **Supported Weather Conditions**:
   - All standard Met.no weather conditions are supported:
     - clear-night, cloudy, fog, hail, lightning, lightning-rainy
     - partlycloudy, pouring, rainy, snowy, snowy-rainy
     - sunny, windy, windy-variant, exceptional

### Requirements for Icon Conversion

The conversion scripts require:
- Python 3.6+
- Inkscape (for SVG to PNG conversion)
- ImageMagick (for image processing)
- PIL/Pillow (Python Imaging Library, automatically installed if missing)

Install these dependencies before running the scripts:
```bash
# On Ubuntu/Debian
sudo apt-get install inkscape imagemagick python3-pil

# On macOS with Homebrew
brew install inkscape imagemagick
pip3 install pillow
```

## Using Animated Weather Icons

The library now supports fully animated weather icons in addition to static images. You can now display weather conditions with beautiful animations that bring your weather display to life.

### Animation Modes

The library supports three animation modes:

1. **Static Mode (ANIMATION_STATIC)**: Displays only the first frame of each animation (default mode).
2. **Embedded Animation (ANIMATION_EMBEDDED)**: Uses frame sequences stored in the device's flash memory.
3. **Online Animation (ANIMATION_ONLINE)**: Fetches animated GIFs from your GitHub repository.

### Setting Up Animated Icons

```cpp
#include <WeatherAnimations.h>

// Create WeatherAnimations instance
WeatherAnimations weatherAnim(ssid, password, haIP, haToken);

void setup() {
  // Initialize with your preferred display
  weatherAnim.begin(TFT_DISPLAY);  // TFT display for colour animations
  
  // Set to online animation mode (for TFT displays)
  weatherAnim.setAnimationMode(ANIMATION_ONLINE);
  
  // Set animation sources (animated GIFs)
  weatherAnim.setOnlineAnimationSource(WEATHER_CLEAR, 
    "https://raw.githubusercontent.com/your-username/weather-icons/main/production/tft_animated/sunny-day.gif");
  weatherAnim.setOnlineAnimationSource(WEATHER_RAIN, 
    "https://raw.githubusercontent.com/your-username/weather-icons/main/production/tft_animated/rainy.gif");
  
  // Or for embedded animations on OLED displays
  weatherAnim.begin(OLED_DISPLAY);
  weatherAnim.setAnimationMode(ANIMATION_EMBEDDED);
}

void loop() {
  // The update function now handles animating the icons
  weatherAnim.update();
  delay(10);
}
```

### Converting SVG to Animated Icons

Use the provided `convert_animated_weather_icons.py` script to extract animation frames from SVG files and convert them to:
1. Animated GIFs for TFT displays
2. Sequences of bitmap frames for OLED displays

```bash
python3 convert_animated_weather_icons.py /path/to/weather-icons
```

This will:
- Extract animation frames from the SVG files based on their animation timeline
- Create animated GIFs for TFT displays
- Generate bitmap sequences for OLED displays
- Create a C++ header file with the bitmap data
- Generate a URL mapping file for online fetching

### Combining Transitions and Animations

You can combine transitions with animations for even more impressive visual effects:

```cpp
// Start with a rain animation sliding in from the right
weatherAnim.runTransition(WEATHER_RAIN, TRANSITION_RIGHT_TO_LEFT, 1500);

// When the transition completes, the animation will continue to play
while(!weatherAnim.runTransition(WEATHER_RAIN, TRANSITION_RIGHT_TO_LEFT, 1500)) {
  delay(10);
}

// The animation continues playing after the transition
```

### Tips for Animated Icons

- **For OLED displays**: Use the embedded animation mode with simple animations (fewer frames)
- **For TFT displays**: Use online animation mode with GIFs for full colour animations
- **Memory usage**: Be mindful of memory usage, especially with embedded animations on ESP8266
- **Performance**: If animations are too slow, reduce the number of frames or use simpler animations
- **Customization**: Create your own animations by modifying the SVGs in the weather-icons repository

## Using Transition Animations

The library now includes support for smooth transition animations that can be used as interstitials between screens. These transitions add a polished look to your user interface when switching between different content.

### Available Transition Effects

- **Right to Left**: Content slides in from the right edge
- **Left to Right**: Content slides in from the left edge
- **Top to Bottom**: Content slides in from the top
- **Bottom to Top**: Content slides in from the bottom
- **Fade**: Content fades in gradually

### Using Transitions in Your Sketch

```cpp
#include <WeatherAnimations.h>

// Create WeatherAnimations instance
WeatherAnimations weatherAnim(ssid, password, haIP, haToken);

void setup() {
  // Initialize with your preferred display
  weatherAnim.begin(OLED_DISPLAY, 0x3C);
}

void loop() {
  // Your regular code...
  
  // When you want to show a transition, call runTransition:
  // Parameters: weatherCondition, direction, duration in milliseconds
  weatherAnim.runTransition(WEATHER_RAIN, TRANSITION_RIGHT_TO_LEFT, 1500);
  
  // The transition is running. You can wait for it to complete:
  while(!weatherAnim.runTransition(WEATHER_RAIN, TRANSITION_RIGHT_TO_LEFT, 1500)) {
    // The function returns true when the transition is complete
    delay(10); // Small delay to avoid overloading the processor
  }
  
  // Now the transition is complete, you can continue with other operations
}
```

### Tips for Using Transitions

- Keep transitions between 500-2000ms for best visual effect
- For quick UI changes, use `TRANSITION_FADE` with shorter durations
- For showcasing weather changes, longer sliding transitions work well
- The transition automatically uses the correct animation based on display type (OLED or TFT)
- When using as an interstitial, consider triggering the transition before loading new content

## Requirements

- Arduino IDE
- ESP8266 or ESP32 board (for Wi-Fi connectivity)
- OLED (SSD1306) or TFT display
- Libraries:
  - `Adafruit_GFX`, `Adafruit_SSD1306` (for OLED)
  - `TFT_eSPI` (for TFT)
  - `WiFi`, `HTTPClient`

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request or open an Issue on GitHub.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
