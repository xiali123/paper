#!/bin/bash

# PaperCrawler E2E Test Quick Start Script
# This script helps you quickly start the backend and frontend servers for testing

echo "================================================"
echo "PaperCrawler E2E Test - Quick Start"
echo "================================================"
echo ""

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to check if a port is in use
check_port() {
    local port=$1
    if netstat -an | grep -q ":${port}.*LISTEN"; then
        return 0  # Port is in use
    else
        return 1  # Port is free
    fi
}

# Function to start backend
start_backend() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  Step 1: Starting Backend Server${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""

    # Check if backend port is already in use
    if check_port 8080; then
        echo -e "${YELLOW}⚠️  Port 8080 is already in use${NC}"
        echo "Backend server might already be running."
        echo ""
        read -p "Do you want to skip backend startup? (y/n): " skip_backend
        if [ "$skip_backend" = "y" ]; then
            echo -e "${GREEN}✓ Skipping backend startup${NC}"
            return 0
        fi
    fi

    echo "Starting backend server..."
    cd backend

    # Check if executable exists
    if [ ! -f "build/Release/PaperCrawlerServer.exe" ]; then
        echo -e "${YELLOW}⚠️  Backend executable not found${NC}"
        echo "Please build the backend first:"
        echo "  cd backend"
        echo "  mkdir -p build && cd build"
        echo "  cmake .."
        echo "  cmake --build . --config Release"
        echo ""
        return 1
    fi

    # Start backend in background
    echo "Running: ./build/Release/PaperCrawlerServer.exe"
    echo ""

    # Use start command for Windows to run in background
    start /B build/Release/PaperCrawlerServer.exe > ../backend.log 2>&1 &

    # Wait for backend to start
    echo "Waiting for backend to start..."
    sleep 3

    # Check if backend is responding
    if curl -s http://localhost:8080/api/health > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Backend server started successfully${NC}"
        echo "  URL: http://localhost:8080"
        echo "  Health: http://localhost:8080/api/health"
        echo ""
    else
        echo -e "${YELLOW}⚠️  Backend may not be responding yet${NC}"
        echo "Check the log file: backend.log"
        echo ""
    fi

    cd ..
}

# Function to start frontend
start_frontend() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  Step 2: Starting Frontend Dev Server${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""

    # Check if frontend port is already in use
    if check_port 5173; then
        echo -e "${YELLOW}⚠️  Port 5173 is already in use${NC}"
        echo "Frontend dev server might already be running."
        echo ""
        read -p "Do you want to skip frontend startup? (y/n): " skip_frontend
        if [ "$skip_frontend" = "y" ]; then
            echo -e "${GREEN}✓ Skipping frontend startup${NC}"
            return 0
        fi
    fi

    echo "Starting frontend dev server..."
    cd frontend

    # Check if node_modules exists
    if [ ! -d "node_modules" ]; then
        echo -e "${YELLOW}⚠️  node_modules not found${NC}"
        echo "Installing dependencies..."
        npm install
    fi

    # Start frontend in background
    echo "Running: npm run dev"
    echo ""

    # Use start command for Windows
    start /B npm run dev > ../frontend.log 2>&1 &

    # Wait for frontend to start
    echo "Waiting for frontend to start..."
    sleep 5

    # Check if frontend is responding
    if curl -s http://localhost:5173 > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Frontend dev server started successfully${NC}"
        echo "  URL: http://localhost:5173"
        echo "  Network: http://localhost:5173"
        echo ""
    else
        echo -e "${YELLOW}⚠️  Frontend may not be responding yet${NC}"
        echo "Check the log file: frontend.log"
        echo ""
    fi

    cd ..
}

# Function to run API tests
run_tests() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  Step 3: Running API Tests${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""

    cd backend
    chmod +x test_api_e2e.sh
    bash test_api_e2e.sh
    cd ..
}

# Function to open browser
open_browser() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  Step 4: Opening Browser${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""

    echo "Opening AI Review page..."
    # Use start command on Windows to open URL
    start http://localhost:5173/ai/review

    echo -e "${GREEN}✓ Browser opened${NC}"
    echo ""
    echo "Test URLs:"
    echo "  - AI Review:        http://localhost:5173/ai/review"
    echo "  - Literature Review: http://localhost:5173/ai/literature-review"
    echo "  - Research Plan:    http://localhost:5173/ai/research-plan"
    echo ""
}

# Main execution
main() {
    echo "This script will:"
    echo "  1. Start the backend server (port 8080)"
    echo "  2. Start the frontend dev server (port 5173)"
    echo "  3. Run API endpoint tests"
    echo "  4. Open browser to AI Review page"
    echo ""
    read -p "Continue? (y/n): " confirm

    if [ "$confirm" != "y" ]; then
        echo "Aborted."
        exit 0
    fi

    echo ""

    # Start servers
    start_backend
    start_frontend

    # Run tests
    run_tests

    # Open browser
    echo ""
    read -p "Open browser now? (y/n): " open_browser_confirm
    if [ "$open_browser_confirm" = "y" ]; then
        open_browser
    fi

    # Summary
    echo ""
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${GREEN}  Setup Complete!${NC}"
    echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
    echo "Servers are running in the background."
    echo ""
    echo "Backend:"
    echo "  - URL: http://localhost:8080"
    echo "  - Health: http://localhost:8080/api/health"
    echo "  - Log: backend.log"
    echo ""
    echo "Frontend:"
    echo "  - URL: http://localhost:5173"
    echo "  - Log: frontend.log"
    echo ""
    echo "To stop the servers:"
    echo "  - Press Ctrl+C in the terminal windows"
    echo "  - Or kill the processes using Task Manager"
    echo ""
    echo "Happy testing! 🚀"
}

# Run main function
main
