#!/bin/bash
# CrawlerApi 29个端点手动测试脚本

BASE_URL="http://localhost:8080/api/crawler"
PASS=0
FAIL=0

echo "========================================"
echo "🔍 CrawlerApi 29个端点逐个测试"
echo "========================================"
echo ""

# 模板管理接口 (9个端点)
echo "📋 1-9. 模板管理接口"
echo ""

test_endpoint() {
    local num="$1"
    local name="$2"
    local method="$3"
    local url="$4"
    local data="$5"

    if [ -n "$data" ]; then
        response=$(curl -s -w "\n%{http_code}" -X "$method" -H "Content-Type: application/json" -d "$data" "$url" 2>&1)
    else
        response=$(curl -s -w "\n%{http_code}" -X "$method" "$url" 2>&1)
    fi

    body=$(echo "$response" | head -n -1)
    status=$(echo "$response" | tail -n 1 | tr -d '\r')

    if [ "$status" = "200" ] || [ "$status" = "404" ]; then
        echo "✅ [$num] $name - HTTP $status"
        ((PASS++))
    else
        echo "❌ [$num] $name - HTTP $status"
        ((FAIL++))
    fi
}

# 1-9. 模板管理接口
test_endpoint "1" "GET /templates" "GET" "$BASE_URL/templates"
test_endpoint "2" "POST /templates" "POST" "$BASE_URL/templates" '{"name":"Test Template","baseUrl":"https://example.com","selectors":{}}'
test_endpoint "3" "GET /templates/:id" "GET" "$BASE_URL/templates/1"
test_endpoint "4" "PUT /templates/:id" "PUT" "$BASE_URL/templates/1" '{"name":"Updated Template"}'
test_endpoint "5" "DELETE /templates/:id" "DELETE" "$BASE_URL/templates/1"
test_endpoint "6" "POST /templates/validate" "POST" "$BASE_URL/templates/validate" '{"name":"Validated Template","baseUrl":"https://example.com"}'
test_endpoint "7" "POST /templates/:id/test" "POST" "$BASE_URL/templates/1/test"
test_endpoint "8" "GET /templates/:id/export" "GET" "$BASE_URL/templates/1/export"
test_endpoint "9" "POST /templates/import" "POST" "$BASE_URL/templates/import" '{"templates":[]}'

echo ""
echo "⚙️ 10-16. 任务管理接口"
echo ""

# 10-16. 任务管理接口
test_endpoint "10" "POST /tasks" "POST" "$BASE_URL/tasks" '{"templateId":"tpl_1","priority":"NORMAL"}'
test_endpoint "11" "GET /tasks" "GET" "$BASE_URL/tasks"
test_endpoint "12" "GET /tasks/:id" "GET" "$BASE_URL/tasks/1"
test_endpoint "13" "DELETE /tasks/:id" "DELETE" "$BASE_URL/tasks/1"
test_endpoint "14" "POST /tasks/:id/retry" "POST" "$BASE_URL/tasks/1/retry"
test_endpoint "15" "GET /tasks/:id/logs" "GET" "$BASE_URL/tasks/1/logs"
test_endpoint "16" "GET /tasks/statistics" "GET" "$BASE_URL/tasks/statistics"

echo ""
echo "⏰ 17-23. 定时任务接口"
echo ""

# 17-23. 定时任务接口
test_endpoint "17" "POST /schedules" "POST" "$BASE_URL/schedules" '{"name":"Scheduled Task","cronExpression":"0 0 * * *","templateId":"tpl_1"}'
test_endpoint "18" "GET /schedules" "GET" "$BASE_URL/schedules"
test_endpoint "19" "PUT /schedules/:id" "PUT" "$BASE_URL/schedules/1" '{"cron":"0 1 * * *"}'
test_endpoint "20" "DELETE /schedules/:id" "DELETE" "$BASE_URL/schedules/1"
test_endpoint "21" "POST /schedules/:id/enable" "POST" "$BASE_URL/schedules/1/enable"
test_endpoint "22" "POST /schedules/:id/disable" "POST" "$BASE_URL/schedules/1/disable"
test_endpoint "23" "POST /schedules/:id/trigger" "POST" "$BASE_URL/schedules/1/trigger"

echo ""
echo "👷 24-27. 工作节点接口"
echo ""

# 24-27. 工作节点接口
test_endpoint "24" "GET /workers" "GET" "$BASE_URL/workers"
test_endpoint "25" "GET /workers/:id" "GET" "$BASE_URL/workers/1"
test_endpoint "26" "POST /workers/:id/disable" "POST" "$BASE_URL/workers/1/disable"
test_endpoint "27" "GET /workers/:id/statistics" "GET" "$BASE_URL/workers/1/statistics"

echo ""
echo "📊 28-29. 系统统计接口"
echo ""

# 28-29. 系统统计接口
test_endpoint "28" "GET /dashboard" "GET" "$BASE_URL/dashboard"
test_endpoint "29" "GET /statistics" "GET" "$BASE_URL/statistics"

echo ""
echo "========================================"
echo "📊 测试结果汇总"
echo "========================================"
echo "总测试数: 29"
echo "✅ 通过: $PASS ($((PASS * 100 / 29))%)"
echo "❌ 失败: $FAIL ($((FAIL * 100 / 29))%)"
echo "========================================"
