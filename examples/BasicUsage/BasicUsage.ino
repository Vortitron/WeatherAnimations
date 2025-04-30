/*
 * Basic OLED Usage Example for WeatherAnimations Library
 * 
 * This example demonstrates how to initialize the WeatherAnimations library,
 * connect to Home Assistant, and display weather animations on an OLED display.
 * 
 * Setup:
 * 1. Copy examples/config_example.h to examples/BasicUsage/config.h
 * 2. Edit config.h with your WiFi and Home Assistant credentials
 */

#include <WeatherAnimations.h>
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
	const int oledAddress = 0x3C;
#else
	// Use the configuration from config.h
	const char* ssid = WIFI_SSID;
	const char* password = WIFI_PASSWORD;
	const char* haIP = HA_IP;
	const char* haToken = HA_TOKEN;
	const char* weatherEntity = HA_WEATHER_ENTITY;
	const int oledAddress = OLED_ADDRESS;
#endif

// Create WeatherAnimations instance
WeatherAnimations weatherAnim(ssid, password, haIP, haToken);

void setup() {
	// Initialize serial for debugging (if available on your board)
	#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD)
	Serial.begin(115200);
	Serial.println("Starting WeatherAnimations OLED example");
	
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
	
	// Initialize the library with OLED display
	// Parameters: display type, I2C address (typically 0x3C or 0x3D), manage WiFi connection
	weatherAnim.begin(OLED_DISPLAY, oledAddress, true);
	
	// Set the custom weather entity ID
	weatherAnim.setWeatherEntity(weatherEntity);
	
	// Set mode to continuous weather display
	// Options: CONTINUOUS_WEATHER or SIMPLE_TRANSITION
	weatherAnim.setMode(CONTINUOUS_WEATHER);
	
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