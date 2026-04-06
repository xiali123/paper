#!/bin/bash
# CrawlerApiModule API测试脚本
# 测试所有32个API端点

BASE_URL="http://localhost:8080"
PREFIX="/api/crawler"

echo "======================================"
echo "CrawlerApiModule API测试"
echo "======================================"
echo ""

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 测试计数器
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试函数
test_api() {
    local name="$1"
    local method="$2"
    local endpoint="$3"
    local data="$4"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    echo -n "[$TOTAL_TESTS] $name ... "

    if [ -n "$data" ]; then
        response=$(curl -s -X "$method" "$BASE_URL$PREFIX$endpoint" \
            -H "Content-Type: application/json" \
            -d "$data" \
            -w "\n%{http_code}")
    else
        response=$(curl -s -X "$method" "$BASE_URL$PREFIX$endpoint" \
            -w "\n%{http_code}")
    fi

    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n -1)

    if [ "$http_code" -ge 200 ] && [ "$http_code" -lt 300 ]; then
        echo -e "${GREEN}PASSED${NC} (HTTP $http_code)"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAILED${NC} (HTTP $http_code)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        echo "  Response: $body"
    fi
}

echo "1. 模板管理接口 (8个)"
echo "-------------------"

test_api "创建模板" "POST" "/templates" '{
  "templateId": "test_template_001",
  "name": "测试模板",
  "baseUrl": "https://example.com",
  "method": "GET"
}'

test_api "列出模板" "GET" "/templates"

test_api "获取模板详情" "GET" "/templates/test_template_001"

test_api "验证模板" "POST" "/templates/validate" '{
  "templateId": "test_template_001",
  "name": "测试模板",
  "baseUrl": "https://example.com"
}'

test_api "测试模板" "POST" "/templates/test_template_001/test" '{}'

test_api "更新模板" "PUT" "/templates/test_template_001" '{
  "name": "更新后的测试模板"
}'

test_api "导出模板" "GET" "/templates/test_template_001/export"

test_api "删除模板" "DELETE" "/templates/test_template_001"

echo ""
echo "2. 任务管理接口 (6个)"
echo "-------------------"

test_api "创建任务" "POST" "/tasks" '{
  "templateId": "cvpr_template",
  "priority": "NORMAL"
}'

test_api "列出任务" "GET" "/tasks"

test_api "获取任务详情" "GET" "/tasks/task_123"

test_api "取消任务" "DELETE" "/tasks/task_123"

test_api "重试任务" "POST" "/tasks/task_123/retry"

test_api "获取任务日志" "GET" "/tasks/task_123/logs"

echo ""
echo "3. 定时任务接口 (7个)"
echo "-------------------"

test_api "创建定时任务" "POST" "/schedules" '{
  "name": "每日论文爬取",
  "templateId": "cvpr_template",
  "cronExpression": "0 2 * * *"
}'

test_api "列出定时任务" "GET" "/schedules"

test_api "更新定时任务" "PUT" "/schedules/schedule_123" '{
  "name": "更新后的定时任务"
}'

test_api "删除定时任务" "DELETE" "/schedules/schedule_123"

test_api "启用定时任务" "POST" "/schedules/schedule_123/enable"

test_api "禁用定时任务" "POST" "/schedules/schedule_123/disable"

test_api "触发定时任务" "POST" "/schedules/schedule_123/trigger"

echo ""
echo "4. 工作节点接口 (4个)"
echo "-------------------"

test_api "列出工作节点" "GET" "/workers"

test_api "获取工作节点详情" "GET" "/workers/worker_001"

test_api "禁用工作节点" "POST" "/workers/worker_001/disable"

test_api "获取工作节点统计" "GET" "/workers/worker_001/statistics"

echo ""
echo "5. 统计接口 (2个)"
echo "-----------------"

test_api "获取系统仪表盘" "GET" "/dashboard"

test_api "获取系统统计" "GET" "/statistics"

echo ""
echo "6. 任务统计接口 (1个)"
echo "-------------------"

test_api "获取任务统计" "GET" "/tasks/statistics"

echo ""
echo "======================================"
echo "测试结果汇总"
echo "======================================"
echo -e "总测试数: $TOTAL_TESTS"
echo -e "${GREEN}通过: $PASSED_TESTS${NC}"
echo -e "${RED}失败: $FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n${GREEN}🎉 所有测试通过！${NC}"
    exit 0
else
    echo -e "\n${RED}❌ 有 $FAILED_TESTS 个测试失败${NC}"
    exit 1
fi
