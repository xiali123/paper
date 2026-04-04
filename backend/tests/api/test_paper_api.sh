#!/bin/bash
# PaperApi API测试脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../lib/test_utils.sh"

MODULE_NAME="PaperApi"
BASE_URL="/api/papers"

echo "=========================================="
echo "测试模块: $MODULE_NAME"
echo "=========================================="

echo ""
echo "📄 测试论文管理接口..."

test_endpoint "GET /papers" "GET" "${BASE_URL}" "200"
test_endpoint "GET /papers/:id" "GET" "${BASE_URL}/1" "200"
test_endpoint "GET /papers/search" "GET" "${BASE_URL}/search?q=test" "200"
test_endpoint "GET /papers/stats" "GET" "${BASE_URL}/stats" "200"

print_summary
save_results "${SCRIPT_DIR}/../reports/${MODULE_NAME}_results.json" "$MODULE_NAME"

[ $FAILED_TESTS -eq 0 ] && exit 0 || exit 1
