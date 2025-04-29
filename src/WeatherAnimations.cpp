#include "WeatherAnimations.h"
#include <Adafruit_SSD1306.h>
#include "WeatherAnimationsAnimations.h"

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
      _lastFetchTime(0), _fetchCooldown(300000), _isTransitioning(false) { // 5 minutes cooldown
    // Initialize animations array
    for (int i = 0; i < 5; i++) {
        _animations[i].frames = nullptr;
        _animations[i].frameCount = 0;
        _animations[i].frameDelay = 0;
        _onlineAnimationURLs[i] = nullptr;
        _onlineAnimationCache[i].imageData = nullptr;
        _onlineAnimationCache[i].dataSize = 0;
        _onlineAnimationCache[i].isLoaded = false;
    }
    // Set default animations for OLED (monochrome)
    setAnimation(WEATHER_CLEAR, clearSkyFrames, 2, 500);
    setAnimation(WEATHER_CLOUDY, cloudyFrames, 2, 500);
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
            Serial.println("Failed to connect to Wi-Fi");
        }
    } else if (!_manageWiFi) {
        Serial.println("Wi-Fi management disabled, assuming connection is handled externally.");
    }
}

void WeatherAnimations::setMode(uint8_t mode) {
    if (mode == SIMPLE_TRANSITION || mode == CONTINUOUS_WEATHER) {
        _mode = mode;
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
        Serial.print(".");
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
            Serial.println("No Wi-Fi connection available.");
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
        Serial.println("Home Assistant Response:");
        Serial.println(payload);
        // Parse JSON (simplified parsing for demonstration)
        if (payload.indexOf("clear") != -1 || payload.indexOf("sunny") != -1) {
            _currentWeather = WEATHER_CLEAR;
        } else if (payload.indexOf("cloudy") != -1 || payload.indexOf("overcast") != -1) {
            _currentWeather = WEATHER_CLOUDY;
        } else if (payload.indexOf("rain") != -1 || payload.indexOf("drizzle") != -1) {
            _currentWeather = WEATHER_RAIN;
        } else if (payload.indexOf("snow") != -1 || payload.indexOf("sleet") != -1) {
            _currentWeather = WEATHER_SNOW;
        } else if (payload.indexOf("storm") != -1 || payload.indexOf("thunder") != -1) {
            _currentWeather = WEATHER_STORM;
        }
        http.end();
        return true;
    } else {
        Serial.print("HTTP Error: ");
        Serial.println(httpCode);
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
    
    // Regular animation display (existing code)
    if (_displayType == OLED_DISPLAY && oledDisplay != nullptr) {
        oledDisplay->clearDisplay();
        
        // Check if animation is set for current weather
        if (_animations[_currentWeather].frameCount > 0) {
            static uint8_t currentFrame = 0;
            static unsigned long lastFrameTime = 0;
            unsigned long currentTime = millis();
            
            if (currentTime - lastFrameTime >= _animations[_currentWeather].frameDelay) {
                oledDisplay->drawBitmap(0, 0, _animations[_currentWeather].frames[currentFrame], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
                currentFrame = (currentFrame + 1) % _animations[_currentWeather].frameCount;
                lastFrameTime = currentTime;
                
                // In simple transition mode, stop after one cycle
                if (_mode == SIMPLE_TRANSITION && currentFrame == 0) {
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
        // TFT display handling (colour)
        #if defined(ESP8266) || defined(ESP32)
        if (tftDisplay != nullptr) {
            // Try to use online animation if available
            if (_onlineAnimationURLs[_currentWeather] != nullptr && WiFi.status() == WL_CONNECTED) {
                // If the animation isn't already loaded, fetch it
                if (!_onlineAnimationCache[_currentWeather].isLoaded) {
                    fetchOnlineAnimation(_currentWeather);
                }
                // Render the animation
                renderTFTAnimation(_currentWeather);
            } else {
                // Fallback to basic text display
                tftDisplay->fillScreen(TFT_BLACK);
                tftDisplay->setCursor(10, 10);
                tftDisplay->setTextColor(TFT_WHITE);
                tftDisplay->setTextSize(2);
                
                switch (_currentWeather) {
                    case WEATHER_CLEAR:
                        tftDisplay->println("Clear Sky");
                        tftDisplay->fillCircle(120, 160, 40, TFT_YELLOW); // Sun
                        break;
                    case WEATHER_CLOUDY:
                        tftDisplay->println("Cloudy");
                        tftDisplay->fillRoundRect(80, 140, 100, 40, 20, TFT_WHITE); // Cloud
                        break;
                    case WEATHER_RAIN:
                        tftDisplay->println("Rainy");
                        tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_LIGHTGREY); // Cloud
                        // Rain drops
                        for (int i = 0; i < 10; i++) {
                            tftDisplay->drawLine(90 + i*10, 170, 90 + i*10 + 5, 190, TFT_BLUE);
                        }
                        break;
                    case WEATHER_SNOW:
                        tftDisplay->println("Snowy");
                        tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_LIGHTGREY); // Cloud
                        // Snowflakes
                        for (int i = 0; i < 10; i++) {
                            tftDisplay->drawPixel(90 + i*10, 180, TFT_WHITE);
                            tftDisplay->drawPixel(90 + i*10 + 1, 180, TFT_WHITE);
                            tftDisplay->drawPixel(90 + i*10, 180 + 1, TFT_WHITE);
                            tftDisplay->drawPixel(90 + i*10 + 1, 180 + 1, TFT_WHITE);
                        }
                        break;
                    case WEATHER_STORM:
                        tftDisplay->println("Stormy");
                        tftDisplay->fillRoundRect(80, 120, 100, 40, 20, TFT_DARKGREY); // Cloud
                        // Lightning bolt
                        tftDisplay->fillTriangle(120, 170, 130, 200, 110, 190, TFT_YELLOW);
                        break;
                    default:
                        tftDisplay->println("Unknown");
                }
            }
        } else {
            Serial.println("TFT display not initialized.");
        }
        #else
        Serial.println("TFT display support not available on this platform.");
        #endif
    }
}

bool WeatherAnimations::fetchOnlineAnimation(uint8_t weatherCondition) {
    if (_onlineAnimationURLs[weatherCondition] == nullptr || WiFi.status() != WL_CONNECTED) {
        return false;
    }
    
    Serial.print("Fetching online animation for condition: ");
    Serial.println(weatherCondition);
    
    HTTPClient http;
    http.begin(_onlineAnimationURLs[weatherCondition]);
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
                    Serial.println("Online animation data loaded successfully.");
                }
            } else {
                Serial.println("Failed to allocate memory for animation data.");
            }
        }
        
        http.end();
        return _onlineAnimationCache[weatherCondition].isLoaded;
    } else {
        Serial.print("Failed to fetch online animation, HTTP code: ");
        Serial.println(httpCode);
        http.end();
        return false;
    }
}

