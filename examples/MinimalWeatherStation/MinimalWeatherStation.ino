/*
 * Minimal Weather Station Example for WeatherAnimations Library
 * 
 * This example demonstrates how to use the WeatherAnimations library
 * with Serial debugging disabled from the library itself.
 */

// Define this before including the library to disable Serial debug messages
#define WA_DISABLE_SERIAL

#include <WeatherAnimations.h>
#include <Arduino.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// Home Assistant settings
const char* haIP = "YourHomeAssistantIP";
const char* haToken = "YourHomeAssistantToken";
const char* weatherEntity = "weather.forecast_home";

// OLED display settings
const int OLED_ADDRESS = 0x3C;

// WeatherAnimations instance
WeatherAnimations oledWeather(ssid, password, haIP, haToken);

void setup() {
	// Initialize serial for our own debug messages
	Serial.begin(115200);
	Serial.println("Starting Minimal Weather Station Example");
	
	// Initialize OLED display
	Wire.begin();
	
	// Initialize the library with OLED display
	oledWeather.begin(OLED_DISPLAY, OLED_ADDRESS, true);
	
	// Configure the weather entity
	oledWeather.setWeatherEntity(weatherEntity);
	
	// Set animation mode to embedded animations
	oledWeather.setAnimationMode(ANIMATION_EMBEDDED);
	
	// Set continuous weather display mode
	oledWeather.setMode(CONTINUOUS_WEATHER);
	
	Serial.println("Setup complete");
}

void loop() {
	// Update display and check for new weather data
	oledWeather.update();
	
	// Small delay to prevent too frequent updates
	delay(50);
} 