#include "WeatherAnimations.h"
#include <Arduino.h>

#if !defined(ESP32)
    #define ESP32
#endif

// Include platform-specific libraries for ESP32 only
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// Only include TFT_eSPI for ESP32/ESP8266 platforms
#if defined(ESP32) || defined(ESP8266)
#include <TFT_eSPI.h>
#endif

#include "WeatherAnimationsAnimations.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Default OLED dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Default TFT dimensions (can be adjusted based on your specific display)
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

// Global display objects (will be initialized based on display type)
Adafruit_SSD1306* oledDisplay = nullptr;

// Only define tftDisplay for ESP32/ESP8266 platforms
#if defined(ESP32) || defined(ESP8266)
TFT_eSPI* tftDisplay = nullptr;
#endif

using namespace WeatherAnimationsLib;

WeatherAnimations::WeatherAnimations(const char* ssid, const char* password, const char* haIP, const char* haToken)
    : _ssid(ssid), _password(password), _haIP(haIP), _haToken(haToken),
      _displayType(OLED_SSD1306), _i2cAddr(0x3C), _mode(CONTINUOUS_WEATHER),
      _manageWiFi(true), _currentWeather(WEATHER_CLEAR), _weatherEntityID("weather.forecast"),
      _indoorTempEntity("sensor.t_h_sensor_temperature"), _outdoorTempEntity("sensor.sam_outside_temperature"),
      _indoorTemp(0), _outdoorTemp(0), _minForecastTemp(0), _maxForecastTemp(0), _hasTemperatureData(false),
      _lastFetchTime(0), _fetchCooldown(300000), _isTransitioning(false),
      _lastFrameTime(0), _currentFrame(0), _animationMode(ANIMATION_ONLINE)
{
    // Zero-initialize animation structure
    for (int i = 0; i < 5; i++) {
        _animations[i].frames = nullptr;
        _animations[i].frameCount = 0;
        _animations[i].frameDelay = 200;
        _onlineAnimationURLs[i] = nullptr;
        _onlineAnimationCache[i].imageData = nullptr;
        _onlineAnimationCache[i].dataSize = 0;
        _onlineAnimationCache[i].isLoaded = false;
        _onlineAnimationCache[i].isAnimated = false;
        _onlineAnimationCache[i].frameCount = 0;
        _onlineAnimationCache[i].frameDelay = 200;
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
        // Try to get animations from online resources first
        if (!initializeAnimationsFromOnline(_displayType)) {
            // If failed, generate fallback animations
            WA_SERIAL_PRINTLN("Failed to load animations from online, using fallbacks");
            generateFallbackAnimations();
        } else {
            WA_SERIAL_PRINTLN("Successfully loaded animations from online resources");
        }
    } else {
        // If not using online animations or not connected, use fallbacks
        generateFallbackAnimations();
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
    WA_SERIAL_PRINTLN("Update loop running.");
    // Fetch new weather data if connected and cooldown period has passed
    if (WiFi.status() == WL_CONNECTED) {
        unsigned long currentTime = millis();
        if (currentTime - _lastFetchTime >= _fetchCooldown) {
            WA_SERIAL_PRINTLN("Attempting to fetch weather data...");
            bool weatherSuccess = fetchWeatherData();
            bool tempSuccess = fetchTemperatureData();
            
            if (weatherSuccess || tempSuccess) {
                _lastFetchTime = currentTime;
                WA_SERIAL_PRINTLN("Weather and/or temperature data fetched successfully.");
            } else {
                WA_SERIAL_PRINTLN("Failed to fetch weather and temperature data.");
            }
        } else {
            WA_SERIAL_PRINTLN("Waiting for cooldown period to fetch new data.");
        }
    } else {
        WA_SERIAL_PRINTLN("WiFi not connected, skipping weather data fetch.");
    }
    
    // Display animation based on current weather and mode
    WA_SERIAL_PRINTLN("Updating display with current weather animation.");
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
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
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
        
        // Extract min/max forecast temperatures
        int forecastTempMinIdx = payload.indexOf("\"forecast_temp_min\":");
        if (forecastTempMinIdx > 0) {
            forecastTempMinIdx += 19; // Length of the search string
            int endIdx = payload.indexOf(",", forecastTempMinIdx);
            if (endIdx < 0) endIdx = payload.indexOf("}", forecastTempMinIdx);
            if (endIdx > forecastTempMinIdx) {
                String minTempStr = payload.substring(forecastTempMinIdx, endIdx);
                _minForecastTemp = minTempStr.toFloat();
                WA_SERIAL_PRINT("Min forecast temp: ");
                WA_SERIAL_PRINTLN(_minForecastTemp);
            }
        }
        
        int forecastTempMaxIdx = payload.indexOf("\"forecast_temp_max\":");
        if (forecastTempMaxIdx > 0) {
            forecastTempMaxIdx += 19; // Length of the search string
            int endIdx = payload.indexOf(",", forecastTempMaxIdx);
            if (endIdx < 0) endIdx = payload.indexOf("}", forecastTempMaxIdx);
            if (endIdx > forecastTempMaxIdx) {
                String maxTempStr = payload.substring(forecastTempMaxIdx, endIdx);
                _maxForecastTemp = maxTempStr.toFloat();
                WA_SERIAL_PRINT("Max forecast temp: ");
                WA_SERIAL_PRINTLN(_maxForecastTemp);
            }
        }
        
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
        
        // Save the previous weather to check if it changed
        uint8_t previousWeather = _currentWeather;
        
        // Set animation based on Home Assistant weather condition
        if (setAnimationFromHACondition(condition.c_str(), isDaytime)) {
            // If the weather changed and we're using online animations, refresh them
            if (_animationMode == ANIMATION_ONLINE && previousWeather != _currentWeather) {
                WA_SERIAL_PRINTLN("Weather changed, refreshing animations");
                // Only reload the animation for the current weather to save bandwidth
                // Use fetchAnimationFrames instead of fetchAnimationData
                if (_onlineAnimationURLs[_currentWeather] != nullptr) {
                    bool fetchSuccess = false;
                    // Use the stored URL as a base URL for frame fetching
                    switch(_currentWeather) {
                        case WEATHER_CLEAR:
                            fetchSuccess = fetchAnimationFrames(_onlineAnimationURLs[_currentWeather], (uint8_t**)clearSkyFrames, 2, 1024);
                            break;
                        case WEATHER_CLOUDY:
                            fetchSuccess = fetchAnimationFrames(_onlineAnimationURLs[_currentWeather], (uint8_t**)cloudySkyFrames, 2, 1024);
                            break;
                        case WEATHER_RAIN:
                            fetchSuccess = fetchAnimationFrames(_onlineAnimationURLs[_currentWeather], (uint8_t**)rainFrames, 3, 1024);
                            break;
                        case WEATHER_SNOW:
                            fetchSuccess = fetchAnimationFrames(_onlineAnimationURLs[_currentWeather], (uint8_t**)snowFrames, 3, 1024);
                            break;
                        case WEATHER_STORM:
                            fetchSuccess = fetchAnimationFrames(_onlineAnimationURLs[_currentWeather], (uint8_t**)stormFrames, 2, 1024);
                            break;
                    }
                    
                    if (!fetchSuccess) {
                        WA_SERIAL_PRINTLN("Some frames failed to load, continuing with available frames");
                    }
                }
            }
            
            http.end();
            return true;
        }
    }
    
    http.end();
    return false;
}

