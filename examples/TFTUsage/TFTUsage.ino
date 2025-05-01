/*
 * TFT Display Usage Example for WeatherAnimations Library
 * 
 * This example demonstrates how to initialize the WeatherAnimations library,
 * connect to Home Assistant, and display weather animations on a TFT display.
 * 
 * It utilizes online animation sources (GIFs or JPGs) for better visual quality.
 * 
 * Setup:
 * 1. Copy examples/config_example.h to examples/TFTUsage/config.h
 * 2. Edit config.h with your WiFi and Home Assistant credentials
 */

// Include the local library source directly
#include "../../src/WeatherAnimations.h"
#include <Arduino.h>

// Try to include the configuration file
// If it doesn't exist, we'll use default values
#if __has_include("config.h")
	#include "config.h"
	#define CONFIG_EXISTS
#endif

// Define button pins
const int encoderPUSH = 27; // Button to cycle through animations
const int backButton = 14;  // Button to return to live HA data

// Weather condition constants
const uint8_t WEATHER_TYPES[] = {
	WEATHER_CLEAR,
	WEATHER_CLOUDY,
	WEATHER_RAIN,
	WEATHER_SNOW,
	WEATHER_STORM
};
const int WEATHER_TYPE_COUNT = 5;

// Button state variables
int currentWeatherIndex = 0;
bool manualMode = false;
bool lastEncoderPushState = HIGH;
bool lastBackButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Network and API credentials
// These will be overridden by config.h if it exists
#ifndef CONFIG_EXISTS
	const char* ssid = "YourWiFiSSID";
	const char* password = "YourWiFiPassword";
	const char* haIP = "YourHomeAssistantIP";
	const char* haToken = "YourHomeAssistantToken";
	const char* weatherEntity = "weather.forecast_home";
	
	// Animation URLs (default to GitHub links)
	const char* clearSkyDayURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/sunny-day.gif";
	const char* clearSkyNightURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/clear-night.gif";
	const char* cloudyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/cloudy.gif";
	const char* rainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/rainy.gif";
	const char* heavyRainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/pouring.gif";
	const char* snowURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/snowy.gif";
	const char* stormURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/lightning.gif";
	const char* stormRainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/lightning-rainy.gif";
#else
	// Use the configuration from config.h
	const char* ssid = WIFI_SSID;
	const char* password = WIFI_PASSWORD;
	const char* haIP = HA_IP;
	const char* haToken = HA_TOKEN;
	const char* weatherEntity = HA_WEATHER_ENTITY;
	
	// Animation URLs from config
	const char* clearSkyDayURL = ANIM_CLEAR_DAY;
	const char* clearSkyNightURL = ANIM_CLEAR_NIGHT;
	const char* cloudyURL = ANIM_CLOUDY;
	const char* rainURL = ANIM_RAIN;
	const char* heavyRainURL = ANIM_HEAVY_RAIN;
	const char* snowURL = ANIM_SNOW;
	const char* stormURL = ANIM_STORM;
	const char* stormRainURL = ANIM_STORM_RAIN;
#endif

// Create WeatherAnimations instance
WeatherAnimations weatherAnim(ssid, password, haIP, haToken);

void setup() {
	// Initialize serial for debugging (if available on your board)
	#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD)
	Serial.begin(115200);
	Serial.println("Starting WeatherAnimations TFT example");
	
	#ifdef CONFIG_EXISTS
	Serial.println("Using configuration from config.h");
	#else
	Serial.println("WARNING: No config.h found. Using default values.");
	Serial.println("Copy config_example.h to config.h and customize it.");
	#endif
	#endif
	
	// Initialize button pins
	pinMode(encoderPUSH, INPUT_PULLUP);
	pinMode(backButton, INPUT_PULLUP);
	
	// Initialize the library with TFT display
	// Parameters: display type, CS pin (or 0 for default), manage WiFi connection
	weatherAnim.begin(TFT_DISPLAY, 0, true);
	
	// Set online animation sources for TFT display
	// You can set different animations for different times of day
	weatherAnim.setOnlineAnimationSource(WEATHER_CLEAR, true, clearSkyDayURL);   // Day version
	weatherAnim.setOnlineAnimationSource(WEATHER_CLEAR, false, clearSkyNightURL); // Night version
	weatherAnim.setOnlineAnimationSource(WEATHER_CLOUDY, cloudyURL);
	weatherAnim.setOnlineAnimationSource(WEATHER_RAIN, rainURL);
	weatherAnim.setOnlineAnimationSource(WEATHER_POURING, heavyRainURL);
	weatherAnim.setOnlineAnimationSource(WEATHER_SNOW, snowURL);
	weatherAnim.setOnlineAnimationSource(WEATHER_LIGHTNING, stormURL);
	weatherAnim.setOnlineAnimationSource(WEATHER_LIGHTNING_RAINY, stormRainURL);
	
	// Set the custom weather entity ID
	weatherAnim.setWeatherEntity(weatherEntity);
	
	// Set mode to continuous weather display
	// Options: CONTINUOUS_WEATHER or SIMPLE_TRANSITION
	weatherAnim.setMode(CONTINUOUS_WEATHER);
	
	// Set refresh interval for animations (in milliseconds)
	weatherAnim.setRefreshInterval(30000); // 30 seconds
	
	#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD)
	Serial.println("Setup complete, starting weather updates...");
	#endif
}

void handleButtons() {
	// Read current button states
	bool encoderPushState = digitalRead(encoderPUSH);
	bool backButtonState = digitalRead(backButton);
	
	// Handle encoder push button (with debounce)
	if (encoderPushState != lastEncoderPushState) {
		lastDebounceTime = millis();
	}
	
	if ((millis() - lastDebounceTime) > debounceDelay) {
		// If the push button state has changed and is now LOW (pressed)
		if (encoderPushState == LOW && lastEncoderPushState == HIGH) {
			// Enter manual mode if not already
			manualMode = true;
			
			// Cycle to next weather type
			currentWeatherIndex = (currentWeatherIndex + 1) % WEATHER_TYPE_COUNT;
			
			// Display the selected animation
			weatherAnim.runTransition(WEATHER_TYPES[currentWeatherIndex], TRANSITION_RIGHT_TO_LEFT, 500);
			
			#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD)
			Serial.print("Manual mode: Showing animation ");
			Serial.println(currentWeatherIndex);
			#endif
		}
	}
	
	// Handle back button (with debounce)
	if (backButtonState != lastBackButtonState) {
		lastDebounceTime = millis();
	}
	
	if ((millis() - lastDebounceTime) > debounceDelay) {
		// If the back button state has changed and is now LOW (pressed)
		if (backButtonState == LOW && lastBackButtonState == HIGH) {
			// Exit manual mode
			if (manualMode) {
				manualMode = false;
				#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD)
				Serial.println("Returning to live weather data");
				#endif
			}
		}
	}
	
	// Update button states
	lastEncoderPushState = encoderPushState;
	lastBackButtonState = backButtonState;
}

void loop() {
	// Check button states
	handleButtons();
	
	// Update weather data and display animations only when not in manual mode
	if (!manualMode) {
		weatherAnim.update();
	}
	
	// Add a small delay to prevent excessive updates
	delay(100);
} 