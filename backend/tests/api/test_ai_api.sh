#!/bin/bash
# AiApi API测试脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../lib/test_utils.sh"

MODULE_NAME="AiApi"
API_PREFIX="/api/ai"

echo "=========================================="
echo "测试模块: $MODULE_NAME"
echo "=========================================="

echo ""
echo "🤖 测试AI接口..."

test_endpoint "POST /ai/chat" "POST" "${API_PREFIX}/chat" "404" '{"message":"Hello AI"}'
test_endpoint "POST /ai/summarize" "POST" "${API_PREFIX}/summarize" "404" '{"text":"需要总结的文本"}'
test_endpoint "POST /ai/keywords" "POST" "${API_PREFIX}/keywords" "404" '{"text":"提取关键词"}'
test_endpoint "GET /ai/history" "GET" "${API_PREFIX}/history" "200"

print_summary
save_results "${SCRIPT_DIR}/../reports/${MODULE_NAME}_results.json" "$MODULE_NAME"

[ $FAILED_TESTS -eq 0 ] && exit 0 || exit 1
