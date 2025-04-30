/*
 * Full Weather Station Example for WeatherAnimations Library
 * 
 * This comprehensive example demonstrates how to:
 * 1. Use both OLED and TFT displays simultaneously
 * 2. Display weather forecasts with animations
 * 3. Handle user input for modes and settings
 * 4. Connect to multiple weather data sources
 * 
 * Hardware requirements:
 * - ESP32 or other compatible board
 * - SSD1306 OLED display (128x64 or similar)
 * - ST7789 or ILI9341 TFT display (240x240 recommended)
 * - Rotary encoder with push button
 * - Optional: BME280/BME680 sensor for local readings
 * 
 * Setup:
 * 1. Copy examples/config_example.h to examples/FullWeatherStation/config.h
 * 2. Edit config.h with your WiFi and Home Assistant credentials
 */

// Uncomment this line to disable Serial debug messages from the WeatherAnimations library
// #define WA_DISABLE_SERIAL

#include <WeatherAnimations.h>
#include <Arduino.h>
#include <Wire.h>

// Try to include the configuration file
// If it doesn't exist, we'll use default values
#if __has_include("config.h")
	#include "config.h"
	#define CONFIG_EXISTS
#endif

// Optional: Include sensor libraries if you have local sensors
// #include <Adafruit_BME280.h>

// Pin definitions
const int encoderPUSH = 27;  // Encoder push button
const int encoderCLK = 26;   // Encoder clock pin
const int encoderDT = 25;    // Encoder data pin
const int backButton = 14;   // Back button
const int modeButton = 12;   // Mode selection button
const int animModeButton = 13; // Animation mode selection button

// OLED display settings (using I2C)
#ifndef CONFIG_EXISTS
	const int OLED_ADDRESS = 0x3C;
	const int OLED_SDA = 21;
	const int OLED_SCL = 22;
#else
	const int OLED_ADDRESS = OLED_ADDRESS;
	const int OLED_SDA = 21;  // Default pins, can be customized in your config if needed
	const int OLED_SCL = 22;
#endif

// TFT display settings (using SPI)
const int TFT_CS = 5;
const int TFT_DC = 4;
const int TFT_RST = 2;

// Network and Home Assistant settings
#ifndef CONFIG_EXISTS
	// Default values if no config.h exists
	const char* ssid = "YourWiFiSSID";
	const char* password = "YourWiFiPassword";
	const char* haIP = "YourHomeAssistantIP";
	const char* haToken = "YourHomeAssistantToken";
	const char* weatherEntity = "weather.forecast_home";
	const char* temperatureEntity = "sensor.outside_temperature";
	const char* humidityEntity = "sensor.outside_humidity";
	const char* pressureEntity = "sensor.outside_pressure";
	
	// Animation URLs (default to GitHub links)
	const char* clearDayURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/sunny-day.gif";
	const char* clearNightURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/clear-night.gif";
	const char* cloudyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/cloudy.gif";
	const char* partlyCloudyDayURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/partlycloudy-day.gif";
	const char* partlyCloudyNightURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/partlycloudy-night.gif";
	const char* rainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/rainy.gif";
	const char* snowURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/snowy.gif";
	const char* stormURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/lightning.gif";
	const char* fogURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/fog.gif";
#else
	// Use settings from config.h
	const char* ssid = WIFI_SSID;
	const char* password = WIFI_PASSWORD;
	const char* haIP = HA_IP;
	const char* haToken = HA_TOKEN;
	const char* weatherEntity = HA_WEATHER_ENTITY;
	const char* temperatureEntity = HA_TEMPERATURE_ENTITY;
	const char* humidityEntity = HA_HUMIDITY_ENTITY;
	const char* pressureEntity = HA_PRESSURE_ENTITY;
	
	// Animation URLs from config
	const char* clearDayURL = ANIM_CLEAR_DAY;
	const char* clearNightURL = ANIM_CLEAR_NIGHT;
	const char* cloudyURL = ANIM_CLOUDY;
	const char* partlyCloudyDayURL = ANIM_PARTLYCLOUDY_DAY;
	const char* partlyCloudyNightURL = ANIM_PARTLYCLOUDY_NIGHT;
	const char* rainURL = ANIM_RAIN;
	const char* snowURL = ANIM_SNOW;
	const char* stormURL = ANIM_STORM;
	const char* fogURL = ANIM_FOG;