void WeatherAnimations::renderTFTAnimation(uint8_t weatherCondition) {
    #if defined(ESP8266) || defined(ESP32)
    if (tftDisplay == nullptr || !_onlineAnimationCache[weatherCondition].isLoaded) {
        return;
    }
    
    // This is a placeholder for the actual rendering logic
    // In a real implementation, you would parse the downloaded data
    // and render it appropriately based on its format (JPEG, BMP, raw pixels, etc.)
    
    // For now, just display a message that animation data is available
    tftDisplay->fillScreen(TFT_BLACK);
    tftDisplay->setCursor(10, 10);
    tftDisplay->setTextColor(TFT_GREEN);
    tftDisplay->setTextSize(1);
    tftDisplay->println("Animation data received");
    tftDisplay->setCursor(10, 30);
    tftDisplay->print("Size: ");
    tftDisplay->println(_onlineAnimationCache[weatherCondition].dataSize);
    
    // Display the first few bytes as hex (for debugging)
    tftDisplay->setCursor(10, 50);
    tftDisplay->println("Data preview (hex):");
    for (int i = 0; i < min(_onlineAnimationCache[weatherCondition].dataSize, (size_t)16); i++) {
        tftDisplay->print(_onlineAnimationCache[weatherCondition].imageData[i], HEX);
        tftDisplay->print(" ");
    }
    #endif
}

