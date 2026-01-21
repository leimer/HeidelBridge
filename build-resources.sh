#!/bin/bash

# ====================================================================
# build-resources.sh - Convert web files to C++ header files
# ====================================================================
#
# PURPOSE:
#   Converts static web files (HTML, CSS, JS, images) from the data/
#   directory into C++ header files that can be embedded in firmware.
#
# WHEN TO RUN:
#   ✅ Run this script ONLY when you modify web interface files
#   ❌ NOT needed for normal firmware builds (headers already exist)
#
# USAGE:
#   ./build-resources.sh
#
# WHAT IT DOES:
#   1. Reads files from ./data/ directory
#   2. Converts each file to a byte array using xxd
#   3. Creates C++ header files in ./data/headers/
#   4. Headers are included in WebServer.cpp for serving web content
#
# REQUIREMENTS:
#   - xxd utility (usually part of vim package)
#   - Bash shell
#
# For more information, see docs/BuildProcess.md
# ====================================================================

# Hardcoded input directory
INPUT_DIR="./data"
OUTPUT_DIR="$INPUT_DIR/headers"

# Ensure the directory exists
if [ ! -d "$INPUT_DIR" ]; then
    echo "Error: Directory '$INPUT_DIR' not found."
    exit 1
fi

# Create the output directory for header files
mkdir -p "$OUTPUT_DIR"

# Process each file in the directory (excluding already converted .h files)
find "$INPUT_DIR" -type f ! -name "*.h" | while read -r file; do
    filename=$(basename -- "$file")
    varname=$(echo "$filename" | tr '.-' '_')  # Convert dots and hyphens to underscores
    headerfile="$OUTPUT_DIR/${filename}.h"

    echo "Converting $file -> $headerfile"
    
    echo "#pragma once" > "$headerfile"
    echo "" >> "$headerfile"
    echo "const unsigned char ${varname}[] PROGMEM = {" >> "$headerfile"
    # Generate only the hex values without variable declaration
    xxd -i "$file" | sed -E '/unsigned char|unsigned int/d' >> "$headerfile"
    echo "" >> "$headerfile"
    echo "const unsigned int ${varname}_len = sizeof(${varname});" >> "$headerfile"
done

echo "Conversion complete. Header files saved in '$OUTPUT_DIR'."
