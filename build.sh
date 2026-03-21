#!/bin/bash
# PaperCrawler Build Script for Linux/macOS

set -e

echo "========================================"
echo "PaperCrawler Build Script"
echo "========================================"

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

echo "Detected OS: $OS"

# Install dependencies based on OS
if [ "$OS" == "linux" ]; then
    echo "Checking dependencies..."
    if ! command -v cmake &> /dev/null; then
        echo "Installing CMake..."
        sudo apt-get update && sudo apt-get install -y cmake
    fi

    if ! dpkg -l | grep -q libcurl4-openssl-dev; then
        echo "Installing libcurl..."
        sudo apt-get install -y libcurl4-openssl-dev
    fi

    if ! dpkg -l | grep -q libssl-dev; then
        echo "Installing OpenSSL..."
        sudo apt-get install -y libssl-dev
    fi

    if ! dpkg -l | grep -q libmysqlclient-dev; then
        echo "Installing MySQL client..."
        sudo apt-get install -y libmysqlclient-dev
    fi
elif [ "$OS" == "macos" ]; then
    echo "Checking dependencies..."
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Please install from https://brew.sh/"
        exit 1
    fi

    if ! command -v cmake &> /dev/null; then
        echo "Installing CMake..."
        brew install cmake
    fi
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring project..."
cmake -DCMAKE_BUILD_TYPE=Release ..
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build project
echo "Building project..."
if [ "$OS" == "linux" ]; then
    make -j$(nproc)
else
    make -j$(sysctl -n hw.ncpu)
fi

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "========================================"
echo "Build completed successfully!"
echo "Executable: build/bin/PaperCrawler"
echo "========================================"