#endif

// Display modes
enum DisplayMode {
	CURRENT_WEATHER,
	FORECAST_WEATHER,
	SENSOR_DATA,
	SETTINGS
};

// State variables
DisplayMode currentMode = CURRENT_WEATHER;
int forecastDay = 0;  // 0 = today, 1 = tomorrow, etc.
int encoderPos = 0;
int lastEncoderPos = 0;
bool displayingDetails = false;
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 300000; // 5 minutes

// Animation mode flag
bool onlineAnimationsEnabled = true;
bool lastAnimModeButtonState = HIGH;

// WeatherAnimations instances for each display
WeatherAnimations oledWeather(ssid, password, haIP, haToken);
WeatherAnimations tftWeather(ssid, password, haIP, haToken);

// Optional: Sensor instance
// Adafruit_BME280 bme;

// Forward declarations for helper functions
void handleEncoderRotation();
void handleButtonPresses();
void updateDisplays();
void showWeatherDetails();
void changeMode(DisplayMode newMode);
float readLocalTemperature();
float readLocalHumidity();
float readLocalPressure();

void setup() {
	// Initialize serial for debugging
	Serial.begin(115200);
	Serial.println("Starting Full Weather Station Example");
	
	#ifdef CONFIG_EXISTS
	Serial.println("Using configuration from config.h");
	#else
	Serial.println("WARNING: No config.h found. Using default values.");
	Serial.println("Copy config_example.h to config.h and customize it.");
	#endif
	
	// Initialize I2C for OLED
	Wire.begin(OLED_SDA, OLED_SCL);
	
	// Initialize input pins
	pinMode(encoderPUSH, INPUT_PULLUP);
	pinMode(encoderCLK, INPUT_PULLUP);
	pinMode(encoderDT, INPUT_PULLUP);
	pinMode(backButton, INPUT_PULLUP);
	pinMode(modeButton, INPUT_PULLUP);
	pinMode(animModeButton, INPUT_PULLUP); // Added for animation mode toggle
	
	// Initialize OLED display
	oledWeather.begin(OLED_DISPLAY, OLED_ADDRESS, false); // Don't manage WiFi here
	
	// Initialize TFT display
	tftWeather.begin(TFT_DISPLAY, TFT_CS, TFT_DC, TFT_RST, true); // Manage WiFi here
	
	// Set weather entities and animation modes
	oledWeather.setWeatherEntity(weatherEntity);
	tftWeather.setWeatherEntity(weatherEntity);
	
	// Set animation mode to online by default
	oledWeather.setAnimationMode(ANIMATION_ONLINE);
	tftWeather.setAnimationMode(ANIMATION_ONLINE);
	
	// Set online animation URLs for basic weather types
	// URLs for OLED animations (frame-based PNG files)
	const char* clearSkyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/sunny-day_frame_";
	const char* cloudyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/cloudy_frame_";
	const char* rainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/rainy_frame_";
	const char* snowURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/snowy_frame_";
	const char* stormURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/lightning_frame_";
	
	// Set OLED animation sources for basic weather types
	oledWeather.setOnlineAnimationSource(WEATHER_CLEAR, clearSkyURL);
	oledWeather.setOnlineAnimationSource(WEATHER_CLOUDY, cloudyURL);
	oledWeather.setOnlineAnimationSource(WEATHER_RAIN, rainURL);
	oledWeather.setOnlineAnimationSource(WEATHER_SNOW, snowURL);
	oledWeather.setOnlineAnimationSource(WEATHER_STORM, stormURL);
	
	// Set TFT animation sources - using GIFs directly from our JSON file
	tftWeather.setOnlineAnimationSource(WEATHER_CLEAR, clearDayURL);
	tftWeather.setOnlineAnimationSource(WEATHER_CLOUDY, cloudyURL);
	tftWeather.setOnlineAnimationSource(WEATHER_RAIN, rainURL);
	tftWeather.setOnlineAnimationSource(WEATHER_SNOW, snowURL);
	tftWeather.setOnlineAnimationSource(WEATHER_STORM, stormURL);
	
	// Set mode to continuous weather display
	oledWeather.setMode(CONTINUOUS_WEATHER);
	tftWeather.setMode(CONTINUOUS_WEATHER);
	
	// Set forecast day (0 = today, 1 = tomorrow, etc.)
	oledWeather.setForecastDay(0);
	tftWeather.setForecastDay(0);
	
	// Optional: Initialize local sensors
	/*
	if (!bme.begin(0x76)) {
		Serial.println("Could not find BME280 sensor!");
	} else {
		Serial.println("BME280 sensor initialized");
	}
	*/
	
	Serial.println("Setup complete");
}

