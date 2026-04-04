#!/bin/bash
# ExportApi API测试脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../lib/test_utils.sh"

MODULE_NAME="ExportApi"
BASE_URL="/api/export"

echo "=========================================="
echo "测试模块: $MODULE_NAME"
echo "=========================================="

echo ""
echo "📤 测试导出接口..."

test_endpoint "POST /export/papers" "POST" "${BASE_URL}/papers" "404" '{"format":"csv","paperIds":[1,2,3]}'
test_endpoint "GET /export/status/:id" "GET" "${BASE_URL}/status/1" "200"
test_endpoint "GET /export/download/:id" "GET" "${BASE_URL}/download/1" "404"

print_summary
save_results "${SCRIPT_DIR}/../reports/${MODULE_NAME}_results.json" "$MODULE_NAME"

[ $FAILED_TESTS -eq 0 ] && exit 0 || exit 1
