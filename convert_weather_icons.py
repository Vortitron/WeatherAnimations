#!/usr/bin/env python3
import os
import sys
import subprocess
import re
import json
from pathlib import Path
import shutil

"""
Weather Icons Converter Script

This script converts SVG weather icons from the basmilius/weather-icons repository
into two formats:
1. PNG images for TFT displays
2. Monochrome bitmap arrays for OLED displays (128x64 pixels)

Requirements:
- Python 3.6+
- Inkscape (for SVG to PNG conversion)
- ImageMagick (for image processing)
- PIL/Pillow (Python Imaging Library)

Usage:
python3 convert_weather_icons.py /path/to/weather-icons

"""

# Try to import required libraries, install if missing
try:
    from PIL import Image
except ImportError:
    print("Installing required Python libraries...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "Pillow"])
    from PIL import Image

# Weather conditions mapping (Met.no to icon filename)
WEATHER_CONDITIONS = {
    "clear-night": "clear-night",
    "cloudy": "cloudy",
    "fog": "fog",
    "hail": "hail",
    "lightning": "thunderstorms",
    "lightning-rainy": "thunderstorms-rain",
    "partlycloudy": {"day": "partly-cloudy-day", "night": "partly-cloudy-night"},
    "pouring": "extreme-rain",
    "rainy": "rain",
    "snowy": "snow",
    "snowy-rainy": "sleet",
    "sunny": {"day": "clear-day", "night": "clear-night"},
    "windy": "wind",
    "windy-variant": "extreme-wind",
    "exceptional": "extreme"
}

# Constants
TFT_WIDTH = 240
TFT_HEIGHT = 240  # Make it square for better icon display
OLED_WIDTH = 128
OLED_HEIGHT = 64
ICON_STYLE = "line"  # Use outlined icons for better visibility on OLED

def check_dependencies():
    """Check if required external tools are installed"""
    missing_deps = []
    
    # Check Inkscape
    try:
        subprocess.run(["inkscape", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except FileNotFoundError:
        missing_deps.append("Inkscape")
    
    # Check ImageMagick
    try:
        subprocess.run(["convert", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except FileNotFoundError:
        missing_deps.append("ImageMagick")
    
    if missing_deps:
        print(f"Error: Missing dependencies: {', '.join(missing_deps)}")
        print("Please install them and try again.")
        print("- Inkscape: https://inkscape.org/release")
        print("- ImageMagick: https://imagemagick.org/script/download.php")
        sys.exit(1)

def create_output_dirs(base_path):
    """Create output directories for TFT and OLED files"""
    tft_dir = os.path.join(base_path, "production", "tft")
    oled_dir = os.path.join(base_path, "production", "oled")
    os.makedirs(tft_dir, exist_ok=True)
    os.makedirs(oled_dir, exist_ok=True)
    return tft_dir, oled_dir

def convert_svg_to_png(svg_path, output_path, width, height):
    """Convert SVG to PNG using Inkscape"""
    try:
        cmd = [
            "inkscape",
            "--export-filename", output_path,
            "--export-width", str(width),
            "--export-height", str(height),
            svg_path
        ]
        subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error converting {svg_path}: {e}")
        return False

def convert_png_to_monochrome(png_path, output_path):
    """Convert PNG to monochrome using ImageMagick"""
    try:
        cmd = [
            "convert", png_path,
            "-colorspace", "gray",
            "-colors", "2",
            "-white-threshold", "50%",
            "-black-threshold", "50%",
            "-resize", f"{OLED_WIDTH}x{OLED_HEIGHT}",
            output_path
        ]
        subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error converting to monochrome {png_path}: {e}")
        return False

def png_to_c_array(png_path, condition_name):
    """Convert monochrome PNG to C array for OLED display"""
    try:
        img = Image.open(png_path).convert('1')  # Convert to 1-bit monochrome
        width, height = img.size
        
        # Create byte array for OLED display (1 byte = 8 pixels vertically)
        byte_width = (width + 7) // 8
        byte_height = height
        bitmap_data = []
        
        for y in range(0, height, 8):
            for x in range(width):
                byte_val = 0
                for bit in range(min(8, height - y)):
                    if x < width and y + bit < height:
                        if img.getpixel((x, y + bit)) == 0:  # Black pixel (OLED on)
                            byte_val |= 1 << bit
                bitmap_data.append(byte_val)
        
        # Format as C array
        var_name = re.sub(r'[^a-zA-Z0-9]', '_', condition_name).lower()
        c_array = f"const uint8_t {var_name}Frame[{len(bitmap_data)}] PROGMEM = {{\n"
        c_array += "    "
        for i, val in enumerate(bitmap_data):
            c_array += f"0x{val:02X}, "
            if (i + 1) % 16 == 0:
                c_array += "\n    "
        c_array += "\n};\n\n"
        
        frame_ptr = f"const uint8_t* {var_name}Frames[1] = {{{var_name}Frame}};\n\n"
        
        return c_array + frame_ptr
    except Exception as e:
        print(f"Error creating C array from {png_path}: {e}")
        return ""

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} /path/to/weather-icons")
        sys.exit(1)
    
    weather_icons_path = sys.argv[1]
    if not os.path.isdir(weather_icons_path):
        print(f"Error: {weather_icons_path} is not a valid directory")
        sys.exit(1)
    
    # Check for required dependencies
    check_dependencies()
    
    # Create output directories
    tft_dir, oled_dir = create_output_dirs(weather_icons_path)
    
    # Create header file for OLED bitmaps
    header_file_path = os.path.join(os.path.dirname(weather_icons_path), "WeatherAnimationsIcons.h")
    header_content = """#ifndef WEATHER_ANIMATIONS_ICONS_H
#define WEATHER_ANIMATIONS_ICONS_H

#include <Arduino.h>

// Generated bitmap data for weather icons
// Original icons from https://github.com/basmilius/weather-icons

"""
    
    # Process each weather condition
    processed_icons = []
    for condition, icon_name in WEATHER_CONDITIONS.items():
        # Handle day/night variants
        if isinstance(icon_name, dict):
            variants = [("day", icon_name["day"]), ("night", icon_name["night"])]
        else:
            variants = [("", icon_name)]
        
        for variant_suffix, icon_filename in variants:
            # Create condition name with optional day/night suffix
            condition_full = condition + ("-" + variant_suffix if variant_suffix else "")
            
            # Construct SVG file path
            svg_filename = f"{icon_filename}.svg"
            svg_path = os.path.join(weather_icons_path, "production", ICON_STYLE, "svg", svg_filename)
            
            if not os.path.exists(svg_path):
                print(f"Warning: SVG file not found: {svg_path}")
                continue
            
            print(f"Processing: {condition_full} -> {svg_filename}")
            
            # Convert for TFT (colour PNG)
            tft_png_filename = f"{condition_full}.png"
            tft_png_path = os.path.join(tft_dir, tft_png_filename)
            convert_svg_to_png(svg_path, tft_png_path, TFT_WIDTH, TFT_HEIGHT)
            
            # Convert for OLED (monochrome bitmap)
            oled_png_filename = f"{condition_full}.png"
            oled_png_path = os.path.join(oled_dir, oled_png_filename)
            convert_svg_to_png(svg_path, oled_png_path, OLED_WIDTH, OLED_HEIGHT)
            convert_png_to_monochrome(oled_png_path, oled_png_path)
            
            # Generate C array for OLED bitmap
            c_array = png_to_c_array(oled_png_path, condition_full)
            header_content += c_array
            
            processed_icons.append({
                "condition": condition,
                "variant": variant_suffix,
                "icon_filename": icon_filename,
                "tft_png": tft_png_filename,
                "oled_png": oled_png_filename,
                "variable_name": re.sub(r'[^a-zA-Z0-9]', '_', condition_full).lower() + "Frames"
            })
    
    # Add icon mapping to header file
    header_content += "// Icon mapping structure\n"
    header_content += "struct IconMapping {\n"
    header_content += "    const char* condition;\n"
    header_content += "    const char* variant; // 'day', 'night', or empty string\n"
    header_content += "    const uint8_t** frames;\n"
    header_content += "    uint8_t frameCount;\n"
    header_content += "};\n\n"
    
    header_content += "const IconMapping weatherIcons[] = {\n"
    for icon in processed_icons:
        header_content += f"    {{\"{icon['condition']}\", \"{icon['variant']}\", {icon['variable_name']}, 1}},\n"
    header_content += "    {NULL, NULL, NULL, 0} // End marker\n};\n\n"
    
    # Generate helper function to find icon
    header_content += """// Helper function to find icon by condition and time of day
const IconMapping* findWeatherIcon(const char* condition, bool isDay) {
    const char* variant = isDay ? "day" : "night";
    
    // First try to find exact match with day/night variant
    for (size_t i = 0; weatherIcons[i].condition != NULL; i++) {
        if (strcmp(weatherIcons[i].condition, condition) == 0) {
            // If this condition has a day/night variant, match it
            if (weatherIcons[i].variant[0] != '\\0') {
                if (strcmp(weatherIcons[i].variant, variant) == 0) {
                    return &weatherIcons[i];
                }
            } else {
                // For conditions without day/night variants, return the first match
                return &weatherIcons[i];
            }
        }
    }
    
    // If no exact match with variant, return any match with the condition
    for (size_t i = 0; weatherIcons[i].condition != NULL; i++) {
        if (strcmp(weatherIcons[i].condition, condition) == 0) {
            return &weatherIcons[i];
        }
    }
    
    // Fallback to cloudy if no match
    for (size_t i = 0; weatherIcons[i].condition != NULL; i++) {
        if (strcmp(weatherIcons[i].condition, "cloudy") == 0) {
            return &weatherIcons[i];
        }
    }
    
    // If all else fails, return the first icon
    return &weatherIcons[0];
}
"""
    
    header_content += "#endif // WEATHER_ANIMATIONS_ICONS_H\n"
    
    # Write the header file
    with open(header_file_path, 'w') as f:
        f.write(header_content)
    
    # Create JSON mapping file for URL-based fetching
    mapping_file = os.path.join(os.path.dirname(weather_icons_path), "weather_icon_urls.json")
    url_base = "https://raw.githubusercontent.com/vortitron/weather-icons/main/production"
    url_mapping = {
        "tft": {},
        "oled": {}
    }
    
    for icon in processed_icons:
        condition = icon["condition"]
        variant = icon["variant"]
        
        # Create keys in the format "condition" or "condition-day"/"condition-night"
        key = condition if not variant else f"{condition}-{variant}"
        
        # Add TFT and OLED URLs
        url_mapping["tft"][key] = f"{url_base}/tft/{icon['tft_png']}"
        url_mapping["oled"][key] = f"{url_base}/oled/{icon['oled_png']}"
    
    with open(mapping_file, 'w') as f:
        json.dump(url_mapping, f, indent=2)
    
    print(f"\nProcessed {len(processed_icons)} icons.")
    print(f"Generated C header file: {header_file_path}")
    print(f"Generated URL mapping: {mapping_file}")
    print(f"PNG files for TFT are in: {tft_dir}")
    print(f"Monochrome PNGs for OLED are in: {oled_dir}")
    print("\nTo use these icons:")
    print("1. Include the generated header file in your project")
    print("2. Use the findWeatherIcon() function to get bitmap data for OLED")
    print("3. Use the URL mapping for TFT display online fetching")

if __name__ == "__main__":
    main() 