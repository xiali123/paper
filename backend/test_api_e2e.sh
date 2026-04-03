#!/bin/bash

# PaperCrawler API End-to-End Test Script
# Tests all AI Research Co-Pilot API endpoints

echo "================================================"
echo "PaperCrawler API End-to-End Test"
echo "================================================"
echo ""

# Configuration
BACKEND_URL="http://localhost:8080"
TEST_USER_ID=1
TEST_PAPER_ID=1

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Helper functions
log_test() {
    echo ""
    echo "=========================================="
    echo "Test: $1"
    echo "=========================================="
    ((TOTAL_TESTS++))
}

log_pass() {
    echo -e "${GREEN}✓ PASSED${NC}: $1"
    echo "Details: $2"
    ((PASSED_TESTS++))
}

log_fail() {
    echo -e "${RED}✗ FAILED${NC}: $1"
    echo "Error: $2"
    ((FAILED_TESTS++))
}

log_info() {
    echo -e "${YELLOW}ℹ INFO${NC}: $1"
}

# Check if backend is running
check_backend() {
    log_test "Backend Server Health Check"

    response=$(curl -s -o /dev/null -w "%{http_code}" "${BACKEND_URL}/api/health" 2>/dev/null)

    if [ "$response" = "200" ]; then
        log_pass "Backend server is running" "HTTP 200 - Health check passed"
        return 0
    else
        log_fail "Backend server is not accessible" "HTTP ${response} - Is the server running?"
        return 1
    fi
}

# Test 1: AI Review API
test_ai_review() {
    log_test "AI Review API - Generate Review"

    # Prepare request
    request=$(cat <<EOF
{
  "paperId": ${TEST_PAPER_ID},
  "userId": ${TEST_USER_ID},
  "targetJournal": "Nature",
  "researchField": "Computer Science",
  "includeComparison": true,
  "reviewStyle": "balanced"
}
EOF
)

    # Send request
    response=$(curl -s -X POST \
        -H "Content-Type: application/json" \
        -d "$request" \
        "${BACKEND_URL}/api/ai-co-pilot/review" \
        -w "\nHTTP_CODE:%{http_code}" \
        2>/dev/null)

    http_code=$(echo "$response" | grep "HTTP_CODE" | cut -d':' -f2)
    body=$(echo "$response" | grep -v "HTTP_CODE")

    if [ "$http_code" = "200" ] || [ "$http_code" = "201" ]; then
        # Check if response contains expected fields
        if echo "$body" | grep -q "reviewScore\|success"; then
            log_pass "AI Review API works" "HTTP ${http_code} - Response contains reviewScore"
        else
            log_fail "AI Review API response invalid" "Missing reviewScore field"
        fi
    else
        log_fail "AI Review API request failed" "HTTP ${http_code} - ${body:0:100}"
    fi
}

# Test 2: Literature Review API
test_literature_review() {
    log_test "Literature Review API - Generate Review"

    request=$(cat <<EOF
{
  "title": "Deep Learning in Natural Language Processing",
  "userId": ${TEST_USER_ID},
  "researchField": "Computer Science",
  "paperCount": 50,
  "keywords": "machine learning, NLP, deep learning",
  "timeRange": "5"
}
EOF
)

    response=$(curl -s -X POST \
        -H "Content-Type: application/json" \
        -d "$request" \
        "${BACKEND_URL}/api/ai-co-pilot/literature-review/generate" \
        -w "\nHTTP_CODE:%{http_code}" \
        2>/dev/null)

    http_code=$(echo "$response" | grep "HTTP_CODE" | cut -d':' -f2)
    body=$(echo "$response" | grep -v "HTTP_CODE")

    if [ "$http_code" = "200" ] || [ "$http_code" = "201" ]; then
        if echo "$body" | grep -q "title\|success"; then
            log_pass "Literature Review API works" "HTTP ${http_code} - Response contains title"
        else
            log_fail "Literature Review API response invalid" "Missing expected fields"
        fi
    else
        log_fail "Literature Review API request failed" "HTTP ${http_code}"
    fi
}

