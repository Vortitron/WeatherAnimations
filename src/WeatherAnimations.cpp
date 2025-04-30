#include "WeatherAnimations.h"
#include <Arduino.h>

#if !defined(ESP32)
    #define ESP32
#endif

// Include platform-specific libraries
#if defined(ESP8266)
	#include <ESP8266WiFi.h>
	#include <ESP8266HTTPClient.h>
	#include <time.h>
    #include <TFT_eSPI.h>
#elif defined(ESP32)
	#include <WiFi.h>
	#include <HttpClient.h>
	#include <time.h>
    #include <TFT_eSPI.h>
#endif

#include "WeatherAnimationsAnimations.h"
#include <Adafruit_SSD1306.h>

// Default OLED dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Default TFT dimensions (can be adjusted based on your specific display)
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

// Global display objects (will be initialized based on display type)
Adafruit_SSD1306* oledDisplay = nullptr;
#if defined(ESP8266) || defined(ESP32)
TFT_eSPI* tftDisplay = nullptr;
#endif

WeatherAnimations::WeatherAnimations(const char* ssid, const char* password, const char* haIP, const char* haToken)
    : _ssid(ssid), _password(password), _haIP(haIP), _haToken(haToken),
      _displayType(OLED_DISPLAY), _i2cAddr(0x3C), _mode(CONTINUOUS_WEATHER),
      _manageWiFi(true), _currentWeather(WEATHER_CLEAR), _weatherEntityID("weather.forecast"),
      _lastFetchTime(0), _fetchCooldown(300000), _isTransitioning(false),
      _lastFrameTime(0), _currentFrame(0), _animationMode(ANIMATION_STATIC) { // 5 minutes cooldown
    
    // Initialize animations array
    for (int i = 0; i < 5; i++) {
        _animations[i].frames = nullptr;
        _animations[i].frameCount = 0;
        _animations[i].frameDelay = 0;
        _onlineAnimationURLs[i] = nullptr;
        _onlineAnimationCache[i].imageData = nullptr;
        _onlineAnimationCache[i].dataSize = 0;
        _onlineAnimationCache[i].isLoaded = false;
        _onlineAnimationCache[i].isAnimated = false;
        _onlineAnimationCache[i].frameCount = 0;
        _onlineAnimationCache[i].frameDelay = 0;
        for (int j = 0; j < 10; j++) {
            _onlineAnimationCache[i].frameData[j] = nullptr;
        }
    }
    // Set default animations for OLED (monochrome)
    setAnimation(WEATHER_CLEAR, clearSkyFrames, 2, 500);
    setAnimation(WEATHER_CLOUDY, cloudySkyFrames, 2, 500);
    setAnimation(WEATHER_RAIN, rainFrames, 3, 300);
    setAnimation(WEATHER_SNOW, snowFrames, 3, 300);
    setAnimation(WEATHER_STORM, stormFrames, 2, 200);
}

void WeatherAnimations::begin(uint8_t displayType, uint8_t i2cAddr, bool manageWiFi) {
    _displayType = displayType;
    _i2cAddr = i2cAddr;
    _manageWiFi = manageWiFi;
    
    // Initialize display based on type
    initDisplay();
    
    // Connect to Wi-Fi only if we are managing it and not already connected
    if (_manageWiFi && WiFi.status() != WL_CONNECTED) {
        if (!connectToWiFi()) {
            // Log error (for now, just a serial print)
            WA_SERIAL_PRINTLN("Failed to connect to Wi-Fi");
        }
    } else if (!_manageWiFi) {
        WA_SERIAL_PRINTLN("Wi-Fi management disabled, assuming connection is handled externally.");
    }
    
    // If we're using online animations and we're connected to Wi-Fi, preload the icons
    if (_animationMode == ANIMATION_ONLINE && WiFi.status() == WL_CONNECTED) {
        WA_SERIAL_PRINTLN("Preloading weather icons...");
        preloadWeatherIcons();
    }
}

void WeatherAnimations::setMode(uint8_t mode) {
    if (mode == SIMPLE_TRANSITION || mode == CONTINUOUS_WEATHER) {
        _mode = mode;
    }
}

void WeatherAnimations::setAnimationMode(uint8_t animationMode) {
    if (animationMode == ANIMATION_STATIC || 
        animationMode == ANIMATION_EMBEDDED || 
        animationMode == ANIMATION_ONLINE) {
        _animationMode = animationMode;
    }
}

void WeatherAnimations::update() {
    // Fetch new weather data if connected and cooldown period has passed
    if (WiFi.status() == WL_CONNECTED) {
        unsigned long currentTime = millis();
        if (currentTime - _lastFetchTime >= _fetchCooldown) {
            if (fetchWeatherData()) {
                _lastFetchTime = currentTime;
            }
        }
    }
    
    // Display animation based on current weather and mode
    displayAnimation();
}

