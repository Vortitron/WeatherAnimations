/*
 * Basic Usage Example for WeatherAnimations Library
 * 
 * This example demonstrates how to initialize the WeatherAnimations library,
 * connect to Home Assistant, and display weather animations on an OLED display.
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

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  
  // Initialize the library with OLED display, and specify if Wi-Fi is managed by the library
  // Set manageWiFi to false if Wi-Fi connection is handled elsewhere in your project
  weatherAnim.begin(OLED_DISPLAY, 0x3C, true);
  
  // Set the custom weather entity ID
  weatherAnim.setWeatherEntity(weatherEntity);
  
  // Set mode to continuous weather display
  weatherAnim.setMode(CONTINUOUS_WEATHER);
  
  Serial.println("Setup complete, starting weather updates...");
}

void loop() {
  // Update weather data and display animations
  weatherAnim.update();
  
  // Add a small delay to prevent excessive updates
  delay(100);
} 