#!/bin/bash
# PaperCrawler Project - Stop All Services Script
# Author: Claude AI Agent
# Date: 2026-03-21

echo "========================================"
echo "  PaperCrawler - Stopping All Services"
echo "========================================"
echo ""

# Stop Backend
if [ -f .backend.pid ]; then
    BACKEND_PID=$(cat .backend.pid)
    echo "[1/3] Stopping backend (PID: $BACKEND_PID)..."
    kill $BACKEND_PID 2>/dev/null
    rm .backend.pid
    echo "[OK] Backend stopped"
else
    echo "[1/3] Backend PID not found, skipping..."
fi

# Stop Frontend
if [ -f .frontend.pid ]; then
    FRONTEND_PID=$(cat .frontend.pid)
    echo "[2/3] Stopping frontend (PID: $FRONTEND_PID)..."
    kill $FRONTEND_PID 2>/dev/null
    rm .frontend.pid
    echo "[OK] Frontend stopped"
else
    echo "[2/3] Frontend PID not found, skipping..."
fi

# Stop MUI Demo App
if [ -f .mui.pid ]; then
    MUI_PID=$(cat .mui.pid)
    echo "[3/3] Stopping MUI Demo App (PID: $MUI_PID)..."
    kill $MUI_PID 2>/dev/null
    rm .mui.pid
    echo "[OK] MUI Demo App stopped"
else
    echo "[3/3] MUI Demo App PID not found, skipping..."
fi

echo ""
echo "========================================"
echo "  All Services Stopped"
echo "========================================"