uint8_t WeatherAnimations::getCurrentWeather() const {
    return _currentWeather;
}

void WeatherAnimations::setAnimation(uint8_t weatherCondition, const uint8_t* frames[], uint8_t frameCount, uint16_t frameDelay) {
    if (weatherCondition < 5) { // Only support defined weather conditions
        _animations[weatherCondition].frames = frames;
        _animations[weatherCondition].frameCount = frameCount;
        _animations[weatherCondition].frameDelay = frameDelay;
    }
}

void WeatherAnimations::setWeatherEntity(const char* entityID) {
    _weatherEntityID = entityID;
}

void WeatherAnimations::setOnlineAnimationSource(uint8_t weatherCondition, const char* url) {
    if (weatherCondition < 5) {
        _onlineAnimationURLs[weatherCondition] = url;
        // Reset cache status for this condition
        if (_onlineAnimationCache[weatherCondition].imageData != nullptr) {
            free(_onlineAnimationCache[weatherCondition].imageData);
            _onlineAnimationCache[weatherCondition].imageData = nullptr;
        }
        _onlineAnimationCache[weatherCondition].dataSize = 0;
        _onlineAnimationCache[weatherCondition].isLoaded = false;
    }
}

bool WeatherAnimations::connectToWiFi() {
    if (!_manageWiFi) {
        return WiFi.status() == WL_CONNECTED;
    }
    WiFi.begin(_ssid, _password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(1000);
        WA_SERIAL_PRINT(".");
        attempts++;
    }
    return WiFi.status() == WL_CONNECTED;
}

bool WeatherAnimations::fetchWeatherData() {
    if (WiFi.status() != WL_CONNECTED) {
        if (_manageWiFi) {
            connectToWiFi();
        }
        if (WiFi.status() != WL_CONNECTED) {
            WA_SERIAL_PRINTLN("No Wi-Fi connection available.");
            return false;
        }
    }
    
    HTTPClient http;
    String url = String("http://") + _haIP + ":8123/api/states/" + _weatherEntityID;
    http.begin(url);
    http.addHeader("Authorization", String("Bearer ") + _haToken);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        WA_SERIAL_PRINTLN("Home Assistant Response:");
        WA_SERIAL_PRINTLN(payload);
        
        // Parse JSON (extended parsing)
        String condition = "";
        bool isDaytime = true;
        
        // Extract condition from JSON (simplistic parsing)
        int stateStart = payload.indexOf("\"state\":\"") + 9;
        if (stateStart > 9) {
            int stateEnd = payload.indexOf("\",", stateStart);
            if (stateEnd > stateStart) {
                condition = payload.substring(stateStart, stateEnd);
            }
        }
        
        // Check for daytime attribute (if available)
        bool isDayFound = false;
        int isDayStart = payload.indexOf("\"is_daytime\":") + 13;
        if (isDayStart > 13) {
            // Could be true or false
            if (payload.substring(isDayStart, isDayStart + 4) == "true") {
                isDaytime = true;
                isDayFound = true;
            } else if (payload.substring(isDayStart, isDayStart + 5) == "false") {
                isDaytime = false;
                isDayFound = true;
            }
        }
        
        // If no daytime attribute, guess based on time
        if (!isDayFound) {
            // Simple heuristic: 6 AM to 6 PM is daytime
            time_t now;
            time(&now);
            struct tm *timeinfo = localtime(&now);
            isDaytime = (timeinfo->tm_hour >= 6 && timeinfo->tm_hour < 18);
        }
        
        // If condition is empty or invalid, try to detect from payload text
        if (condition.length() == 0) {
            if (payload.indexOf("clear") != -1 || payload.indexOf("sunny") != -1) {
                condition = payload.indexOf("night") != -1 ? "clear-night" : "sunny";
            } else if (payload.indexOf("cloud") != -1) {
                condition = payload.indexOf("partly") != -1 ? "partlycloudy" : "cloudy";
            } else if (payload.indexOf("fog") != -1) {
                condition = "fog";
            } else if (payload.indexOf("hail") != -1) {
                condition = "hail";
            } else if (payload.indexOf("lightning") != -1 || payload.indexOf("thunder") != -1) {
                condition = payload.indexOf("rain") != -1 ? "lightning-rainy" : "lightning";
            } else if (payload.indexOf("pouring") != -1) {
                condition = "pouring";
            } else if (payload.indexOf("rain") != -1 || payload.indexOf("drizzle") != -1) {
                condition = "rainy";
            } else if (payload.indexOf("snow") != -1) {
                condition = payload.indexOf("rain") != -1 ? "snowy-rainy" : "snowy";
            } else if (payload.indexOf("wind") != -1) {
                condition = payload.indexOf("extreme") != -1 ? "windy-variant" : "windy";
            } else {
                condition = "cloudy"; // Default
            }
        }
        
        WA_SERIAL_PRINT("Detected weather condition: ");
        WA_SERIAL_PRINTLN(condition);
        WA_SERIAL_PRINT("Is daytime: ");
        WA_SERIAL_PRINTLN(isDaytime ? "Yes" : "No");
        
        // Set animation based on the parsed condition
        setAnimationFromHACondition(condition.c_str(), isDaytime);
        
        http.end();
        return true;
    } else {
        WA_SERIAL_PRINT("HTTP Error: ");
        WA_SERIAL_PRINTLN(httpCode);
        http.end();
        return false;
    }
}

