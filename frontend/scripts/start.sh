#!/bin/bash

echo "========================================"
echo " PaperCrawler Frontend - Quick Start"
echo "========================================"
echo

echo "[1/3] Installing dependencies..."
npm install
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to install dependencies"
    exit 1
fi

echo
echo "[2/3] Starting development server..."
echo
echo "Frontend will be available at: http://localhost:5173"
echo "Backend API expected at: http://localhost:8080"
echo
echo "Press Ctrl+C to stop the server"
echo

npm run dev
