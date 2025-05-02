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

// Animation URLs for multiple frames (base URLs - we'll append frame numbers)
const char* clearSkyDayBaseURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/sunny-day_frame_";
const char* clearSkyNightBaseURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/clear-night_frame_";
const char* cloudyBaseURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/cloudy_frame_";
const char* rainBaseURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/rainy_frame_";
const char* heavyRainBaseURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/pouring_frame_";
const char* snowBaseURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/snowy_frame_";
const char* stormBaseURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/lightning_frame_";
const char* stormRainBaseURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/lightning-rainy_frame_";

// Animation state variables
bool isTransitioning = false;
unsigned long transitionStartTime = 0;
const unsigned long transitionDuration = 1000; // ms
uint8_t lastWeatherType = WEATHER_CLEAR;

// Network and API credentials
// These will be overridden by config.h if it exists
#ifndef CONFIG_EXISTS
	const char* ssid = "YourWiFiSSID";
	const char* password = "YourWiFiPassword";
	const char* haIP = "YourHomeAssistantIP";
	const char* haToken = "YourHomeAssistantToken";
	const char* weatherEntity = "weather.forecast_home";
	
	// Animation URLs for OLED display - using PNG frames
	const char* clearSkyDayURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/sunny-day_frame_000.png";
	const char* clearSkyNightURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/clear-night_frame_000.png";
	const char* cloudyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/cloudy_frame_000.png";
	const char* rainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/rainy_frame_000.png";
	const char* heavyRainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/pouring_frame_000.png";
	const char* snowURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/snowy_frame_000.png";
	const char* stormURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/lightning_frame_000.png";
	const char* stormRainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/lightning-rainy_frame_000.png";
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
	
	// Animation URLs from config (if defined)
	#ifdef ANIM_CLEAR_DAY
	const char* clearSkyDayURL = ANIM_CLEAR_DAY;
	#else
	const char* clearSkyDayURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/sunny-day_frame_000.png";
	#endif
	
	#ifdef ANIM_CLEAR_NIGHT
	const char* clearSkyNightURL = ANIM_CLEAR_NIGHT;
	#else
	const char* clearSkyNightURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/clear-night_frame_000.png";
	#endif
	
	#ifdef ANIM_CLOUDY
	const char* cloudyURL = ANIM_CLOUDY;
	#else
	const char* cloudyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/cloudy_frame_000.png";
	#endif
	
	#ifdef ANIM_RAIN
	const char* rainURL = ANIM_RAIN;
	#else
	const char* rainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/rainy_frame_000.png";
	#endif
	
	#ifdef ANIM_HEAVY_RAIN
	const char* heavyRainURL = ANIM_HEAVY_RAIN;
	#else
	const char* heavyRainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/pouring_frame_000.png";
	#endif
	
	#ifdef ANIM_SNOW
	const char* snowURL = ANIM_SNOW;
	#else
	const char* snowURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/snowy_frame_000.png";
	#endif
	
	#ifdef ANIM_STORM
	const char* stormURL = ANIM_STORM;
	#else
	const char* stormURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/lightning_frame_000.png";
	#endif
	
	#ifdef ANIM_STORM_RAIN
	const char* stormRainURL = ANIM_STORM_RAIN;
	#else
	const char* stormRainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/lightning-rainy_frame_000.png";
	#endif
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
	
	display.display();
}

// Function to get frame URL for animation
String getFrameURL(const char* baseURL, int frameNum) {
	char buffer[150];
	sprintf(buffer, "%s%03d.png", baseURL, frameNum);
	return String(buffer);
}

