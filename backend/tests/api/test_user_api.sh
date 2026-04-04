#!/bin/bash
# UserApi API测试脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../lib/test_utils.sh"

MODULE_NAME="UserApi"
API_PREFIX="/api/users"

echo "=========================================="
echo "测试模块: $MODULE_NAME"
echo "=========================================="

echo ""
echo "👤 测试用户管理接口..."

test_endpoint "GET /users" "GET" "${API_PREFIX}" "200"
test_endpoint "GET /users/:id" "GET" "${API_PREFIX}/1" "200"
test_endpoint "POST /users" "POST" "${API_PREFIX}" "404" '{"username":"test","email":"test@example.com"}'
test_endpoint "PUT /users/:id" "PUT" "${API_PREFIX}/1" "404" '{"email":"new@example.com"}'
test_endpoint "DELETE /users/:id" "DELETE" "${API_PREFIX}/1" "404"

print_summary
save_results "${SCRIPT_DIR}/../reports/${MODULE_NAME}_results.json" "$MODULE_NAME"

[ $FAILED_TESTS -eq 0 ] && exit 0 || exit 1
