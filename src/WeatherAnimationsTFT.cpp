#include "WeatherAnimations.h"
#include "WeatherAnimationsAnimations.h"

// Only include TFT code if we're actually using it
#if defined(ESP32) || defined(ESP8266)

#include <TFT_eSPI.h>

// Reference to the global display object
extern TFT_eSPI* tftDisplay;

// Define debug print macro to match the rest of the library
#ifdef SERIAL_DEBUG
  #define WA_SERIAL_PRINTLN(x) Serial.println(x)
#else
  #define WA_SERIAL_PRINTLN(x)
#endif

// Implementation of renderTFTAnimation method
void WeatherAnimationsLib::WeatherAnimations::renderTFTAnimation(uint8_t weatherCondition) {
	// Check if TFT display is initialized
	if (tftDisplay == nullptr) {
		WA_SERIAL_PRINTLN("TFT display not initialized");
		return;
	}
	
	// If we have cached frames, use them
	if (_onlineAnimationCache[weatherCondition].isLoaded && 
		_onlineAnimationCache[weatherCondition].isAnimated &&
		_onlineAnimationCache[weatherCondition].frameCount > 0) {
		
		// In a real implementation, you would decode and display 
		// the appropriate frame from the GIF data
		// For now, we'll use a simplified fallback rendering
		
		// Clear a portion of the screen for our animation
		tftDisplay->fillRect(60, 60, 120, 120, TFT_BLACK);
		
		// Draw different things based on the frame number to show animation
		// This is a simplified version, should be replaced with actual GIF rendering
		switch (weatherCondition) {
			case WEATHER_CLEAR: {
				// Animated sun
				tftDisplay->fillCircle(120, 120, 30, TFT_YELLOW);
				// Draw rays with varying length based on current frame
				for (int i = 0; i < 8; i++) {
					float angle = i * PI / 4.0;
					int rayLength = (_currentFrame % 2 == 0) ? 20 : 15;
					int x1 = 120 + cos(angle) * 30;
					int y1 = 120 + sin(angle) * 30;
					int x2 = 120 + cos(angle) * (30 + rayLength);
					int y2 = 120 + sin(angle) * (30 + rayLength);
					tftDisplay->drawLine(x1, y1, x2, y2, TFT_YELLOW);
				}
				break;
			}
				
			case WEATHER_CLOUDY: {
				// Animated cloud (slight movement)
				int offset = (_currentFrame % 2 == 0) ? 0 : 5;
				tftDisplay->fillRoundRect(90 + offset, 120, 70, 25, 10, TFT_LIGHTGREY);
				tftDisplay->fillRoundRect(80 + offset, 100, 50, 30, 15, TFT_WHITE);
				break;
			}
				
			case WEATHER_RAIN: {
				// Cloud with animated rain drops
				tftDisplay->fillRoundRect(80, 80, 80, 30, 15, TFT_LIGHTGREY);
				// Draw rain drops at different positions based on frame
				for (int i = 0; i < 6; i++) {
					int dropOffset = (i + _currentFrame) % 3 * 15;
					tftDisplay->drawLine(85 + i*15, 110 + dropOffset, 90 + i*15, 120 + dropOffset, TFT_BLUE);
				}
				break;
			}
				
			case WEATHER_SNOW: {
				// Cloud with animated snowflakes
				tftDisplay->fillRoundRect(80, 80, 80, 30, 15, TFT_LIGHTGREY);
				// Draw snowflakes at different positions based on frame
				for (int i = 0; i < 6; i++) {
					int flakeOffset = (i + _currentFrame) % 3 * 15;
					tftDisplay->drawPixel(85 + i*15, 110 + flakeOffset, TFT_WHITE);
					tftDisplay->drawPixel(85 + i*15 + 1, 110 + flakeOffset, TFT_WHITE);
					tftDisplay->drawPixel(85 + i*15, 110 + flakeOffset + 1, TFT_WHITE);
					tftDisplay->drawPixel(85 + i*15 + 1, 110 + flakeOffset + 1, TFT_WHITE);
				}
				break;
			}
				
			case WEATHER_STORM: {
				// Cloud with animated lightning
				tftDisplay->fillRoundRect(80, 80, 80, 30, 15, TFT_DARKGREY);
				// Draw lightning in some frames only
				if (_currentFrame % 3 != 0) {
					tftDisplay->fillTriangle(120, 110, 110, 130, 130, 130, TFT_YELLOW);
					if (_currentFrame % 2 == 0) {
						tftDisplay->fillTriangle(125, 130, 115, 150, 135, 150, TFT_YELLOW);
					}
				}
				break;
			}
				
			default: {
				// Unknown weather, show a question mark
				tftDisplay->setTextSize(4);
				tftDisplay->setTextColor(TFT_WHITE);
				tftDisplay->setCursor(110, 100);
				tftDisplay->print("?");
				break;
			}
		}
	} else {
		// If no animation data is available, display a simple static icon
		displayTextFallback(weatherCondition);
	}
}

#else // If not ESP32 or ESP8266

// Empty implementation for platforms without TFT support
void WeatherAnimationsLib::WeatherAnimations::renderTFTAnimation(uint8_t weatherCondition) {
	// Do nothing - TFT not supported on this platform
}

#endif // ESP32 || ESP8266 