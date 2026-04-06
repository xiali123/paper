#!/bin/bash

# Frontend API Testing Script
# This script tests the complete API flow for the Stats page

echo "🧪 Frontend API Testing Script"
echo "================================"
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test 1: Check if backend is running
echo "📡 Test 1: Checking backend service..."
if curl -s http://localhost:8080/api/stats/overview > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Backend service is running${NC}"
    echo "Backend response:"
    curl -s http://localhost:8080/api/stats/overview | python3 -m json.tool 2>/dev/null || curl -s http://localhost:8080/api/stats/overview
    echo ""
else
    echo -e "${RED}❌ Backend service is not running${NC}"
    echo "Please start the backend server first:"
    echo "  cd backend && ./start_server.sh"
    exit 1
fi

# Test 2: Check if frontend is running
echo "📡 Test 2: Checking frontend service..."
if curl -s http://localhost:5173 > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Frontend service is running${NC}"
    echo ""
else
    echo -e "${YELLOW}⚠️  Frontend service might not be running${NC}"
    echo "Start the frontend with:"
    echo "  cd frontend && npm run dev"
    echo ""
fi

# Test 3: Test API proxy through Vite
echo "📡 Test 3: Testing API through Vite proxy..."
if curl -s http://localhost:5173/api/stats/overview > /dev/null 2>&1; then
    echo -e "${GREEN}✅ Vite proxy is working correctly${NC}"
    echo "Proxy response:"
    curl -s http://localhost:5173/api/stats/overview | python3 -m json.tool 2>/dev/null || curl -s http://localhost:5173/api/stats/overview
    echo ""
else
    echo -e "${YELLOW}⚠️  Vite proxy might not be accessible${NC}"
    echo "This is expected if frontend dev server is not running"
    echo ""
fi

# Test 4: Check file modifications
echo "📁 Test 4: Verifying file modifications..."
FILES=(
    "e:/PaperCrawler/frontend/src/utils/request.ts"
    "e:/PaperCrawler/frontend/src/composables/useStats.ts"
    "e:/PaperCrawler/frontend/src/views/Stats.vue"
)

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        echo -e "${GREEN}✅ $file exists${NC}"

        # Check if file contains debug logging
        if grep -q "console.log" "$file"; then
            echo -e "  ${GREEN}✓ Contains debug logging${NC}"
        fi
    else
        echo -e "${RED}❌ $file not found${NC}"
    fi
done
echo ""

# Test 5: Browser console test guide
echo "🌐 Test 5: Browser Console Test Guide"
echo "-------------------------------------"
echo "Follow these steps to test in browser:"
echo ""
echo "1. Open http://localhost:5173/stats in your browser"
echo "2. Open Developer Tools (F12)"
echo "3. Go to Console tab"
echo "4. Look for these messages:"
echo "   - 🎯 Stats component mounted"
echo "   - 🔄 Fetching overview stats..."
echo "   - ✅ API Success (or ❌ API Error if something is wrong)"
echo "   - ✅ Received overview data"
echo "   - ✅ Overview data updated"
echo ""
echo "5. Check Network tab:"
echo "   - Filter by 'Fetch/XHR'"
echo "   - Look for '/api/stats/overview' request"
echo "   - Verify status is 200 OK"
echo "   - Check response contains data"
echo ""
echo "6. Check Vue DevTools (if installed):"
echo "   - Select Stats component"
echo "   - Verify stats.loading is false"
echo "   - Verify stats.hasData is true"
echo "   - Verify stats.overview contains data"
echo ""

# Summary
echo "📊 Summary"
echo "---------"
echo "Modified files:"
echo "  ✅ src/utils/request.ts - Enhanced logging and error handling"
echo "  ✅ src/composables/useStats.ts - Added debug logging"
echo "  ✅ src/views/Stats.vue - Added state watchers and mount logging"
echo ""
echo "Next steps:"
echo "  1. Ensure backend is running: cd backend && ./start_server.sh"
echo "  2. Start frontend: cd frontend && npm run dev"
echo "  3. Open browser to http://localhost:5173/stats"
echo "  4. Open browser console and observe debug messages"
echo "  5. Verify data displays correctly"
echo ""
echo "For detailed debugging guide, see: FRONTEND_DEBUGGING_GUIDE.md"
echo ""
