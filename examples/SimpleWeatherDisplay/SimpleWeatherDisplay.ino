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

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

// For regular use, uncomment this instead:
// #include <WeatherAnimations.h>

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

// Create display instance
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Button pin for waking from weather display
#define WAKE_BUTTON 27

// WiFi and Home Assistant settings - replace with your own
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* haIP = "YourHomeAssistantIP";
const char* haToken = "YourHomeAssistantToken";
const char* weatherEntity = "weather.forecast_home";

// Create WeatherAnimations instance
WeatherAnimations weatherAnim(ssid, password, haIP, haToken);

// Idle timeout variables
unsigned long lastUserActivityTime = 0;
const unsigned long idleTimeoutDuration = 60000; // 60 seconds idle before showing weather
bool isDisplayingIdle = false;

// Function to show idle weather animation
void showWeatherDisplay() {
	if (!isDisplayingIdle) {
		Serial.println("Starting idle weather animation");
		isDisplayingIdle = true;
		
		// Use animated continuous weather mode
		weatherAnim.setMode(CONTINUOUS_WEATHER);
		
		// Update current weather from Home Assistant
		weatherAnim.updateWeatherData();
		
		// Display the animation
		weatherAnim.update();
	}
}

// Function to hide weather display
void hideWeatherDisplay() {
	if (isDisplayingIdle) {
		Serial.println("Exiting weather animation");
		isDisplayingIdle = false;
		
		// Clear the display
		display.clearDisplay();
		display.setTextSize(1);
		display.setTextColor(SSD1306_WHITE);
		display.setCursor(0, 0);
		display.println("Weather Display");
		display.println("Idle timeout: 60 sec");
		display.println("Press button to wake");
		display.display();
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
	
	// Initialize display
	if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
		Serial.println("SSD1306 allocation failed");
		while (1); // Don't proceed if display init failed
	}
	
	// Show initial message
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(SSD1306_WHITE);
	display.setCursor(0, 0);
	display.println("Weather Display");
	display.println("---------------");
	display.println("Will show weather");
	display.println("animation after");
	display.println("60 seconds of");
	display.println("inactivity.");
	display.println("Press button to wake");
	display.display();
	
	// Initialize the WeatherAnimations library
	// Parameters: display type, I2C address, manage WiFi connection
	weatherAnim.begin(OLED_SH1106, OLED_ADDR, true);
	
	// Set animation mode to embedded (not online)
	weatherAnim.setAnimationMode(ANIMATION_EMBEDDED);
	
	// Set the custom weather entity ID
	weatherAnim.setWeatherEntity(weatherEntity);
	
	// Initialize WiFi connection
	weatherAnim.connectToWiFi(ssid, password);
	
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