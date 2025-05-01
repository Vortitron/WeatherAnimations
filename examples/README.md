# WeatherAnimations Library Examples

This directory contains various examples demonstrating how to use the WeatherAnimations library.

## Setting Up for Development

When developing the library itself, you'll want to use the local source code rather than installing the library. Here's how to do that in the Arduino IDE:

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
- **TFTUsage**: Shows how to use the library with TFT displays
- **MinimalWeatherStation**: A minimal weather station implementation
- **FullWeatherStation**: A complete weather station with additional features 