void WeatherAnimations::displayAnimation() {
    // If currently in transition mode, handle that instead of normal display
    if (_isTransitioning) {
        unsigned long currentTime = millis();
        unsigned long elapsedTime = currentTime - _transitionStartTime;
        
        // Calculate progress (0.0 to 1.0)
        float progress = min(1.0f, (float)elapsedTime / _transitionDuration);
        
        // Display the transition frame
        displayTransitionFrame(_currentWeather, progress);
        
        // End transition when complete
        if (progress >= 1.0f) {
            _isTransitioning = false;
        }
        
        return;
    }
    
    // Regular animation display based on animation mode
    if (_displayType == OLED_DISPLAY && oledDisplay != nullptr) {
        oledDisplay->clearDisplay();
        
        // Check if animation is set for current weather
        if (_animations[_currentWeather].frameCount > 0) {
            unsigned long currentTime = millis();
            
            // For embedded animations, we need to handle frame timing
            if (_animationMode == ANIMATION_EMBEDDED || _animationMode == ANIMATION_STATIC) {
                if (currentTime - _lastFrameTime >= _animations[_currentWeather].frameDelay) {
                    // For static mode, only use the first frame
                    if (_animationMode == ANIMATION_STATIC) {
                        _currentFrame = 0;
                    } else {
                        // For embedded animations, cycle through frames
                        _currentFrame = (_currentFrame + 1) % _animations[_currentWeather].frameCount;
                    }
                    _lastFrameTime = currentTime;
                }
                
                // Draw the current frame
                oledDisplay->drawBitmap(0, 0, _animations[_currentWeather].frames[_currentFrame], 
                                       SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
                
                // In simple transition mode, stop after one cycle
                if (_mode == SIMPLE_TRANSITION && _currentFrame == 0 && 
                    _animationMode == ANIMATION_EMBEDDED) {
                    oledDisplay->clearDisplay();
                }
            }
        } else {
            // Default text display if no animation is set
            oledDisplay->setTextSize(1);
            oledDisplay->setTextColor(WHITE);
            oledDisplay->setCursor(0, 0);
            switch (_currentWeather) {
                case WEATHER_CLEAR:   oledDisplay->println("Clear Sky"); break;
                case WEATHER_CLOUDY:  oledDisplay->println("Cloudy"); break;
                case WEATHER_RAIN:    oledDisplay->println("Rainy"); break;
                case WEATHER_SNOW:    oledDisplay->println("Snowy"); break;
                case WEATHER_STORM:   oledDisplay->println("Stormy"); break;
                default:              oledDisplay->println("Unknown");
            }
        }
        oledDisplay->display();
    } else if (_displayType == TFT_DISPLAY) {
        #if defined(ESP8266) || defined(ESP32)
        if (tftDisplay != nullptr) {
            // For TFT display, we can handle animated GIFs if in online animation mode
            if (_animationMode == ANIMATION_ONLINE && 
                _onlineAnimationCache[_currentWeather].isLoaded &&
                _onlineAnimationCache[_currentWeather].isAnimated) {
                
                unsigned long currentTime = millis();
                
                // Update the frame based on timing
                if (currentTime - _lastFrameTime >= _onlineAnimationCache[_currentWeather].frameDelay) {
                    _currentFrame = (_currentFrame + 1) % _onlineAnimationCache[_currentWeather].frameCount;
                    _lastFrameTime = currentTime;
                    
                    // Clear the screen once per cycle for clean animation
                    if (_currentFrame == 0) {
                        tftDisplay->fillScreen(TFT_BLACK);
                    }
                    
                    // Display the current frame
                    // This is a simplified approach - in a real implementation, you would decode
                    // the GIF frames and display them properly
                    if (_onlineAnimationCache[_currentWeather].frameData[_currentFrame] != nullptr) {
                        // Example of drawing the frame (implementation would depend on the format)
                        // For demonstration purposes, we're just showing a placeholder
                        tftDisplay->setCursor(10, 10);
                        tftDisplay->setTextColor(TFT_WHITE);
                        tftDisplay->setTextSize(2);
                        
                        switch (_currentWeather) {
                            case WEATHER_CLEAR:   
                                tftDisplay->println("Clear Sky");
                                tftDisplay->fillCircle(120, 160, 40, TFT_YELLOW); 
                                break;
                            case WEATHER_CLOUDY:  
                                tftDisplay->println("Cloudy");
                                tftDisplay->fillRoundRect(80, 140, 100, 40, 20, TFT_WHITE);
                                break;
                            case WEATHER_RAIN:    
                                tftDisplay->println("Rainy");
                                tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_LIGHTGREY);
                                for (int i = 0; i < 10; i++) {
                                    int offset = (_currentFrame * 5) % 40; // Moving rain effect
                                    tftDisplay->drawLine(90 + i*10, 170 + offset, 
                                                       90 + i*10 + 5, 190 + offset, TFT_BLUE);
                                }
                                break;
                            case WEATHER_SNOW:    
                                tftDisplay->println("Snowy");
                                tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_LIGHTGREY);
                                for (int i = 0; i < 10; i++) {
                                    int offset = (_currentFrame * 3) % 30; // Falling snow effect
                                    tftDisplay->drawPixel(90 + i*10, 180 + offset, TFT_WHITE);
                                    tftDisplay->drawPixel(90 + i*10 + 1, 180 + offset, TFT_WHITE);
                                    tftDisplay->drawPixel(90 + i*10, 180 + offset + 1, TFT_WHITE);
                                    tftDisplay->drawPixel(90 + i*10 + 1, 180 + offset + 1, TFT_WHITE);
                                }
                                break;
                            case WEATHER_STORM:   
                                tftDisplay->println("Stormy");
                                tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_DARKGREY);
                                if (_currentFrame % 3 == 0) {
                                    // Flash lightning effect every few frames
                                    tftDisplay->fillTriangle(120, 170, 130, 200, 110, 190, TFT_YELLOW);
                                }
                                break;
                            default:              
                                tftDisplay->println("Unknown");
                        }
                    }
                }
            } else {
                // Fallback to basic text display if no animation data is available
                tftDisplay->fillScreen(TFT_BLACK);
                tftDisplay->setCursor(10, 10);
                tftDisplay->setTextColor(TFT_WHITE);
                tftDisplay->setTextSize(2);
                
                switch (_currentWeather) {
                    case WEATHER_CLEAR:
                        tftDisplay->println("Clear Sky");
                        tftDisplay->fillCircle(120, 160, 40, TFT_YELLOW);
                        break;
                    case WEATHER_CLOUDY:
                        tftDisplay->println("Cloudy");
                        tftDisplay->fillRoundRect(80, 140, 100, 40, 20, TFT_WHITE);
                        break;
                    case WEATHER_RAIN:
                        tftDisplay->println("Rainy");
                        tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_LIGHTGREY);
                        for (int i = 0; i < 10; i++) {
                            tftDisplay->drawLine(90 + i*10, 170, 90 + i*10 + 5, 190, TFT_BLUE);
                        }
                        break;
                    case WEATHER_SNOW:
                        tftDisplay->println("Snowy");
                        tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_LIGHTGREY);
                        for (int i = 0; i < 10; i++) {
                            tftDisplay->drawPixel(90 + i*10, 180, TFT_WHITE);
                            tftDisplay->drawPixel(90 + i*10 + 1, 180, TFT_WHITE);
                            tftDisplay->drawPixel(90 + i*10, 180 + 1, TFT_WHITE);
                            tftDisplay->drawPixel(90 + i*10 + 1, 180 + 1, TFT_WHITE);
                        }
                        break;
                    case WEATHER_STORM:
                        tftDisplay->println("Stormy");
                        tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_DARKGREY);
                        tftDisplay->fillTriangle(120, 170, 130, 200, 110, 190, TFT_YELLOW);
                        break;
                    default:
                        tftDisplay->println("Unknown");
                }
            }
        }
        #endif
    }
}

