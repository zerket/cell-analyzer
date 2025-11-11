#!/bin/bash
# Build script for CellAnalyzer on WSL/Linux

set -e  # Exit on error

echo "=== Building CellAnalyzer for Linux/WSL ==="

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check for required dependencies
echo "Checking dependencies..."

if ! command -v cmake &> /dev/null; then
    echo -e "${RED}ERROR: cmake not found. Install with: sudo apt install cmake${NC}"
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo -e "${RED}ERROR: g++ not found. Install with: sudo apt install build-essential${NC}"
    exit 1
fi

# Check for Qt6
if ! dpkg -l | grep -q libqt6; then
    echo -e "${RED}WARNING: Qt6 may not be installed.${NC}"
    echo "Install with: sudo apt install qt6-base-dev qt6-base-dev-tools"
fi

# Check for OpenCV
if ! pkg-config --exists opencv4; then
    echo -e "${RED}WARNING: OpenCV4 may not be installed.${NC}"
    echo "Install with: sudo apt install libopencv-dev"
fi

# Create build directory
BUILD_DIR="build-linux"
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning existing build directory..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo -e "${GREEN}Configuring CMake...${NC}"
cmake -DCMAKE_BUILD_TYPE=Release ..

echo -e "${GREEN}Building project...${NC}"
cmake --build . --config Release -j$(nproc)

echo -e "${GREEN}=== Build Complete ===${NC}"
echo "Executable: $PWD/CellAnalyzer"
echo ""
echo "To run with arguments:"
echo "  ./build-linux/CellAnalyzer --image path/to/image.jpg"
echo "  ./build-linux/CellAnalyzer --conf 0.25 --iou 0.7"
