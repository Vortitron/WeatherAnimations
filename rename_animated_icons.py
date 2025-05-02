#!/usr/bin/env python3
"""
A script to rename variables in WeatherAnimationsAnimatedIcons.h to avoid conflicts
with WeatherAnimationsAnimations.cpp.

This script adds the prefix "animated_" to variable names in the file.
"""

import re
import os
import sys

def rename_variables(input_file, output_file):
    # Read the original file
    with open(input_file, 'r') as f:
        content = f.read()
    
    # Find all frame variable names (e.g., cloudyFrame1, rainFrame2, etc.)
    frame_pattern = r'(const\s+uint8_t\s+)([a-zA-Z0-9_]+Frame\d+)(\s*\[\d+\]\s*PROGMEM\s*=\s*)'
    frame_names = set(re.findall(frame_pattern, content))
    
    # Extract just the variable names
    var_names = [match[1] for match in frame_names]
    
    # Create mapping of old names to new names with prefix
    name_mapping = {name: f"animated_{name}" for name in var_names}
    
    # Replace all variable declarations
    for old_name in name_mapping:
        # Replace variable declarations
        content = re.sub(
            f'(const\\s+uint8_t\\s+)({old_name})(\\s*\\[\\d+\\]\\s*PROGMEM\\s*=\\s*)',
            f'\\1{name_mapping[old_name]}\\3',
            content
        )
        
        # Replace references to the variable
        content = re.sub(
            f'\\b{old_name}\\b(?!\\[\\d+\\]\\s*PROGMEM)',
            name_mapping[old_name],
            content
        )
    
    # Find frame arrays like "const uint8_t* cloudyFrames[10]"
    array_pattern = r'(const\s+uint8_t\*\s+)([a-zA-Z0-9_]+Frames)(\s*\[\d+\]\s*=\s*\{)'
    array_names = set(re.findall(array_pattern, content))
    
    # Create mapping for array names
    array_mapping = {match[1]: f"animated_{match[1]}" for match in array_names}
    
    # Replace array declarations
    for old_name in array_mapping:
        # Replace array declarations
        content = re.sub(
            f'(const\\s+uint8_t\\*\\s+)({old_name})(\\s*\\[\\d+\\]\\s*=\\s*\\{{)',
            f'\\1{array_mapping[old_name]}\\3',
            content
        )
        
        # Replace references to the array
        content = re.sub(
            f'\\b{old_name}\\b(?!\\[\\d+\\]\\s*=)',
            array_mapping[old_name],
            content
        )
    
    # Update the getAnimationFrame function's return statements directly
    content = re.sub(
        r'return sunny_dayFrames\[frameIndex\];',
        'return animated_sunny_dayFrames[frameIndex];',
        content
    )
    content = re.sub(
        r'return cloudyFrames\[frameIndex\];',
        'return animated_cloudyFrames[frameIndex];',
        content
    )
    content = re.sub(
        r'return rainyFrames\[frameIndex\];',
        'return animated_rainyFrames[frameIndex];',
        content
    )
    content = re.sub(
        r'return snowyFrames\[frameIndex\];',
        'return animated_snowyFrames[frameIndex];',
        content
    )
    content = re.sub(
        r'return lightningFrames\[frameIndex\];',
        'return animated_lightningFrames[frameIndex];',
        content
    )
    content = re.sub(
        r'return cloudyFrames\[0\];',
        'return animated_cloudyFrames[0];',
        content
    )
    
    # Write the modified content to the output file
    with open(output_file, 'w') as f:
        f.write(content)
    
    print(f"Successfully created {output_file} with renamed variables.")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} input_file output_file")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    if not os.path.exists(input_file):
        print(f"Error: Input file '{input_file}' not found.")
        sys.exit(1)
    
    rename_variables(input_file, output_file) 