bool WeatherAnimations::fetchTemperatureData() {
    if (WiFi.status() != WL_CONNECTED) {
        if (_manageWiFi) {
            connectToWiFi();
        }
        if (WiFi.status() != WL_CONNECTED) {
            WA_SERIAL_PRINTLN("No Wi-Fi connection available.");
            return false;
        }
    }
    
    // Fetch indoor temperature
    bool indoorSuccess = false;
    if (_indoorTempEntity != nullptr) {
        HTTPClient httpIndoor;
        String urlIndoor = String("http://") + _haIP + ":8123/api/states/" + _indoorTempEntity;
        httpIndoor.begin(urlIndoor);
        httpIndoor.addHeader("Authorization", String("Bearer ") + _haToken);
        int httpCodeIndoor = httpIndoor.GET();
        
        if (httpCodeIndoor == 200) {
            String payload = httpIndoor.getString();
            WA_SERIAL_PRINTLN("Indoor Temperature Response:");
            WA_SERIAL_PRINTLN(payload);
            
            _indoorTemp = extractTemperatureFromHA(payload);
            if (_indoorTemp != -999.0f) {
                indoorSuccess = true;
                WA_SERIAL_PRINT("Indoor temperature: ");
                WA_SERIAL_PRINTLN(_indoorTemp);
            }
        } else {
            WA_SERIAL_PRINT("Failed to fetch indoor temperature, HTTP code: ");
            WA_SERIAL_PRINTLN(httpCodeIndoor);
        }
        
        httpIndoor.end();
    }
    
    // Fetch outdoor temperature
    bool outdoorSuccess = false;
    if (_outdoorTempEntity != nullptr) {
        HTTPClient httpOutdoor;
        String urlOutdoor = String("http://") + _haIP + ":8123/api/states/" + _outdoorTempEntity;
        httpOutdoor.begin(urlOutdoor);
        httpOutdoor.addHeader("Authorization", String("Bearer ") + _haToken);
        int httpCodeOutdoor = httpOutdoor.GET();
        
        if (httpCodeOutdoor == 200) {
            String payload = httpOutdoor.getString();
            WA_SERIAL_PRINTLN("Outdoor Temperature Response:");
            WA_SERIAL_PRINTLN(payload);
            
            _outdoorTemp = extractTemperatureFromHA(payload);
            if (_outdoorTemp != -999.0f) {
                outdoorSuccess = true;
                WA_SERIAL_PRINT("Outdoor temperature: ");
                WA_SERIAL_PRINTLN(_outdoorTemp);
            }
        } else {
            WA_SERIAL_PRINT("Failed to fetch outdoor temperature, HTTP code: ");
            WA_SERIAL_PRINTLN(httpCodeOutdoor);
        }
        
        httpOutdoor.end();
    }
    
    // Update temperature data flag
    _hasTemperatureData = indoorSuccess || outdoorSuccess;
    
    return _hasTemperatureData;
}

