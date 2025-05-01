#ifndef WEATHER_ANIMATIONS_H
#define WEATHER_ANIMATIONS_H

// Define ARDUINO to be > 100 to use Arduino.h instead of WProgram.h
#ifndef ARDUINO
#define ARDUINO 10800 // Arduino 1.8.0 or higher
#endif

// Arduino core must be included first
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// Define Serial macros to allow disabling debug output
#ifndef WA_DISABLE_SERIAL
	#define WA_SERIAL_PRINT(x) Serial.print(x)
	#define WA_SERIAL_PRINTLN(x) Serial.println(x)
	#define WA_SERIAL_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
#else
	#define WA_SERIAL_PRINT(x)
	#define WA_SERIAL_PRINTLN(x)
	#define WA_SERIAL_PRINTF(fmt, ...)
#endif

// Include platform-specific libraries for ESP32 only
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>
#include <TFT_eSPI.h>

// Define display types
#define OLED_SH1106 1
#define TFT_DISPLAY 2

// Define operation modes
#define SIMPLE_TRANSITION 1
#define CONTINUOUS_WEATHER 0

// Define transition directions
#define TRANSITION_LEFT_TO_RIGHT 0
#define TRANSITION_RIGHT_TO_LEFT 1
#define TRANSITION_TOP_TO_BOTTOM 2
#define TRANSITION_BOTTOM_TO_TOP 3
#define TRANSITION_FADE 4

// Define animation modes
#define ANIMATION_STATIC 0
#define ANIMATION_EMBEDDED 1
#define ANIMATION_ONLINE 2

// Weather condition codes (simplified for demonstration)
#define WEATHER_CLEAR 0
#define WEATHER_CLOUDY 1
#define WEATHER_RAIN 2
#define WEATHER_SNOW 3
#define WEATHER_STORM 4

    class WeatherAnimations {
public:
    // Constructor with Wi-Fi and Home Assistant credentials
    WeatherAnimations(const char* ssid, const char* password, const char* haIP, const char* haToken);
    
    // Destructor to clean up resources
    ~WeatherAnimations();
    
    // Initialize the library and connect to Wi-Fi and display
    void begin(uint8_t displayType = OLED_SH1106, uint8_t i2cAddr = 0x3C, bool manageWiFi = true);
    
    // Set the operation mode
    void setMode(uint8_t mode);
    
    // Set animation mode (static, embedded animated, or online animated)
    void setAnimationMode(uint8_t animationMode);
    
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
    
    // Run a transition animation between screens
    // Returns true when animation is complete
    bool runTransition(uint8_t weatherCondition, uint8_t direction, uint16_t duration = 1000);
    
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
    uint8_t _animationMode;
    
    // Wi-Fi management flag
    bool _manageWiFi;
    
    // Current weather state
    uint8_t _currentWeather;
    
    // Custom weather entity ID
    const char* _weatherEntityID;
    
    // Cooldown for weather data fetching
    unsigned long _lastFetchTime;
    unsigned long _fetchCooldown; // in milliseconds
    
    // Transition animation state
    uint8_t _transitionDirection;
    unsigned long _transitionStartTime;
    unsigned long _transitionDuration;
    bool _isTransitioning;
    
    // Animation timing
    unsigned long _lastFrameTime;
    uint8_t _currentFrame;
    
    // Animation data structure
    struct Animation {
        const uint8_t** frames;
        uint8_t frameCount;
        uint16_t frameDelay;
    };
    Animation _animations[5]; // For 5 weather conditions
    
    // Online animation sources
    const char* _onlineAnimationURLs[5]; // URLs for online animation data
    
    // Structures for online animation data caching
    struct OnlineAnimation {
        uint8_t* imageData;
        size_t dataSize;
        bool isLoaded;
        bool isAnimated;      // Is this an animated GIF?
        uint8_t frameCount;   // Number of frames if animated
        uint16_t frameDelay;  // Delay between frames if animated
        uint8_t* frameData[10]; // Up to 10 frames for GIFs (pointers to frame data)
    };
    OnlineAnimation _onlineAnimationCache[5]; // Cache for online animations
    
    // Internal methods
    bool connectToWiFi();
    bool fetchWeatherData();
    void displayAnimation();
    void initDisplay();
    bool fetchOnlineAnimation(uint8_t weatherCondition);
    void renderTFTAnimation(uint8_t weatherCondition);
    void displayTransitionFrame(uint8_t weatherCondition, float progress);
    void displayTextFallback(uint8_t weatherCondition);
    const char* getWeatherText(uint8_t weatherCondition);
    
    // Set animation based on Home Assistant weather condition
    bool setAnimationFromHACondition(const char* condition, bool isDaytime);
    
    // Load animated GIF for TFT display
    bool loadAnimatedGif(uint8_t weatherCondition, const char* url);
    
    // Parse GIF data to extract frames
    bool parseGifFrames(uint8_t weatherCondition);
};

#endif // WEATHER_ANIMATIONS_H 