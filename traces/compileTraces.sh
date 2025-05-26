#!/bin/bash

INPUT_DIR="$1"
OUTPUT_DIR="hex"
CURR_DIR="$(pwd)"
SCRIPT_DIR="$CURR_DIR/../scripts/"

# Check if input directory is provided and exists
if [ -z "$INPUT_DIR" ] || [ ! -d "$INPUT_DIR" ]; then
    echo "Usage: $0 <input_directory>"
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

TRACE_FILES=("$INPUT_DIR"*.txt)

for file in "${TRACE_FILES[@]}"; do
    # Check if file is a text file
    echo "Compiling $file..."
    python3 "$SCRIPT_DIR/mips-lite_compiler.py" -i "$file" -o "$OUTPUT_DIR/$(basename "$file" .txt).txt"
done
