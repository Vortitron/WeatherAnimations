/*
 * Basic OLED Usage Example for WeatherAnimations Library with SH1106 Display
 * 
 * This example demonstrates how to initialize the WeatherAnimations library,
 * connect to Home Assistant, and display weather animations on an SH1106 OLED display.
 * 
 * Hardware requirements:
 * - ESP32 board
 * - SH1106 OLED display (I2C, typically at address 0x3C)
 * - 3 push buttons (connected to the pins defined below)
 * 
 * Required Libraries:
 * - Adafruit GFX Library
 * - Adafruit SH110X
 * - WeatherAnimations
 * 
 * Setup:
 * 1. Copy examples/config_example.h to examples/BasicUsage/config.h
 * 2. Edit config.h with your WiFi and Home Assistant credentials
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Include the library source files directly for development
#include "../../src/WeatherAnimations.h"

// Try to include the configuration file
// If it doesn't exist, we'll use default values
#if __has_include("config.h")
	#include "config.h"
	#define CONFIG_EXISTS
#endif

// Define button pins
const int encoderPUSH = 27; // Button to cycle through animations
const int backButton = 14;  // Button to return to live HA data
const int leftButton = 12;  // Button to switch from fixed to animated

// Animation mode flag
bool animatedMode = false; // Start with static mode

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
bool manualMode = true; // Start in manual mode by default
bool lastEncoderPushState = HIGH;
bool lastBackButtonState = HIGH;
bool lastLeftButtonState = HIGH;
unsigned long lastEncoderDebounceTime = 0;
unsigned long lastBackDebounceTime = 0;
unsigned long lastLeftDebounceTime = 0;
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

// Create SH1106 display instance - width, height, address, reset pin
Adafruit_SH1106G display(128, 64, &Wire, -1);

// Create WeatherAnimations instance
WeatherAnimations weatherAnim(ssid, password, haIP, haToken);

// Function to get weather type name
const char* getWeatherTypeName(uint8_t weatherType) {
	switch(weatherType) {
		case WEATHER_CLEAR: return "CLEAR";
		case WEATHER_CLOUDY: return "CLOUDY";
		case WEATHER_RAIN: return "RAIN";
		case WEATHER_SNOW: return "SNOW";
		case WEATHER_STORM: return "STORM";
		default: return "UNKNOWN";
	}
}

// Function to display current state on serial
void printCurrentState() {
	Serial.println("\n----- Current State -----");
	Serial.print("Mode: ");
	Serial.println(manualMode ? "MANUAL" : "LIVE DATA");
	Serial.print("Animation Type: ");
	Serial.println(animatedMode ? "ANIMATED" : "STATIC");
	
	if (manualMode) {
		Serial.print("Current Weather: ");
		Serial.println(getWeatherTypeName(WEATHER_TYPES[currentWeatherIndex]));
	}
	Serial.println("------------------------");
}

void setup() {
	// Initialize serial for debugging
	Serial.begin(115200);
	Serial.println("\n\n===================================");
	Serial.println("WeatherAnimations SH1106 ESP32 Example");
	Serial.println("===================================");
	
	#ifdef CONFIG_EXISTS
	Serial.println("Using configuration from config.h");
	#else
	Serial.println("WARNING: No config.h found. Using default values.");
	Serial.println("Copy config_example.h to config.h and customize it.");
	#endif
	
	// Initialize button pins
	pinMode(encoderPUSH, INPUT_PULLUP);
	pinMode(backButton, INPUT_PULLUP);
	pinMode(leftButton, INPUT_PULLUP);
	
	// Initialize the SH1106 OLED display
	delay(250); // Wait for the display to power up
	Serial.println("Initializing SH1106 OLED display...");
	display.begin(oledAddress, true); // Address, reset
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(SH110X_WHITE);
	display.setCursor(0, 0);
	display.println("Starting...");
	display.display();
	Serial.println("SH1106 OLED display initialized successfully");
	
	// Initialize the WeatherAnimations library
	Serial.println("Initializing WeatherAnimations library...");
	weatherAnim.begin(OLED_SH1106, oledAddress, true);
	
	// Set the custom weather entity ID
	weatherAnim.setWeatherEntity(weatherEntity);
	
	// Set mode to continuous weather display
	weatherAnim.setMode(CONTINUOUS_WEATHER);
	
	// Start with embedded animations for testing
	weatherAnim.setAnimationMode(ANIMATION_EMBEDDED);
	
	// Set base URLs for online animations
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
	
	Serial.println("Setup complete, starting weather updates...");
	
	// Show initial state
	printCurrentState();
	
	// Start with the first weather animation
	weatherAnim.runTransition(WEATHER_TYPES[currentWeatherIndex], TRANSITION_FADE, 500);
}

void handleButtons() {
	// Read current button states
	bool encoderPushState = digitalRead(encoderPUSH);
	bool backButtonState = digitalRead(backButton);
	bool leftButtonState = digitalRead(leftButton);
	
	// Handle encoder push button (with debounce)
	if (encoderPushState != lastEncoderPushState) {
		lastEncoderDebounceTime = millis();
	}
	
	if ((millis() - lastEncoderDebounceTime) > debounceDelay) {
		// If the push button state has changed and is now LOW (pressed)
		if (encoderPushState == LOW && lastEncoderPushState == HIGH) {
			Serial.println("\n>>> ENCODER BUTTON PRESSED");
			// Enter manual mode if not already
			manualMode = true;
			
			// Cycle to next weather type
			currentWeatherIndex = (currentWeatherIndex + 1) % WEATHER_TYPE_COUNT;
			
			// Display the selected animation with right to left transition
			weatherAnim.runTransition(WEATHER_TYPES[currentWeatherIndex], TRANSITION_RIGHT_TO_LEFT, 500);
			
			Serial.print("Changed to weather type: ");
			Serial.println(getWeatherTypeName(WEATHER_TYPES[currentWeatherIndex]));
			
			// Show current state
			printCurrentState();
		}
	}
	
	// Handle back button (with debounce)
	if (backButtonState != lastBackButtonState) {
		lastBackDebounceTime = millis();
	}
	
	if ((millis() - lastBackDebounceTime) > debounceDelay) {
		// If the back button state has changed and is now LOW (pressed)
		if (backButtonState == LOW && lastBackButtonState == HIGH) {
			Serial.println("\n>>> BACK BUTTON PRESSED");
			
			// Exit manual mode if currently in it
			if (manualMode) {
				manualMode = false;
				Serial.println("Switching to LIVE weather data mode");
				printCurrentState();
			}
		}
	}
	
	// Handle left button (with debounce)
	if (leftButtonState != lastLeftButtonState) {
		lastLeftDebounceTime = millis();
	}
	
	if ((millis() - lastLeftDebounceTime) > debounceDelay) {
		// If the left button state has changed and is now LOW (pressed)
		if (leftButtonState == LOW && lastLeftButtonState == HIGH) {
			Serial.println("\n>>> LEFT BUTTON PRESSED");
			
			// Toggle animation mode
			animatedMode = !animatedMode;
			
			// Set the animation mode in the library
			weatherAnim.setAnimationMode(animatedMode ? ANIMATION_ONLINE : ANIMATION_STATIC);
			
			Serial.print("Animation mode changed to: ");
			Serial.println(animatedMode ? "ANIMATED" : "STATIC");
			
			// Force a refresh of the current display with fade transition
			if (manualMode) {
				weatherAnim.runTransition(WEATHER_TYPES[currentWeatherIndex], TRANSITION_FADE, 500);
			}
			
			// Show current state
			printCurrentState();
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