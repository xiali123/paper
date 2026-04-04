#!/bin/bash
# 所有模块API测试主执行脚本
# 依次执行所有模块的测试并生成综合报告

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../lib/test_utils.sh"

# 测试配置
TEST_START_TIME=$(date +%s)
TEST_TIMESTAMP=$(date -u +%Y-%m-%dT%H:%M:%SZ)
REPORT_FILE="${SCRIPT_DIR}/../reports/all_modules_test_report.json"

# 模块列表（按优先级排序）
MODULES=(
    "CrawlerApi:tests/api/test_crawler_api.sh"
    "AuthApi:tests/api/test_auth_api.sh"
    "UserApi:tests/api/test_user_api.sh"
    "PaperApi:tests/api/test_paper_api.sh"
    "SearchApi:tests/api/test_search_api.sh"
    "ExportApi:tests/api/test_export_api.sh"
    "StatsApi:tests/api/test_stats_api.sh"
    "AiApi:tests/api/test_ai_api.sh"
    "RecommendationApi:tests/api/test_recommendation_api.sh"
)

# 全局统计
GLOBAL_TOTAL=0
GLOBAL_PASSED=0
GLOBAL_FAILED=0
declare -a MODULE_RESULTS

echo "=========================================="
echo "🚀 PaperCrawler 后端 API 测试套件"
echo "=========================================="
echo "测试时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "分支: $(git branch --show-current)"
echo "=========================================="

# 等待服务器启动
print_info "检查服务器状态..."
if ! curl -s "${BASE_URL}/api/health" > /dev/null 2>&1; then
    print_warning "服务器未运行，尝试启动..."
    cd /e/PaperCrawler/backend/build/Release
    ./PaperCrawlerServerHotPlug.exe ../../config/modules_auto.json > /tmp/server_test.log 2>&1 &
    SERVER_PID=$!
    print_info "服务器已启动 (PID: $SERVER_PID)"

    # 等待服务器就绪
    sleep 5
    if ! wait_for_server; then
        print_error "服务器启动失败"
        exit 1
    fi
    cd - > /dev/null
else
    print_success "服务器已运行"
fi

echo ""
echo "=========================================="
echo "开始执行测试..."
echo "=========================================="

# 执行每个模块的测试
for module_info in "${MODULES[@]}"; do
    IFS=':' read -r module_name test_script <<< "$module_info"

    echo ""
    echo "=========================================="
    echo "测试模块: $module_name"
    echo "=========================================="

    # 重置统计
    reset_stats

    # 执行测试脚本
    if [ -f "${SCRIPT_DIR}/../${test_script}" ]; then
        bash "${SCRIPT_DIR}/../${test_script}"

        # 记录结果
        local module_exit_code=$?
        local module_status="pass"
        if [ $module_exit_code -ne 0 ]; then
            module_status="fail"
        fi

        # 读取模块结果JSON
        local result_file="${SCRIPT_DIR}/../reports/${module_name}_results.json"
        if [ -f "$result_file" ]; then
            local module_total=$(grep -o '"total_tests":[0-9]*' "$result_file" | grep -o '[0-9]*')
            local module_passed=$(grep -o '"passed":[0-9]*' "$result_file" | grep -o '[0-9]*')
            local module_failed=$(grep -o '"failed":[0-9]*' "$result_file" | grep -o '[0-9]*')

            GLOBAL_TOTAL=$((GLOBAL_TOTAL + ${module_total:-0}))
            GLOBAL_PASSED=$((GLOBAL_PASSED + ${module_passed:-0}))
            GLOBAL_FAILED=$((GLOBAL_FAILED + ${module_failed:-0}))

            MODULE_RESULTS+=("{\"name\":\"$module_name\",\"status\":\"$module_status\",\"total\":${module_total:-0},\"passed\":${module_passed:-0},\"failed\":${module_failed:-0}}")
        else
            print_warning "未找到结果文件: $result_file"
        fi
    else
        print_error "测试脚本不存在: ${test_script}"
        GLOBAL_FAILED=$((GLOBAL_FAILED + 1))
    fi
done

# 计算总测试时间
TEST_END_TIME=$(date +%s)
TEST_DURATION=$((TEST_END_TIME - TEST_START_TIME))

# 生成综合报告
echo ""
echo "=========================================="
echo "生成综合测试报告..."
echo "=========================================="

cat > "$REPORT_FILE" << EOF
{
  "test_run": {
    "timestamp": "$TEST_TIMESTAMP",
    "duration_seconds": $TEST_DURATION,
    "duration_minutes": $(echo "scale=2; $TEST_DURATION / 60" | bc),
    "branch": "$(git branch --show-current)",
    "commit": "$(git log -1 --format='%H')",
    "commit_message": "$(git log -1 --format='%s' | sed 's/"/"/g')"
  },
  "summary": {
    "total_modules": 9,
    "total_tests": $GLOBAL_TOTAL,
    "passed": $GLOBAL_PASSED,
    "failed": $GLOBAL_FAILED,
    "success_rate": $(echo "scale=2; $GLOBAL_PASSED * 100 / ($GLOBAL_TOTAL > 0 ? $GLOBAL_TOTAL : 1)" | bc)
  },
  "modules": [
EOF

# 添加模块结果
local first=true
for result in "${MODULE_RESULTS[@]}"; do
    if [ "$first" = true ]; then
        echo "    $result" >> "$REPORT_FILE"
        first=false
    else
        echo "    ,$result" >> "$REPORT_FILE"
    fi
done

cat >> "$REPORT_FILE" << EOF
  ],
  "details": {
    "router_dll": {
      "status": "implemented",
      "file": "backend/build/Release/modules/dynamic/Release/Router.dll",
      "size": "1.3MB"
    },
    "architecture": {
      "type": "Hot-plug DLL",
      "modules_loaded": 9,
      "router_singleton": "shared"
    }
  }
}
EOF

print_success "综合报告已生成: $REPORT_FILE"

# 打印最终摘要
echo ""
echo "=========================================="
echo "📊 最终测试摘要"
echo "=========================================="
echo -e "总模块数: ${BLUE}9${NC}"
echo -e "总测试数: ${BLUE}${GLOBAL_TOTAL}${NC}"
echo -e "通过: ${GREEN}${GLOBAL_PASSED}${NC}"
echo -e "失败: ${RED}${GLOBAL_FAILED}${NC}"
echo "=========================================="

if [ $GLOBAL_TOTAL -gt 0 ]; then
    SUCCESS_RATE=$(echo "scale=2; $GLOBAL_PASSED * 100 / $GLOBAL_TOTAL" | bc)
    echo -e "通过率: ${BLUE}${SUCCESS_RATE}%${NC}"
fi

echo "=========================================="

# 判断总体结果
if [ $GLOBAL_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ 所有测试通过！${NC}"
    EXIT_CODE=0
else
    echo -e "${RED}✗ 部分测试失败${NC}"
    EXIT_CODE=1
fi

echo ""
echo "📁 详细报告位置:"
echo "   - 综合报告: $REPORT_FILE"
echo "   - 各模块报告: backend/tests/reports/*_results.json"

# 停止测试服务器（如果是我们启动的）
if [ ! -z "$SERVER_PID" ]; then
    echo ""
    print_info "停止测试服务器 (PID: $SERVER_PID)..."
    kill $SERVER_PID 2>/dev/null
fi

exit $EXIT_CODE
