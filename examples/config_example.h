/*
 * Configuration Example for WeatherAnimations Library
 * 
 * HOW TO USE:
 * 1. Copy this file to "config.h" in the same directory as your sketch
 * 2. Add "#include "config.h"" to your sketch
 * 3. Edit "config.h" with your personal settings
 * 4. Add "config.h" to your .gitignore file to keep credentials private
 */

#ifndef WEATHER_ANIMATIONS_CONFIG_H
#define WEATHER_ANIMATIONS_CONFIG_H

// WiFi credentials
#define WIFI_SSID       "YourWiFiSSID"
#define WIFI_PASSWORD   "YourWiFiPassword"

// Home Assistant settings
#define HA_IP           "YourHomeAssistantIP"  // e.g., "192.168.1.100"
#define HA_PORT         8123                   // Default Home Assistant port
#define HA_TOKEN        "YourHomeAssistantToken"
#define HA_WEATHER_ENTITY "weather.forecast_home"  // Your weather entity ID

// Optional: Additional Home Assistant entities
#define HA_TEMPERATURE_ENTITY "sensor.outside_temperature"
#define HA_HUMIDITY_ENTITY    "sensor.outside_humidity"
#define HA_PRESSURE_ENTITY    "sensor.outside_pressure"

// Display settings
#define OLED_ADDRESS    0x3C    // I2C address for OLED display (typically 0x3C or 0x3D)

// Animation URLs (for TFT display)
// You can replace these with your own URLs or keep the defaults
#define ANIM_CLEAR_DAY      "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/sunny-day.gif"
#define ANIM_CLEAR_NIGHT    "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/clear-night.gif"
#define ANIM_CLOUDY         "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/cloudy.gif"
#define ANIM_FOG            "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/fog.gif"
#define ANIM_PARTLYCLOUDY_DAY   "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/partlycloudy-day.gif"
#define ANIM_PARTLYCLOUDY_NIGHT "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/partlycloudy-night.gif"
#define ANIM_RAIN           "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/rainy.gif"
#define ANIM_HEAVY_RAIN     "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/pouring.gif"
#define ANIM_SNOW           "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/snowy.gif"
#define ANIM_STORM          "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/lightning.gif"
#define ANIM_STORM_RAIN     "https://raw.githubusercontent.com/vortitron/weather-icons/main/production/tft_animated/lightning-rainy.gif"

#endif // WEATHER_ANIMATIONS_CONFIG_H 