void WeatherAnimations::initDisplay() {
    if (_displayType == OLED_DISPLAY) {
        oledDisplay = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
        if (!oledDisplay->begin(SSD1306_SWITCHCAPVCC, _i2cAddr)) {
            Serial.println(F("SSD1306 allocation failed"));
            delete oledDisplay;
            oledDisplay = nullptr;
        }
    } else if (_displayType == TFT_DISPLAY) {
        #if defined(ESP8266) || defined(ESP32)
        tftDisplay = new TFT_eSPI();
        tftDisplay->init();
        tftDisplay->fillScreen(TFT_BLACK);
        tftDisplay->setRotation(0);
        Serial.println("TFT display initialized.");
        #else
        Serial.println("TFT display not supported on this platform.");
        #endif
    }
}

bool WeatherAnimations::setAnimationFromHACondition(const char* condition, bool isDaytime) {
    // Find the appropriate icon based on condition and time of day
    const IconMapping* icon = findWeatherIcon(condition, isDaytime);
    if (icon == nullptr) {
        Serial.println("Warning: Could not find icon for condition");
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
    
    // For OLED display, set animation directly using bitmap data
    if (_displayType == OLED_DISPLAY) {
        setAnimation(weatherCode, icon->frames, icon->frameCount, 500); // 500ms delay between frames
    }
    
    // For TFT display, set URL to fetch the icon online
    if (_displayType == TFT_DISPLAY) {
        // Generate URL based on the condition and variant
        char url[150];
        sprintf(url, "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft/%s%s%s.png", 
                condition,
                (icon->variant[0] != '\0' ? "-" : ""),
                icon->variant);
        
        setOnlineAnimationSource(weatherCode, url);
    }
    
    // Update current weather
    _currentWeather = weatherCode;
    
    return true;
}

bool WeatherAnimations::fetchWeatherData() {
    if (WiFi.status() != WL_CONNECTED) {
        if (_manageWiFi) {
            connectToWiFi();
        }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("No Wi-Fi connection available.");
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
        Serial.println("Home Assistant Response:");
        Serial.println(payload);
        
        // Parse JSON (extended parsing)
        String condition = "";
        bool isDaytime = true;
        
        // Extract condition from JSON (simplistic parsing)
        int stateStart = payload.indexOf(""state":"") + 9;
        if (stateStart > 9) {
            int stateEnd = payload.indexOf("",", stateStart);
            if (stateEnd > stateStart) {
                condition = payload.substring(stateStart, stateEnd);
            }
        }
        
        // Check for daytime attribute (if available)
        bool isDayFound = false;
        int isDayStart = payload.indexOf(""is_daytime":") + 13;
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
        
        Serial.print("Detected weather condition: ");
        Serial.println(condition);
        Serial.print("Is daytime: ");
        Serial.println(isDaytime ? "Yes" : "No");
        
        // Set animation based on the parsed condition
        setAnimationFromHACondition(condition.c_str(), isDaytime);
        
        http.end();
        return true;
    } else {
        Serial.print("HTTP Error: ");
        Serial.println(httpCode);
        http.end();
        return false;
    }
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
                        int16_t barHeight = max(1, (int16_t)(2 * (1.0f - progress)));
                        if (i % 4 < barHeight) {
                            oledDisplay->fillRect(0, i, width, 1, BLACK);
                        }
                    }
                    oledDisplay->display();
                    return;
            }
            
            // Draw the bitmap at the calculated position
            oledDisplay->drawBitmap(x, y, frameData, width, height, WHITE);
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
    } else if (_displayType == TFT_DISPLAY) {
        #if defined(ESP8266) || defined(ESP32)
        if (tftDisplay != nullptr) {
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
                    return;
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
}

 