bool WeatherAnimations::fetchOnlineAnimation(uint8_t weatherCondition) {
    if (_onlineAnimationURLs[weatherCondition] == nullptr || WiFi.status() != WL_CONNECTED) {
        return false;
    }
    
    WA_SERIAL_PRINT("Fetching online animation for condition: ");
    WA_SERIAL_PRINTLN(weatherCondition);
    
    const char* url = _onlineAnimationURLs[weatherCondition];
    
    // Check if the URL is for a GIF file (animated)
    bool isGif = false;
    if (strstr(url, ".gif") != nullptr || strstr(url, ".GIF") != nullptr) {
        isGif = true;
    }
    
    if (isGif && _animationMode == ANIMATION_ONLINE) {
        // For GIFs, use the specialized loader
        return loadAnimatedGif(weatherCondition, url);
    }
    
    // For static images, use the original method
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        // Get the data size and allocate memory
        int dataSize = http.getSize();
        
        if (dataSize > 0) {
            // Free previous memory if any
            if (_onlineAnimationCache[weatherCondition].imageData != nullptr) {
                free(_onlineAnimationCache[weatherCondition].imageData);
            }
            
            // Allocate memory for the new data
            _onlineAnimationCache[weatherCondition].imageData = (uint8_t*)malloc(dataSize);
            
            if (_onlineAnimationCache[weatherCondition].imageData) {
                // Get the data
                WiFiClient* stream = http.getStreamPtr();
                size_t size = stream->available();
                if (size) {
                    int bytesRead = stream->readBytes(_onlineAnimationCache[weatherCondition].imageData, size);
                    _onlineAnimationCache[weatherCondition].dataSize = bytesRead;
                    _onlineAnimationCache[weatherCondition].isLoaded = true;
                    _onlineAnimationCache[weatherCondition].isAnimated = false;
                    WA_SERIAL_PRINTLN("Online animation data loaded successfully.");
                }
            } else {
                WA_SERIAL_PRINTLN("Failed to allocate memory for animation data.");
            }
        }
        
        http.end();
        return _onlineAnimationCache[weatherCondition].isLoaded;
    } else {
        WA_SERIAL_PRINT("Failed to fetch online animation, HTTP code: ");
        WA_SERIAL_PRINTLN(httpCode);
        http.end();
        return false;
    }
}

