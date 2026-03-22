#!/bin/bash
# PaperCrawler Project - All Services Startup Script
# Author: Claude AI Agent
# Date: 2026-03-21

echo "========================================"
echo "  PaperCrawler - Starting All Services"
echo "========================================"
echo ""

# Function to check if port is in use
port_in_use() {
    netstat -ano 2>/dev/null | grep ":$1 " | grep "LISTENING" > /dev/null
    return $?
}

# Start Backend
echo "[1/3] Checking backend service..."
if port_in_use 8080; then
    echo "[OK] Backend is already running on port 8080"
else
    echo "[INFO] Starting backend service..."
    cd backend
    ./start_server.sh &
    BACKEND_PID=$!
    cd ..
    sleep 3
    echo "[OK] Backend started on port 8080 (PID: $BACKEND_PID)"
fi

echo ""

# Start Frontend
echo "[2/3] Checking frontend service..."
if port_in_use 5173; then
    echo "[OK] Frontend is already running on port 5173"
else
    echo "[INFO] Starting frontend service..."
    cd frontend
    npm run dev &
    FRONTEND_PID=$!
    cd ..
    sleep 5
    echo "[OK] Frontend started on port 5173 (PID: $FRONTEND_PID)"
fi

echo ""

# Optional: Start MUI Demo App
echo "[3/3] Checking MUI Demo App..."
if port_in_use 3006; then
    echo "[OK] MUI Demo App is already running on port 3006"
else
    echo "[INFO] Starting MUI Demo App (optional)..."
    cd /f/test_line/mui-demo-app
    npm run dev &
    MUI_PID=$!
    cd - > /dev/null
    sleep 5
    echo "[OK] MUI Demo App started on port 3006 (PID: $MUI_PID)"
fi

echo ""
echo "========================================"
echo "  All Services Started Successfully!"
echo "========================================"
echo ""
echo "🌐 Access URLs:"
echo "   • PaperCrawler:    http://localhost:5173"
echo "   • Backend API:     http://localhost:8080"
echo "   • Health Check:    http://localhost:8080/health"
echo "   • MUI Demo App:    http://localhost:3006"
echo ""
echo "📚 Documentation:"
echo "   • API Docs:        backend/API_DOCUMENTATION.md"
echo "   • Quick Start:     OPTIMIZATION_SUMMARY.md"
echo "   • Test Results:    TEST_RESULTS.md"
echo ""
echo "💡 Tips:"
echo "   • Press Ctrl+C to stop services"
echo "   • Check terminal logs for details"
echo "   • Use browser DevTools (F12) for debugging"
echo ""
echo "✅ Ready to use! Open browser and visit:"
echo "   http://localhost:5173"
echo ""

# Save PIDs for cleanup
echo $BACKEND_PID > .backend.pid
echo $FRONTEND_PID > .frontend.pid
[ -n "$MUI_PID" ] && echo $MUI_PID > .mui.pid

echo "Service PIDs saved for cleanup."
echo "Run './stop-all.sh' to stop all services."
