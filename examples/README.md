# WeatherAnimations Library Examples

This directory contains various examples demonstrating how to use the WeatherAnimations library.

## Setting Up for Development

When developing the library itself, you'll want to use the local source code rather than installing the library. Here's how to do that in the Arduino IDE:

### Option 1: Use Development Versions of Sketches (Recommended for Development)

We've created special development versions of the example sketches (named with '_Dev' suffix) that directly include the library source files. This approach lets you modify the library code and immediately see the changes without installing or zipping the library:

- Open `BasicUsage_Dev.ino` or `TFTUsage_Dev.ino` in the respective folders.
- Make changes to the library files in the `src/` directory as needed.
- Compile and upload as usual; the Arduino IDE will use the directly included source files.

### Option 2: Reference Local Library in Arduino IDE

Alternatively, you can tell Arduino IDE to temporarily reference the local library:

1. Open one of the example sketches (e.g., BasicUsage.ino or TFTUsage.ino)
2. In the Arduino IDE menu, select: **Sketch > Include Library > Add .ZIP Library...**
3. Navigate to the **root directory** of this project (the WeatherAnimations folder that contains the src/ folder)
4. Click "Open" (without creating a zip file)

The Arduino IDE will temporarily reference the local library source code, allowing you to make changes to the library and immediately see the effects in your example sketches.

## Configuration

Each example needs configuration settings for WiFi, Home Assistant connection, etc.

1. Copy `examples/config_example.h` to the example's directory (e.g., `examples/BasicUsage/config.h`)
2. Edit the new config.h file with your WiFi credentials, Home Assistant IP and token, etc.

## Available Examples

- **BasicUsage**: Demonstrates basic OLED display functionality
- **BasicUsage_Dev**: Development version that directly includes library source
- **TFTUsage**: Shows how to use the library with TFT displays
- **TFTUsage_Dev**: Development version that directly includes library source
- **MinimalWeatherStation**: A minimal weather station implementation
- **FullWeatherStation**: A complete weather station with additional features 