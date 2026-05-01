#!/bin/bash

# PaperCrawler API Test Script
# Tests all API endpoints with example requests

BASE_URL="http://localhost:8080"

echo "=========================================="
echo "  PaperCrawler API Test Suite"
echo "=========================================="
echo ""

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

test_count=0
pass_count=0
fail_count=0

run_test() {
    local test_name="$1"
    local endpoint="$2"
    local expected_code="$3"

    test_count=$((test_count + 1))
    echo -n "Test $test_count: $test_name ... "

    response=$(curl -s -w "\n%{http_code}" "$BASE_URL$endpoint")
    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | sed '$d')

    if [ "$http_code" = "$expected_code" ]; then
        echo -e "${GREEN}PASS${NC} (HTTP $http_code)"
        pass_count=$((pass_count + 1))
    else
        echo -e "${RED}FAIL${NC} (Expected: $expected_code, Got: $http_code)"
        fail_count=$((fail_count + 1))
        echo "Response: $body"
    fi
}

echo "1. Health Check"
echo "--------------"
run_test "Health endpoint" "/health" "200"
echo ""

echo "2. Search Endpoints"
echo "------------------"
run_test "Basic search" "/api/search?q=deep+learning" "200"
run_test "Search with pagination" "/api/search?q=computer+vision&offset=0&limit=5" "200"
run_test "Search with year filter" "/api/search?q=machine+learning&year=2023" "200"
run_test "Search with level filter" "/api/search?q=ai&level=A" "200"
echo ""

echo "3. Paper Detail Endpoints"
echo "------------------------"
run_test "Get paper by ID" "/api/papers/1" "200"
run_test "Invalid paper ID" "/api/papers/999999" "404"
echo ""

echo "4. Recent Papers Endpoint"
echo "-------------------------"
run_test "Get recent papers" "/api/papers/recent" "200"
run_test "Get recent papers with limit" "/api/papers/recent?limit=10" "200"
echo ""

echo "5. Statistics Endpoints"
echo "-----------------------"
run_test "Statistics overview" "/api/stats/overview" "200"
echo ""

echo "6. Export Endpoints"
echo "------------------"
run_test "Export CSV" "/api/export/csv" "200"
run_test "Export JSON" "/api/export/json" "200"
run_test "Export BibTeX" "/api/export/bibtex/1" "200"
echo ""

echo "7. Error Handling"
echo "-----------------"
run_test "Invalid endpoint" "/api/invalid" "404"
echo ""

echo "=========================================="
echo "  Test Results"
echo "=========================================="
echo -e "Total tests: $test_count"
echo -e "${GREEN}Passed: $pass_count${NC}"
echo -e "${RED}Failed: $fail_count${NC}"
echo ""

if [ $fail_count -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
