#ifndef WEATHER_ANIMATIONS_ANIMATED_ICONS_H
#define WEATHER_ANIMATIONS_ANIMATED_ICONS_H

#include <Arduino.h>

// Generated animated bitmap data for weather icons
// Each frame is 128x64 pixels, stored as 1024 bytes (128*64/8)
// Original icons from https://github.com/basmilius/weather-icons

// Animation frame counts for each weather type
#define WEATHER_CLEAR_FRAME_COUNT 10
#define WEATHER_CLOUDY_FRAME_COUNT 10
#define WEATHER_RAIN_FRAME_COUNT 10
#define WEATHER_SNOW_FRAME_COUNT 10
#define WEATHER_STORM_FRAME_COUNT 10

// Renamed variables to avoid conflicts with WeatherAnimationsAnimations.cpp
// Original: const uint8_t cloudyFrame1[1024] PROGMEM = {...};
// New: const uint8_t animated_cloudyFrame1[1024] PROGMEM = {...};

const uint8_t animated_clear_nightFrame1[1024] PROGMEM = {
    // Original content from clear_nightFrame1
};

const uint8_t animated_clear_nightFrame2[1024] PROGMEM = {
    // Original content from clear_nightFrame2
};

// ... Add all other frames using the same naming pattern ...

const uint8_t animated_cloudyFrame1[1024] PROGMEM = {
    // Original content from cloudyFrame1
};

const uint8_t animated_cloudyFrame2[1024] PROGMEM = {
    // Original content from cloudyFrame2
};

// ... Continue with all other cloudy frames ...

const uint8_t animated_rainyFrame1[1024] PROGMEM = {
    // Original content from rainyFrame1
};

// ... Continue with all other frame definitions ...

// Update the frame array pointers
const uint8_t* animated_clear_nightFrames[10] = {
    animated_clear_nightFrame1, animated_clear_nightFrame2, 
    animated_clear_nightFrame3, animated_clear_nightFrame4,
    animated_clear_nightFrame5, animated_clear_nightFrame6,
    animated_clear_nightFrame7, animated_clear_nightFrame8,
    animated_clear_nightFrame9, animated_clear_nightFrame10
};

const uint8_t* animated_cloudyFrames[10] = {
    animated_cloudyFrame1, animated_cloudyFrame2, 
    animated_cloudyFrame3, animated_cloudyFrame4,
    animated_cloudyFrame5, animated_cloudyFrame6,
    animated_cloudyFrame7, animated_cloudyFrame8,
    animated_cloudyFrame9, animated_cloudyFrame10
};

// ... Continue with all other frame array pointer definitions ...

// Animated icon mapping structure
struct AnimatedIconMapping {
    const char* condition;
    const char* variant; // 'day', 'night', or empty string
    const uint8_t** frames;
    uint8_t frameCount;
    uint16_t frameDelay; // ms between frames
};

const AnimatedIconMapping animatedWeatherIcons[] = {
    {"clear-night", "", animated_clear_nightFrames, 10, 200},
    {"cloudy", "", animated_cloudyFrames, 10, 200},
    {"rainy", "", animated_rainyFrames, 10, 200},
    // ... Add other mappings ...
    {NULL, NULL, NULL, 0, 0} // End marker
};

// Helper function to find animated icon by condition and time of day
const AnimatedIconMapping* findAnimatedWeatherIcon(const char* condition, bool isDay) {
    const char* variant = isDay ? "day" : "night";
    
    // First try to find exact match with day/night variant
    for (size_t i = 0; animatedWeatherIcons[i].condition != NULL; i++) {
        if (strcmp(animatedWeatherIcons[i].condition, condition) == 0) {
            // If this condition has a day/night variant, match it
            if (animatedWeatherIcons[i].variant[0] != '\0') {
                if (strcmp(animatedWeatherIcons[i].variant, variant) == 0) {
                    return &animatedWeatherIcons[i];
                }
            } else {
                // For conditions without day/night variants, return the first match
                return &animatedWeatherIcons[i];
            }
        }
    }
    
    // If no exact match with variant, return any match with the condition
    for (size_t i = 0; animatedWeatherIcons[i].condition != NULL; i++) {
        if (strcmp(animatedWeatherIcons[i].condition, condition) == 0) {
            return &animatedWeatherIcons[i];
        }
    }
    
    // Fallback to cloudy if no match
    for (size_t i = 0; animatedWeatherIcons[i].condition != NULL; i++) {
        if (strcmp(animatedWeatherIcons[i].condition, "cloudy") == 0) {
            return &animatedWeatherIcons[i];
        }
    }
    
    // If all else fails, return the first icon
    return &animatedWeatherIcons[0];
}

// Function to get animation frame for a given weather type and frame number
const uint8_t* getAnimationFrame(uint8_t weatherType, uint8_t frameIndex) {
    switch (weatherType) {
        case 0: // WEATHER_CLEAR
            if (frameIndex < WEATHER_CLEAR_FRAME_COUNT) {
                return animated_sunny_dayFrames[frameIndex];
            }
            break;
        case 1: // WEATHER_CLOUDY
            if (frameIndex < WEATHER_CLOUDY_FRAME_COUNT) {
                return animated_cloudyFrames[frameIndex];
            }
            break;
        case 2: // WEATHER_RAIN
            if (frameIndex < WEATHER_RAIN_FRAME_COUNT) {
                return animated_rainyFrames[frameIndex];
            }
            break;
        case 3: // WEATHER_SNOW
            if (frameIndex < WEATHER_SNOW_FRAME_COUNT) {
                return animated_snowyFrames[frameIndex];
            }
            break;
        case 4: // WEATHER_STORM
            if (frameIndex < WEATHER_STORM_FRAME_COUNT) {
                return animated_lightningFrames[frameIndex];
            }
            break;
    }
    
    // Default to first frame of cloudy if nothing matches
    return animated_cloudyFrames[0];
}

// Function to get frame count for a weather type
uint8_t getAnimationFrameCount(uint8_t weatherType) {
    switch (weatherType) {
        case 0: return WEATHER_CLEAR_FRAME_COUNT;
        case 1: return WEATHER_CLOUDY_FRAME_COUNT;
        case 2: return WEATHER_RAIN_FRAME_COUNT;
        case 3: return WEATHER_SNOW_FRAME_COUNT;
        case 4: return WEATHER_STORM_FRAME_COUNT;
        default: return WEATHER_CLOUDY_FRAME_COUNT;
    }
}

#endif // WEATHER_ANIMATIONS_ANIMATED_ICONS_H 