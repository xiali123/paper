#!/bin/bash
# StatsApi API测试脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../lib/test_utils.sh"

MODULE_NAME="StatsApi"
API_PREFIX="/api/stats"

echo "=========================================="
echo "测试模块: $MODULE_NAME"
echo "=========================================="

echo ""
echo "📊 测试统计接口..."

test_endpoint "GET /stats/overview" "GET" "${API_PREFIX}/overview" "200"
test_endpoint "GET /stats/papers" "GET" "${API_PREFIX}/papers" "200"
test_endpoint "GET /stats/users" "GET" "${API_PREFIX}/users" "200"
test_endpoint "GET /stats/crawler" "GET" "${API_PREFIX}/crawler" "200"

print_summary
save_results "${SCRIPT_DIR}/../reports/${MODULE_NAME}_results.json" "$MODULE_NAME"

[ $FAILED_TESTS -eq 0 ] && exit 0 || exit 1