// Preload animation frames for a given weather type
void preloadAnimationFrames(uint8_t weatherType) {
	// Do nothing if not connected to WiFi
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("Cannot preload - WiFi not connected");
		return;
	}
	
	Serial.print("Preloading animation frames for weather type: ");
	Serial.println(getWeatherTypeName(weatherType));
	
	// Get the base URL for animations
	const char* baseURL = NULL;
	switch(weatherType) {
		case WEATHER_CLEAR: baseURL = clearSkyDayBaseURL; break;
		case WEATHER_CLOUDY: baseURL = cloudyBaseURL; break;
		case WEATHER_RAIN: baseURL = rainBaseURL; break;
		case WEATHER_SNOW: baseURL = snowBaseURL; break;
		case WEATHER_STORM: baseURL = stormBaseURL; break;
	}
	
	if (!baseURL) {
		Serial.println("No base URL found for this weather type");
		return;
	}
	
	// Use HTTPClient to download each frame
	HTTPClient http;
	
	// Start with frame 0
	for (int i = 0; i < 3; i++) { // Only preload first few frames
		String frameURL = getFrameURL(baseURL, i);
		Serial.print("Preloading frame ");
		Serial.print(i);
		Serial.print(": ");
		Serial.println(frameURL);
		
		// Set the animation source (this will trigger a download)
		weatherAnim.setOnlineAnimationSource(weatherType, frameURL.c_str());
		
		// Brief delay to allow download to start
		delay(100);
	}
	
	Serial.println("Preloading complete");
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
		
		// Preload animation frames in the background
		preloadAnimationFrames(newWeatherType);
		
		Serial.print("Starting transition type: ");
		Serial.println(transitionType);
	} else {
		// For static mode, use direct drawing
		drawStaticWeather(newWeatherType);
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
	
	// Set animation mode to online to use online resources
	weatherAnim.setAnimationMode(ANIMATION_ONLINE);
	
	// Set the custom weather entity ID
	weatherAnim.setWeatherEntity(weatherEntity);
	
	// Configure static frames for each weather type first
	Serial.println("Setting up animation frames...");
	
	// Set static frames for each weather type - use exact URLs
	Serial.println("Setting up static PNG images for each weather type...");
	weatherAnim.setOnlineAnimationSource(WEATHER_CLEAR, clearSkyDayURL);
	Serial.print("CLEAR: "); 
	Serial.println(clearSkyDayURL);

	weatherAnim.setOnlineAnimationSource(WEATHER_CLOUDY, cloudyURL);
	Serial.print("CLOUDY: ");
	Serial.println(cloudyURL);

	weatherAnim.setOnlineAnimationSource(WEATHER_RAIN, rainURL);
	Serial.print("RAIN: ");
	Serial.println(rainURL);

	weatherAnim.setOnlineAnimationSource(WEATHER_SNOW, snowURL);
	Serial.print("SNOW: ");
	Serial.println(snowURL);

	weatherAnim.setOnlineAnimationSource(WEATHER_STORM, stormURL);
	Serial.print("STORM: ");
	Serial.println(stormURL);

	// Use SIMPLE_TRANSITION mode for manual control
	weatherAnim.setMode(SIMPLE_TRANSITION);

	// Set display update interval (shorter for better responsiveness)
	weatherAnim.setRefreshInterval(100);

	// Connect to WiFi for loading images
	Serial.println("Connecting to WiFi for online animations...");
	if (connectToWiFi()) {
		Serial.println("WiFi Connected - attempting to load initial images");
		
		// Force the library to load the first image
		weatherAnim.displayWeather(WEATHER_TYPES[currentWeatherIndex]);
	} else {
		Serial.println("WiFi connection failed - will use built-in animations");
	}

	// Give time for the first image to load
	Serial.println("Waiting for initial image to load...");
	delay(1000);

	// Display initial weather in static mode
	lastWeatherType = WEATHER_TYPES[currentWeatherIndex];
	drawStaticWeather(lastWeatherType);
	
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
				
				// If switching to animated mode, preload the frames
				if (animatedMode) {
					preloadAnimationFrames(currentWeatherType);
				}
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
	
	// Handle animation states
	unsigned long currentMillis = millis();
	
	// If transitioning, check if transition is complete
	if (isTransitioning) {
		if (currentMillis - transitionStartTime >= transitionDuration) {
			isTransitioning = false;
			Serial.println("Transition complete");
		}
	}
	
	// Handle animation if in animated mode and manual mode
	if (animatedMode && manualMode && !isTransitioning) {
		// Only update frames if not in a transition
		if (currentMillis - lastFrameTime >= frameDelay) {
			lastFrameTime = currentMillis;
			
			// Cycle through frames (limit to actual frame count for each animation)
			currentFrame = (currentFrame + 1) % MAX_FRAMES;
			
			// Get the base URL for the current weather type
			const char* baseURL = NULL;
			switch(lastWeatherType) {
				case WEATHER_CLEAR: baseURL = clearSkyDayBaseURL; break;
				case WEATHER_CLOUDY: baseURL = cloudyBaseURL; break;
				case WEATHER_RAIN: baseURL = rainBaseURL; break;
				case WEATHER_SNOW: baseURL = snowBaseURL; break;
				case WEATHER_STORM: baseURL = stormBaseURL; break;
			}
			
			if (baseURL) {
				// Get the URL for this frame
				String frameURL = getFrameURL(baseURL, currentFrame);
				
				// Log the frame change (every 5 frames to avoid flooding)
				if (currentFrame % 5 == 0) {
					Serial.print("Showing frame ");
					Serial.print(currentFrame + 1);
					Serial.print("/");
					Serial.print(MAX_FRAMES);
					Serial.print(" for weather: ");
					Serial.println(getWeatherTypeName(lastWeatherType));
					Serial.print("URL: ");
					Serial.println(frameURL);
				}
				
				// Update the animation source
				weatherAnim.setOnlineAnimationSource(lastWeatherType, frameURL.c_str());
				
				// Force a direct display update to show the new frame
				weatherAnim.displayWeather(lastWeatherType);
			}
			
			// Also use our manual drawing as a fallback 
			drawAnimatedWeather(lastWeatherType, currentFrame);
		}
		
		// Allow the library to process any updates
		weatherAnim.update();
	} 
	// If in live mode (not manual), use the WeatherAnimations library
	else if (!manualMode) {
		// Let the WeatherAnimations library handle updates
		weatherAnim.update();
		
		// Periodically refresh weather data
		static unsigned long lastFetchTime = 0;
		if (currentMillis - lastFetchTime >= 300000) { // 5 minutes
			lastFetchTime = currentMillis;
			
			// Fetch weather data ourselves for display
			uint8_t currentWeather = fetchWeatherData();
			
			// Update library with current weather if needed
			if (currentWeather != weatherAnim.getCurrentWeather()) {
				// Run transition to the new weather
				lastWeatherType = currentWeather;
				isTransitioning = true;
				transitionStartTime = currentMillis;
				
				if (animatedMode) {
					// Use library transition effect
					weatherAnim.runTransition(currentWeather, TRANSITION_FADE, transitionDuration);
				} else {
					// Just update static display
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