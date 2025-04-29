/*
 * Basic Usage Example for WeatherAnimations Library
 * 
 * This example demonstrates how to initialize the WeatherAnimations library,
 * connect to Home Assistant, and display weather animations on an OLED or TFT display.
 */

#include <WeatherAnimations.h>

// Replace with your Wi-Fi and Home Assistant credentials
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
const char* haIP = "YourHomeAssistantIP"; // e.g., "192.168.1.100"
const char* haToken = "YourHomeAssistantToken"; // Long-lived access token from Home Assistant

// Custom weather entity ID from Home Assistant (e.g., using Met.no integration)
const char* weatherEntity = "weather.forecast_home";

// Create WeatherAnimations instance
WeatherAnimations weatherAnim(ssid, password, haIP, haToken);

// Uncomment the following line to use TFT display instead of OLED
// #define USE_TFT_DISPLAY

// Online animation sources (URLs to animation or image files)
const char* clearSkyAnimURL = "https://example.com/animations/clear_sky.jpg";
const char* cloudyAnimURL = "https://example.com/animations/cloudy.jpg";
const char* rainAnimURL = "https://example.com/animations/rain.jpg";
const char* snowAnimURL = "https://example.com/animations/snow.jpg";
const char* stormAnimURL = "https://example.com/animations/storm.jpg";

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  
  // Initialize the library with OLED or TFT display, and specify if Wi-Fi is managed by the library
  // Set manageWiFi to false if Wi-Fi connection is handled elsewhere in your project
  #ifdef USE_TFT_DISPLAY
    weatherAnim.begin(TFT_DISPLAY, 0, true);
    
    // Set online animation sources for TFT display
    weatherAnim.setOnlineAnimationSource(WEATHER_CLEAR, clearSkyAnimURL);
    weatherAnim.setOnlineAnimationSource(WEATHER_CLOUDY, cloudyAnimURL);
    weatherAnim.setOnlineAnimationSource(WEATHER_RAIN, rainAnimURL);
    weatherAnim.setOnlineAnimationSource(WEATHER_SNOW, snowAnimURL);
    weatherAnim.setOnlineAnimationSource(WEATHER_STORM, stormAnimURL);
  #else
    weatherAnim.begin(OLED_DISPLAY, 0x3C, true);
    // OLED display uses built-in monochrome animations
  #endif
  
  // Set the custom weather entity ID
  weatherAnim.setWeatherEntity(weatherEntity);
  
  // Set mode to continuous weather display
  // Options: CONTINUOUS_WEATHER or SIMPLE_TRANSITION
  weatherAnim.setMode(CONTINUOUS_WEATHER);
  
  Serial.println("Setup complete, starting weather updates...");
}

void loop() {
  // Update weather data and display animations
  weatherAnim.update();
  
  // Add a small delay to prevent excessive updates
  delay(100);
} 