float WeatherAnimations::extractTemperatureFromHA(const String& payload) {
    // Extract state value from Home Assistant response
    int stateStart = payload.indexOf("\"state\":\"") + 9;
    if (stateStart > 9) {
        int stateEnd = payload.indexOf("\",", stateStart);
        if (stateEnd > stateStart) {
            String tempStr = payload.substring(stateStart, stateEnd);
            return tempStr.toFloat();
        }
    }
    
    return -999.0f; // Error value
}

void WeatherAnimations::displayAnimation() {
    WA_SERIAL_PRINTLN("Entering displayAnimation method.");
    // If currently in transition mode, handle that instead of normal display
    if (_isTransitioning) {
        WA_SERIAL_PRINTLN("Handling transition animation.");
        unsigned long currentMillis = millis();
        unsigned long elapsedTime = currentMillis - _transitionStartTime;
        
        // Calculate progress (0.0 to 1.0)
        float progress = min(1.0f, (float)elapsedTime / _transitionDuration);
        WA_SERIAL_PRINT("Transition progress: ");
        WA_SERIAL_PRINTLN(progress);
        
        // Display the transition frame
        displayTransitionFrame(_currentWeather, progress);
        
        // End transition when complete
        if (progress >= 1.0f) {
            _isTransitioning = false;
            WA_SERIAL_PRINTLN("Transition completed.");
        }
        
        return;
    }
    
    WA_SERIAL_PRINTLN("Handling regular animation display.");
    // Regular animation display based on animation mode
    if (_displayType == OLED_SSD1306 || _displayType == OLED_SH1106) {
        if (oledDisplay == nullptr) {
            WA_SERIAL_PRINTLN("OLED display not initialized, cannot draw animation.");
            return;
        }
        
        oledDisplay->clearDisplay();
        
        // Draw header with weather type name
        oledDisplay->setTextSize(1);
        oledDisplay->setTextColor(SSD1306_WHITE);
        oledDisplay->setCursor(0, 0);
        oledDisplay->println("Weather:");
        oledDisplay->setTextSize(2);
        oledDisplay->setCursor(0, 12);
        oledDisplay->println(getWeatherText(_currentWeather));
        
        if (_animationMode == ANIMATION_STATIC) {
            // Draw static weather icon using BasicUsage style
            drawStaticWeatherIcon(_currentWeather);
        } else {
            // Check if we have animation frames
            if (_animations[_currentWeather].frameCount > 0) {
                // Get current frame based on timing
                uint8_t frameIndex = (millis() / _animations[_currentWeather].frameDelay) % _animations[_currentWeather].frameCount;
                
                // Draw animated weather icon using BasicUsage style
                drawAnimatedWeatherIcon(_currentWeather, frameIndex);
            } else {
                // Fall back to static icon if no animation frames
                drawStaticWeatherIcon(_currentWeather);
            }
        }
        
        // Add temperature data at the bottom if available
        if (_hasTemperatureData) {
            oledDisplay->setTextSize(1);
            oledDisplay->setCursor(0, 45);
            
            // Show indoor and outdoor temps
            oledDisplay->print("In:");
            oledDisplay->print(_indoorTemp, 1);
            oledDisplay->print("C  Out:");
            oledDisplay->print(_outdoorTemp, 1);
            oledDisplay->print("C");
            
            // Show forecast min/max on last line
            oledDisplay->setCursor(0, 56);
            oledDisplay->print("Min:");
            oledDisplay->print(_minForecastTemp, 1);
            oledDisplay->print("C Max:");
            oledDisplay->print(_maxForecastTemp, 1);
            oledDisplay->print("C");
        }
        
        oledDisplay->display();
        WA_SERIAL_PRINTLN("Updated OLED display with BasicUsage style.");
    } 
#if defined(ESP32) || defined(ESP8266)
    else if (_displayType == TFT_DISPLAY && tftDisplay != nullptr) {
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
                renderTFTAnimation(_currentWeather);
                
                // Add temperature data if available
                if (_hasTemperatureData) {
                    // Preserve area for temperature display
                    tftDisplay->fillRect(0, TFT_HEIGHT - 40, TFT_WIDTH, 40, TFT_BLACK);
                    
                    tftDisplay->setCursor(10, TFT_HEIGHT - 35);
                    tftDisplay->setTextColor(TFT_WHITE);
                    tftDisplay->setTextSize(1);
                    
                    tftDisplay->print("Indoor: ");
                    tftDisplay->print(_indoorTemp, 1);
                    tftDisplay->print("C  Outdoor: ");
                    tftDisplay->print(_outdoorTemp, 1);
                    tftDisplay->println("C");
                    
                    tftDisplay->setCursor(10, TFT_HEIGHT - 20);
                    tftDisplay->print("Forecast: Min ");
                    tftDisplay->print(_minForecastTemp, 1);
                    tftDisplay->print("C  Max ");
                    tftDisplay->print(_maxForecastTemp, 1);
                    tftDisplay->println("C");
                }
            }
        } else {
            // Static display or fallback
            tftDisplay->fillScreen(TFT_BLACK);
            displayTextFallback(_currentWeather);
        }
    } 
