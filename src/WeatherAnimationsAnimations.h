#ifndef WEATHER_ANIMATIONS_ANIMATIONS_H
#define WEATHER_ANIMATIONS_ANIMATIONS_H

#include <Arduino.h>
#include "WeatherAnimationsIcons.h"

// Animation frames for 128x64 OLED display
// Each frame is 128x64 pixels, stored as 1024 bytes (128*64/8)

// Base URLs for fetching weather animations from online sources
extern const char* CLEAR_SKY_URL;
extern const char* CLOUDY_URL;
extern const char* RAIN_URL;
extern const char* SNOW_URL;
extern const char* STORM_URL;

// Clear Sky Animation (2 frames)
extern uint8_t clearSkyFrame1[1024];
extern uint8_t clearSkyFrame2[1024];
extern const uint8_t* clearSkyFrames[2];

// Cloudy Animation (2 frames)
extern uint8_t cloudyFrame1[1024];
extern uint8_t cloudyFrame2[1024];
extern const uint8_t* cloudySkyFrames[2];

// Rain Animation (3 frames)
extern uint8_t rainFrame1[1024];
extern uint8_t rainFrame2[1024];
extern uint8_t rainFrame3[1024];
extern const uint8_t* rainFrames[3];

// Snow Animation (3 frames)
extern uint8_t snowFrame1[1024];
extern uint8_t snowFrame2[1024];
extern uint8_t snowFrame3[1024];
extern const uint8_t* snowFrames[3];

// Storm Animation (2 frames)
extern uint8_t stormFrame1[1024];
extern uint8_t stormFrame2[1024];
extern const uint8_t* stormFrames[2];

// Functions for fetching and initializing animations
bool pngToBitmap(uint8_t* pngData, size_t pngSize, uint8_t* bitmap, size_t bitmapSize);
bool fetchAnimationFrames(const char* baseURL, uint8_t** frames, int frameCount, size_t frameSize);
bool initializeAnimationsFromOnline(uint8_t displayType);
void generateFallbackAnimations();

// Helper functions for drawing fallback animations
void drawCloud(int centerX, int centerY, int width, int height, uint8_t* buffer);
void drawRainDrop(int x, int y, uint8_t* buffer);
void drawSnowflake(int x, int y, uint8_t* buffer);
void drawLightning(int x, int y, uint8_t* buffer);

#endif // WEATHER_ANIMATIONS_ANIMATIONS_H 