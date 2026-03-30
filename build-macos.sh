#!/bin/bash

# Build script for DRPG Engine on macOS
# This script installs dependencies (if needed) and builds the project

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  DRPG Engine macOS Build Script${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Check for Homebrew
check_homebrew() {
    if ! command -v brew &> /dev/null; then
        echo -e "${YELLOW}Homebrew not found. Installing Homebrew...${NC}"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    else
        echo -e "${GREEN}✓ Homebrew found${NC}"
    fi
}

# Check and install dependencies
install_dependencies() {
    echo ""
    echo -e "${YELLOW}Checking dependencies...${NC}"

    DEPS=("cmake" "sdl2" "openal-soft" "zlib")
    MISSING_DEPS=()

    for dep in "${DEPS[@]}"; do
        if brew list "$dep" &> /dev/null; then
            echo -e "${GREEN}✓ $dep is installed${NC}"
        else
            MISSING_DEPS+=("$dep")
        fi
    done

    if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
        echo ""
        echo -e "${YELLOW}Installing missing dependencies: ${MISSING_DEPS[*]}${NC}"
        brew install "${MISSING_DEPS[@]}"
    fi

    echo -e "${GREEN}✓ All dependencies satisfied${NC}"
}

# Build the project
build_project() {
    echo ""
    echo -e "${YELLOW}Building DRPG Engine...${NC}"

    # Get the directory where this script is located
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "$SCRIPT_DIR"

    # Create build directory
    BUILD_DIR="build"
    if [ -d "$BUILD_DIR" ]; then
        echo -e "${YELLOW}Removing existing build directory...${NC}"
        rm -rf "$BUILD_DIR"
    fi

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Configure with CMake
    echo ""
    echo -e "${YELLOW}Configuring with CMake...${NC}"
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15

    # Build
    echo ""
    echo -e "${YELLOW}Compiling...${NC}"
    cmake --build . --config Release -j "$(sysctl -n hw.ncpu)"

    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Build completed successfully!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo -e "Executable location: ${YELLOW}$SCRIPT_DIR/$BUILD_DIR/DRPGEngine${NC}"
    echo ""
    echo -e "${YELLOW}Note:${NC} To run the game, you need the game data file:"
    echo -e "  Place ${YELLOW}'Doom 2 RPG.ipa'${NC} in the same directory as the executable."
    echo ""
}

# Main
main() {
    check_homebrew
    install_dependencies
    build_project
}

main "$@"
