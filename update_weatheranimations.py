#!/usr/bin/env python3
import os
import sys
import json
import re
import shutil

"""
WeatherAnimations Integration Helper

This script helps integrate the converted weather icons with the WeatherAnimations library.
It generates code to replace placeholder animations in WeatherAnimationsAnimations.cpp
and creates a new method for handling Home Assistant weather states.

Usage:
python3 update_weatheranimations.py /path/to/WeatherAnimations /path/to/weather_icon_urls.json

"""

def update_header_file(src_dir, icon_header_path):
    """
    Update WeatherAnimationsAnimations.h to include the new icons header
    """
    header_path = os.path.join(src_dir, "WeatherAnimationsAnimations.h")
    
    if not os.path.exists(header_path):
        print(f"Error: {header_path} not found")
        return False
    
    with open(header_path, 'r') as f:
        content = f.read()
    
    # Check if already includes the icons header
    if "WeatherAnimationsIcons.h" in content:
        print("Icons header already included in WeatherAnimationsAnimations.h")
        return True
    
    # Add include for icons header after the Arduino.h include
    new_content = re.sub(
        r'(#include\s+<Arduino\.h>)',
        r'\1\n#include "WeatherAnimationsIcons.h"',
        content
    )
    
    # Add extern declarations for the WeatherAnimationsIcons.h variables
    if "#endif" in new_content:
        extern_code = """
// Import icon mapping from WeatherAnimationsIcons.h
extern const IconMapping weatherIcons[];
extern const IconMapping* findWeatherIcon(const char* condition, bool isDay);
"""
        new_content = new_content.replace("#endif", extern_code + "\n#endif")
    
    with open(header_path, 'w') as f:
        f.write(new_content)
    
    print(f"Updated {header_path} to include the icons header")
    return True

def update_impl_file(src_dir, url_mapping):
    """
    Update WeatherAnimations.cpp to use the new weather icons
    """
    cpp_path = os.path.join(src_dir, "WeatherAnimations.cpp")
    
    if not os.path.exists(cpp_path):
        print(f"Error: {cpp_path} not found")
        return False
    
    with open(cpp_path, 'r') as f:
        content = f.read()
    
    # Add new method to set animation sources based on Home Assistant condition
    set_animation_method = """
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
                (icon->variant[0] != '\\0' ? "-" : ""),
                icon->variant);
        
        setOnlineAnimationSource(weatherCode, url);
    }
    
    // Update current weather
    _currentWeather = weatherCode;
    
    return true;
}
"""
    
    # Update fetchWeatherData method to use the new setAnimationFromHACondition method
    updated_fetch_weather = """
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
"""
    
    # Find location to insert new method (before the closing brace of the class)
    if "void WeatherAnimations::initDisplay()" in content:
        # Add new method after the last method in the class
        parts = content.split("void WeatherAnimations::initDisplay()")
        if len(parts) == 2:
            # Find the end of the initDisplay method
            method_parts = parts[1].split("}")
            
            # Add our new method and updated fetchWeatherData method
            modified_content = parts[0] + "void WeatherAnimations::initDisplay()" + method_parts[0] + "}\n\n"
            modified_content += set_animation_method + "\n\n" + updated_fetch_weather
            
            # Add back the remaining content
            modified_content += "".join(method_parts[2:])
            
            with open(cpp_path, 'w') as f:
                f.write(modified_content)
                
            print(f"Updated {cpp_path} with new setAnimationFromHACondition method")
            return True
    
    print(f"Could not update {cpp_path} - structure not as expected")
    return False

def update_header_declaration(src_dir):
    """
    Update WeatherAnimations.h to add declaration for the new method
    """
    header_path = os.path.join(src_dir, "WeatherAnimations.h")
    
    if not os.path.exists(header_path):
        print(f"Error: {header_path} not found")
        return False
    
    with open(header_path, 'r') as f:
        content = f.read()
    
    # Check if the method is already declared
    if "setAnimationFromHACondition" in content:
        print("Method already declared in WeatherAnimations.h")
        return True
    
    # Add method declaration to private section
    if "private:" in content:
        private_section = content.split("private:")[1]
        methods_section = private_section.split("};")[0]
        
        # Add our new method declaration before the last closing brace
        new_method_decl = """
    // Set animation based on Home Assistant weather condition
    bool setAnimationFromHACondition(const char* condition, bool isDaytime);
"""
        
        modified_content = content.replace(methods_section, methods_section + new_method_decl)
        
        with open(header_path, 'w') as f:
            f.write(modified_content)
            
        print(f"Updated {header_path} with new method declaration")
        return True
    
    print(f"Could not update {header_path} - structure not as expected")
    return False

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} /path/to/WeatherAnimations /path/to/weather_icon_urls.json")
        sys.exit(1)
    
    weather_animations_path = sys.argv[1]
    url_mapping_path = sys.argv[2]
    
    src_dir = os.path.join(weather_animations_path, "src")
    
    if not os.path.isdir(src_dir):
        print(f"Error: Source directory {src_dir} not found")
        sys.exit(1)
    
    if not os.path.exists(url_mapping_path):
        print(f"Error: URL mapping file {url_mapping_path} not found")
        sys.exit(1)
    
    with open(url_mapping_path, 'r') as f:
        url_mapping = json.load(f)
    
    # Copy the WeatherAnimationsIcons.h file to the src directory
    icon_header_path = os.path.join(os.path.dirname(url_mapping_path), "WeatherAnimationsIcons.h")
    if os.path.exists(icon_header_path):
        shutil.copy(icon_header_path, src_dir)
        print(f"Copied {icon_header_path} to {src_dir}")
    else:
        print(f"Warning: {icon_header_path} not found, please run convert_weather_icons.py first")
    
    # Update header files and implementation
    update_header_file(src_dir, icon_header_path)
    update_header_declaration(src_dir)
    update_impl_file(src_dir, url_mapping)
    
    print("\nIntegration complete!")
    print("Here's how to use the new functionality:")
    print("1. The library now supports setting animations directly from Home Assistant weather conditions")
    print("2. It automatically detects day/night and selects the appropriate icon")
    print("3. For TFT displays, icons are fetched from your GitHub repository")
    print("4. For OLED displays, embedded bitmaps are used for better performance")
    print("\nMake sure to update the README.md to document these changes!")

if __name__ == "__main__":
    main() 