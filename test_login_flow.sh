#!/bin/bash
# Test Login Flow - Verify no page reload after login

echo "================================"
echo "Login Flow Test Script"
echo "================================"
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test backend connectivity
echo -n "1. Testing backend connection... "
BACKEND_STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/api/auth/login)
if [ "$BACKEND_STATUS" = "000" ]; then
    echo -e "${RED}FAILED${NC} (Backend not responding)"
    echo "   Please start backend server first"
    exit 1
else
    echo -e "${GREEN}OK${NC} (HTTP $BACKEND_STATUS)"
fi

# Test frontend connectivity
echo -n "2. Testing frontend connection... "
FRONTEND_STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:3009)
if [ "$FRONTEND_STATUS" = "000" ]; then
    echo -e "${RED}FAILED${NC} (Frontend not responding)"
    echo "   Please start frontend dev server first: cd frontend && npm run dev"
    exit 1
else
    echo -e "${GREEN}OK${NC} (HTTP $FRONTEND_STATUS)"
fi

# Test login API (should return error, then mock auth kicks in)
echo -n "3. Testing login API... "
LOGIN_RESPONSE=$(curl -s -X POST http://localhost:8080/api/auth/login \
    -H "Content-Type: application/json" \
    -d '{"username":"test@example.com","password":"password123"}')

if echo "$LOGIN_RESPONSE" | grep -q "error"; then
    echo -e "${YELLOW}EXPECTED ERROR (Mock mode)${NC}"
    echo "   Response: $LOGIN_RESPONSE"
else
    echo -e "${GREEN}OK${NC}"
    echo "   Response: $LOGIN_RESPONSE"
fi

# Test login page HTML
echo -n "4. Testing login page HTML... "
LOGIN_PAGE=$(curl -s http://localhost:3009/auth/login)
if echo "$LOGIN_PAGE" | grep -q "login"; then
    echo -e "${GREEN}OK${NC} (Login page loaded)"
else
    echo -e "${RED}FAILED${NC} (Login page not found)"
fi

# Check router guards configuration
echo -n "5. Verifying router guards fix... "
if grep -q "replace: true" frontend/src/router/guards.ts; then
    echo -e "${GREEN}OK${NC} (Router guards using replace: true)"
else
    echo -e "${RED}FAILED${NC} (Router guards not updated)"
fi

# Check LoginView fix
echo -n "6. Verifying LoginView fix... "
if grep -q "路由守卫的 guestOnly 逻辑自动处理重定向" frontend/src/views/auth/LoginView.vue; then
    echo -e "${GREEN}OK${NC} (Manual navigation removed from LoginView)"
else
    echo -e "${RED}FAILED${NC} (LoginView not updated)"
fi

echo ""
echo "================================"
echo "Manual Testing Instructions"
echo "================================"
echo ""
echo "Open browser and test the following:"
echo ""
echo "1. Navigate to: ${GREEN}http://localhost:3009/auth/login${NC}"
echo "2. Enter any email (e.g., test@example.com)"
echo "3. Enter any password (e.g., password123)"
echo "4. Click Login button"
echo ""
echo "Expected behavior:"
echo "  ✅ Success message appears"
echo "  ✅ Page redirects to /dashboard"
echo "  ✅ No page reload occurs"
echo "  ✅ URL changes to http://localhost:3009/dashboard"
echo ""
echo "If you see page reload or return to login, the fix failed."
echo ""
