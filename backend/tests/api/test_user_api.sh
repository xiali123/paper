#!/bin/bash
# UserApi API测试脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../lib/test_utils.sh"

MODULE_NAME="UserApi"
BASE_URL="/api/users"

echo "=========================================="
echo "测试模块: $MODULE_NAME"
echo "=========================================="

echo ""
echo "👤 测试用户管理接口..."

test_endpoint "GET /users" "GET" "${BASE_URL}" "200"
test_endpoint "GET /users/:id" "GET" "${BASE_URL}/1" "200"
test_endpoint "POST /users" "POST" "${BASE_URL}" "404" '{"username":"test","email":"test@example.com"}'
test_endpoint "PUT /users/:id" "PUT" "${BASE_URL}/1" "404" '{"email":"new@example.com"}'
test_endpoint "DELETE /users/:id" "DELETE" "${BASE_URL}/1" "404"

print_summary
save_results "${SCRIPT_DIR}/../reports/${MODULE_NAME}_results.json" "$MODULE_NAME"

[ $FAILED_TESTS -eq 0 ] && exit 0 || exit 1