#endif
    else {
        WA_SERIAL_PRINTLN("No display initialized or unsupported display type.");
    }
    
    // Continuous animation update for the continuous weather mode
    if (_mode == CONTINUOUS_WEATHER) {
        // For continuous display, we need to periodically refresh
        delay(16); // ~60 fps maximum update rate
    }
    
    WA_SERIAL_PRINTLN("Exiting displayAnimation method.");
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
    if (_displayType == OLED_SSD1306 || _displayType == OLED_SH1106) {
        // Use SSD1306 library for both SSD1306 and SH1106 displays (compatibility mode)
        oledDisplay = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
        if (oledDisplay->begin(SSD1306_SWITCHCAPVCC, _i2cAddr)) {
            oledDisplay->clearDisplay();
            oledDisplay->display();
            WA_SERIAL_PRINTLN("SSD1306 display initialized.");
        } else {
            WA_SERIAL_PRINTLN("SSD1306 display initialization failed.");
            delete oledDisplay;
            oledDisplay = nullptr;
        }
    } 
#if defined(ESP32) || defined(ESP8266)
    else if (_displayType == TFT_DISPLAY) {
        tftDisplay = new TFT_eSPI();
        tftDisplay->init();
        tftDisplay->fillScreen(TFT_BLACK);
        tftDisplay->setRotation(0);
        WA_SERIAL_PRINTLN("TFT display initialized.");
    }
#endif
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
    if (_displayType == OLED_SSD1306) {
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
    if ((_displayType == OLED_SSD1306 || _displayType == OLED_SH1106) && oledDisplay != nullptr) {
        oledDisplay->clearDisplay();
        
        // Draw header with weather type name
        oledDisplay->setTextSize(1);
        oledDisplay->setTextColor(SSD1306_WHITE);
        oledDisplay->setCursor(0, 0);
        oledDisplay->println("Weather:");
        oledDisplay->setTextSize(2);
        oledDisplay->setCursor(0, 12);
        oledDisplay->println(getWeatherText(weatherCondition));
        
        // Determine transition effect based on transition direction
        switch (_transitionDirection) {
            case TRANSITION_FADE:
                // For fade transitions, we implement a pixel-by-pixel fade
                if (random(100) < progress * 100) {
                    // Draw the weather icon with random pixels based on progress
                    drawStaticWeatherIcon(weatherCondition);
                }
                break;
                
            case TRANSITION_RIGHT_TO_LEFT:
            case TRANSITION_LEFT_TO_RIGHT:
            case TRANSITION_TOP_TO_BOTTOM:
            case TRANSITION_BOTTOM_TO_TOP: {
                // For directional transitions, calculate offsets
                int16_t x = 0;
                int16_t y = 0;
                
                switch (_transitionDirection) {
                    case TRANSITION_RIGHT_TO_LEFT:
                        x = SCREEN_WIDTH * (1.0f - progress);
                        break;
                    case TRANSITION_LEFT_TO_RIGHT:
                        x = -SCREEN_WIDTH * (1.0f - progress);
                        break;
                    case TRANSITION_TOP_TO_BOTTOM:
                        y = -SCREEN_HEIGHT * (1.0f - progress);
                        break;
                    case TRANSITION_BOTTOM_TO_TOP:
                        y = SCREEN_HEIGHT * (1.0f - progress);
                        break;
                }
                
                // Save current cursor position
                int16_t curX = oledDisplay->getCursorX();
                int16_t curY = oledDisplay->getCursorY();
                
                // Prepare for offscreen drawing
                oledDisplay->setCursor(curX + x, curY + y);
                
                // Draw the icon offset by the transition amount
                // We'll need to manually offset all drawing commands
                
                // Save and restore cursor after drawing
                oledDisplay->setCursor(curX, curY);
                
                // Draw the icon with proper offset
                // (This is simplified - in practice, you'd need to offset all drawing commands)
                switch (weatherCondition) {
                    case WEATHER_CLEAR:
                        oledDisplay->fillCircle(96 + x, 32 + y, 16, SSD1306_WHITE);
                        break;
                    case WEATHER_CLOUDY:
                        oledDisplay->fillRoundRect(86 + x, 34 + y, 36, 18, 8, SSD1306_WHITE);
                        oledDisplay->fillRoundRect(78 + x, 24 + y, 28, 20, 8, SSD1306_WHITE);
                        break;
                    case WEATHER_RAIN:
                        oledDisplay->fillRoundRect(86 + x, 24 + y, 36, 16, 8, SSD1306_WHITE);
                        for (int i = 0; i < 6; i++) {
                            oledDisplay->drawLine(86 + i*7 + x, 42 + y, 89 + i*7 + x, 52 + y, SSD1306_WHITE);
                        }
                        break;
                    case WEATHER_SNOW:
                        oledDisplay->fillRoundRect(86 + x, 24 + y, 36, 16, 8, SSD1306_WHITE);
                        for (int i = 0; i < 6; i++) {
                            oledDisplay->drawCircle(89 + i*7 + x, 48 + y, 2, SSD1306_WHITE);
                        }
                        break;
                    case WEATHER_STORM:
                        oledDisplay->fillRoundRect(86 + x, 24 + y, 36, 16, 8, SSD1306_WHITE);
                        oledDisplay->fillTriangle(100 + x, 42 + y, 90 + x, 52 + y, 95 + x, 52 + y, SSD1306_WHITE);
                        oledDisplay->fillTriangle(95 + x, 52 + y, 105 + x, 52 + y, 98 + x, 62 + y, SSD1306_WHITE);
                        break;
                }
                break;
            }
        }
        
        // Add temperature data at the bottom if available
        if (_hasTemperatureData) {
            oledDisplay->setTextSize(1);
            oledDisplay->setCursor(0, 45);
            
            // Show indoor and outdoor temps
            oledDisplay->print("In:");
            oledDisplay->print(_indoorTemp, 1);
            oledDisplay->print("C  Out:");
            oledDisplay->print(_outdoorTemp, 1);
            oledDisplay->print("C");
            
            // Show forecast min/max on last line
            oledDisplay->setCursor(0, 56);
            oledDisplay->print("Min:");
            oledDisplay->print(_minForecastTemp, 1);
            oledDisplay->print("C Max:");
            oledDisplay->print(_maxForecastTemp, 1);
            oledDisplay->print("C");
        }
        
        oledDisplay->display();
    } 
#if defined(ESP32) || defined(ESP8266)
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
                // For fade, adjust opacity based on progress
                // This is a simplified version - you'd need to implement
                // alpha blending for a proper fade
                tftDisplay->setTextColor(TFT_WHITE, TFT_BLACK);
                tftDisplay->setTextSize(2);
                tftDisplay->setCursor(10, 10);
                tftDisplay->println(getWeatherText(weatherCondition));
                
                // Create a fade pattern
                if (progress < 1.0f) {
                    uint16_t stripeCount = 10 * (1.0f - progress);
                    for (uint16_t i = 0; i < stripeCount; i++) {
                        uint16_t y = (i * height) / stripeCount;
                        tftDisplay->fillRect(0, y, width, height / stripeCount / 2, TFT_BLACK);
                    }
                }
                break;
        }
        
        if (_transitionDirection != TRANSITION_FADE) {
            // Draw the weather text with the calculated offset
            tftDisplay->setTextColor(TFT_WHITE, TFT_BLACK);
            tftDisplay->setTextSize(2);
            tftDisplay->setCursor(10 + x, 10 + y);
            tftDisplay->println(getWeatherText(weatherCondition));
            
            // Draw a simple weather icon
            switch (weatherCondition) {
                case WEATHER_CLEAR:
                    tftDisplay->fillCircle(120 + x, 160, 40, TFT_YELLOW);
                    break;
                case WEATHER_CLOUDY:
                    tftDisplay->fillRoundRect(80, 140, 100, 40, 20, TFT_WHITE);
                    break;
                case WEATHER_RAIN:
                    tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_LIGHTGREY);
                    for (int i = 0; i < 10; i++) {
                        tftDisplay->drawLine(90 + i*10, 170, 90 + i*10 + 5, 190, TFT_BLUE);
                    }
                    break;
                case WEATHER_SNOW:
                    tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_LIGHTGREY);
                    for (int i = 0; i < 10; i++) {
                        tftDisplay->drawPixel(90 + i*10, 180, TFT_WHITE);
                        tftDisplay->drawPixel(90 + i*10 + 1, 180, TFT_WHITE);
                        tftDisplay->drawPixel(90 + i*10, 180 + 1, TFT_WHITE);
                        tftDisplay->drawPixel(90 + i*10 + 1, 180 + 1, TFT_WHITE);
                    }
                    break;
                case WEATHER_STORM:
                    tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_DARKGREY);
                    tftDisplay->fillTriangle(120, 170, 130, 200, 110, 190, TFT_YELLOW);
                    break;
                default:
                    tftDisplay->println("Unknown");
            }
        }
    } 
