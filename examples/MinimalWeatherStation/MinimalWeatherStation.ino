/*
 * Minimal Weather Station Example for WeatherAnimations Library
 * 
 * This example demonstrates how to use the WeatherAnimations library
 * with Serial debugging disabled from the library itself.
 * It also supports fetching animated weather icons from online sources.
 */

// Define this before including the library to disable Serial debug messages
#define WA_DISABLE_SERIAL

#include <WeatherAnimations.h>
#include <Arduino.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// Home Assistant settings
const char* haIP = "YourHomeAssistantIP";
const char* haToken = "YourHomeAssistantToken";
const char* weatherEntity = "weather.forecast_home";

// OLED display settings
const int OLED_ADDRESS = 0x3C;

// Animation URLs
const char* clearSkyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/sunny-day_frame_";
const char* cloudyURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/cloudy_frame_";
const char* rainURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/rainy_frame_";
const char* snowURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/snowy_frame_";
const char* stormURL = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/oled_animated/lightning_frame_";

// WeatherAnimations instance
WeatherAnimations oledWeather(ssid, password, haIP, haToken);

// Button to toggle animation mode
const int modeButton = 12;
bool animatedMode = true;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void setup() {
	// Initialize serial for our own debug messages
	Serial.begin(115200);
	Serial.println("Starting Minimal Weather Station Example");
	
	// Set up button input
	pinMode(modeButton, INPUT_PULLUP);
	
	// Initialize OLED display
	Wire.begin();
	
	// Initialize the library with OLED display
	oledWeather.begin(OLED_DISPLAY, OLED_ADDRESS, true);
	
	// Configure the weather entity
	oledWeather.setWeatherEntity(weatherEntity);
	
	// Set animation mode to online animations
	oledWeather.setAnimationMode(ANIMATION_ONLINE);
	
	// Set continuous weather display mode
	oledWeather.setMode(CONTINUOUS_WEATHER);
	
	// Set online animation sources
	oledWeather.setOnlineAnimationSource(WEATHER_CLEAR, clearSkyURL);
	oledWeather.setOnlineAnimationSource(WEATHER_CLOUDY, cloudyURL);
	oledWeather.setOnlineAnimationSource(WEATHER_RAIN, rainURL);
	oledWeather.setOnlineAnimationSource(WEATHER_SNOW, snowURL);
	oledWeather.setOnlineAnimationSource(WEATHER_STORM, stormURL);
	
	Serial.println("Setup complete");
}

void handleButton() {
	// Read the current button state
	bool buttonState = digitalRead(modeButton);
	
	// If the button state changed, reset the debounce timer
	if (buttonState != lastButtonState) {
		lastDebounceTime = millis();
	}
	
	// If the button has been stable for long enough
	if ((millis() - lastDebounceTime) > debounceDelay) {
		// If the button was pressed (went from HIGH to LOW)
		if (buttonState == LOW && lastButtonState == HIGH) {
			// Toggle animation mode
			animatedMode = !animatedMode;
			
			// Set the animation mode
			oledWeather.setAnimationMode(animatedMode ? ANIMATION_ONLINE : ANIMATION_STATIC);
			
			Serial.print("Animation mode changed to: ");
			Serial.println(animatedMode ? "Online/Animated" : "Static");
		}
	}
	
	// Update last button state
	lastButtonState = buttonState;
}

void loop() {
	// Handle button press to toggle animation mode
	handleButton();
	
	// Update display and check for new weather data
	oledWeather.update();
	
	// Small delay to prevent too frequent updates
	delay(50);
} 