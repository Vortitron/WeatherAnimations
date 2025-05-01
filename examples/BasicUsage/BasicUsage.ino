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
const int leftButton = 12; //switch from fixed to animated

// Animation mode flag
bool animatedMode = false; // Start with static mode
bool lastLeftButtonState = HIGH;

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
	// WIFI_SSID, WIFI_PASSWORD, etc. should be defined in config.h
	#ifndef WIFI_SSID
	#define WIFI_SSID "YourWiFiSSID"
	#endif
	#ifndef WIFI_PASSWORD
	#define WIFI_PASSWORD "YourWiFiPassword"
	#endif
	#ifndef HA_IP
	#define HA_IP "YourHomeAssistantIP" 
	#endif
	#ifndef HA_TOKEN
	#define HA_TOKEN "YourHomeAssistantToken"
	#endif
	#ifndef HA_WEATHER_ENTITY
	#define HA_WEATHER_ENTITY "weather.forecast_home"
	#endif
	#ifndef OLED_ADDRESS
	#define OLED_ADDRESS 0x3C
	#endif
	
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
	pinMode(leftButton, INPUT_PULLUP);
	
	// Initialize the library with OLED display
	// Parameters: display type, I2C address (typically 0x3C or 0x3D), manage WiFi connection
	weatherAnim.begin(OLED_DISPLAY, oledAddress, true);
	
	// Set the custom weather entity ID
	weatherAnim.setWeatherEntity(weatherEntity);
	
	// Set mode to continuous weather display
	// Options: CONTINUOUS_WEATHER or SIMPLE_TRANSITION
	weatherAnim.setMode(CONTINUOUS_WEATHER);
	
	// Set animation mode to static initially
	// Can be toggled with left button
	weatherAnim.setAnimationMode(ANIMATION_STATIC);
	
	// Set base URLs for online animations (used when in ANIMATION_ONLINE mode)
	// These URLs point to the base paths of our animation frames
	const char* clearSkyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/sunny-day_frame_";
	const char* cloudyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/cloudy_frame_";
	const char* rainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/rainy_frame_";
	const char* snowURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/snowy_frame_";
	const char* stormURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/lightning_frame_";
	
	// Set online animation sources
	weatherAnim.setOnlineAnimationSource(WEATHER_CLEAR, clearSkyURL);
	weatherAnim.setOnlineAnimationSource(WEATHER_CLOUDY, cloudyURL);
	weatherAnim.setOnlineAnimationSource(WEATHER_RAIN, rainURL);
	weatherAnim.setOnlineAnimationSource(WEATHER_SNOW, snowURL);
	weatherAnim.setOnlineAnimationSource(WEATHER_STORM, stormURL);
	
	#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD)
	Serial.println("Setup complete, starting weather updates...");
	#endif
}

void handleButtons() {
	// Read current button states
	bool encoderPushState = digitalRead(encoderPUSH);
	bool backButtonState = digitalRead(backButton);
	bool leftButtonState = digitalRead(leftButton);
	
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
	
	// Handle left button (with debounce)
	if (leftButtonState != lastLeftButtonState) {
		lastDebounceTime = millis();
	}
	
	if ((millis() - lastDebounceTime) > debounceDelay) {
		// If the left button state has changed and is now LOW (pressed)
		if (leftButtonState == LOW && lastLeftButtonState == HIGH) {
			// Toggle animation mode
			animatedMode = !animatedMode;
			
			// Set the animation mode
			weatherAnim.setAnimationMode(animatedMode ? ANIMATION_ONLINE : ANIMATION_STATIC);
			
			#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD)
			Serial.print("Animation mode changed to: ");
			Serial.println(animatedMode ? "Online/Animated" : "Static");
			#endif
			
			// Force a refresh of the current display
			if (manualMode) {
				weatherAnim.runTransition(WEATHER_TYPES[currentWeatherIndex], TRANSITION_FADE, 500);
			}
		}
	}
	
	// Update button states
	lastEncoderPushState = encoderPushState;
	lastBackButtonState = backButtonState;
	lastLeftButtonState = leftButtonState;
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