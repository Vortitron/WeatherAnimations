#!/usr/bin/env python3
import os
import sys
import subprocess
import re
import json
import xml.etree.ElementTree as ET
import tempfile
from pathlib import Path
import shutil

"""
Animated Weather Icons Converter Script

This script extracts animation frames from SVG weather icons and converts them to:
1. Animated GIF files for TFT displays
2. Sequences of bitmap frames for OLED displays

Requirements:
- Python 3.6+
- Inkscape (for SVG to PNG conversion)
- ImageMagick (for image processing and GIF creation)
- PIL/Pillow (Python Imaging Library)
- cairosvg (for SVG manipulation)

Usage:
python3 convert_animated_weather_icons.py /path/to/weather-icons

"""

# Try to import required libraries, install if missing
try:
    from PIL import Image
    import cairosvg
except ImportError:
    print("Installing required Python libraries...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "Pillow", "cairosvg"])
    from PIL import Image
    import cairosvg

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
TFT_HEIGHT = 240
OLED_WIDTH = 128
OLED_HEIGHT = 64
ICON_STYLE = "line"  # Use outlined icons for better visibility on OLED
FRAME_COUNT = 10     # Number of frames to extract per animation
ANIM_DURATION = 2000 # Total animation duration in ms

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
    tft_dir = os.path.join(base_path, "production", "tft_animated")
    oled_dir = os.path.join(base_path, "production", "oled_animated")
    os.makedirs(tft_dir, exist_ok=True)
    os.makedirs(oled_dir, exist_ok=True)
    return tft_dir, oled_dir

def get_svg_animation_params(svg_path):
    """Extract animation parameters from SVG file"""
    try:
        # Parse the SVG file
        with open(svg_path, 'r') as f:
            svg_content = f.read()
        
        # Find animation elements with durations
        animations = re.findall(r'<animate[^>]*dur="([^"]*)"', svg_content)
        if not animations:
            animations = re.findall(r'<animateTransform[^>]*dur="([^"]*)"', svg_content)
        
        # Parse duration (e.g., "2s" or "500ms")
        if animations:
            dur_str = animations[0]
            if 's' in dur_str:
                duration_ms = float(dur_str.replace('s', '')) * 1000
            elif 'ms' in dur_str:
                duration_ms = float(dur_str.replace('ms', ''))
            else:
                duration_ms = ANIM_DURATION  # Default
            return int(duration_ms)
        
        return ANIM_DURATION  # Default if no animation found
    except Exception as e:
        print(f"Error extracting animation params from {svg_path}: {e}")
        return ANIM_DURATION

def extract_svg_frames(svg_path, output_dir, frame_count):
    """
    Extract animation frames from an SVG file
    Returns a list of temporary PNG file paths
    """
    try:
        # Get animation duration
        duration_ms = get_svg_animation_params(svg_path)
        frame_paths = []
        
        # Create temporary directory for frames
        with tempfile.TemporaryDirectory() as temp_dir:
            # Extract frames using Inkscape's ability to specify animation time
            for i in range(frame_count):
                # Calculate time point in the animation
                time_ms = (i * duration_ms) / frame_count
                
                # Frame output path
                frame_path = os.path.join(temp_dir, f"frame_{i:03d}.png")
                frame_paths.append(frame_path)
                
                # Use Inkscape to render the SVG at this specific animation time
                cmd = [
                    "inkscape",
                    "--export-filename", frame_path,
                    "--export-width", str(TFT_WIDTH),
                    "--export-height", str(TFT_HEIGHT),
                    "--export-background", "transparent",
                    f"--export-frame={time_ms/1000}",  # Time in seconds
                    svg_path
                ]
                subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            
            return frame_paths, duration_ms
    except Exception as e:
        print(f"Error extracting SVG frames from {svg_path}: {e}")
        return [], ANIM_DURATION

def create_animated_gif(frame_paths, output_path, duration_ms):
    """Create animated GIF from extracted frames"""
    try:
        # Calculate delay between frames in 1/100 of a second
        delay = int((duration_ms / len(frame_paths)) / 10)
        delay = max(2, delay)  # Ensure minimum delay (ImageMagick requires at least 2)
        
        # Construct ImageMagick command to create animated GIF
        cmd = ["convert", "-loop", "0", "-delay", str(delay)]
        cmd.extend(frame_paths)
        cmd.append(output_path)
        
        subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return True
    except Exception as e:
        print(f"Error creating animated GIF: {e}")
        return False

def convert_frames_to_monochrome(frame_paths, output_dir, base_name):
    """Convert frames to monochrome and resize for OLED"""
    monochrome_paths = []
    
    try:
        for i, frame_path in enumerate(frame_paths):
            output_path = os.path.join(output_dir, f"{base_name}_frame_{i:03d}.png")
            monochrome_paths.append(output_path)
            
            # Convert to monochrome and resize
            cmd = [
                "convert", frame_path,
                "-colorspace", "gray",
                "-colors", "2",
                "-white-threshold", "50%",
                "-black-threshold", "50%",
                "-resize", f"{OLED_WIDTH}x{OLED_HEIGHT}",
                output_path
            ]
            subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
        return monochrome_paths
    except Exception as e:
        print(f"Error converting frames to monochrome: {e}")
        return []

def frames_to_c_arrays(frame_paths, condition_name):
    """Convert monochrome PNG frames to C arrays for OLED display"""
    try:
        # C array output
        var_name = re.sub(r'[^a-zA-Z0-9]', '_', condition_name).lower()
        c_arrays = []
        
        # Create individual frame arrays
        for i, frame_path in enumerate(frame_paths):
            img = Image.open(frame_path).convert('1')  # Convert to 1-bit monochrome
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
            c_array = f"const uint8_t {var_name}Frame{i}[{len(bitmap_data)}] PROGMEM = {{\n"
            c_array += "    "
            for j, val in enumerate(bitmap_data):
                c_array += f"0x{val:02X}, "
                if (j + 1) % 16 == 0:
                    c_array += "\n    "
            c_array += "\n};\n\n"
            c_arrays.append(c_array)
        
        # Create array of frame pointers
        frame_ptrs = f"const uint8_t* {var_name}Frames[{len(frame_paths)}] = {{"
        for i in range(len(frame_paths)):
            if i > 0:
                frame_ptrs += ", "
            frame_ptrs += f"{var_name}Frame{i}"
        frame_ptrs += "}};\n\n"
        
        return "".join(c_arrays) + frame_ptrs, len(frame_paths)
    except Exception as e:
        print(f"Error creating C arrays from frames: {e}")
        return "", 0

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
    
    # Create header file for OLED bitmap frames
    header_file_path = os.path.join(os.path.dirname(weather_icons_path), "WeatherAnimationsAnimatedIcons.h")
    header_content = """#ifndef WEATHER_ANIMATIONS_ANIMATED_ICONS_H
#define WEATHER_ANIMATIONS_ANIMATED_ICONS_H

#include <Arduino.h>

// Generated animated bitmap data for weather icons
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
            
            print(f"Processing animation: {condition_full} -> {svg_filename}")
            
            # Extract frames from SVG
            frame_paths, duration_ms = extract_svg_frames(svg_path, tft_dir, FRAME_COUNT)
            if not frame_paths:
                print(f"  Skipping {condition_full} - could not extract frames")
                continue
            
            # Create animated GIF for TFT
            tft_gif_filename = f"{condition_full}.gif"
            tft_gif_path = os.path.join(tft_dir, tft_gif_filename)
            if create_animated_gif(frame_paths, tft_gif_path, duration_ms):
                print(f"  Created animated GIF: {tft_gif_path}")
            
            # Convert frames to monochrome for OLED
            monochrome_paths = convert_frames_to_monochrome(frame_paths, oled_dir, condition_full)
            if monochrome_paths:
                print(f"  Created {len(monochrome_paths)} monochrome frames for OLED")
                
                # Generate C arrays for OLED frames
                c_arrays, frame_count = frames_to_c_arrays(monochrome_paths, condition_full)
                header_content += c_arrays
                
                processed_icons.append({
                    "condition": condition,
                    "variant": variant_suffix,
                    "icon_filename": icon_filename,
                    "tft_gif": tft_gif_filename,
                    "oled_frame_count": frame_count,
                    "variable_name": re.sub(r'[^a-zA-Z0-9]', '_', condition_full).lower() + "Frames",
                    "frame_delay": int(duration_ms / frame_count)
                })
    
    # Add icon mapping to header file
    header_content += "// Animated icon mapping structure\n"
    header_content += "struct AnimatedIconMapping {\n"
    header_content += "    const char* condition;\n"
    header_content += "    const char* variant; // 'day', 'night', or empty string\n"
    header_content += "    const uint8_t** frames;\n"
    header_content += "    uint8_t frameCount;\n"
    header_content += "    uint16_t frameDelay; // ms between frames\n"
    header_content += "};\n\n"
    
    header_content += "const AnimatedIconMapping animatedWeatherIcons[] = {\n"
    for icon in processed_icons:
        header_content += f"    {{\"{icon['condition']}\", \"{icon['variant']}\", {icon['variable_name']}, {icon['oled_frame_count']}, {icon['frame_delay']}}},\n"
    header_content += "    {NULL, NULL, NULL, 0, 0} // End marker\n};\n\n"
    
    # Generate helper function to find icon
    header_content += """// Helper function to find animated icon by condition and time of day
const AnimatedIconMapping* findAnimatedWeatherIcon(const char* condition, bool isDay) {
    const char* variant = isDay ? "day" : "night";
    
    // First try to find exact match with day/night variant
    for (size_t i = 0; animatedWeatherIcons[i].condition != NULL; i++) {
        if (strcmp(animatedWeatherIcons[i].condition, condition) == 0) {
            // If this condition has a day/night variant, match it
            if (animatedWeatherIcons[i].variant[0] != '\\0') {
                if (strcmp(animatedWeatherIcons[i].variant, variant) == 0) {
                    return &animatedWeatherIcons[i];
                }
            } else {
                // For conditions without day/night variants, return the first match
                return &animatedWeatherIcons[i];
            }
        }
    }
    
    // If no exact match with variant, return any match with the condition
    for (size_t i = 0; animatedWeatherIcons[i].condition != NULL; i++) {
        if (strcmp(animatedWeatherIcons[i].condition, condition) == 0) {
            return &animatedWeatherIcons[i];
        }
    }
    
    // Fallback to cloudy if no match
    for (size_t i = 0; animatedWeatherIcons[i].condition != NULL; i++) {
        if (strcmp(animatedWeatherIcons[i].condition, "cloudy") == 0) {
            return &animatedWeatherIcons[i];
        }
    }
    
    // If all else fails, return the first icon
    return &animatedWeatherIcons[0];
}
"""
    
    header_content += "#endif // WEATHER_ANIMATIONS_ANIMATED_ICONS_H\n"
    
    # Write the header file
    with open(header_file_path, 'w') as f:
        f.write(header_content)
    
    # Create JSON mapping file for URL-based fetching
    mapping_file = os.path.join(os.path.dirname(weather_icons_path), "animated_weather_icon_urls.json")
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
        
        # Add TFT GIF URL
        url_mapping["tft"][key] = f"{url_base}/tft_animated/{icon['tft_gif']}"
        
        # Add OLED frame URLs (for devices that can't use the C arrays)
        url_mapping["oled"][key] = []
        for i in range(icon["oled_frame_count"]):
            frame_url = f"{url_base}/oled_animated/{key}_frame_{i:03d}.png"
            url_mapping["oled"][key].append(frame_url)
    
    with open(mapping_file, 'w') as f:
        json.dump(url_mapping, f, indent=2)
    
    print(f"\nProcessed {len(processed_icons)} animated icons.")
    print(f"Generated C header file: {header_file_path}")
    print(f"Generated URL mapping: {mapping_file}")
    print(f"Animated GIFs for TFT are in: {tft_dir}")
    print(f"Frame sequences for OLED are in: {oled_dir}")
    print("\nTo use these animated icons:")
    print("1. Include the generated header file in your project")
    print("2. Use the findAnimatedWeatherIcon() function to get bitmap data for OLED")
    print("3. Use the URL mapping for TFT display online fetching of GIFs")

if __name__ == "__main__":
    main() 