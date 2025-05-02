/*
 * ESP32 Weather Display Example with SSD1306 Library
 * 
 * This example uses the Adafruit_SSD1306 library to work with SH1106 displays
 * on ESP32, as this combination often works better than the SH110X library.
 * It provides a simpler implementation focused on reliability.
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

//STOP REMOVING THESE .. We need them for the library to work 
// Include the library source files directly for development - comment out if using installed library
#include "../../src/WeatherAnimations.h"
#include "../../src/WeatherAnimations.cpp"
#include "../../src/WeatherAnimationsAnimations.h"
#include "../../src/WeatherAnimationsAnimations.cpp"
#include "../../src/WeatherAnimationsIcons.h"
#include "../../src/WeatherAnimationsIcons.cpp"

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
bool manualMode = true; // Start in manual mode by default
bool animatedMode = false; // Start with static mode
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

// Network and API credentials
// These will be overridden by config.h if it exists
#ifndef CONFIG_EXISTS
	const char* ssid = "YourWiFiSSID";
	const char* password = "YourWiFiPassword";
	const char* haIP = "YourHomeAssistantIP";
	const char* haToken = "YourHomeAssistantToken";
	const char* weatherEntity = "weather.forecast_home";
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
	
	const char* ssid = WIFI_SSID;
	const char* password = WIFI_PASSWORD;
	const char* haIP = HA_IP;
	const char* haToken = HA_TOKEN;
	const char* weatherEntity = HA_WEATHER_ENTITY;
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
	WiFi.begin(ssid, password);
	Serial.print("Connecting to WiFi");
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
	
	// Draw weather icon using the library
	weatherAnim.drawSSD1306Icon(&display, weatherType, 96, 32);
	
	// Draw mode indicators at bottom
	display.setTextSize(1);
	display.setCursor(0, 56);
	display.print(manualMode ? "MANUAL" : "LIVE");
	display.setCursor(70, 56);
	display.print(animatedMode ? "ANIM" : "STATIC");
	
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
	
	// Draw animated weather icon using the library
	weatherAnim.drawSSD1306AnimatedIcon(&display, weatherType, 96, 32, frame);
	
	// Draw mode indicators at bottom
	display.setTextSize(1);
	display.setCursor(0, 56);
	display.print(manualMode ? "MANUAL" : "LIVE");
	display.setCursor(70, 56);
	display.print(animatedMode ? "ANIM" : "STATIC");
	display.setCursor(110, 56);
	display.print(frame+1);
	
	display.display();
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
	display.println("Connecting...");
	display.display();
	
	Serial.println("OLED display initialized successfully");
	
	// Initialize the WeatherAnimations library
	weatherAnim.begin(SSD1306_DISPLAY, 0, false);
	
	// Connect to WiFi if not in manual mode
	if (!manualMode) {
		connectToWiFi();
	}
	
	// Display initial weather
	drawStaticWeather(WEATHER_TYPES[currentWeatherIndex]);
	
	Serial.println("Setup complete!");
	printCurrentState();
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
	
	if ((millis() - lastEncoderDebounceTime) > debounceDelay && lastEncoderDebounceTime != 0) {
		// If button has been stable in LOW state
		if (encoderPushState == LOW && lastEncoderPushState == LOW) {
			Serial.println("\n>>> ENCODER BUTTON PRESSED");
			// Enter manual mode if not already
			manualMode = true;
			
			// Cycle to next weather type
			currentWeatherIndex = (currentWeatherIndex + 1) % WEATHER_TYPE_COUNT;
			
			Serial.print("Changed to weather type: ");
			Serial.println(getWeatherTypeName(WEATHER_TYPES[currentWeatherIndex]));
			
			// Update display
			if (animatedMode) {
				currentFrame = 0;
				drawAnimatedWeather(WEATHER_TYPES[currentWeatherIndex], currentFrame);
			} else {
				drawStaticWeather(WEATHER_TYPES[currentWeatherIndex]);
			}
			
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
				if (animatedMode) {
					currentFrame = 0;
					drawAnimatedWeather(WEATHER_TYPES[currentWeatherIndex], currentFrame);
				} else {
					drawStaticWeather(WEATHER_TYPES[currentWeatherIndex]);
				}
			} else {
				// Fetch current weather
				uint8_t currentWeather = fetchWeatherData();
				
				if (animatedMode) {
					currentFrame = 0;
					drawAnimatedWeather(currentWeather, currentFrame);
				} else {
					drawStaticWeather(currentWeather);
				}
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
	
	// Handle animation if in animated mode
	if (animatedMode) {
		unsigned long currentMillis = millis();
		if (currentMillis - lastFrameTime >= frameDelay) {
			lastFrameTime = currentMillis;
			currentFrame = (currentFrame + 1) % 4; // 4 frames of animation
			
			if (manualMode) {
				drawAnimatedWeather(WEATHER_TYPES[currentWeatherIndex], currentFrame);
			} else {
				// Periodically refresh weather data
				static unsigned long lastFetchTime = 0;
				if (currentMillis - lastFetchTime >= 300000) { // 5 minutes
					lastFetchTime = currentMillis;
					uint8_t currentWeather = fetchWeatherData();
					drawAnimatedWeather(currentWeather, currentFrame);
				} else {
					// Just update animation frame
					drawAnimatedWeather(fetchWeatherData(), currentFrame);
				}
			}
		}
	} else if (!manualMode) {
		// Periodically refresh weather data in static mode
		static unsigned long lastFetchTime = 0;
		unsigned long currentMillis = millis();
		if (currentMillis - lastFetchTime >= 300000) { // 5 minutes
			lastFetchTime = currentMillis;
			uint8_t currentWeather = fetchWeatherData();
			drawStaticWeather(currentWeather);
		}
	}
	
	// Add a small delay to prevent excessive updates
	delay(50);
} 