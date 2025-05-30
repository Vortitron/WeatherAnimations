/*
 * ESP32 Weather Display Example with SSD1306 Library
 * 
 * This example uses the Adafruit_SSD1306 library to work with SH1106 displays
 * on ESP32, as this combination often works better than the SH110X library.
 * It provides a simpler implementation focused on reliability.
 * 
 * Features:
 * - Weather condition display with static or animated icons
 * - Indoor and outdoor temperature readings from Home Assistant
 * - Min/max forecast temperature display
 * - Manual or automatic mode via button controls
 * 
 * Hardware:
 * - ESP32 board
 * - SH1106 OLED display (I2C, address 0x3C)
 * - Push buttons on pins defined below
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Using simpler directly drawn animations instead of the animated icons header
// This avoids the naming conflicts and provides better reliability
#include "../../src/WeatherAnimations.h"
#include "../../src/WeatherAnimations.cpp"
#include "../../src/WeatherAnimationsAnimations.h"
#include "../../src/WeatherAnimationsAnimations.cpp"
#include "../../src/WeatherAnimationsIcons.h"
#include "../../src/WeatherAnimationsIcons.cpp"
// We're not using the animated icons header for now
// #include "../../src/WeatherAnimationsAnimatedIcons.h"

// Include the library source files as a zip file - uncomment if using installed library
// #include <WeatherAnimations.h>

// Try to include the configuration file
#if __has_include("config.h")
	#include "config.h"
	#define CONFIG_EXISTS
#endif

// Define button pins
const int encoderPUSH = 27; // Button to cycle through animations
const int backButton = 14;  // Button to return to live HA data
const int leftButton = 12;  // Button to switch from fixed to animated

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

// Create display instance using SSD1306 library
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Weather constants
#define WEATHER_CLEAR 0
#define WEATHER_CLOUDY 1
#define WEATHER_RAIN 2
#define WEATHER_SNOW 3
#define WEATHER_STORM 4

// Weather types array
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
bool manualMode = true; // Ensure we start in manual mode by default
bool animatedMode = false; // Start with static mode (not animated)
bool lastEncoderPushState = HIGH;
bool lastBackButtonState = HIGH;
bool lastLeftButtonState = HIGH;
unsigned long lastEncoderDebounceTime = 0;
unsigned long lastBackDebounceTime = 0;
unsigned long lastLeftDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Animation variables
unsigned long lastFrameTime = 0;
uint8_t currentFrame = 0;
const int frameDelay = 200; // ms between frames
const int MAX_FRAMES = 15;  // Most animations have up to 15 frames

// Animation state variables
bool isTransitioning = false;
unsigned long transitionStartTime = 0;
const unsigned long transitionDuration = 1000; // ms
uint8_t lastWeatherType = WEATHER_CLEAR;

// Idle timeout variables
unsigned long lastUserActivityTime = 0;
const unsigned long idleTimeoutDuration = 60000; // 60 seconds idle before showing weather
bool isDisplayingIdle = false;
bool isDisplayOn = true;

// Network and API credentials
// These will be overridden by config.h if it exists
#ifndef CONFIG_EXISTS
	const char* ssid = "YourWiFiSSID";
	const char* password = "YourWiFiPassword";
	const char* haIP = "YourHomeAssistantIP";
	const char* haToken = "YourHomeAssistantToken";
	const char* weatherEntity = "weather.forecast_home";
	const char* indoorTempEntity = "sensor.indoor_temperature";
	const char* outdoorTempEntity = "sensor.outdoor_temperature";
#else
	// Use the configuration from config.h
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
	#ifndef HA_INDOOR_TEMP_ENTITY
	#define HA_INDOOR_TEMP_ENTITY "sensor.indoor_temperature"
	#endif
	#ifndef HA_OUTDOOR_TEMP_ENTITY
	#define HA_OUTDOOR_TEMP_ENTITY "sensor.outdoor_temperature"
	#endif
	
	const char* ssid = WIFI_SSID;
	const char* password = WIFI_PASSWORD;
	const char* haIP = HA_IP;
	const char* haToken = HA_TOKEN;
	const char* weatherEntity = HA_WEATHER_ENTITY;
	const char* indoorTempEntity = HA_INDOOR_TEMP_ENTITY;
	const char* outdoorTempEntity = HA_OUTDOOR_TEMP_ENTITY;
#endif

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

// Connect to WiFi
bool connectToWiFi() {
	WiFi.mode(WIFI_STA);
  WiFi.disconnect();
	WiFi.begin(ssid, password);
	Serial.print("Connecting to WiFi: ");
	Serial.print(ssid);

	int attempts = 0;
	while (WiFi.status() != WL_CONNECTED && attempts < 20) {
		delay(500);
		Serial.print(".");
		attempts++;
	}
	Serial.println();
	
	if (WiFi.status() == WL_CONNECTED) {
		Serial.print("Connected, IP address: ");
		Serial.println(WiFi.localIP());
		return true;
	} else {
		Serial.println("Failed to connect to WiFi");
		return false;
	}
}

// Fetch weather data from Home Assistant
uint8_t fetchWeatherData() {
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("WiFi not connected");
		return WEATHER_CLOUDY; // Default
	}
	
	HTTPClient http;
	String url = String("http://") + haIP + ":8123/api/states/" + weatherEntity;
	http.begin(url);
	http.addHeader("Authorization", String("Bearer ") + haToken);
	http.addHeader("Content-Type", "application/json");
	
	int httpCode = http.GET();
	Serial.print("HTTP GET result: ");
	Serial.println(httpCode);
	
	if (httpCode == 200) {
		String payload = http.getString();
		Serial.println("Received data from Home Assistant");
		
		// Parse JSON
		DynamicJsonDocument doc(1024);
		DeserializationError error = deserializeJson(doc, payload);
		
		if (error) {
			Serial.print("JSON parsing failed: ");
			Serial.println(error.c_str());
			http.end();
			return WEATHER_CLOUDY; // Default
		}
		
		// Extract weather state
		const char* state = doc["state"];
		Serial.print("Weather state: ");
		Serial.println(state);
		
		// Note: In the actual library, it will also fetch temperature data from
		// the configured temperature entities and store indoor, outdoor, min, and max temps
		
		http.end();
		
		// Map to our weather types
		if (strcmp(state, "sunny") == 0 || strcmp(state, "clear-night") == 0) {
			return WEATHER_CLEAR;
		} else if (strcmp(state, "cloudy") == 0 || strcmp(state, "partlycloudy") == 0) {
			return WEATHER_CLOUDY;
		} else if (strcmp(state, "rainy") == 0 || strcmp(state, "pouring") == 0) {
			return WEATHER_RAIN;
		} else if (strcmp(state, "snowy") == 0 || strcmp(state, "snowy-rainy") == 0) {
			return WEATHER_SNOW;
		} else if (strcmp(state, "lightning") == 0 || strcmp(state, "lightning-rainy") == 0) {
			return WEATHER_STORM;
		} else {
			return WEATHER_CLOUDY; // Default
		}
	} else {
		Serial.print("HTTP GET failed: ");
		Serial.println(httpCode);
		http.end();
		return WEATHER_CLOUDY; // Default
	}
}

// Draw static weather icon
void drawStaticWeather(uint8_t weatherType) {
	display.clearDisplay();
	
	// Draw header
	display.setTextSize(1);
	display.setTextColor(SSD1306_WHITE);
	display.setCursor(0, 0);
	display.println("Weather:");
	display.setTextSize(2);
	display.setCursor(0, 12);
	display.println(getWeatherTypeName(weatherType));
	
	// Draw weather icon - using basic drawing instead of library
	switch (weatherType) {
		case WEATHER_CLEAR:
			// Draw sun
			display.fillCircle(96, 32, 16, SSD1306_WHITE);
			break;
		case WEATHER_CLOUDY:
			// Draw cloud
			display.fillRoundRect(86, 34, 36, 18, 8, SSD1306_WHITE);
			display.fillRoundRect(78, 24, 28, 20, 8, SSD1306_WHITE);
			break;
		case WEATHER_RAIN:
			// Draw cloud with rain
			display.fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
			for (int i = 0; i < 6; i++) {
				display.drawLine(86 + i*7, 42, 89 + i*7, 52, SSD1306_WHITE);
			}
			break;
		case WEATHER_SNOW:
			// Draw cloud with snow
			display.fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
			for (int i = 0; i < 6; i++) {
				display.drawCircle(89 + i*7, 48, 2, SSD1306_WHITE);
			}
			break;
		case WEATHER_STORM:
			// Draw cloud with lightning
			display.fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
			display.fillTriangle(100, 42, 90, 52, 95, 52, SSD1306_WHITE);
			display.fillTriangle(95, 52, 105, 52, 98, 62, SSD1306_WHITE);
			break;
		default:
			// Unknown weather, draw question mark
			display.setTextSize(3);
			display.setCursor(90, 30);
			display.print("?");
			break;
	}
	
	// Draw mode indicators at bottom
	display.setTextSize(1);
	display.setCursor(0, 56);
	display.print(manualMode ? "MANUAL" : "LIVE");
	display.setCursor(70, 56);
	display.print(animatedMode ? "ANIM" : "STATIC");
	
	// If we have temperature data from the weather animations library,
	// we can display it (this will be empty in this example but shows how
	// you could add it manually if desired)
	if (!manualMode) {
		// Assume when in LIVE mode, the temperature data might be available
		// This would normally come from the weatherAnim object but we're drawing manually here
		display.setCursor(0, 45);
		display.print("Temp data in LIVE mode");
	}
	
	display.display();
}

// Draw animated weather (simple animation frame)
void drawAnimatedWeather(uint8_t weatherType, uint8_t frame) {
	display.clearDisplay();
	
	// Draw header
	display.setTextSize(1);
	display.setTextColor(SSD1306_WHITE);
	display.setCursor(0, 0);
	display.println("Weather:");
	display.setTextSize(2);
	display.setCursor(0, 12);
	display.println(getWeatherTypeName(weatherType));
	
	// Variables needed for animations - declare before switch
	int offset = 0;
	
	// Draw animated weather icon based on frame
	switch (weatherType) {
		case WEATHER_CLEAR:
			// Animated sun (rays expand/contract)
			display.fillCircle(96, 32, 12, SSD1306_WHITE);
			if (frame % 2 == 0) {
				// Draw longer rays
				for (int i = 0; i < 8; i++) {
					float angle = i * PI / 4.0;
					int x1 = 96 + cos(angle) * 14;
					int y1 = 32 + sin(angle) * 14;
					int x2 = 96 + cos(angle) * 22;
					int y2 = 32 + sin(angle) * 22;
					display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
				}
			} else {
				// Draw shorter rays
				for (int i = 0; i < 8; i++) {
					float angle = i * PI / 4.0;
					int x1 = 96 + cos(angle) * 14;
					int y1 = 32 + sin(angle) * 14;
					int x2 = 96 + cos(angle) * 18;
					int y2 = 32 + sin(angle) * 18;
					display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
				}
			}
			break;
		case WEATHER_CLOUDY:
			// Animated cloud (moves slightly)
			offset = (frame % 2 == 0) ? 0 : 2;
			display.fillRoundRect(86 + offset, 34, 36, 18, 8, SSD1306_WHITE);
			display.fillRoundRect(78 + offset, 24, 28, 20, 8, SSD1306_WHITE);
			break;
		case WEATHER_RAIN:
			// Animated rain (drops move)
			display.fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
			for (int i = 0; i < 6; i++) {
				int height = ((i + frame) % 3) * 4; // Vary drop heights
				display.drawLine(86 + i*7, 42 + height, 89 + i*7, 52 + height, SSD1306_WHITE);
			}
			break;
		case WEATHER_SNOW:
			// Animated snow (flakes move)
			display.fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
			for (int i = 0; i < 6; i++) {
				int offset_y = ((i + frame) % 3) * 3;
				int offset_x = ((i + frame) % 2) * 2 - 1;
				display.drawCircle(89 + i*7 + offset_x, 48 + offset_y, 2, SSD1306_WHITE);
			}
			break;
		case WEATHER_STORM:
			// Animated lightning (flash)
			display.fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
			if (frame % 3 != 0) { // Show lightning most frames
				display.fillTriangle(100, 42, 90, 52, 95, 52, SSD1306_WHITE);
				display.fillTriangle(95, 52, 105, 52, 98, 62, SSD1306_WHITE);
			}
			break;
	}
	
	// Draw mode indicators at bottom
	display.setTextSize(1);
	display.setCursor(0, 56);
	display.print(manualMode ? "MANUAL" : "LIVE");
	display.setCursor(70, 56);
	display.print(animatedMode ? "ANIM" : "STATIC");
	display.setCursor(110, 56);
	display.print(frame+1);
	
	// If we have temperature data from the weather animations library,
	// we can display it (this will be empty in this example but shows how
	// you could add it manually if desired)
	if (!manualMode) {
		// Assume when in LIVE mode, the temperature data might be available
		// This would normally come from the weatherAnim object but we're drawing manually here
		display.setCursor(0, 45);
		display.print("Temp data in LIVE mode");
	}
	
	display.display();
}

// Update the weather type - to be called when changing weather
void updateWeatherType(uint8_t newWeatherType, bool animate) {
	Serial.print("Updating to weather type: ");
	Serial.println(getWeatherTypeName(newWeatherType));
	
	// Update lastWeatherType to keep track of current selection
	lastWeatherType = newWeatherType;
	
	// Reset animation frame
	currentFrame = 0;
	
	if (animate) {
		// If in animated mode, start a transition
		isTransitioning = true;
		transitionStartTime = millis();
		
		// Use a random transition type for variety
		uint8_t transitionType = random(5);
		weatherAnim.runTransition(newWeatherType, transitionType, transitionDuration);
		
		Serial.print("Starting transition type: ");
		Serial.println(transitionType);
	} else {
		// For static mode, use our own drawing and a quick transition
		weatherAnim.runTransition(newWeatherType, TRANSITION_FADE, 10);
		
		// For static mode, use our own drawing as well
		drawStaticWeather(newWeatherType);
	}
}

// Turn on weather display when idle
void showIdleWeather() {
	if (!isDisplayingIdle) {
		Serial.println("Switching to idle weather display");
		isDisplayingIdle = true;
		
		// Fetch current weather if in live mode
		uint8_t weatherToShow;
		if (!manualMode) {
			weatherToShow = fetchWeatherData();
		} else {
			weatherToShow = WEATHER_TYPES[currentWeatherIndex];
		}
		
		// Always use animated mode for idle display
		animatedMode = true;
		updateWeatherType(weatherToShow, true);
	}
}

// Turn off weather display on user activity
void hideIdleWeather() {
	if (isDisplayingIdle) {
		Serial.println("Exiting idle weather display");
		isDisplayingIdle = false;
		
		// Clear the display
		display.clearDisplay();
		display.setTextSize(1);
		display.setCursor(0, 0);
		display.println("Weather Display");
		display.println("Press any button");
		display.display();
	}
}

// Check for any button activity
bool checkAnyButtonPressed() {
	bool encoderPushState = digitalRead(encoderPUSH) == LOW;
	bool backButtonState = digitalRead(backButton) == LOW;
	bool leftButtonState = digitalRead(leftButton) == LOW;
	
	return encoderPushState || backButtonState || leftButtonState;
}

// Function to get frame count for a weather type
uint8_t getAnimationFrameCount(uint8_t weatherType) {
	switch (weatherType) {
		case WEATHER_CLEAR:
			return 5; // Sun rays animation has 5 frames
		case WEATHER_CLOUDY:
			return 2; // Cloud movement has 2 frames
		case WEATHER_RAIN:
			return 3; // Rain drops have 3 frames
		case WEATHER_SNOW:
			return 3; // Snowflakes have 3 frames
		case WEATHER_STORM:
			return 3; // Lightning flash has 3 frames
		default:
			return 2; // Default to 2 frames
	}
}

void setup() {
	// Initialize serial for debugging
	Serial.begin(115200);
	delay(500);
	
	Serial.println("\n\n===================================");
	Serial.println("ESP32 Weather Display with SSD1306");
	Serial.println("===================================");
	
	#ifdef CONFIG_EXISTS
	Serial.println("Using configuration from config.h");
	#else
	Serial.println("WARNING: No config.h found. Using default values.");
	#endif
	
	// Initialize button pins
	pinMode(encoderPUSH, INPUT_PULLUP);
	pinMode(backButton, INPUT_PULLUP);
	pinMode(leftButton, INPUT_PULLUP);
	
	// Initialize display
	Serial.println("Initializing OLED display...");
	if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
		Serial.println("SSD1306 allocation failed");
		while (1); // Don't proceed if display init failed
	}
	
	// Initial display
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(SSD1306_WHITE);
	display.setCursor(0, 0);
	display.println("Weather Display");
	display.println("---------------");
	display.println("Manual Mode");
	display.println("Press rotary button");
	display.println("to change weather");
	display.println("Press left button");
	display.println("to toggle animation");
	display.display();
	delay(2000); // Show instructions for 2 seconds
	
	Serial.println("OLED display initialized successfully");
	
	// Initialize the WeatherAnimations library
	weatherAnim.begin(OLED_SH1106, OLED_ADDR, false);

	// Set animation mode to EMBEDDED instead of ONLINE
	weatherAnim.setAnimationMode(ANIMATION_EMBEDDED);
	
	// The library will automatically fetch temperature data from the configured entities
	// Temperature data will be displayed at the bottom of animations if available

	// Set the custom weather entity ID
	weatherAnim.setWeatherEntity(weatherEntity);
	
	// Set temperature sensor entities
	weatherAnim.setTemperatureEntities(indoorTempEntity, outdoorTempEntity);

	// Configure for simple transition mode
	Serial.println("Setting up for embedded animations...");
	weatherAnim.setMode(SIMPLE_TRANSITION);

	// Connect to WiFi for future weather data
	Serial.println("Connecting to WiFi for weather and temperature data...");
	if (connectToWiFi()) {
		Serial.println("WiFi Connected");
	} else {
		Serial.println("WiFi connection failed - will use manual mode only");
	}

	// Display initial weather in static mode
	lastWeatherType = WEATHER_TYPES[currentWeatherIndex];
	drawStaticWeather(lastWeatherType);

	// Initialize user activity tracking
	lastUserActivityTime = millis();
	isDisplayingIdle = false;

	Serial.println("Setup complete!");
	printCurrentState();
}

void handleButtons() {
	// Read current button states
	bool encoderPushState = digitalRead(encoderPUSH);
	bool backButtonState = digitalRead(backButton);
	bool leftButtonState = digitalRead(leftButton);
	
	// Check if any button was pressed to update activity time
	if (encoderPushState == LOW || backButtonState == LOW || leftButtonState == LOW) {
		lastUserActivityTime = millis();
		// If we were in idle display mode, exit it
		if (isDisplayingIdle) {
			hideIdleWeather();
			return; // Skip other button handling when exiting idle mode
		}
	}
	
	// Handle encoder push button (with debounce)
	if (encoderPushState != lastEncoderPushState) {
		lastEncoderDebounceTime = millis();
	}
	
	if ((millis() - lastEncoderDebounceTime) > debounceDelay && lastEncoderDebounceTime != 0) {
		// If button has been stable in LOW state
		if (encoderPushState == LOW && lastEncoderPushState == LOW) {
			Serial.println("\n>>> ENCODER BUTTON PRESSED");
			
			// Enter manual mode if not already
			manualMode = true;
			
			// Cycle to next weather type
			currentWeatherIndex = (currentWeatherIndex + 1) % WEATHER_TYPE_COUNT;
			uint8_t newWeatherType = WEATHER_TYPES[currentWeatherIndex];
			
			Serial.print("Changed to weather type: ");
			Serial.println(getWeatherTypeName(newWeatherType));
			
			// Update the display using our new helper function
			updateWeatherType(newWeatherType, animatedMode);
			
			// Show current state
			printCurrentState();
			
			// Reset debounce timer after handling the press
			lastEncoderDebounceTime = 0;
		}
	}
	
	// Handle back button (with debounce)
	if (backButtonState != lastBackButtonState) {
		lastBackDebounceTime = millis();
	}
	
	if ((millis() - lastBackDebounceTime) > debounceDelay && lastBackDebounceTime != 0) {
		// If button has been stable in LOW state
		if (backButtonState == LOW && lastBackButtonState == LOW) {
			Serial.println("\n>>> BACK BUTTON PRESSED");
			
			// Exit manual mode if currently in it
			if (manualMode) {
				manualMode = false;
				Serial.println("Switching to LIVE weather data mode");
				
				// Connect to WiFi if not connected
				if (WiFi.status() != WL_CONNECTED) {
					connectToWiFi();
				}
				
				// Fetch current weather
				uint8_t currentWeather = fetchWeatherData();
				
				// Update display
				if (animatedMode) {
					currentFrame = 0;
					drawAnimatedWeather(currentWeather, currentFrame);
				} else {
					drawStaticWeather(currentWeather);
				}
				
				printCurrentState();
			}
			
			// Reset debounce timer after handling the press
			lastBackDebounceTime = 0;
		}
	}
	
	// Handle left button (with debounce)
	if (leftButtonState != lastLeftButtonState) {
		lastLeftDebounceTime = millis();
	}
	
	if ((millis() - lastLeftDebounceTime) > debounceDelay && lastLeftDebounceTime != 0) {
		// If button has been stable in LOW state
		if (leftButtonState == LOW && lastLeftButtonState == LOW) {
			Serial.println("\n>>> LEFT BUTTON PRESSED");
			
			// Toggle animation mode
			animatedMode = !animatedMode;
			
			Serial.print("Animation mode changed to: ");
			Serial.println(animatedMode ? "ANIMATED" : "STATIC");
			
			// Update display
			if (manualMode) {
				uint8_t currentWeatherType = WEATHER_TYPES[currentWeatherIndex];
				
				// Update using the unified helper function
				updateWeatherType(currentWeatherType, animatedMode);
			} else {
				// In live mode, get the current weather and update
				uint8_t currentWeather = fetchWeatherData();
				updateWeatherType(currentWeather, animatedMode);
			}
			
			// Show current state
			printCurrentState();
			
			// Reset debounce timer after handling the press
			lastLeftDebounceTime = 0;
		}
	}
	
	// Update button states
	lastEncoderPushState = encoderPushState;
	lastBackButtonState = backButtonState;
	lastLeftButtonState = leftButtonState;
}

void loop() {
	// Handle button presses
	handleButtons();
	
	// Check for idle timeout
	unsigned long currentMillis = millis();
	if (!isDisplayingIdle && (currentMillis - lastUserActivityTime >= idleTimeoutDuration)) {
		showIdleWeather();
	}
	
	// Handle animation states
	if (isDisplayingIdle || (!isTransitioning && isDisplayOn)) {
		// Handle animation if in animated mode and not in a transition
		if (animatedMode && (isDisplayingIdle || manualMode)) {
			// Only update frames at the specified delay
			if (currentMillis - lastFrameTime >= frameDelay) {
				lastFrameTime = currentMillis;
				
				// Cycle through frames
				currentFrame = (currentFrame + 1) % getAnimationFrameCount(lastWeatherType);
				
				// Use our manual drawing to show the animation frame
				drawAnimatedWeather(lastWeatherType, currentFrame);
				
				// Log the frame change (every 5 frames to avoid flooding)
				if (currentFrame % 5 == 0) {
					Serial.print("Showing frame ");
					Serial.print(currentFrame + 1);
					Serial.print("/");
					Serial.print(getAnimationFrameCount(lastWeatherType));
					Serial.print(" for weather: ");
					Serial.println(getWeatherTypeName(lastWeatherType));
				}
			}
		}
	}
	// Handle transition if one is active
	else if (isTransitioning) {
		if (currentMillis - transitionStartTime >= transitionDuration) {
			isTransitioning = false;
			Serial.println("Transition complete");
		}
	}
	
	// If in live mode (not manual and not idle), periodically update weather data
	if (!manualMode && !isDisplayingIdle && isDisplayOn) {
		static unsigned long lastFetchTime = 0;
		if (currentMillis - lastFetchTime >= 300000) { // 5 minutes
			lastFetchTime = currentMillis;
			
			// Fetch current weather
			uint8_t currentWeather = fetchWeatherData();
			
			// Only update if weather has changed
			if (currentWeather != lastWeatherType) {
				lastWeatherType = currentWeather;
				
				// Use library transition effect
				isTransitioning = true;
				transitionStartTime = currentMillis;
				
				if (animatedMode) {
					weatherAnim.runTransition(currentWeather, TRANSITION_FADE, transitionDuration);
				} else {
					drawStaticWeather(currentWeather);
				}
				
				Serial.print("Weather changed to: ");
				Serial.println(getWeatherTypeName(currentWeather));
			}
		}
	}
	
	// Add a small delay to prevent excessive updates
	delay(10);
} 