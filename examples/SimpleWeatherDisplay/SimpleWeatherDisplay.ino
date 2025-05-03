/*
 * Simple Weather Display Example
 * 
 * This example shows how to use the WeatherAnimations library to display
 * weather animations after an idle timeout and dismiss with any button press.
 * 
 * Hardware:
 * - ESP32 board
 * - OLED display (I2C, SSD1306 or SH1106)
 * - Optional: Any button connected to a digital pin (default: GPIO 27)
 */

// Define display type - for this example we're only using OLED, not TFT
#define USE_OLED_ONLY
// If you need TFT support, comment out USE_OLED_ONLY and uncomment the following line:
// #define USE_TFT_DISPLAY

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Include the WeatherAnimations library
// For development, include the source files directly
#include "../../src/WeatherAnimations.h"
#include "../../src/WeatherAnimations.cpp"
#include "../../src/WeatherAnimationsAnimations.h"
#include "../../src/WeatherAnimationsAnimations.cpp"
#include "../../src/WeatherAnimationsIcons.h"
#include "../../src/WeatherAnimationsIcons.cpp"

// Include TFT implementation only if needed
#if !defined(USE_OLED_ONLY) && defined(USE_TFT_DISPLAY)
#include "../../src/WeatherAnimationsTFT.cpp"
#endif

// For regular use, uncomment this instead:
// #include <WeatherAnimations.h>

// Display settings
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Button pin for waking from weather display
#define WAKE_BUTTON 27

// WiFi and Home Assistant settings - replace with your own
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* haIP = "YourHomeAssistantIP";
const char* haToken = "YourHomeAssistantToken";
const char* weatherEntity = "weather.forecast_home";

// Create WeatherAnimations instance
WeatherAnimationsLib::WeatherAnimations weatherAnim(ssid, password, haIP, haToken);

// Idle timeout variables
unsigned long lastUserActivityTime = 0;
const unsigned long idleTimeoutDuration = 60000; // 60 seconds idle before showing weather
bool isDisplayingIdle = false;

// Function forward declarations
void showWeatherDisplay();
void hideWeatherDisplay();

// Function to show idle weather animation
void showWeatherDisplay() {
	if (!isDisplayingIdle) {
		Serial.println("Starting idle weather animation");
		isDisplayingIdle = true;
		
		// Use animated continuous weather mode
		weatherAnim.setMode(CONTINUOUS_WEATHER);
		
		// Display the animation - update() will fetch weather data automatically
		weatherAnim.update();
	}
}

// Function to hide weather display
void hideWeatherDisplay() {
	if (isDisplayingIdle) {
		Serial.println("Exiting weather animation");
		isDisplayingIdle = false;
		
		// Use the library to display a welcome message using a quick fade transition
		weatherAnim.setAnimationMode(ANIMATION_STATIC);
		weatherAnim.runTransition(WEATHER_CLEAR, TRANSITION_FADE, 500);
	}
}

void setup() {
	// Initialize serial for debugging
	Serial.begin(115200);
	delay(500);
	
	Serial.println("\n\n===================================");
	Serial.println("Simple Weather Display Example");
	Serial.println("===================================");
	
	// Initialize button pin
	pinMode(WAKE_BUTTON, INPUT_PULLUP);
	
	// Initialize the WeatherAnimations library with SSD1306 display
	// Explicitly use OLED_SSD1306 and not TFT_DISPLAY
	#ifdef USE_OLED_ONLY
	Serial.println("Using OLED display (SSD1306) for this example");
	weatherAnim.begin(OLED_SSD1306, OLED_ADDR, true);
	#else
	weatherAnim.begin(OLED_SSD1306, OLED_ADDR, true);
	#endif
	
	// Set animation mode to embedded (not online)
	weatherAnim.setAnimationMode(ANIMATION_EMBEDDED);
	
	// Set the custom weather entity ID
	weatherAnim.setWeatherEntity(weatherEntity);
	
	// Show welcome message
	weatherAnim.setAnimationMode(ANIMATION_STATIC);
	weatherAnim.runTransition(WEATHER_CLEAR, TRANSITION_FADE, 500);
	
	// Set back to embedded animations for weather display
	weatherAnim.setAnimationMode(ANIMATION_EMBEDDED);
	
	// Initialize user activity tracking
	lastUserActivityTime = millis();
	isDisplayingIdle = false;
	
	Serial.println("Setup complete - waiting for idle timeout");
}

void loop() {
	// Read button state
	bool buttonPressed = (digitalRead(WAKE_BUTTON) == LOW);
	
	// Check if the button was pressed to update activity time
	if (buttonPressed) {
		lastUserActivityTime = millis();
		// If we were in idle display mode, exit it
		if (isDisplayingIdle) {
			hideWeatherDisplay();
		}
	}
	
	// Check for idle timeout
	if (!isDisplayingIdle && (millis() - lastUserActivityTime >= idleTimeoutDuration)) {
		showWeatherDisplay();
	}
	
	// If in idle mode, update the animation
	if (isDisplayingIdle) {
		weatherAnim.update();
	}
	
	// Add a small delay to prevent excessive updates
	delay(10);
} 