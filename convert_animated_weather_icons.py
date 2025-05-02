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
import math

# Enable detailed debug output
DEBUG = True

def debug(message):
    if DEBUG:
        print(f"DEBUG: {message}")

print("Starting animated weather icons conversion...")

# Try to import required libraries, install if missing
try:
    from PIL import Image
    try:
        from PIL.Image import LANCZOS
    except ImportError:
        # For older versions of Pillow
        LANCZOS = Image.ANTIALIAS
    import cairosvg
    print("Successfully imported all required libraries")
except ImportError as e:
    print(f"Import error: {e}")
    print("Installing required Python libraries...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "Pillow", "cairosvg"])
        from PIL import Image
        try:
            from PIL.Image import LANCZOS
        except ImportError:
            LANCZOS = Image.ANTIALIAS
        import cairosvg
        print("Successfully installed and imported all required libraries")
    except Exception as e:
        print(f"Failed to install required libraries: {e}")
        sys.exit(1)

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
    
    # Check ImageMagick
    try:
        subprocess.run(["convert", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except FileNotFoundError:
        missing_deps.append("ImageMagick")
    
    if missing_deps:
        print(f"Error: Missing dependencies: {', '.join(missing_deps)}")
        print("Please install them and try again.")
        print("- ImageMagick: https://imagemagick.org/script/download.php")
        sys.exit(1)

def create_output_dirs(base_path):
    """Create output directories for TFT and OLED files"""
    tft_dir = os.path.join(base_path, "production", "tft_animated")
    oled_dir = os.path.join(base_path, "production", "oled_animated")
    temp_dir = os.path.join(base_path, "production", "temp_frames")
    os.makedirs(tft_dir, exist_ok=True)
    os.makedirs(oled_dir, exist_ok=True)
    os.makedirs(temp_dir, exist_ok=True)
    return tft_dir, oled_dir, temp_dir

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

def extract_svg_frames(svg_path, output_dir, frame_count, permanent_temp_dir):
    """
    Extract animation frames from an SVG file
    Returns a list of permanent PNG file paths
    """
    try:
        # Get animation duration
        duration_ms = get_svg_animation_params(svg_path)
        frame_paths = []
        
        # Create a unique subfolder name for this icon
        icon_name = os.path.splitext(os.path.basename(svg_path))[0]
        icon_temp_dir = os.path.join(permanent_temp_dir, icon_name)
        os.makedirs(icon_temp_dir, exist_ok=True)
        
        # Create temporary directory for frames
        with tempfile.TemporaryDirectory() as temp_dir:
            # First convert SVG to PNG at full size
            base_png = os.path.join(temp_dir, "base.png")
            
            print(f"Converting SVG to PNG: {svg_path} -> {base_png}")
            try:
                # Use cairosvg to convert SVG to PNG
                cairosvg.svg2png(url=svg_path, write_to=base_png, 
                                output_width=TFT_WIDTH, output_height=TFT_HEIGHT)
                
                if not os.path.exists(base_png):
                    print(f"Error: Base PNG file not created at {base_png}")
                    return [], duration_ms
                
                print(f"Base PNG created successfully at {base_png}")
            except Exception as e:
                print(f"Error converting SVG to PNG with cairosvg: {e}")
                return [], duration_ms
            
            # Load the base image
            base_image = Image.open(base_png)
            print(f"Base image size: {base_image.size}")
            
            # Generate frames using a simple pulsing effect for all weather icons
            for i in range(frame_count):
                # Frame output path (temporary)
                temp_frame_path = os.path.join(temp_dir, f"frame_{i:03d}.png")
                
                # Apply a simple animation effect based on frame number
                progress = i / (frame_count - 1) if frame_count > 1 else 0  # 0.0 to 1.0
                
                # Use a simple fade in/out pulsing effect for all icons
                try:
                    # Vary opacity for a pulsing effect
                    opacity = int(155 + 100 * math.sin(progress * 2 * math.pi))
                    opacity = max(155, min(255, opacity))  # Keep between 155-255 for visibility
                    
                    # Create a copy with the desired opacity
                    frame = Image.new("RGBA", base_image.size, (0, 0, 0, 0))
                    
                    # Get RGBA data
                    base_data = base_image.convert("RGBA")
                    
                    # Apply a slight position shift for more movement
                    offset_x = int(5 * math.sin(progress * 2 * math.pi))
                    offset_y = int(5 * math.cos(progress * 2 * math.pi))
                    
                    # Paste with the calculated offset
                    frame.paste(base_data, (offset_x, offset_y), base_data)
                    
                    # Save the frame to temp location
                    frame.save(temp_frame_path)
                    
                    # Now copy to permanent location
                    permanent_frame_path = os.path.join(icon_temp_dir, f"frame_{i:03d}.png")
                    shutil.copy2(temp_frame_path, permanent_frame_path)
                    
                    print(f"Saved frame {i} to {permanent_frame_path}")
                    
                    # Verify the frame was saved
                    if os.path.exists(permanent_frame_path):
                        frame_paths.append(permanent_frame_path)
                    else:
                        print(f"Error: Frame file not created at {permanent_frame_path}")
                        
                except Exception as e:
                    print(f"Error creating frame {i}: {e}")
            
            print(f"Created {len(frame_paths)} frames for {svg_path}")
            return frame_paths, duration_ms
    except Exception as e:
        print(f"Error extracting SVG frames from {svg_path}: {e}")
        return [], ANIM_DURATION

def create_animated_gif(frame_paths, output_path, duration_ms):
    """Create animated GIF from extracted frames"""
    try:
        # Check if we have any frames
        if not frame_paths:
            print("No frames to create GIF from")
            return False
            
        # Check if all frames exist
        for frame_path in frame_paths:
            if not os.path.exists(frame_path):
                print(f"Frame file not found: {frame_path}")
                return False
                
        # Calculate delay between frames in 1/100 of a second
        delay = int((duration_ms / len(frame_paths)) / 10)
        delay = max(2, delay)  # Ensure minimum delay (ImageMagick requires at least 2)
        
        # Construct ImageMagick command to create animated GIF
        cmd = ["convert", "-loop", "0", "-delay", str(delay)]
        cmd.extend(frame_paths)
        cmd.append(output_path)
        
        # Create the output directory if it doesn't exist
        output_dir = os.path.dirname(output_path)
        os.makedirs(output_dir, exist_ok=True)
        
        # Print the command for debugging
        print("Running convert command:", " ".join(cmd))
        
        result = subprocess.run(cmd, check=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if result.returncode != 0:
            print(f"Convert command failed with code {result.returncode}")
            print(f"Error output: {result.stderr.decode('utf-8')}")
            return False
            
        # Verify the GIF was created
        if os.path.exists(output_path):
            print(f"Successfully created GIF: {output_path}")
            return True
        else:
            print(f"Failed to create GIF: {output_path}")
            return False
    except Exception as e:
        print(f"Error creating animated GIF: {e}")
        return False

def convert_frames_to_monochrome(frame_paths, output_dir, base_name):
    """Convert frames to monochrome and resize for OLED"""
    monochrome_paths = []
    
    try:
        # Check if we have any frames
        if not frame_paths:
            print("No frames to convert to monochrome")
            return []
            
        # Create the output directory if it doesn't exist
        os.makedirs(output_dir, exist_ok=True)
        
        for i, frame_path in enumerate(frame_paths):
            # Check if source frame exists
            if not os.path.exists(frame_path):
                print(f"Frame file not found: {frame_path}")
                continue
                
            output_path = os.path.join(output_dir, f"{base_name}_frame_{i:03d}.png")
            
            try:
                # Use PIL directly instead of ImageMagick for better compatibility
                img = Image.open(frame_path)
                
                # Convert to grayscale and resize
                img = img.convert("L")  # Convert to grayscale
                img = img.resize((OLED_WIDTH, OLED_HEIGHT), LANCZOS)
                
                # Threshold to true monochrome (binary)
                threshold = 128
                img = img.point(lambda x: 255 if x > threshold else 0, mode='1')
                
                # Save the monochrome image
                img.save(output_path)
                
                if os.path.exists(output_path):
                    print(f"Successfully converted to monochrome: {output_path}")
                    monochrome_paths.append(output_path)
                else:
                    print(f"Failed to save monochrome image: {output_path}")
                    
            except Exception as e:
                print(f"Error processing frame {i}: {e}")
                continue
        
        return monochrome_paths
    except Exception as e:
        print(f"Error converting frames to monochrome: {e}")
        return []

def frames_to_c_arrays(frame_paths, condition_name):
    """Convert monochrome PNG frames to C arrays for OLED display in the format needed by WeatherAnimations"""
    try:
        # C array output
        var_name = re.sub(r'[^a-zA-Z0-9]', '_', condition_name).lower()
        c_arrays = []
        
        # Create individual frame arrays
        for i, frame_path in enumerate(frame_paths):
            img = Image.open(frame_path).convert('1')  # Convert to 1-bit monochrome
            width, height = img.size
            
            # OLED displays need 1024 bytes for 128x64 display (128*64/8)
            # Each byte represents 8 vertical pixels
            byte_width = math.ceil(width / 8)
            byte_height = height
            bitmap_size = 1024  # Fixed size for the WeatherAnimations library
            bitmap_data = [0] * bitmap_size  # Initialize with zeros
            
            for y in range(0, height):
                for x in range(0, width):
                    # For OLED, each byte is 8 vertical pixels
                    # Calculate which byte in the array we're updating
                    byte_index = (y // 8) * width + x
                    
                    # Only process if within bounds
                    if byte_index < bitmap_size:
                        # If pixel is black (0 in monochrome), set the bit
                        # OLED displays use 1 for lit pixels, 0 for off
                        if img.getpixel((x, y)) == 0:  # Black pixel
                            bit_position = y % 8
                            bitmap_data[byte_index] |= (1 << bit_position)
            
            # Format as C array with PROGMEM directive for Arduino
            c_array = f"const uint8_t {var_name}Frame{i+1}[1024] PROGMEM = {{\n"
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
            frame_ptrs += f"{var_name}Frame{i+1}"
        frame_ptrs += "};\n\n"
        
        return "".join(c_arrays) + frame_ptrs, len(frame_paths)
    except Exception as e:
        print(f"Error creating C arrays from frames: {e}")
        return "", 0

def main():
    print("Starting main function...")
    
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} /path/to/weather-icons")
        sys.exit(1)
    
    weather_icons_path = sys.argv[1]
    print(f"Using weather icons path: {weather_icons_path}")
    
    if not os.path.isdir(weather_icons_path):
        print(f"Error: {weather_icons_path} is not a valid directory")
        sys.exit(1)
    
    # Check for required dependencies
    print("Checking dependencies...")
    check_dependencies()
    
    # Create output directories
    print("Creating output directories...")
    tft_dir, oled_dir, temp_frames_dir = create_output_dirs(weather_icons_path)
    print(f"TFT directory: {tft_dir}")
    print(f"OLED directory: {oled_dir}")
    print(f"Temp frames directory: {temp_frames_dir}")
    
    # Create header file for OLED bitmap frames
    header_file_path = os.path.join(os.path.dirname(weather_icons_path), "../src", "WeatherAnimationsAnimatedIcons.h")
    print(f"Will create header file at: {header_file_path}")
    header_content = """#ifndef WEATHER_ANIMATIONS_ANIMATED_ICONS_H
#define WEATHER_ANIMATIONS_ANIMATED_ICONS_H

#include <Arduino.h>

// Generated animated bitmap data for weather icons
// Each frame is 128x64 pixels, stored as 1024 bytes (128*64/8)
// Original icons from https://github.com/basmilius/weather-icons

"""
    
    # Additional data for the WeatherAnimations library
    header_content += """// Animation frame counts for each weather type
#define WEATHER_CLEAR_FRAME_COUNT 10
#define WEATHER_CLOUDY_FRAME_COUNT 10
#define WEATHER_RAIN_FRAME_COUNT 10
#define WEATHER_SNOW_FRAME_COUNT 10
#define WEATHER_STORM_FRAME_COUNT 10

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
            frame_paths, duration_ms = extract_svg_frames(svg_path, tft_dir, FRAME_COUNT, temp_frames_dir)
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
    
    # At the end, add compatibility functions for the library
    header_content += """
// Function to get animation frame for a given weather type and frame number
const uint8_t* getAnimationFrame(uint8_t weatherType, uint8_t frameIndex) {
    switch (weatherType) {
        case 0: // WEATHER_CLEAR
            if (frameIndex < WEATHER_CLEAR_FRAME_COUNT) {
                return sunny_dayFrames[frameIndex];
            }
            break;
        case 1: // WEATHER_CLOUDY
            if (frameIndex < WEATHER_CLOUDY_FRAME_COUNT) {
                return cloudyFrames[frameIndex];
            }
            break;
        case 2: // WEATHER_RAIN
            if (frameIndex < WEATHER_RAIN_FRAME_COUNT) {
                return rainyFrames[frameIndex];
            }
            break;
        case 3: // WEATHER_SNOW
            if (frameIndex < WEATHER_SNOW_FRAME_COUNT) {
                return snowyFrames[frameIndex];
            }
            break;
        case 4: // WEATHER_STORM
            if (frameIndex < WEATHER_STORM_FRAME_COUNT) {
                return lightningFrames[frameIndex];
            }
            break;
    }
    
    // Default to first frame of cloudy if nothing matches
    return cloudyFrames[0];
}

// Function to get frame count for a weather type
uint8_t getAnimationFrameCount(uint8_t weatherType) {
    switch (weatherType) {
        case 0: return WEATHER_CLEAR_FRAME_COUNT;
        case 1: return WEATHER_CLOUDY_FRAME_COUNT;
        case 2: return WEATHER_RAIN_FRAME_COUNT;
        case 3: return WEATHER_SNOW_FRAME_COUNT;
        case 4: return WEATHER_STORM_FRAME_COUNT;
        default: return WEATHER_CLOUDY_FRAME_COUNT;
    }
}

#endif // WEATHER_ANIMATIONS_ANIMATED_ICONS_H
"""
    
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