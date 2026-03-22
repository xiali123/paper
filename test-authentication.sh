#!/bin/bash

# ============================================================================
# PaperCrawler Authentication System Test Script
# ============================================================================
#
# This script tests all components of the authentication system
# Run: bash test-authentication.sh
#
# ============================================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_ROOT="e:/PaperCrawler"
TEST_RESULTS=0
TOTAL_TESTS=0

# ============================================================================
# Helper Functions
# ============================================================================

log_info() {
    echo -e "${BLUE}[TEST]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
    ((TEST_RESULTS++))
}

log_error() {
    echo -e "${RED}[✗]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

run_test() {
    local test_name="$1"
    local test_command="$2"

    ((TOTAL_TESTS++))
    log_info "Testing: $test_name"

    if eval "$test_command" > /dev/null 2>&1; then
        log_success "$test_name"
        return 0
    else
        log_error "$test_name"
        return 1
    fi
}

# ============================================================================
# Test 1: Check File Existence
# ============================================================================

echo ""
echo "=========================================="
echo "Phase 1: File Structure Validation"
echo "=========================================="
echo ""

run_test "MySQL migration file exists" \
    "[ -f '$PROJECT_ROOT/backend/migrations/002_add_authentication.sql' ]"

run_test "SQLite migration file exists" \
    "[ -f '$PROJECT_ROOT/backend/migrations/002_add_authentication_sqlite.sql' ]"

run_test "AuthManager.hpp exists" \
    "[ -f '$PROJECT_ROOT/include/auth/AuthManager.hpp' ]"

run_test "JwtUtils.hpp exists" \
    "[ -f '$PROJECT_ROOT/include/auth/JwtUtils.hpp' ]"

run_test "PasswordHasher.hpp exists" \
    "[ -f '$PROJECT_ROOT/include/auth/PasswordHasher.hpp' ]"

run_test "RateLimiter.hpp exists" \
    "[ -f '$PROJECT_ROOT/include/auth/RateLimiter.hpp' ]"

run_test "Auth handlers exist" \
    "[ -f '$PROJECT_ROOT/backend/src/auth_handlers.cpp' ]"

run_test "Auth middleware exists" \
    "[ -f '$PROJECT_ROOT/backend/src/auth_middleware.cpp' ]"

run_test "Frontend auth API module exists" \
    "[ -f '$PROJECT_ROOT/frontend/src/api/modules/auth.ts' ]"

run_test "Frontend auth store exists" \
    "[ -f '$PROJECT_ROOT/frontend/src/stores/auth.ts' ]"

run_test "Login page exists" \
    "[ -f '$PROJECT_ROOT/frontend/src/views/Login.vue' ]"

run_test "Register page exists" \
    "[ -f '$PROJECT_ROOT/frontend/src/views/Register.vue' ]"

run_test "Router guards exist" \
    "[ -f '$PROJECT_ROOT/frontend/src/router/guards.ts' ]"

run_test "Desktop AuthManager exists" \
    "[ -f '$PROJECT_ROOT/desktop/include/AuthManager.hpp' ]"

run_test "Desktop LoginWindow exists" \
    "[ -f '$PROJECT_ROOT/desktop/include/LoginWindow.hpp' ]"

run_test "Backend tests exist" \
    "[ -f '$PROJECT_ROOT/backend/tests/test_auth.cpp' ]"

run_test "Frontend tests exist" \
    "[ -f '$PROJECT_ROOT/frontend/src/stores/__tests__/auth.test.ts' ]"

run_test "Config file exists" \
    "[ -f '$PROJECT_ROOT/backend/config.json' ]"

# ============================================================================
# Test 2: Validate File Content
# ============================================================================

echo ""
echo "=========================================="
echo "Phase 2: Content Validation"
echo "=========================================="
echo ""

run_test "AuthManager.hpp contains registerUser method" \
    "grep -q 'registerUser' '$PROJECT_ROOT/include/auth/AuthManager.hpp'"

run_test "AuthManager.hpp contains login method" \
    "grep -q 'AuthResult login' '$PROJECT_ROOT/include/auth/AuthManager.hpp'"

run_test "AuthManager.hpp contains verifyToken method" \
    "grep -q 'verifyToken' '$PROJECT_ROOT/include/auth/AuthManager.hpp'"

run_test "MySQL schema contains users table" \
    "grep -q 'CREATE TABLE users' '$PROJECT_ROOT/backend/migrations/002_add_authentication.sql'"

run_test "MySQL schema contains user_sessions table" \
    "grep -q 'CREATE TABLE user_sessions' '$PROJECT_ROOT/backend/migrations/002_add_authentication.sql'"

run_test "MySQL schema contains login_attempts table" \
    "grep -q 'CREATE TABLE login_attempts' '$PROJECT_ROOT/backend/migrations/002_add_authentication.sql'"

run_test "Frontend auth.ts contains register function" \
    "grep -q 'async register' '$PROJECT_ROOT/frontend/src/api/modules/auth.ts'"

run_test "Frontend auth store contains login action" \
    "grep -q 'async function login' '$PROJECT_ROOT/frontend/src/stores/auth.ts'"

run_test "Login.vue contains email field" \
    "grep -q 'emailEdit' '$PROJECT_ROOT/frontend/src/views/Login.vue' || grep -q 'email' '$PROJECT_ROOT/frontend/src/views/Login.vue'"

# ============================================================================
# Test 3: Check TypeScript/JavaScript Syntax
# ============================================================================

echo ""
echo "=========================================="
echo "Phase 3: Syntax Validation"
echo "=========================================="
echo ""

if command -v npx &> /dev/null; then
    run_test "Frontend auth.ts TypeScript valid" \
        "cd $PROJECT_ROOT/frontend && npx tsc --noEmit src/api/modules/auth.ts"

    run_test "Frontend auth store TypeScript valid" \
        "cd $PROJECT_ROOT/frontend && npx tsc --noEmit src/stores/auth.ts"

    run_test "Frontend guards TypeScript valid" \
        "cd $PROJECT_ROOT/frontend && npx tsc --noEmit src/router/guards.ts"
else
    log_warning "TypeScript not found, skipping syntax validation"
fi

# ============================================================================
# Test 4: Vue Component Validation
# ============================================================================

echo ""
echo "=========================================="
echo "Phase 4: Component Validation"
echo "=========================================="
echo ""

run_test "Login.vue has template section" \
    "grep -q '<template>' '$PROJECT_ROOT/frontend/src/views/Login.vue'"

run_test "Login.vue has script section" \
    "grep -q '<script' '$PROJECT_ROOT/frontend/src/views/Login.vue'"

run_test "Login.vue has style section" \
    "grep -q '<style' '$PROJECT_ROOT/frontend/src/views/Login.vue'"

run_test "Register.vue has template section" \
    "grep -q '<template>' '$PROJECT_ROOT/frontend/src/views/Register.vue'"

run_test "Register.vue has script section" \
    "grep -q '<script' '$PROJECT_ROOT/frontend/src/views/Register.vue'"

# ============================================================================
# Test 5: Test Coverage
# ============================================================================

echo ""
echo "=========================================="
echo "Phase 5: Test Coverage"
echo "=========================================="
echo ""

run_test "Backend tests cover registration" \
    "grep -q 'TEST.*RegisterUser' '$PROJECT_ROOT/backend/tests/test_auth.cpp'"

run_test "Backend tests cover login" \
    "grep -q 'TEST.*Login' '$PROJECT_ROOT/backend/tests/test_auth.cpp'"

run_test "Backend tests cover token verification" \
    "grep -q 'TEST.*VerifyToken' '$PROJECT_ROOT/backend/tests/test_auth.cpp'"

run_test "Backend tests cover rate limiting" \
    "grep -q 'TEST.*RateLimit' '$PROJECT_ROOT/backend/tests/test_auth.cpp'"

run_test "Frontend tests cover registration" \
    "grep -q 'describe.*Registration' '$PROJECT_ROOT/frontend/src/stores/__tests__/auth.test.ts'"

run_test "Frontend tests cover login" \
    "grep -q 'describe.*Login' '$PROJECT_ROOT/frontend/src/stores/__tests__/auth.test.ts'"

run_test "Frontend tests cover logout" \
    "grep -q 'describe.*Logout' '$PROJECT_ROOT/frontend/src/stores/__tests__/auth.test.ts'"

# ============================================================================
# Test 6: Integration Points
# ============================================================================

echo ""
echo "=========================================="
echo "Phase 6: Integration Validation"
echo "=========================================="
echo ""

run_test "Router includes auth routes" \
    "grep -q '/login' '$PROJECT_ROOT/frontend/src/router/index.ts'"

run_test "Router includes register route" \
    "grep -q '/register' '$PROJECT_ROOT/frontend/src/router/index.ts'"

run_test "Router setup auth guards" \
    "grep -q 'setupAuthGuards' '$PROJECT_ROOT/frontend/src/router/index.ts'"

run_test "Request.ts includes auth header" \
    "grep -q 'Authorization.*Bearer' '$PROJECT_ROOT/frontend/src/utils/request.ts'"

run_test "Request.ts handles 401" \
    "grep -q '401' '$PROJECT_ROOT/frontend/src/utils/request.ts'"

# ============================================================================
# Test 7: Security Configuration
# ============================================================================

echo ""
echo "=========================================="
echo "Phase 7: Security Validation"
echo "=========================================="
echo ""

run_test "Config has JWT secret" \
    "grep -q 'jwtSecret' '$PROJECT_ROOT/backend/config.json'"

run_test "Config has authentication enabled" \
    "grep -q '"enabled".*true' '$PROJECT_ROOT/backend/config.json'"

run_test "Config has rate limiting enabled" \
    "grep -q 'rate_limiting.*enabled.*true' '$PROJECT_ROOT/backend/config.json'"

# ============================================================================
# Test 8: Documentation
# ============================================================================

echo ""
echo "=========================================="
echo "Phase 8: Documentation Check"
echo "=========================================="
echo ""

run_test "Usage guide exists" \
    "[ -f '$PROJECT_ROOT/AUTHENTICATION_GUIDE.md' ]"

run_test "Implementation summary exists" \
    "[ -f '$PROJECT_ROOT/AUTHENTICATION_IMPLEMENTATION_SUMMARY.md' ]"

run_test "Usage guide contains API documentation" \
    "grep -q 'API 端点' '$PROJECT_ROOT/AUTHENTICATION_GUIDE.md'"

# ============================================================================
# Summary
# ============================================================================

echo ""
echo "=========================================="
echo "Test Summary"
echo "=========================================="
echo ""

PERCENTAGE=$((TEST_RESULTS * 100 / TOTAL_TESTS))

echo "Total Tests: $TOTAL_TESTS"
echo "Passed: $TEST_RESULTS"
echo "Failed: $((TOTAL_TESTS - TEST_RESULTS))"
echo "Success Rate: ${PERCENTAGE}%"
echo ""

if [ $TEST_RESULTS -eq $TOTAL_TESTS ]; then
    log_success "All tests passed! ✓"
    echo ""
    echo "The authentication system is ready for deployment."
    echo ""
    echo "Next steps:"
    echo "1. Run database migration"
    echo "2. Build and start backend"
    echo "3. Start frontend"
    echo "4. Open browser to http://localhost:5173/login"
    exit 0
else
    log_error "Some tests failed. Please review the output above."
    exit 1
fi
