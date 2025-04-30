#ifndef WEATHER_ANIMATIONS_ICONS_H
#define WEATHER_ANIMATIONS_ICONS_H

#include <Arduino.h>

// Icon mapping structure
struct IconMapping {
	const char* condition;
	const char* variant; // 'day', 'night', or empty string
	const char* url; // URL to fetch the icon from
	bool isLoaded;
	uint8_t* iconData;
	size_t dataSize;
};

// Declare the standard icon mappings with online URLs
// These will be initialized in WeatherAnimationsIcons.cpp
extern IconMapping weatherIcons[];

// Helper function to find icon by condition and time of day
// This declaration allows it to be used in other files
const IconMapping* findWeatherIcon(const char* condition, bool isDay);

// Helper function to load icon data from the internet
bool loadWeatherIcon(IconMapping* icon);

// Helper function to fetch all icons in advance
void preloadWeatherIcons();

// Helper function to clear icon data and free memory
void clearWeatherIcons();

#endif // WEATHER_ANIMATIONS_ICONS_H 