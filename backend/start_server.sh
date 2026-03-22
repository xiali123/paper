#!/bin/bash

# PaperCrawler API Server Startup Script

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=========================================="
echo "  PaperCrawler API Server"
echo "=========================================="
echo ""

# Check if build directory exists
if [ ! -d "build" ]; then
    echo -e "${YELLOW}Build directory not found. Creating...${NC}"
    mkdir -p build
    cd build
    cmake ..
    cmake --build .
    cd ..
fi

# Check if executable exists
if [ ! -f "build/PaperCrawlerServer" ]; then
    echo -e "${YELLOW}Executable not found. Building...${NC}"
    cd build
    cmake --build .
    cd ..
fi

# Check if config exists
if [ ! -f "config/config.json" ]; then
    echo -e "${RED}Configuration file not found!${NC}"
    echo "Please copy config.example.json to config/config.json"
    echo "and update it with your database credentials."
    exit 1
fi

# Check if MySQL is running
if ! command -v mysql &> /dev/null; then
    echo -e "${YELLOW}Warning: mysql command not found. Ensure MySQL is installed.${NC}"
fi

echo -e "${GREEN}Starting PaperCrawler API Server...${NC}"
echo ""
echo "Server will be available at: http://localhost:8080"
echo "Press Ctrl+C to stop the server"
echo ""

# Start the server
./build/PaperCrawlerServer