#endif
    else {
        WA_SERIAL_PRINTLN("No display initialized or unsupported display type.");
    }
}

void WeatherAnimations::displayTextFallback(uint8_t weatherCondition) {
    if ((_displayType == OLED_SSD1306 || _displayType == OLED_SH1106) && oledDisplay != nullptr) {
        oledDisplay->clearDisplay();
        oledDisplay->setTextSize(1);
        oledDisplay->setTextColor(SSD1306_WHITE);
        oledDisplay->setCursor(0, 0);
        oledDisplay->println(getWeatherText(weatherCondition));
        
        // Display temperature information if available
        if (_hasTemperatureData) {
            oledDisplay->setCursor(0, 54);
            
            // Indoor temp
            oledDisplay->print("In:");
            oledDisplay->print(_indoorTemp, 1);
            oledDisplay->print("C ");
            
            // Outdoor temp
            oledDisplay->print("Out:");
            oledDisplay->print(_outdoorTemp, 1);
            oledDisplay->print("C");
            
            // Min/Max forecast on second line
            oledDisplay->setCursor(0, 54);
            oledDisplay->print("Min:");
            oledDisplay->print(_minForecastTemp, 1);
            oledDisplay->print("C Max:");
            oledDisplay->print(_maxForecastTemp, 1);
            oledDisplay->print("C");
        }
        
        // Draw a simple weather icon based on condition
        switch (weatherCondition) {
            case WEATHER_CLEAR:
                oledDisplay->fillCircle(64, 32, 16, SSD1306_WHITE);
                break;
            case WEATHER_CLOUDY:
                oledDisplay->fillRoundRect(44, 22, 50, 20, 10, SSD1306_WHITE);
                oledDisplay->fillRoundRect(34, 32, 70, 18, 10, SSD1306_WHITE);
                break;
            case WEATHER_RAIN:
                oledDisplay->fillRoundRect(44, 20, 50, 16, 8, SSD1306_WHITE);
                for (int i = 0; i < 5; i++) {
                    oledDisplay->drawLine(44 + i*10, 38, 47 + i*10, 46, SSD1306_WHITE);
                }
                break;
            case WEATHER_SNOW:
                oledDisplay->fillRoundRect(44, 20, 50, 16, 8, SSD1306_WHITE);
                for (int i = 0; i < 5; i++) {
                    oledDisplay->drawCircle(44 + i*10, 42, 2, SSD1306_WHITE);
                }
                break;
            case WEATHER_STORM:
                oledDisplay->fillRoundRect(44, 20, 50, 16, 8, SSD1306_WHITE);
                oledDisplay->fillTriangle(64, 36, 58, 46, 64, 46, SSD1306_WHITE);
                oledDisplay->fillTriangle(64, 46, 70, 46, 64, 54, SSD1306_WHITE);
                break;
            default:
                oledDisplay->setTextSize(2);
                oledDisplay->setCursor(10, 30);
                oledDisplay->println("?");
                break;
        }
        
        oledDisplay->display();
    } 
#if defined(ESP32) || defined(ESP8266)
    else if (_displayType == TFT_DISPLAY && tftDisplay != nullptr) {
        tftDisplay->fillScreen(TFT_BLACK);
        tftDisplay->setCursor(10, 10);
        tftDisplay->setTextColor(TFT_WHITE);
        tftDisplay->setTextSize(2);
        
        switch (weatherCondition) {
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
        
        // Display temperature information if available
        if (_hasTemperatureData) {
            tftDisplay->setCursor(10, 220);
            tftDisplay->setTextSize(1);
            
            tftDisplay->print("Indoor: ");
            tftDisplay->print(_indoorTemp, 1);
            tftDisplay->println("C");
            
            tftDisplay->print("Outdoor: ");
            tftDisplay->print(_outdoorTemp, 1);
            tftDisplay->println("C");
            
            tftDisplay->print("Forecast: ");
            tftDisplay->print(_minForecastTemp, 1);
            tftDisplay->print("C - ");
            tftDisplay->print(_maxForecastTemp, 1);
            tftDisplay->println("C");
        }
    }
#endif
}

const char* WeatherAnimations::getWeatherText(uint8_t weatherCondition) {
    switch (weatherCondition) {
        case WEATHER_CLEAR:   return "Clear Sky";
        case WEATHER_CLOUDY:  return "Cloudy";
        case WEATHER_RAIN:    return "Rainy";
        case WEATHER_SNOW:    return "Snowy";
        case WEATHER_STORM:   return "Stormy";
        default:              return "Unknown";
    }
}

WeatherAnimations::~WeatherAnimations() {
    // Clean up display objects
    if ((_displayType == OLED_SSD1306 || _displayType == OLED_SH1106) && oledDisplay != nullptr) {
        delete oledDisplay;
        oledDisplay = nullptr;
    } else if (_displayType == TFT_DISPLAY && tftDisplay != nullptr) {
        delete tftDisplay;
        tftDisplay = nullptr;
    }
    
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

#if defined(ESP32) || defined(ESP8266)
// Only include renderTFTAnimation for ESP32/ESP8266 platforms
void WeatherAnimations::renderTFTAnimation(uint8_t weatherCondition) {
    if (tftDisplay == nullptr) {
        return;
    }
    
    // Check if we have valid frame data
    if (_onlineAnimationCache[weatherCondition].frameCount > 0 &&
        _onlineAnimationCache[weatherCondition].frameData[_currentFrame] != nullptr) {
        
        // In a real implementation, this would decode and display the actual frame data
        // This is a simplified implementation that draws placeholder graphics
        
        tftDisplay->fillScreen(TFT_BLACK);
        
        // Draw the frame number for debugging
        tftDisplay->setCursor(10, 10);
        tftDisplay->setTextColor(TFT_WHITE);
        tftDisplay->setTextSize(1);
        tftDisplay->print("Frame: ");
        tftDisplay->println(_currentFrame + 1);
        
        // Center coordinates for the animation
        int16_t centerX = TFT_WIDTH / 2;
        int16_t centerY = TFT_HEIGHT / 2;
        
        // Create animated effects based on weather condition and current frame
        switch (weatherCondition) {
            case WEATHER_CLEAR: {
                // Sun with rays that get longer/shorter based on frame
                int16_t rayLength = 15 + (_currentFrame * 5);
                tftDisplay->fillCircle(centerX, centerY, 30, TFT_YELLOW);
                
                // Draw 8 rays around the sun
                for (int i = 0; i < 8; i++) {
                    float angle = i * PI / 4;
                    int16_t x1 = centerX + cos(angle) * 30;
                    int16_t y1 = centerY + sin(angle) * 30;
                    int16_t x2 = centerX + cos(angle) * (30 + rayLength);
                    int16_t y2 = centerY + sin(angle) * (30 + rayLength);
                    tftDisplay->drawLine(x1, y1, x2, y2, TFT_YELLOW);
                }
                break;
            }
            
            case WEATHER_CLOUDY: {
                // Animate clouds moving horizontally
                int16_t offset = _currentFrame * 10;
                if (offset > TFT_WIDTH / 2) offset = 0;
                
                // Draw cloud shapes with offset
                tftDisplay->fillCircle(centerX - 20 + offset, centerY, 20, TFT_LIGHTGREY);
                tftDisplay->fillCircle(centerX + offset, centerY - 10, 25, TFT_LIGHTGREY);
                tftDisplay->fillCircle(centerX + 20 + offset, centerY, 20, TFT_LIGHTGREY);
                tftDisplay->fillRect(centerX - 20 + offset, centerY, 40, 20, TFT_LIGHTGREY);
                
                // Second smaller cloud
                tftDisplay->fillCircle(centerX - 60 - offset, centerY + 40, 15, TFT_LIGHTGREY);
                tftDisplay->fillCircle(centerX - 40 - offset, centerY + 35, 18, TFT_LIGHTGREY);
                tftDisplay->fillRect(centerX - 60 - offset, centerY + 35, 30, 15, TFT_LIGHTGREY);
                break;
            }
            
            case WEATHER_RAIN: {
                // Cloud with animated rain drops
                // Draw the cloud
                tftDisplay->fillCircle(centerX - 20, centerY - 30, 20, TFT_LIGHTGREY);
                tftDisplay->fillCircle(centerX, centerY - 40, 25, TFT_LIGHTGREY);
                tftDisplay->fillCircle(centerX + 20, centerY - 30, 20, TFT_LIGHTGREY);
                tftDisplay->fillRect(centerX - 20, centerY - 30, 40, 20, TFT_LIGHTGREY);
                
                // Draw raindrops at different positions based on frame
                for (int i = 0; i < 10; i++) {
                    int16_t x = centerX - 40 + (i * 10);
                    int16_t yOffset = ((i + _currentFrame) % 4) * 20;
                    int16_t y = centerY + yOffset;
                    
                    // Only draw if within screen bounds
                    if (y < TFT_HEIGHT) {
                        tftDisplay->drawLine(x, y, x + 3, y + 10, TFT_BLUE);
                    }
                }
                break;
            }
            
            case WEATHER_SNOW: {
                // Cloud with animated snowflakes
                // Draw the cloud
                tftDisplay->fillCircle(centerX - 20, centerY - 30, 20, TFT_LIGHTGREY);
                tftDisplay->fillCircle(centerX, centerY - 40, 25, TFT_LIGHTGREY);
                tftDisplay->fillCircle(centerX + 20, centerY - 30, 20, TFT_LIGHTGREY);
                tftDisplay->fillRect(centerX - 20, centerY - 30, 40, 20, TFT_LIGHTGREY);
                
                // Draw snowflakes at different positions based on frame
                for (int i = 0; i < 10; i++) {
                    int16_t x = centerX - 40 + (i * 10);
                    int16_t yOffset = ((i + _currentFrame) % 4) * 20;
                    int16_t y = centerY + yOffset;
                    
                    // Only draw if within screen bounds
                    if (y < TFT_HEIGHT) {
                        // Draw a simple snowflake pattern
                        tftDisplay->drawLine(x, y, x + 4, y + 4, TFT_WHITE);
                        tftDisplay->drawLine(x + 4, y, x, y + 4, TFT_WHITE);
                        tftDisplay->drawLine(x, y + 2, x + 4, y + 2, TFT_WHITE);
                        tftDisplay->drawLine(x + 2, y, x + 2, y + 4, TFT_WHITE);
                    }
                }
                break;
            }
            
            case WEATHER_STORM: {
                // Cloud with animated lightning bolt
                // Draw the storm cloud
                tftDisplay->fillCircle(centerX - 20, centerY - 30, 20, TFT_DARKGREY);
                tftDisplay->fillCircle(centerX, centerY - 40, 25, TFT_DARKGREY);
                tftDisplay->fillCircle(centerX + 20, centerY - 30, 20, TFT_DARKGREY);
                tftDisplay->fillRect(centerX - 20, centerY - 30, 40, 20, TFT_DARKGREY);
                
                // Draw lightning bolt - flash on alternate frames
                if (_currentFrame % 2 == 0) {
                    tftDisplay->fillTriangle(
                        centerX, centerY,
                        centerX - 10, centerY + 30,
                        centerX + 5, centerY + 15,
                        TFT_YELLOW
                    );
                    tftDisplay->fillTriangle(
                        centerX + 5, centerY + 15,
                        centerX - 5, centerY + 45,
                        centerX + 15, centerY + 45,
                        TFT_YELLOW
                    );
                }
                break;
            }
            
            default:
                // Default animation if no specific animation is available
                tftDisplay->setCursor(centerX - 30, centerY);
                tftDisplay->setTextSize(2);
                tftDisplay->println("?");
                break;
        }
    } else {
        // Fallback to text display if no animation frames are available
        displayTextFallback(weatherCondition);
    }
}

// New helper function to draw static weather icons in BasicUsage style
void WeatherAnimations::drawStaticWeatherIcon(uint8_t weatherType) {
    switch (weatherType) {
        case WEATHER_CLEAR:
            // Draw sun
            oledDisplay->fillCircle(96, 32, 16, SSD1306_WHITE);
            break;
        case WEATHER_CLOUDY:
            // Draw cloud
            oledDisplay->fillRoundRect(86, 34, 36, 18, 8, SSD1306_WHITE);
            oledDisplay->fillRoundRect(78, 24, 28, 20, 8, SSD1306_WHITE);
            break;
        case WEATHER_RAIN:
            // Draw cloud with rain
            oledDisplay->fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
            for (int i = 0; i < 6; i++) {
                oledDisplay->drawLine(86 + i*7, 42, 89 + i*7, 52, SSD1306_WHITE);
            }
            break;
        case WEATHER_SNOW:
            // Draw cloud with snow
            oledDisplay->fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
            for (int i = 0; i < 6; i++) {
                oledDisplay->drawCircle(89 + i*7, 48, 2, SSD1306_WHITE);
            }
            break;
        case WEATHER_STORM:
            // Draw cloud with lightning
            oledDisplay->fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
            oledDisplay->fillTriangle(100, 42, 90, 52, 95, 52, SSD1306_WHITE);
            oledDisplay->fillTriangle(95, 52, 105, 52, 98, 62, SSD1306_WHITE);
            break;
        default:
            // Unknown weather, draw question mark
            oledDisplay->setTextSize(3);
            oledDisplay->setCursor(90, 30);
            oledDisplay->print("?");
            break;
    }
}

// New helper function to draw animated weather icons in BasicUsage style
void WeatherAnimations::drawAnimatedWeatherIcon(uint8_t weatherType, uint8_t frame) {
    // Variables needed for animations
    int offset = 0;
    
    switch (weatherType) {
        case WEATHER_CLEAR:
            // Animated sun (rays expand/contract)
            oledDisplay->fillCircle(96, 32, 12, SSD1306_WHITE);
            if (frame % 2 == 0) {
                // Draw longer rays
                for (int i = 0; i < 8; i++) {
                    float angle = i * PI / 4.0;
                    int x1 = 96 + cos(angle) * 14;
                    int y1 = 32 + sin(angle) * 14;
                    int x2 = 96 + cos(angle) * 22;
                    int y2 = 32 + sin(angle) * 22;
                    oledDisplay->drawLine(x1, y1, x2, y2, SSD1306_WHITE);
                }
            } else {
                // Draw shorter rays
                for (int i = 0; i < 8; i++) {
                    float angle = i * PI / 4.0;
                    int x1 = 96 + cos(angle) * 14;
                    int y1 = 32 + sin(angle) * 14;
                    int x2 = 96 + cos(angle) * 18;
                    int y2 = 32 + sin(angle) * 18;
                    oledDisplay->drawLine(x1, y1, x2, y2, SSD1306_WHITE);
                }
            }
            break;
        case WEATHER_CLOUDY:
            // Animated cloud (moves slightly)
            offset = (frame % 2 == 0) ? 0 : 2;
            oledDisplay->fillRoundRect(86 + offset, 34, 36, 18, 8, SSD1306_WHITE);
            oledDisplay->fillRoundRect(78 + offset, 24, 28, 20, 8, SSD1306_WHITE);
            break;
        case WEATHER_RAIN:
            // Animated rain (drops move)
            oledDisplay->fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
            for (int i = 0; i < 6; i++) {
                int height = ((i + frame) % 3) * 4; // Vary drop heights
                oledDisplay->drawLine(86 + i*7, 42 + height, 89 + i*7, 52 + height, SSD1306_WHITE);
            }
            break;
        case WEATHER_SNOW:
            // Animated snow (flakes move)
            oledDisplay->fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
            for (int i = 0; i < 6; i++) {
                int offset_y = ((i + frame) % 3) * 3;
                int offset_x = ((i + frame) % 2) * 2 - 1;
                oledDisplay->drawCircle(89 + i*7 + offset_x, 48 + offset_y, 2, SSD1306_WHITE);
            }
            break;
        case WEATHER_STORM:
            // Animated lightning (flash)
            oledDisplay->fillRoundRect(86, 24, 36, 16, 8, SSD1306_WHITE);
            if (frame % 3 != 0) { // Show lightning most frames
                oledDisplay->fillTriangle(100, 42, 90, 52, 95, 52, SSD1306_WHITE);
                oledDisplay->fillTriangle(95, 52, 105, 52, 98, 62, SSD1306_WHITE);
            }
            break;
    }
}
#else
// Empty implementation for non-ESP32/ESP8266 platforms
void WeatherAnimations::renderTFTAnimation(uint8_t weatherCondition) {
    // Do nothing - TFT not supported on this platform
}
#endif

void WeatherAnimations::setTemperatureEntities(const char* indoorTempEntity, const char* outdoorTempEntity) {
    _indoorTempEntity = indoorTempEntity;
    _outdoorTempEntity = outdoorTempEntity;
}

 