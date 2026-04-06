#!/bin/bash
# ========================================
# PaperCrawler Backend - Test Pagination Fix
# ========================================

echo "========================================"
echo "PaperCrawler - Pagination Fix Test"
echo "========================================"
echo ""

# Check if backend is running
echo "[1/5] Checking if backend is running..."
HEALTH=$(curl -s http://localhost:8080/health)
if [[ $HEALTH == *"ok"* ]]; then
    echo "✓ Backend is running"
else
    echo "✗ Backend is not running!"
    echo "Please start the backend server first:"
    echo "  cd e:/PaperCrawler/backend && ./PaperCrawlerServer.exe"
    exit 1
fi

echo ""
echo "[2/5] Test 1: Simple search (no pagination)"
echo "Request: GET /api/search?q=learning"
echo "-------------------------------------------"
RESULT1=$(curl -s "http://localhost:8080/api/search?q=learning&offset=0&limit=3")
echo "$RESULT1" | head -20

echo ""
echo "[3/5] Test 2: Pagination with offset=0, limit=3"
echo "Request: GET /api/search?q=learning&offset=0&limit=3"
echo "-------------------------------------------"
RESULT2=$(curl -s "http://localhost:8080/api/search?q=learning&offset=0&limit=3")
echo "$RESULT2" | grep -o '"title": "[^"]*"' | head -3

echo ""
echo "[4/5] Test 3: Pagination with offset=3, limit=3"
echo "Request: GET /api/search?q=learning&offset=3&limit=3"
echo "-------------------------------------------"
RESULT3=$(curl -s "http://localhost:8080/api/search?q=learning&offset=3&limit=3")
echo "$RESULT3" | grep -o '"title": "[^"]*"' | head -3

echo ""
echo "[5/5] Verification"
echo "-------------------------------------------"

# Check if titles contain URL parameters (bug)
if echo "$RESULT2" | grep -q "offset="; then
    echo "✗ BUG DETECTED: Titles contain URL parameters!"
    echo "   The fix has NOT been applied yet."
    echo ""
    echo "   Expected: Papers with title containing 'learning'"
    echo "   Actual: Papers with title containing 'learning&offset=0&limit=3'"
    echo ""
    echo "   To apply the fix, run:"
    echo "   cd e:/PaperCrawler/backend && ./apply_pagination_fix.bat"
else
    echo "✓ Fix applied! Titles don't contain URL parameters."

    # Check if pagination actually works (different results)
    TITLE2=$(echo "$RESULT2" | grep -o '"id": [0-9]*' | head -1 | grep -o '[0-9]*')
    TITLE3=$(echo "$RESULT3" | grep -o '"id": [0-9]*' | head -1 | grep -o '[0-9]*')

    if [[ "$TITLE2" != "$TITLE3" ]]; then
        echo "✓ Pagination working! Different results for different offsets."
    else
        echo "⚠ Warning: Same results returned (pagination may not be working)"
    fi
fi

echo ""
echo "========================================"
echo "Test complete"
echo "========================================"