void loop() {
	// Handle user input
	handleEncoderRotation();
	handleButtonPresses();
	
	// Update displays based on current mode
	updateDisplays();
	
	// Update weather data at regular intervals
	if (millis() - lastUpdateTime > updateInterval) {
		oledWeather.refreshWeatherData();
		tftWeather.refreshWeatherData();
		lastUpdateTime = millis();
	}
	
	// Small delay to prevent too frequent updates
	delay(50);
}

void handleEncoderRotation() {
	// Simple encoder reading (actual implementation might need to be more robust)
	static int lastCLK = HIGH;
	int currentCLK = digitalRead(encoderCLK);
	
	if (currentCLK != lastCLK && currentCLK == LOW) {
		if (digitalRead(encoderDT) != currentCLK) {
			encoderPos++;
		} else {
			encoderPos--;
		}
		
		// Use encoder position based on current mode
		switch (currentMode) {
			case CURRENT_WEATHER:
				// Do nothing for current weather
				break;
				
			case FORECAST_WEATHER:
				// Change forecast day (limit to 0-5 days)
				forecastDay = constrain(encoderPos % 6, 0, 5);
				oledWeather.setForecastDay(forecastDay);
				tftWeather.setForecastDay(forecastDay);
				break;
				
			case SENSOR_DATA:
				// Scroll through sensor data
				break;
				
			case SETTINGS:
				// Navigate settings menu
				break;
		}
	}
	
	lastCLK = currentCLK;
}

void handleButtonPresses() {
	// Read button states
	static bool lastEncoderPushState = HIGH;
	static bool lastBackButtonState = HIGH;
	static bool lastModeButtonState = HIGH;
	
	bool encoderPushState = digitalRead(encoderPUSH);
	bool backButtonState = digitalRead(backButton);
	bool modeButtonState = digitalRead(modeButton);
	bool animModeButtonState = digitalRead(animModeButton);
	
	// Handle encoder push button (with simple debounce)
	if (encoderPushState != lastEncoderPushState && encoderPushState == LOW) {
		// Toggle detailed view based on current mode
		displayingDetails = !displayingDetails;
		if (displayingDetails) {
			showWeatherDetails();
		}
	}
	
	// Handle back button
	if (backButtonState != lastBackButtonState && backButtonState == LOW) {
		// Return to main view from detailed view
		displayingDetails = false;
	}
	
	// Handle mode button
	if (modeButtonState != lastModeButtonState && modeButtonState == LOW) {
		// Cycle through display modes
		switch (currentMode) {
			case CURRENT_WEATHER:
				changeMode(FORECAST_WEATHER);
				break;
				
			case FORECAST_WEATHER:
				changeMode(SENSOR_DATA);
				break;
				
			case SENSOR_DATA:
				changeMode(SETTINGS);
				break;
				
			case SETTINGS:
				changeMode(CURRENT_WEATHER);
				break;
		}
	}
	
	// Handle animation mode button
	if (animModeButtonState != lastAnimModeButtonState && animModeButtonState == LOW) {
		// Toggle animation mode
		onlineAnimationsEnabled = !onlineAnimationsEnabled;
		
		// Update animation mode for both displays
		uint8_t newMode = onlineAnimationsEnabled ? ANIMATION_ONLINE : ANIMATION_STATIC;
		oledWeather.setAnimationMode(newMode);
		tftWeather.setAnimationMode(newMode);
		
		Serial.print("Animation mode changed to: ");
		Serial.println(onlineAnimationsEnabled ? "Online/Animated" : "Static");
	}
	
	// Update button states
	lastEncoderPushState = encoderPushState;
	lastBackButtonState = backButtonState;
	lastModeButtonState = modeButtonState;
	lastAnimModeButtonState = animModeButtonState;
}

