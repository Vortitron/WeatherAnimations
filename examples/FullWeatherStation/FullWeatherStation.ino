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
 */

#include <WeatherAnimations.h>
#include <Arduino.h>
#include <Wire.h>

// Optional: Include sensor libraries if you have local sensors
// #include <Adafruit_BME280.h>

// Pin definitions
const int encoderPUSH = 27;  // Encoder push button
const int encoderCLK = 26;   // Encoder clock pin
const int encoderDT = 25;    // Encoder data pin
const int backButton = 14;   // Back button
const int modeButton = 12;   // Mode selection button

// OLED display settings (using I2C)
const int OLED_ADDRESS = 0x3C;
const int OLED_SDA = 21;
const int OLED_SCL = 22;

// TFT display settings (using SPI)
const int TFT_CS = 5;
const int TFT_DC = 4;
const int TFT_RST = 2;

// Replace with your network credentials
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// Home Assistant settings
const char* haIP = "YourHomeAssistantIP"; // e.g., "192.168.1.100"
const char* haToken = "YourHomeAssistantToken";
const char* weatherEntity = "weather.forecast_home";
const char* temperatureEntity = "sensor.outside_temperature";
const char* humidityEntity = "sensor.outside_humidity";
const char* pressureEntity = "sensor.outside_pressure";

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

// WeatherAnimations instances for each display
WeatherAnimations oledWeather(ssid, password, haIP, haToken);
WeatherAnimations tftWeather(ssid, password, haIP, haToken);

// Optional: Sensor instance
// Adafruit_BME280 bme;

// Weather animation URLs
const char* clearDayURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/sunny-day.gif";
const char* clearNightURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/clear-night.gif";
const char* cloudyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/cloudy.gif";
const char* partlyCloudyDayURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/partlycloudy-day.gif";
const char* partlyCloudyNightURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/partlycloudy-night.gif";
const char* rainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/rainy.gif";
const char* snowURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/snowy.gif";
const char* stormURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/lightning.gif";
const char* fogURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/fog.gif";

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
	
	// Initialize I2C for OLED
	Wire.begin(OLED_SDA, OLED_SCL);
	
	// Initialize input pins
	pinMode(encoderPUSH, INPUT_PULLUP);
	pinMode(encoderCLK, INPUT_PULLUP);
	pinMode(encoderDT, INPUT_PULLUP);
	pinMode(backButton, INPUT_PULLUP);
	pinMode(modeButton, INPUT_PULLUP);
	
	// Initialize OLED display
	oledWeather.begin(OLED_DISPLAY, OLED_ADDRESS, false); // Don't manage WiFi here
	
	// Initialize TFT display
	tftWeather.begin(TFT_DISPLAY, TFT_CS, TFT_DC, TFT_RST, true); // Manage WiFi here
	
	// Set weather entities and animation modes
	oledWeather.setWeatherEntity(weatherEntity);
	tftWeather.setWeatherEntity(weatherEntity);
	
	// Set TFT animation sources - each uses a different animation source
	tftWeather.setOnlineAnimationSource(WEATHER_CLEAR, true, clearDayURL);
	tftWeather.setOnlineAnimationSource(WEATHER_CLEAR, false, clearNightURL);
	tftWeather.setOnlineAnimationSource(WEATHER_CLOUDY, cloudyURL);
	tftWeather.setOnlineAnimationSource(WEATHER_PARTLYCLOUDY, true, partlyCloudyDayURL);
	tftWeather.setOnlineAnimationSource(WEATHER_PARTLYCLOUDY, false, partlyCloudyNightURL);
	tftWeather.setOnlineAnimationSource(WEATHER_RAIN, rainURL);
	tftWeather.setOnlineAnimationSource(WEATHER_SNOW, snowURL);
	tftWeather.setOnlineAnimationSource(WEATHER_LIGHTNING, stormURL);
	tftWeather.setOnlineAnimationSource(WEATHER_FOG, fogURL);
	
	// Set animation modes
	oledWeather.setMode(CONTINUOUS_WEATHER);
	tftWeather.setMode(CONTINUOUS_WEATHER);
	
	// Set refresh intervals
	oledWeather.setRefreshInterval(5000);  // 5 seconds
	tftWeather.setRefreshInterval(30000);  // 30 seconds
	
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
	
	// Update button states
	lastEncoderPushState = encoderPushState;
	lastBackButtonState = backButtonState;
	lastModeButtonState = modeButtonState;
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