#include "WeatherAnimationsIcons.h"
#include <HTTPClient.h>

// Base URL for weather icons
const char* WEATHER_ICON_BASE_URL = "https://raw.githubusercontent.com/basmilius/weather-icons/master/production/fill/";

// Define the weather icon mappings with URLs
IconMapping weatherIcons[] = {
	{"clear-night", "", "moon.png", false, nullptr, 0},
	{"cloudy", "", "cloudy.png", false, nullptr, 0},
	{"fog", "", "fog.png", false, nullptr, 0},
	{"hail", "", "hail.png", false, nullptr, 0},
	{"lightning", "", "thunderstorm.png", false, nullptr, 0},
	{"lightning-rainy", "", "thunderstorms-rain.png", false, nullptr, 0},
	{"partlycloudy", "day", "partly-cloudy-day.png", false, nullptr, 0},
	{"partlycloudy", "night", "partly-cloudy-night.png", false, nullptr, 0},
	{"pouring", "", "extreme-rain.png", false, nullptr, 0},
	{"rainy", "", "rain.png", false, nullptr, 0},
	{"snowy", "", "snow.png", false, nullptr, 0},
	{"snowy-rainy", "", "sleet.png", false, nullptr, 0},
	{"sunny", "day", "clear-day.png", false, nullptr, 0},
	{"sunny", "night", "clear-night.png", false, nullptr, 0},
	{"windy", "", "wind.png", false, nullptr, 0},
	{"exceptional", "", "not-available.png", false, nullptr, 0},
	{nullptr, nullptr, nullptr, false, nullptr, 0} // End marker
};

// Helper function to find icon by condition and time of day
const IconMapping* findWeatherIcon(const char* condition, bool isDay) {
	const char* variant = isDay ? "day" : "night";
	
	// First try to find exact match with day/night variant
	for (size_t i = 0; weatherIcons[i].condition != nullptr; i++) {
		if (strcmp(weatherIcons[i].condition, condition) == 0) {
			// If this condition has a day/night variant, match it
			if (weatherIcons[i].variant[0] != '\0') {
				if (strcmp(weatherIcons[i].variant, variant) == 0) {
					return &weatherIcons[i];
				}
			} else {
				// For conditions without day/night variants, return the first match
				return &weatherIcons[i];
			}
		}
	}
	
	// If no exact match with variant, return any match with the condition
	for (size_t i = 0; weatherIcons[i].condition != nullptr; i++) {
		if (strcmp(weatherIcons[i].condition, condition) == 0) {
			return &weatherIcons[i];
		}
	}
	
	// Fallback to cloudy if no match
	for (size_t i = 0; weatherIcons[i].condition != nullptr; i++) {
		if (strcmp(weatherIcons[i].condition, "cloudy") == 0) {
			return &weatherIcons[i];
		}
	}
	
	// If all else fails, return the first icon
	return &weatherIcons[0];
}

// Load icon data from the URL specified in the icon mapping
bool loadWeatherIcon(IconMapping* icon) {
	if (icon == nullptr || icon->url == nullptr) {
		return false;
	}
	
	// If already loaded, return success
	if (icon->isLoaded && icon->iconData != nullptr) {
		return true;
	}
	
	// Clear any existing data
	if (icon->iconData != nullptr) {
		free(icon->iconData);
		icon->iconData = nullptr;
		icon->dataSize = 0;
		icon->isLoaded = false;
	}
	
	// Construct the full URL
	String fullUrl = String(WEATHER_ICON_BASE_URL) + icon->url;
	
	HTTPClient http;
	http.begin(fullUrl);
	
	int httpCode = http.GET();
	if (httpCode != 200) {
		http.end();
		return false;
	}
	
	// Get the data size
	int contentLength = http.getSize();
	if (contentLength <= 0) {
		http.end();
		return false;
	}
	
	// Allocate memory for the icon data
	icon->iconData = (uint8_t*)malloc(contentLength);
	if (icon->iconData == nullptr) {
		http.end();
		return false;
	}
	
	// Get the data
	WiFiClient* stream = http.getStreamPtr();
	size_t bytesRead = 0;
	while (http.connected() && bytesRead < contentLength) {
		if (stream->available()) {
			icon->iconData[bytesRead++] = stream->read();
		}
	}
	
	http.end();
	
	// Update the icon's data info
	icon->dataSize = bytesRead;
	icon->isLoaded = true;
	
	return (bytesRead > 0);
}

// Preload all weather icons in advance
void preloadWeatherIcons() {
	for (size_t i = 0; weatherIcons[i].condition != nullptr; i++) {
		loadWeatherIcon(&weatherIcons[i]);
		delay(100); // Small delay to prevent overwhelming the server
	}
}

// Clear all loaded icon data and free memory
void clearWeatherIcons() {
	for (size_t i = 0; weatherIcons[i].condition != nullptr; i++) {
		if (weatherIcons[i].iconData != nullptr) {
			free(weatherIcons[i].iconData);
			weatherIcons[i].iconData = nullptr;
			weatherIcons[i].dataSize = 0;
			weatherIcons[i].isLoaded = false;
		}
	}
} 