void updateDisplays() {
	// Update displays based on current mode
	switch (currentMode) {
		case CURRENT_WEATHER:
			// Both displays show current weather
			if (!displayingDetails) {
				oledWeather.update();
				tftWeather.update();
			}
			break;
			
		case FORECAST_WEATHER:
			// Both displays show forecast for selected day
			if (!displayingDetails) {
				// Update displays with current forecast day
				oledWeather.update();
				tftWeather.update();
				
				// Show day indicator on OLED
				String dayLabel = "Day: " + String(forecastDay);
				if (forecastDay == 0) dayLabel += " (Today)";
				if (forecastDay == 1) dayLabel += " (Tomorrow)";
				oledWeather.displayText(dayLabel, 0, 0);
			}
			break;
			
		case SENSOR_DATA:
			// Show sensor data on displays
			if (!displayingDetails) {
				// Example to display local sensor readings if available
				/*
				float temp = readLocalTemperature();
				float humidity = readLocalHumidity();
				float pressure = readLocalPressure();
				
				String tempStr = "Temp: " + String(temp, 1) + "°C";
				String humStr = "Humidity: " + String(humidity, 0) + "%";
				String pressStr = "Pressure: " + String(pressure, 0) + " hPa";
				
				oledWeather.displayText(tempStr, 0, 0);
				oledWeather.displayText(humStr, 0, 16);
				oledWeather.displayText(pressStr, 0, 32);
				
				// TFT can show a graph or more detailed info
				*/
			}
			break;
			
		case SETTINGS:
			// Show settings menu
			if (!displayingDetails) {
				oledWeather.displayText("Settings Menu", 0, 0);
				oledWeather.displayText("1. WiFi Setup", 0, 16);
				oledWeather.displayText("2. Display Options", 0, 32);
				oledWeather.displayText("3. Weather Source", 0, 48);
				
				// TFT can show a more detailed settings interface
			}
			break;
	}
}

void showWeatherDetails() {
	// Show detailed information based on current mode
	switch (currentMode) {
		case CURRENT_WEATHER:
			// Show detailed current weather info
			tftWeather.displayWeatherDetails();
			break;
			
		case FORECAST_WEATHER:
			// Show detailed forecast info
			tftWeather.displayForecastDetails(forecastDay);
			break;
			
		default:
			// For other modes, just show a message
			oledWeather.displayText("Detailed view", 0, 32);
			break;
	}
}

void changeMode(DisplayMode newMode) {
	currentMode = newMode;
	displayingDetails = false;
	
	// Reset encoder position for the new mode
	encoderPos = 0;
	
	// Update displays for the new mode
	switch (newMode) {
		case CURRENT_WEATHER:
			Serial.println("Mode: Current Weather");
			oledWeather.setForecastDay(0);
			tftWeather.setForecastDay(0);
			break;
			
		case FORECAST_WEATHER:
			Serial.println("Mode: Forecast Weather");
			forecastDay = 0;
			break;
			
		case SENSOR_DATA:
			Serial.println("Mode: Sensor Data");
			break;
			
		case SETTINGS:
			Serial.println("Mode: Settings");
			break;
	}
}

// Optional sensor reading functions
float readLocalTemperature() {
	/*
	if (bme.begin(0x76)) {
		return bme.readTemperature();
	}
	*/
	return 0.0;
}

float readLocalHumidity() {
	/*
	if (bme.begin(0x76)) {
		return bme.readHumidity();
	}
	*/
	return 0.0;
}

float readLocalPressure() {
	/*
	if (bme.begin(0x76)) {
		return bme.readPressure() / 100.0F;
	}
	*/
	return 0.0;
} 