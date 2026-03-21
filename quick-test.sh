#!/bin/bash

# PaperCrawler Quick Test Script
# This script performs basic validation tests

set -e

echo "========================================"
echo "PaperCrawler Quick Test Suite"
echo "========================================"
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counter
PASSED=0
FAILED=0

# Function to print test result
print_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC}: $2"
        ((PASSED++))
    else
        echo -e "${RED}✗ FAIL${NC}: $2"
        ((FAILED++))
    fi
}

# Test 1: Check file structure
echo ">>> Test 1: Checking project structure..."

test -f "CMakeLists.txt" && print_result 0 "Root CMakeLists.txt exists" || print_result 1 "Root CMakeLists.txt missing"
test -d "core" && print_result 0 "Core directory exists" || print_result 1 "Core directory missing"
test -d "desktop" && print_result 0 "Desktop directory exists" || print_result 1 "Desktop directory missing"
test -d "backend" && print_result 0 "Backend directory exists" || print_result 1 "Backend directory missing"
test -d "frontend" && print_result 0 "Frontend directory exists" || print_result 1 "Frontend directory missing"
test -f "docker-compose.yml" && print_result 0 "Docker compose file exists" || print_result 1 "Docker compose file missing"

echo ""

# Test 2: Check core library files
echo ">>> Test 2: Checking core library files..."

test -f "core/include/core/PaperCrawlerAPI.hpp" && print_result 0 "PaperCrawlerAPI.hpp exists" || print_result 1 "PaperCrawlerAPI.hpp missing"
test -f "core/src/core/PaperCrawlerAPI.cpp" && print_result 0 "PaperCrawlerAPI.cpp exists" || print_result 1 "PaperCrawlerAPI.cpp missing"
test -f "core/CMakeLists.txt" && print_result 0 "Core CMakeLists.txt exists" || print_result 1 "Core CMakeLists.txt missing"

echo ""

# Test 3: Check desktop files
echo ">>> Test 3: Checking desktop client files..."

test -f "desktop/include/MainWindow.hpp" && print_result 0 "MainWindow.hpp exists" || print_result 1 "MainWindow.hpp missing"
test -f "desktop/src/MainWindow.cpp" && print_result 0 "MainWindow.cpp exists" || print_result 1 "MainWindow.cpp missing"
test -f "desktop/CMakeLists.txt" && print_result 0 "Desktop CMakeLists.txt exists" || print_result 1 "Desktop CMakeLists.txt missing"

echo ""

# Test 4: Check backend files
echo ">>> Test 4: Checking backend API files..."

test -f "backend/src/main.cpp" && print_result 0 "Backend main.cpp exists" || print_result 1 "Backend main.cpp missing"
test -f "backend/CMakeLists.txt" && print_result 0 "Backend CMakeLists.txt exists" || print_result 1 "Backend CMakeLists.txt missing"

echo ""

# Test 5: Check frontend files
echo ">>> Test 5: Checking frontend files..."

test -f "frontend/package.json" && print_result 0 "Frontend package.json exists" || print_result 1 "Frontend package.json missing"
test -f "frontend/vite.config.ts" && print_result 0 "Vite config exists" || print_result 1 "Vite config missing"
test -f "frontend/src/main.ts" && print_result 0 "Frontend main.ts exists" || print_result 1 "Frontend main.ts missing"
test -f "frontend/src/App.vue" && print_result 0 "Frontend App.vue exists" || print_result 1 "Frontend App.vue missing"

echo ""

# Test 6: Check configuration files
echo ">>> Test 6: Checking configuration files..."

test -f "config/config.json" && print_result 0 "Config file exists" || print_result 1 "Config file missing"
test -f "docker-compose.yml" && print_result 0 "Docker compose file exists" || print_result 1 "Docker compose file missing"
test -f "sql/init.sql" && print_result 0 "Database init script exists" || print_result 1 "Database init script missing"

echo ""

# Test 7: Check documentation
echo ">>> Test 7: Checking documentation..."

test -f "README.md" && print_result 0 "README.md exists" || print_result 1 "README.md missing"
test -f "README.EXPANDED.md" && print_result 0 "README.EXPANDED.md exists" || print_result 1 "README.EXPANDED.md missing"
test -f "DEPLOYMENT.md" && print_result 0 "DEPLOYMENT.md exists" || print_result 1 "DEPLOYMENT.md missing"
test -f "TEST-GUIDE.md" && print_result 0 "TEST-GUIDE.md exists" || print_result 1 "TEST-GUIDE.md missing"

echo ""

# Summary
echo "========================================"
echo "Test Summary"
echo "========================================"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Review config/config.json and update database settings"
    echo "2. Initialize MySQL database: mysql -u root -p < sql/init.sql"
    echo "3. Choose deployment method:"
    echo "   - Docker: docker-compose up -d"
    echo "   - Manual: Follow TEST-GUIDE.md"
    exit 0
else
    echo -e "${RED}Some tests failed. Please review the output above.${NC}"
    exit 1
fi
