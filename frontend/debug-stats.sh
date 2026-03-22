#!/bin/bash

# Statistics Page Debug Script
# This script helps diagnose and fix the statistics page loading issue

echo "=== PaperCrawler Statistics Page Debug Script ==="
echo ""

# Check if backend is running
echo "1. Checking if backend server is running on port 8080..."
if curl -s http://localhost:8080/api/stats/overview > /dev/null; then
    echo "✅ Backend server is running"
    echo "Response:"
    curl -s http://localhost:8080/api/stats/overview | head -c 200
    echo ""
else
    echo "❌ Backend server is NOT running on port 8080"
    echo "Please start the backend server first"
    exit 1
fi

echo ""

# Check if frontend dev server is running
echo "2. Checking if frontend dev server is running on port 5173..."
if curl -s http://localhost:5173 > /dev/null; then
    echo "✅ Frontend dev server is running"
else
    echo "❌ Frontend dev server is NOT running on port 5173"
    echo "Please start the frontend dev server: cd frontend && npm run dev"
    exit 1
fi

echo ""

# Test proxy connection
echo "3. Testing proxy connection through frontend..."
PROXY_RESPONSE=$(curl -s http://localhost:5173/api/stats/overview)
if [ $? -eq 0 ]; then
    echo "✅ Proxy connection successful"
    echo "Response:"
    echo "$PROXY_RESPONSE" | head -c 200
    echo ""
else
    echo "❌ Proxy connection failed"
fi

echo ""

# Open diagnostic tool
echo "4. Opening diagnostic tool in browser..."
if command -v open &> /dev/null; then
    # macOS
    open http://localhost:5173/diagnostic.html
elif command -v xdg-open &> /dev/null; then
    # Linux
    xdg-open http://localhost:5173/diagnostic.html
elif command -v start &> /dev/null; then
    # Windows
    start http://localhost:5173/diagnostic.html
else
    echo "Could not open browser automatically"
    echo "Please open this URL manually: http://localhost:5173/diagnostic.html"
fi

echo ""
echo "=== Debug Steps ==="
echo "1. The diagnostic tool should open in your browser"
echo "2. Run all tests to identify any issues"
echo "3. Open the Stats page in your app: http://localhost:5173/#/stats"
echo "4. Open browser DevTools (F12) and check the Console tab"
echo "5. Look for log messages with emoji prefixes:"
echo "   🔄 = Fetching data"
echo "   ✅ = Success"
echo "   ❌ = Error"
echo "   📊 = Data state"
echo ""
echo "6. If you see Vue DevTools, inspect the Stats component state:"
echo "   - stats.loading should be: false"
echo "   - stats.hasData should be: true"
echo "   - stats.overview should contain data"
echo "   - stats.error should be: null"
echo ""
echo "For detailed instructions, see: frontend/STATS_DEBUG_GUIDE.md"
echo ""