bool WeatherAnimations::loadAnimatedGif(uint8_t weatherCondition, const char* url) {
    // This is a simplified approach for demonstration
    // In a real-world implementation, you would use a GIF decoder library
    
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        // Get the data size and allocate memory
        int dataSize = http.getSize();
        
        if (dataSize > 0) {
            // Free previous memory if any
            if (_onlineAnimationCache[weatherCondition].imageData != nullptr) {
                free(_onlineAnimationCache[weatherCondition].imageData);
                
                // Free any frame data
                for (int i = 0; i < 10; i++) {
                    if (_onlineAnimationCache[weatherCondition].frameData[i] != nullptr) {
                        free(_onlineAnimationCache[weatherCondition].frameData[i]);
                        _onlineAnimationCache[weatherCondition].frameData[i] = nullptr;
                    }
                }
            }
            
            // Allocate memory for the new data
            _onlineAnimationCache[weatherCondition].imageData = (uint8_t*)malloc(dataSize);
            
            if (_onlineAnimationCache[weatherCondition].imageData) {
                // Get the data
                WiFiClient* stream = http.getStreamPtr();
                size_t size = stream->available();
                if (size) {
                    int bytesRead = stream->readBytes(_onlineAnimationCache[weatherCondition].imageData, size);
                    _onlineAnimationCache[weatherCondition].dataSize = bytesRead;
                    _onlineAnimationCache[weatherCondition].isLoaded = true;
                    _onlineAnimationCache[weatherCondition].isAnimated = true;
                    
                    // Parse the GIF to extract frames
                    if (parseGifFrames(weatherCondition)) {
                        WA_SERIAL_PRINTLN("Animated GIF loaded and parsed successfully.");
                        http.end();
                        return true;
                    } else {
                        WA_SERIAL_PRINTLN("Failed to parse GIF frames.");
                    }
                }
            } else {
                WA_SERIAL_PRINTLN("Failed to allocate memory for GIF data.");
            }
        }
        
        http.end();
        return false;
    } else {
        WA_SERIAL_PRINT("Failed to fetch animated GIF, HTTP code: ");
        WA_SERIAL_PRINTLN(httpCode);
        http.end();
        return false;
    }
}

bool WeatherAnimations::parseGifFrames(uint8_t weatherCondition) {
    // This is a placeholder for GIF parsing logic
    // In a real implementation, you would use a library like AnimatedGIF
    // or implement GIF parsing directly
    
    // For demonstration, we'll create simulated frames
    const int simulatedFrameCount = 4; // Simulate 4 frames
    _onlineAnimationCache[weatherCondition].frameCount = simulatedFrameCount;
    _onlineAnimationCache[weatherCondition].frameDelay = 250; // 250ms between frames
    
    for (int i = 0; i < simulatedFrameCount; i++) {
        // Allocate memory for each frame (just for demonstration)
        _onlineAnimationCache[weatherCondition].frameData[i] = 
            (uint8_t*)malloc(TFT_WIDTH * TFT_HEIGHT * 2); // 2 bytes per pixel for RGB565
        
        if (_onlineAnimationCache[weatherCondition].frameData[i] == nullptr) {
            WA_SERIAL_PRINTLN("Failed to allocate memory for GIF frame.");
            return false;
        }
        
        // In a real implementation, you would fill this with actual frame data
        // For now, we'll just set it as "present" to trigger the animation display
    }
    
    return true;
}

