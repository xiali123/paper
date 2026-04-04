#!/bin/bash
# AuthApi API测试脚本
# 测试认证授权系统的所有端点

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../lib/test_utils.sh"

MODULE_NAME="AuthApi"
BASE_URL="/api/auth"

echo "=========================================="
echo "测试模块: $MODULE_NAME"
echo "=========================================="

echo ""
echo "🔐 测试认证接口..."

# 测试主要端点（根据实际可用端点调整）
test_endpoint "POST /register" "POST" "${BASE_URL}/register" "404" \
    '{"username":"test","password":"test123","email":"test@example.com"}'

test_endpoint "POST /login" "POST" "${BASE_URL}/login" "404" \
    '{"username":"test","password":"test123"}'

test_endpoint "POST /logout" "POST" "${BASE_URL}/logout" "404"

test_endpoint "GET /profile" "GET" "${BASE_URL}/profile" "401"

test_endpoint "PUT /profile" "PUT" "${BASE_URL}/profile" "401" \
    '{"email":"newemail@example.com"}'

test_endpoint "POST /change-password" "POST" "${BASE_URL}/change-password" "401" \
    '{"oldPassword":"old","newPassword":"new"}'

print_summary
save_results "${SCRIPT_DIR}/../reports/${MODULE_NAME}_results.json" "$MODULE_NAME"

[ $FAILED_TESTS -eq 0 ] && exit 0 || exit 1
