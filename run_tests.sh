#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Running Godot Binary Patcher Tests ===${NC}"

# Check if Godot executable exists
if ! command -v godot &> /dev/null && [ ! -f "./godot" ]; then
    echo -e "${RED}Error: Godot executable not found${NC}"
    exit 1
fi

# Use local godot if it exists, otherwise use system godot
if [ -f "./godot" ]; then
    GODOT_CMD="./godot"
else
    GODOT_CMD="godot"
fi

# Check if the GDExtension library exists
if [ ! -f "addons/godot-binary-patcher/bin/libgodot_binary_patcher.so" ] && \
   [ ! -f "addons/godot-binary-patcher/bin/libgodot_binary_patcher.dll" ] && \
   [ ! -f "addons/godot-binary-patcher/bin/libgodot_binary_patcher.dylib" ]; then
    echo -e "${RED}Error: GDExtension library not found. Run build first.${NC}"
    exit 1
fi

# Run the test scene
echo -e "${YELLOW}Running test scene (process timeout: 120s, per-test timeouts handled in GUT code)...${NC}"
timeout 120s $GODOT_CMD --headless --script res://addons/gut/gut_cmdln.gd -gdir=res://test/unit/ -gexit

echo -e "${GREEN}All tests completed!${NC}"