void WeatherAnimations::initDisplay() {
    if (_displayType == OLED_DISPLAY) {
        oledDisplay = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
        if (!oledDisplay->begin(SSD1306_SWITCHCAPVCC, _i2cAddr)) {
            WA_SERIAL_PRINTLN(F("SSD1306 allocation failed"));
            delete oledDisplay;
            oledDisplay = nullptr;
        }
    } else if (_displayType == TFT_DISPLAY) {
        #if defined(ESP8266) || defined(ESP32)
        tftDisplay = new TFT_eSPI();
        tftDisplay->init();
        tftDisplay->fillScreen(TFT_BLACK);
        tftDisplay->setRotation(0);
        WA_SERIAL_PRINTLN("TFT display initialized.");
        #else
        WA_SERIAL_PRINTLN("TFT display not supported on this platform.");
        #endif
    }
}

bool WeatherAnimations::setAnimationFromHACondition(const char* condition, bool isDaytime) {
    // Find the appropriate icon based on condition and time of day
    const IconMapping* icon = findWeatherIcon(condition, isDaytime);
    if (icon == nullptr) {
        WA_SERIAL_PRINTLN("Warning: Could not find icon for condition");
        return false;
    }
    
    uint8_t weatherCode;
    
    // Map condition to weather code used in the library
    if (strcmp(condition, "clear-night") == 0 || strcmp(condition, "sunny") == 0) {
        weatherCode = WEATHER_CLEAR;
    } else if (strcmp(condition, "cloudy") == 0 || strcmp(condition, "partlycloudy") == 0) {
        weatherCode = WEATHER_CLOUDY;
    } else if (strcmp(condition, "rainy") == 0 || strcmp(condition, "pouring") == 0) {
        weatherCode = WEATHER_RAIN;
    } else if (strcmp(condition, "snowy") == 0 || strcmp(condition, "snowy-rainy") == 0) {
        weatherCode = WEATHER_SNOW;
    } else if (strcmp(condition, "lightning") == 0 || strcmp(condition, "lightning-rainy") == 0) {
        weatherCode = WEATHER_STORM;
    } else {
        // Default fallback
        weatherCode = WEATHER_CLOUDY;
    }
    
    // For OLED display, use embedded animations
    if (_displayType == OLED_DISPLAY) {
        switch (weatherCode) {
            case WEATHER_CLEAR:
                setAnimation(weatherCode, clearSkyFrames, 2, 500);
                break;
            case WEATHER_CLOUDY:
                setAnimation(weatherCode, cloudySkyFrames, 2, 500);
                break;
            case WEATHER_RAIN:
                setAnimation(weatherCode, rainFrames, 3, 300);
                break;
            case WEATHER_SNOW:
                setAnimation(weatherCode, snowFrames, 3, 300);
                break;
            case WEATHER_STORM:
                setAnimation(weatherCode, stormFrames, 2, 200);
                break;
        }
    }
    
    // For TFT display or if using online animation mode, set URL to fetch the icon online
    if (_displayType == TFT_DISPLAY || _animationMode == ANIMATION_ONLINE) {
        // Load the icon if not already loaded
        if (!icon->isLoaded) {
            loadWeatherIcon((IconMapping*)icon);
        }
        
        // Generate URL based on the condition and variant for online animations
        char url[150];
        sprintf(url, "https://raw.githubusercontent.com/basmilius/weather-icons/master/production/fill/%s%s%s.png", 
                condition,
                (icon->variant[0] != '\0' ? "-" : ""),
                icon->variant);
        
        setOnlineAnimationSource(weatherCode, url);
    }
    
    // Update current weather
    _currentWeather = weatherCode;
    
    return true;
}

// New method: Run a transition animation between screens
bool WeatherAnimations::runTransition(uint8_t weatherCondition, uint8_t direction, uint16_t duration) {
    // If already transitioning, check if it's complete
    if (_isTransitioning) {
        // Update the display (this will progress the transition)
        displayAnimation();
        
        // Return true if transition is complete
        return !_isTransitioning;
    }
    
    // Start a new transition
    _currentWeather = weatherCondition;
    _transitionDirection = direction;
    _transitionStartTime = millis();
    _transitionDuration = duration;
    _isTransitioning = true;
    
    // Update the display to show the first frame
    displayAnimation();
    
    // Return false since the transition just started
    return false;
}

