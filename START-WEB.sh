#!/bin/bash

# PaperCrawler - Web Version Launcher (Linux)

echo "========================================"
echo "PaperCrawler - Web Version Launcher"
echo "========================================"
echo ""
echo "This will start the backend API and frontend web interface"
echo "(Qt Desktop application requires Qt6 installation - see QT-INSTALL-GUIDE.md)"
echo ""

# Function to check command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for required tools
echo "Checking requirements..."

if ! command_exists cmake; then
    echo "Error: cmake not found. Please install: sudo apt-get install cmake"
    exit 1
fi

if ! command_exists node; then
    echo "Error: node not found. Please install Node.js 20+"
    exit 1
fi

echo "All requirements found!"
echo ""

# Start Backend
echo "========================================"
echo "Step 1: Starting Backend API"
echo "========================================"
echo ""

cd backend
mkdir -p build
cd build

echo "Configuring backend..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "Backend configuration failed!"
    echo "Please ensure dependencies are installed:"
    echo "  sudo apt-get install build-essential cmake libcurl4-openssl-dev libssl-dev libmysqlclient-dev"
    exit 1
fi

echo "Building backend..."
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Backend build failed!"
    exit 1
fi

echo "Starting backend server..."
echo "Backend will start on port 8080..."
./PaperCrawlerServer &
BACKEND_PID=$!

sleep 3

# Check if backend is running
if ! ps -p $BACKEND_PID > /dev/null; then
    echo "Backend failed to start!"
    exit 1
fi

echo "Backend started successfully (PID: $BACKEND_PID)"
echo ""

# Start Frontend
echo "========================================"
echo "Step 2: Starting Frontend"
echo "========================================"
echo ""

cd ../../frontend

if [ ! -d "node_modules" ]; then
    echo "Installing frontend dependencies..."
    npm install
    if [ $? -ne 0 ]; then
        echo "Failed to install frontend dependencies!"
        kill $BACKEND_PID
        exit 1
    fi
fi

echo ""
echo "Starting frontend development server..."
echo "Frontend will be available at: http://localhost:5173"
echo ""
echo "Press Ctrl+C to stop both servers."
echo ""

# Handle cleanup on exit
trap "echo ''; echo 'Stopping...'; kill $BACKEND_PID 2>/dev/null; exit 0" INT TERM

npm run dev

# This line won't be reached due to trap
