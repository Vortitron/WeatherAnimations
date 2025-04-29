#ifndef WEATHER_ANIMATIONS_H
#define WEATHER_ANIMATIONS_H

// Define ARDUINO to be > 100 to use Arduino.h instead of WProgram.h
#ifndef ARDUINO
#define ARDUINO 10800 // Arduino 1.8.0 or higher
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <HttpClient.h>
#include <Adafruit_GFX.h>

// Include TFT_eSPI library for TFT display support
#include <TFT_eSPI.h>

// Define display types
#define OLED_DISPLAY 1
#define TFT_DISPLAY  2

// Define operation modes
#define SIMPLE_TRANSITION   1
#define CONTINUOUS_WEATHER  2

// Weather condition codes (simplified for demonstration)
#define WEATHER_CLEAR   0
#define WEATHER_CLOUDY  1
#define WEATHER_RAIN    2
#define WEATHER_SNOW    3
#define WEATHER_STORM   4

    class WeatherAnimations {
public:
    // Constructor with Wi-Fi and Home Assistant credentials
    WeatherAnimations(const char* ssid, const char* password, const char* haIP, const char* haToken);
    
    // Initialize the library and connect to Wi-Fi and display
    void begin(uint8_t displayType = OLED_DISPLAY, uint8_t i2cAddr = 0x3C, bool manageWiFi = true);
    
    // Set the operation mode
    void setMode(uint8_t mode);
    
    // Update weather data and manage animations
    void update();
    
    // Get current weather condition
    uint8_t getCurrentWeather() const;
    
    // Set custom animation frames for a weather condition
    void setAnimation(uint8_t weatherCondition, const uint8_t* frames[], uint8_t frameCount, uint16_t frameDelay);
    
    // Set custom weather entity ID for Home Assistant
    void setWeatherEntity(const char* entityID);
    
    // Set online animation source URL for a weather condition (for TFT or detailed animations)
    void setOnlineAnimationSource(uint8_t weatherCondition, const char* url);
    
private:
    // Wi-Fi and Home Assistant credentials
    const char* _ssid;
    const char* _password;
    const char* _haIP;
    const char* _haToken;
    
    // Display and mode settings
    uint8_t _displayType;
    uint8_t _i2cAddr;
    uint8_t _mode;
    
    // Wi-Fi management flag
    bool _manageWiFi;
    
    // Current weather state
    uint8_t _currentWeather;
    
    // Custom weather entity ID
    const char* _weatherEntityID;
    
    // Cooldown for weather data fetching
    unsigned long _lastFetchTime;
    unsigned long _fetchCooldown; // in milliseconds
    
    // Animation data structure
    struct Animation {
        const uint8_t** frames;
        uint8_t frameCount;
        uint16_t frameDelay;
    
    // Set animation based on Home Assistant weather condition
    bool setAnimationFromHACondition(const char* condition, bool isDaytime);
};
    Animation _animations[5]; // For 5 weather conditions
    
    // Online animation sources
    const char* _onlineAnimationURLs[5]; // URLs for online animation data
    
    // Structures for online animation data caching
    struct OnlineAnimation {
        uint8_t* imageData;
        size_t dataSize;
        bool isLoaded;
    };
    OnlineAnimation _onlineAnimationCache[5]; // Cache for online animations
    
    // Internal methods
    bool connectToWiFi();
    bool fetchWeatherData();
    void displayAnimation();
    void initDisplay();
    bool fetchOnlineAnimation(uint8_t weatherCondition);
    void renderTFTAnimation(uint8_t weatherCondition);
};

#endif // WEATHER_ANIMATIONS_H 