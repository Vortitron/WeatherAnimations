#ifndef WEATHER_ANIMATIONS_ANIMATIONS_H
#define WEATHER_ANIMATIONS_ANIMATIONS_H

#include <Arduino.h>
#include "WeatherAnimationsIcons.h"

// Animation frames for 128x64 OLED display
// Each frame is 128x64 pixels, stored as 1024 bytes (128*64/8)

// Clear Sky Animation (2 frames)
extern const uint8_t clearSkyFrame1[1024] PROGMEM;
extern const uint8_t clearSkyFrame2[1024] PROGMEM;
extern const uint8_t* clearSkyFrames[2];

// Cloudy Animation (2 frames)
extern const uint8_t cloudyFrame1[1024] PROGMEM;
extern const uint8_t cloudyFrame2[1024] PROGMEM;
extern const uint8_t* cloudyFrames[2];

// Rain Animation (3 frames)
extern const uint8_t rainFrame1[1024] PROGMEM;
extern const uint8_t rainFrame2[1024] PROGMEM;
extern const uint8_t rainFrame3[1024] PROGMEM;
extern const uint8_t* rainFrames[3];

// Snow Animation (3 frames)
extern const uint8_t snowFrame1[1024] PROGMEM;
extern const uint8_t snowFrame2[1024] PROGMEM;
extern const uint8_t snowFrame3[1024] PROGMEM;
extern const uint8_t* snowFrames[3];

// Storm Animation (2 frames)
extern const uint8_t stormFrame1[1024] PROGMEM;
extern const uint8_t stormFrame2[1024] PROGMEM;
extern const uint8_t* stormFrames[2];


// Import icon mapping from WeatherAnimationsIcons.h
extern const IconMapping weatherIcons[];
extern const IconMapping* findWeatherIcon(const char* condition, bool isDay);

#endif // WEATHER_ANIMATIONS_ANIMATIONS_H 