// New method: Display a single frame of the transition animation
void WeatherAnimations::displayTransitionFrame(uint8_t weatherCondition, float progress) {
    if (_displayType == OLED_DISPLAY && oledDisplay != nullptr) {
        // Clear the display
        oledDisplay->clearDisplay();
        
        // Check if animation is available
        if (_animations[weatherCondition].frameCount > 0) {
            // For simplicity, use the first frame of the animation
            const uint8_t* frameData = _animations[weatherCondition].frames[0];
            
            // Get display dimensions
            int16_t width = SCREEN_WIDTH;
            int16_t height = SCREEN_HEIGHT;
            
            // Calculate display position based on transition direction and progress
            int16_t x = 0;
            int16_t y = 0;
            
            switch (_transitionDirection) {
                case TRANSITION_RIGHT_TO_LEFT:
                    // Start from right (off-screen) and move left
                    x = width * (1.0f - progress);
                    break;
                    
                case TRANSITION_LEFT_TO_RIGHT:
                    // Start from left (off-screen) and move right
                    x = -width * (1.0f - progress);
                    break;
                    
                case TRANSITION_TOP_TO_BOTTOM:
                    // Start from top (off-screen) and move down
                    y = -height * (1.0f - progress);
                    break;
                    
                case TRANSITION_BOTTOM_TO_TOP:
                    // Start from bottom (off-screen) and move up
                    y = height * (1.0f - progress);
                    break;
                    
                case TRANSITION_FADE:
                    // Simple approximation of fading by drawing only some pixels
                    // This is imperfect but gives a fade-like effect
                    oledDisplay->drawBitmap(0, 0, frameData, width, height, WHITE);
                    
                    // Draw black rectangles that diminish as progress increases
                    for (int16_t i = 0; i < height; i += 2) {
                        int16_t barHeight = max((int16_t)1, (int16_t)(2 * (1.0f - progress)));
                        if (i % 4 < barHeight) {
                            oledDisplay->fillRect(0, i, width, 1, BLACK);
                        }
                    }
                    break;
            }
            
            // Draw the bitmap at the calculated position
            if (_transitionDirection != TRANSITION_FADE) {
                oledDisplay->drawBitmap(x, y, frameData, width, height, WHITE);
            }
        } else {
            // Default text display if no animation is set
            oledDisplay->setTextSize(1);
            oledDisplay->setTextColor(WHITE);
            oledDisplay->setCursor(0, 0);
            switch (weatherCondition) {
                case WEATHER_CLEAR:   oledDisplay->println("Clear Sky"); break;
                case WEATHER_CLOUDY:  oledDisplay->println("Cloudy"); break;
                case WEATHER_RAIN:    oledDisplay->println("Rainy"); break;
                case WEATHER_SNOW:    oledDisplay->println("Snowy"); break;
                case WEATHER_STORM:   oledDisplay->println("Stormy"); break;
                default:              oledDisplay->println("Unknown");
            }
        }
        oledDisplay->display();
    } 
    #if defined(ESP8266) || defined(ESP32)
    else if (_displayType == TFT_DISPLAY && tftDisplay != nullptr) {
        // For TFT display, we'll implement a simple transition
        // This is a basic implementation - you can enhance it with more complex animations
        
        // Clear the display on the first frame
        if (progress == 0.0f) {
            tftDisplay->fillScreen(TFT_BLACK);
        }
        
        // Get display dimensions
        int16_t width = TFT_WIDTH;
        int16_t height = TFT_HEIGHT;
        
        // Calculate display position based on transition direction and progress
        int16_t x = 0;
        int16_t y = 0;
        
        switch (_transitionDirection) {
            case TRANSITION_RIGHT_TO_LEFT:
                x = width * (1.0f - progress);
                break;
            case TRANSITION_LEFT_TO_RIGHT:
                x = -width * (1.0f - progress);
                break;
            case TRANSITION_TOP_TO_BOTTOM:
                y = -height * (1.0f - progress);
                break;
            case TRANSITION_BOTTOM_TO_TOP:
                y = height * (1.0f - progress);
                break;
            case TRANSITION_FADE:
                // For TFT, we can actually do a proper fade by changing opacity
                // Implement a basic color blend for fade effect
                tftDisplay->fillScreen(TFT_BLACK);
                
                // The more progress, the more visible the content
                uint8_t opacity = progress * 255;
                
                // Draw the weather icon with fading effect
                tftDisplay->setTextColor(TFT_WHITE, TFT_BLACK);
                tftDisplay->setCursor(10, 10);
                tftDisplay->setTextSize(2);
                
                // Adjust text color based on opacity (simple approximation)
                uint16_t textColor = tftDisplay->color565(opacity, opacity, opacity);
                tftDisplay->setTextColor(textColor);
                
                switch (weatherCondition) {
                    case WEATHER_CLEAR:
                        tftDisplay->println("Clear Sky");
                        tftDisplay->fillCircle(120, 160, 40 * progress, TFT_YELLOW); // Sun growing
                        break;
                    case WEATHER_CLOUDY:
                        tftDisplay->println("Cloudy");
                        tftDisplay->fillRoundRect(80, 140, 100 * progress, 40 * progress, 20, TFT_WHITE);
                        break;
                    // Add cases for other weather conditions
                    default:
                        tftDisplay->println("Weather");
                }
                break;
        }
        
        // Try to use online animation if available
        if (_onlineAnimationURLs[weatherCondition] != nullptr && 
            _onlineAnimationCache[weatherCondition].isLoaded) {
            
            // If we have the animation data, we could render it with transition effects
            // This is a placeholder - actual implementation depends on image format
            
            // Display a basic placeholder with transition
            tftDisplay->fillScreen(TFT_BLACK);
            tftDisplay->setCursor(x + 10, y + 10);
            tftDisplay->setTextColor(TFT_WHITE);
            tftDisplay->setTextSize(2);
            
            switch (weatherCondition) {
                case WEATHER_CLEAR:
                    tftDisplay->println("Clear Sky");
                    tftDisplay->fillCircle(x + 120, y + 160, 40, TFT_YELLOW);
                    break;
                case WEATHER_CLOUDY:
                    tftDisplay->println("Cloudy");
                    tftDisplay->fillRoundRect(x + 80, y + 140, 100, 40, 20, TFT_WHITE);
                    break;
                case WEATHER_RAIN:
                    tftDisplay->println("Rainy");
                    tftDisplay->fillRoundRect(x + 80, y + 120, 100, 40, 20, TFT_LIGHTGREY);
                    for (int i = 0; i < 10; i++) {
                        tftDisplay->drawLine(x + 90 + i*10, y + 170, x + 90 + i*10 + 5, y + 190, TFT_BLUE);
                    }
                    break;
                case WEATHER_SNOW:
                    tftDisplay->println("Snowy");
                    tftDisplay->fillRoundRect(x + 80, y + 120, 100, 40, 20, TFT_LIGHTGREY);
                    for (int i = 0; i < 10; i++) {
                        tftDisplay->drawPixel(x + 90 + i*10, y + 180, TFT_WHITE);
                        tftDisplay->drawPixel(x + 90 + i*10 + 1, y + 180, TFT_WHITE);
                        tftDisplay->drawPixel(x + 90 + i*10, y + 180 + 1, TFT_WHITE);
                        tftDisplay->drawPixel(x + 90 + i*10 + 1, y + 180 + 1, TFT_WHITE);
                    }
                    break;
                case WEATHER_STORM:
                    tftDisplay->println("Stormy");
                    tftDisplay->fillRoundRect(x + 80, y + 120, 100, 40, 20, TFT_DARKGREY);
                    tftDisplay->fillTriangle(x + 120, y + 170, x + 130, y + 200, x + 110, y + 190, TFT_YELLOW);
                    break;
                default:
                    tftDisplay->println("Unknown");
            }
        } else {
            // Fallback to basic display with transition
            tftDisplay->fillScreen(TFT_BLACK);
            tftDisplay->setCursor(x + 10, y + 10);
            tftDisplay->setTextColor(TFT_WHITE);
            tftDisplay->setTextSize(2);
            
            switch (weatherCondition) {
                case WEATHER_CLEAR:
                    tftDisplay->println("Clear Sky");
                    tftDisplay->fillCircle(x + 120, y + 160, 40, TFT_YELLOW);
                    break;
                // Add cases for other weather types (same as above)
                default:
                    tftDisplay->println("Weather");
            }
        }
    }
    #endif
}

WeatherAnimations::~WeatherAnimations() {
    // Clean up display objects
    if (_displayType == OLED_DISPLAY && oledDisplay != nullptr) {
        delete oledDisplay;
        oledDisplay = nullptr;
    }
    #if defined(ESP8266) || defined(ESP32)
    else if (_displayType == TFT_DISPLAY && tftDisplay != nullptr) {
        delete tftDisplay;
        tftDisplay = nullptr;
    }
    #endif
    
    // Clean up online animation cache
    for (int i = 0; i < 5; i++) {
        if (_onlineAnimationCache[i].imageData != nullptr) {
            free(_onlineAnimationCache[i].imageData);
            _onlineAnimationCache[i].imageData = nullptr;
        }
        for (int j = 0; j < _onlineAnimationCache[i].frameCount && j < 10; j++) {
            if (_onlineAnimationCache[i].frameData[j] != nullptr) {
                free(_onlineAnimationCache[i].frameData[j]);
                _onlineAnimationCache[i].frameData[j] = nullptr;
            }
        }
    }
    
    // Clean up icon data
    clearWeatherIcons();
}

 