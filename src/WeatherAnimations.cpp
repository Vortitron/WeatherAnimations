#include "WeatherAnimations.h"
#include <Adafruit_SSD1306.h>

// Default OLED dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Global display object (will be initialized based on display type)
Adafruit_SSD1306* oledDisplay = nullptr;

WeatherAnimations::WeatherAnimations(const char* ssid, const char* password, const char* haIP, const char* haToken)
    : _ssid(ssid), _password(password), _haIP(haIP), _haToken(haToken),
      _displayType(OLED_DISPLAY), _i2cAddr(0x3C), _mode(CONTINUOUS_WEATHER),
      _manageWiFi(true), _currentWeather(WEATHER_CLEAR), _weatherEntityID("weather.forecast"),
      _lastFetchTime(0), _fetchCooldown(300000) { // 5 minutes cooldown
    // Initialize animations array
    for (int i = 0; i < 5; i++) {
        _animations[i].frames = nullptr;
        _animations[i].frameCount = 0;
        _animations[i].frameDelay = 0;
    }
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
    }
    http.end();
    return false;
}

void WeatherAnimations::displayAnimation() {
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
                    // Could switch to a static display or clear
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
    }
    // Add TFT support here when needed
}

void WeatherAnimations::initDisplay() {
    if (_displayType == OLED_DISPLAY) {
        oledDisplay = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
        if (!oledDisplay->begin(SSD1306_SWITCHCAPVCC, _i2cAddr)) {
            Serial.println(F("SSD1306 allocation failed"));
            delete oledDisplay;
            oledDisplay = nullptr;
        } else {
            oledDisplay->clearDisplay();
            oledDisplay->display();
        }
    }
    // Add TFT initialization when needed
} 