# Test 3: Research Plan API
test_research_plan() {
    log_test "Research Plan API - Generate Plan"

    request=$(cat <<EOF
{
  "title": "AI-Powered Medical Image Analysis",
  "userId": ${TEST_USER_ID},
  "researchField": "Medicine",
  "duration": 12,
  "budget": 50000,
  "description": "Develop AI system for medical image diagnosis",
  "keywords": "AI, medical imaging, deep learning"
}
EOF
)

    response=$(curl -s -X POST \
        -H "Content-Type: application/json" \
        -d "$request" \
        "${BACKEND_URL}/api/ai-co-pilot/research-plan/generate" \
        -w "\nHTTP_CODE:%{http_code}" \
        2>/dev/null)

    http_code=$(echo "$response" | grep "HTTP_CODE" | cut -d':' -f2)
    body=$(echo "$response" | grep -v "HTTP_CODE")

    if [ "$http_code" = "200" ] || [ "$http_code" = "201" ]; then
        if echo "$body" | grep -q "title\|success"; then
            log_pass "Research Plan API works" "HTTP ${http_code} - Response contains title"
        else
            log_fail "Research Plan API response invalid" "Missing expected fields"
        fi
    else
        log_fail "Research Plan API request failed" "HTTP ${http_code}"
    fi
}

# Test 4: Get Review History
test_review_history() {
    log_test "AI Review API - Get History"

    response=$(curl -s -X GET \
        "${BACKEND_URL}/api/ai-co-pilot/reviews/${TEST_USER_ID}?page=1&limit=20" \
        -w "\nHTTP_CODE:%{http_code}" \
        2>/dev/null)

    http_code=$(echo "$response" | grep "HTTP_CODE" | cut -d':' -f2)

    if [ "$http_code" = "200" ]; then
        log_pass "Get review history works" "HTTP ${http_code} - Returns user's review history"
    else
        log_fail "Get review history failed" "HTTP ${http_code}"
    fi
}

# Test 5: Get Usage Statistics
test_usage_stats() {
    log_test "AI Statistics API - Get Usage Stats"

    response=$(curl -s -X GET \
        "${BACKEND_URL}/api/ai-co-pilot/stats?userId=${TEST_USER_ID}" \
        -w "\nHTTP_CODE:%{http_code}" \
        2>/dev/null)

    http_code=$(echo "$response" | grep "HTTP_CODE" | cut -d':' -f2)

    if [ "$http_code" = "200" ]; then
        log_pass "Get usage stats works" "HTTP ${http_code} - Returns user statistics"
    else
        log_fail "Get usage stats failed" "HTTP ${http_code}"
    fi
}

# Test 6: Get Cost Statistics
test_cost_stats() {
    log_test "AI Statistics API - Get Cost Stats"

    response=$(curl -s -X GET \
        "${BACKEND_URL}/api/ai-co-pilot/costs?userId=${TEST_USER_ID}" \
        -w "\nHTTP_CODE:%{http_code}" \
        2>/dev/null)

    http_code=$(echo "$response" | grep "HTTP_CODE" | cut -d':' -f2)

    if [ "$http_code" = "200" ]; then
        log_pass "Get cost stats works" "HTTP ${http_code} - Returns cost statistics"
    else
        log_fail "Get cost stats failed" "HTTP ${http_code}"
    fi
}

# Main test execution
echo "Starting API End-to-End Tests..."
echo "Backend URL: ${BACKEND_URL}"
echo ""

# Check backend first
if ! check_backend; then
    echo ""
    echo "❌ Cannot proceed - Backend server is not running"
    echo ""
    echo "Please start the backend server:"
    echo "  cd backend"
    echo "  ./build/Release/PaperCrawlerServer.exe"
    echo ""
    exit 1
fi

# Run all tests
test_ai_review
test_literature_review
test_research_plan
test_review_history
test_usage_stats
test_cost_stats

# Summary
echo ""
echo "================================================"
echo "Test Summary"
echo "================================================"
echo "Total Tests: ${TOTAL_TESTS}"
echo "Passed: ${PASSED_TESTS}"
echo "Failed: ${FAILED_TESTS}"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}Status: ALL TESTS PASSED ✓${NC}"
    echo ""
    echo "🎉 All API endpoints are working correctly!"
    echo "✅ Backend integration verified"
    echo "✅ Ready for frontend testing"
    exit 0
else
    SUCCESS_RATE=$(( PASSED_TESTS * 100 / TOTAL_TESTS ))
    echo -e "${YELLOW}Status: ${PASSED_TESTS}/${TOTAL_TESTS} tests passed (${SUCCESS_RATE}%)${NC}"
    echo ""
    echo "⚠️  Some API endpoints need attention"
    echo "📝 Please check the error messages above"
    exit 1
fi
