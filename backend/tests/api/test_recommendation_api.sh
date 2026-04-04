#!/bin/bash
# RecommendationApi API测试脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../lib/test_utils.sh"

MODULE_NAME="RecommendationApi"
BASE_URL="/api/recommendations"

echo "=========================================="
echo "测试模块: $MODULE_NAME"
echo "=========================================="

echo ""
echo "💡 测试推荐接口..."

test_endpoint "GET /recommendations/papers" "GET" "${BASE_URL}/papers?userId=1" "200"
test_endpoint "GET /recommendations/related" "GET" "${BASE_URL}/related?paperId=1" "200"
test_endpoint "GET /recommendations/trending" "GET" "${BASE_URL}/trending" "200"

print_summary
save_results "${SCRIPT_DIR}/../reports/${MODULE_NAME}_results.json" "$MODULE_NAME"

[ $FAILED_TESTS -eq 0 ] && exit 0 || exit 1
