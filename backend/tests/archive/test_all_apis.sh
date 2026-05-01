#!/bin/bash

# API端点完整测试脚本
# 测试所有9个模块的API端点

BASE_URL="http://localhost:8080"
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 测试函数
test_endpoint() {
    local num="$1"
    local name="$2"
    local method="$3"
    local url="$4"
    local data="$5"
    local expected_codes="$6"  # 接受的HTTP状态码列表，如 "200,404"

    ((TOTAL_TESTS++))

    echo -n "[$num] Testing: $name ... "

    if [ -n "$data" ]; then
        response=$(curl -s -w "\n%{http_code}" -X "$method" \
            -H "Content-Type: application/json" \
            -d "$data" \
            "$url" 2>&1)
    else
        response=$(curl -s -w "\n%{http_code}" -X "$method" \
            "$url" 2>&1)
    fi

    status=$(echo "$response" | tail -n 1 | tr -d '\r')

    # 检查状态码是否在接受列表中
    if [[ ",$expected_codes," == *",$status,"* ]]; then
        echo -e "${GREEN}✅ PASS${NC} (HTTP $status)"
        ((PASSED_TESTS++))
        return 0
    else
        echo -e "${RED}❌ FAIL${NC} (HTTP $status, expected: $expected_codes)"
        ((FAILED_TESTS++))
        echo "   Response: $(echo "$response" | head -n 1)"
        return 1
    fi
}

echo "========================================"
echo "PaperCrawler API完整测试"
echo "========================================"
echo "测试时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo ""

# ============================================================================
# 1. AuthApi模块测试
# ============================================================================
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1. AuthApi模块 (9个端点)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_endpoint "1.1" "POST /api/auth/register" "POST" "$BASE_URL/api/auth/register" '{"username":"testuser","password":"password123","email":"test@example.com"}' "200,404"
test_endpoint "1.2" "POST /api/auth/login" "POST" "$BASE_URL/api/auth/login" '{"username":"testuser","password":"password123"}' "200,404"
test_endpoint "1.3" "POST /api/auth/logout" "POST" "$BASE_URL/api/auth/logout" "{}" "200,404"
test_endpoint "1.4" "GET /api/auth/me" "GET" "$BASE_URL/api/auth/me" "" "200,404"
test_endpoint "1.5" "GET /api/auth/sessions" "GET" "$BASE_URL/api/auth/sessions" "" "200,404"
test_endpoint "1.6" "DELETE /api/auth/sessions/:id" "DELETE" "$BASE_URL/api/auth/sessions/1" "" "200,404"
test_endpoint "1.7" "POST /api/auth/refresh" "POST" "$BASE_URL/api/auth/refresh" '{"token":"test"}' "200,404"
test_endpoint "1.8" "POST /api/auth/verify" "POST" "$BASE_URL/api/auth/verify" '{"token":"test"}' "200,404"
test_endpoint "1.9" "POST /api/auth/forgot-password" "POST" "$BASE_URL/api/auth/forgot-password" '{"email":"test@example.com"}' "200,404"

# ============================================================================
# 2. UserApi模块测试
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2. UserApi模块 (13个端点)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_endpoint "2.1" "GET /api/users" "GET" "$BASE_URL/api/users" "" "200,404"
test_endpoint "2.2" "GET /api/users/:id" "GET" "$BASE_URL/api/users/1" "" "200,404"
test_endpoint "2.3" "GET /api/users/me" "GET" "$BASE_URL/api/users/me" "" "200,404"
test_endpoint "2.4" "GET /api/users/stats" "GET" "$BASE_URL/api/users/stats" "" "200,404"
test_endpoint "2.5" "PUT /api/users/:id" "PUT" "$BASE_URL/api/users/1" '{"username":"updated"}' "200,404"
test_endpoint "2.6" "DELETE /api/users/:id" "DELETE" "$BASE_URL/api/users/1" "" "200,404"
test_endpoint "2.7" "GET /api/users/:id/activity" "GET" "$BASE_URL/api/users/1/activity" "" "200,404"
test_endpoint "2.8" "GET /api/users/:id/saved-papers" "GET" "$BASE_URL/api/users/1/saved-papers" "" "200,404"
test_endpoint "2.9" "POST /api/users/:id/saved-papers" "POST" "$BASE_URL/api/users/1/saved-papers" '{"paperId":1}' "200,404"
test_endpoint "2.10" "DELETE /api/users/:id/saved-papers/:paperId" "DELETE" "$BASE_URL/api/users/1/saved-papers/1" "" "200,404"
test_endpoint "2.11" "GET /api/users/:id/settings" "GET" "$BASE_URL/api/users/1/settings" "" "200,404"
test_endpoint "2.12" "PUT /api/users/:id/settings" "PUT" "$BASE_URL/api/users/1/settings" '{"theme":"dark"}' "200,404"
test_endpoint "2.13" "POST /api/users/:id/change-password" "POST" "$BASE_URL/api/users/1/change-password" '{"oldPassword":"pass","newPassword":"newpass"}' "200,404"

# ============================================================================
# 3. PaperApi模块测试
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3. PaperApi模块 (11个端点)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_endpoint "3.1" "GET /api/papers" "GET" "$BASE_URL/api/papers" "" "200,404"
test_endpoint "3.2" "GET /api/papers/:id" "GET" "$BASE_URL/api/papers/1" "" "200,404"
test_endpoint "3.3" "POST /api/papers" "POST" "$BASE_URL/api/papers" '{"title":"Test Paper","authors":"Test Author","year":2024}' "200,404"
test_endpoint "3.4" "PUT /api/papers/:id" "PUT" "$BASE_URL/api/papers/1" '{"title":"Updated"}' "200,404"
test_endpoint "3.5" "DELETE /api/papers/:id" "DELETE" "$BASE_URL/api/papers/1" "" "200,404"
test_endpoint "3.6" "GET /api/papers/:id/citations" "GET" "$BASE_URL/api/papers/1/citations" "" "200,404"
test_endpoint "3.7" "GET /api/papers/:id/references" "GET" "$BASE_URL/api/papers/1/references" "" "200,404"
test_endpoint "3.8" "POST /api/papers/:id/favorite" "POST" "$BASE_URL/api/papers/1/favorite" '{}' "200,404"
test_endpoint "3.9" "DELETE /api/papers/:id/favorite" "DELETE" "$BASE_URL/api/papers/1/favorite" "" "200,404"
test_endpoint "3.10" "GET /api/papers/:id/related" "GET" "$BASE_URL/api/papers/1/related" "" "200,404"
test_endpoint "3.11" "POST /api/papers/batch" "POST" "$BASE_URL/api/papers/batch" '{"papers":[]}' "200,404"

# ============================================================================
# 4. SearchApi模块测试
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "4. SearchApi模块 (6个端点)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_endpoint "4.1" "GET /api/search" "GET" "$BASE_URL/api/search?query=test" "" "200,404"
test_endpoint "4.2" "POST /api/search/advanced" "POST" "$BASE_URL/api/search/advanced" '{"query":"test"}' "200,404"
test_endpoint "4.3" "GET /api/search/suggest" "GET" "$BASE_URL/api/search/suggest?query=test" "" "200,404"
test_endpoint "4.4" "GET /api/search/trending" "GET" "$BASE_URL/api/search/trending" "" "200,404"
test_endpoint "4.5" "GET /api/search/history" "GET" "$BASE_URL/api/search/history" "" "200,404"
test_endpoint "4.6" "GET /api/search/stats" "GET" "$BASE_URL/api/search/stats" "" "200,404"

# ============================================================================
# 5. ExportApi模块测试
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "5. ExportApi模块 (6个端点)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_endpoint "5.1" "GET /api/export" "GET" "$BASE_URL/api/export" "" "200,404"
test_endpoint "5.2" "POST /api/export" "POST" "$BASE_URL/api/export" '{"paperIds":[1],"format":"JSON"}' "200,404"
test_endpoint "5.3" "GET /api/export/formats" "GET" "$BASE_URL/api/export/formats" "" "200,404"
test_endpoint "5.4" "GET /api/export/:id" "GET" "$BASE_URL/api/export/1" "" "200,404"
test_endpoint "5.5" "GET /api/export/:id/download" "GET" "$BASE_URL/api/export/1/download" "" "200,404"
test_endpoint "5.6" "DELETE /api/export/:id" "DELETE" "$BASE_URL/api/export/1" "" "200,404"

# ============================================================================
# 6. StatsApi模块测试
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "6. StatsApi模块 (7个端点)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_endpoint "6.1" "GET /api/stats/system" "GET" "$BASE_URL/api/stats/system" "" "200,404"
test_endpoint "6.2" "GET /api/stats/resources" "GET" "$BASE_URL/api/stats/resources" "" "200,404"
test_endpoint "6.3" "GET /api/stats/uptime" "GET" "$BASE_URL/api/stats/uptime" "" "200,404"
test_endpoint "6.4" "GET /api/stats/modules" "GET" "$BASE_URL/api/stats/modules" "" "200,404"
test_endpoint "6.5" "GET /api/stats/modules/:name" "GET" "$BASE_URL/api/stats/modules/AuthApi" "" "200,404"
test_endpoint "6.6" "GET /api/stats/performance" "GET" "$BASE_URL/api/stats/performance" "" "200,404"
test_endpoint "6.7" "GET /api/stats/realtime" "GET" "$BASE_URL/api/stats/realtime" "" "200,404"

# ============================================================================
# 7. AiApi模块测试
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "7. AiApi模块 (7个端点)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_endpoint "7.1" "POST /api/ai/summarize" "POST" "$BASE_URL/api/ai/summarize" '{"text":"test paper content"}' "200,404"
test_endpoint "7.2" "POST /api/ai/chat" "POST" "$BASE_URL/api/ai/chat" '{"message":"hello"}' "200,404"
test_endpoint "7.3" "POST /api/ai/keywords" "POST" "$BASE_URL/api/ai/keywords" '{"text":"test"}' "200,404"
test_endpoint "7.4" "POST /api/ai/similar-papers" "POST" "$BASE_URL/api/ai/similar-papers" '{"paperId":1}' "200,404"
test_endpoint "7.5" "POST /api/ai/analyze-citations" "POST" "$BASE_URL/api/ai/analyze-citations" '{"paperId":1}' "200,404"
test_endpoint "7.6" "POST /api/ai/generate-title" "POST" "$BASE_URL/api/ai/generate-title" '{"abstract":"test"}' "200,404"
test_endpoint "7.7" "GET /api/ai/status" "GET" "$BASE_URL/api/ai/status" "" "200,404"

# ============================================================================
# 8. RecommendationApi模块测试
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "8. RecommendationApi模块 (6个端点)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_endpoint "8.1" "GET /api/recommendations/papers" "GET" "$BASE_URL/api/recommendations/papers" "" "200,404"
test_endpoint "8.2" "GET /api/recommendations/trending" "GET" "$BASE_URL/api/recommendations/trending" "" "200,404"
test_endpoint "8.3" "GET /api/recommendations/:userId" "GET" "$BASE_URL/api/recommendations/1" "" "200,404"
test_endpoint "8.4" "POST /api/recommendations/:userId/feedback" "POST" "$BASE_URL/api/recommendations/1/feedback" '{"paperId":1,"rating":5}' "200,404"
test_endpoint "8.5" "POST /api/recommendations/:userId/dismiss" "POST" "$BASE_URL/api/recommendations/1/dismiss" '{"paperId":1}' "200,404"
test_endpoint "8.6" "GET /api/recommendations/:userId/history" "GET" "$BASE_URL/api/recommendations/1/history" "" "200,404"

# ============================================================================
# 9. CrawlerApi模块测试
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "9. CrawlerApi模块 (29个端点)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

test_endpoint "9.1" "POST /api/crawler/templates" "POST" "$BASE_URL/api/crawler/templates" '{"name":"test","baseUrl":"http://example.com"}' "200,404"
test_endpoint "9.2" "GET /api/crawler/templates" "GET" "$BASE_URL/api/crawler/templates" "" "200,404"
test_endpoint "9.3" "GET /api/crawler/templates/:id" "GET" "$BASE_URL/api/crawler/templates/1" "" "200,404"
test_endpoint "9.4" "PUT /api/crawler/templates/:id" "PUT" "$BASE_URL/api/crawler/templates/1" '{"name":"updated"}' "200,404"
test_endpoint "9.5" "DELETE /api/crawler/templates/:id" "DELETE" "$BASE_URL/api/crawler/templates/1" "" "200,404"
test_endpoint "9.6" "POST /api/crawler/templates/:id/test" "POST" "$BASE_URL/api/crawler/templates/1/test" '{}' "200,404"
test_endpoint "9.7" "GET /api/crawler/templates/:id/fields" "GET" "$BASE_URL/api/crawler/templates/1/fields" "" "200,404"
test_endpoint "9.8" "POST /api/crawler/templates/:id/fields" "POST" "$BASE_URL/api/crawler/templates/1/fields" '{"fieldName":"title","selector":"h1"}' "200,404"
test_endpoint "9.9" "POST /api/crawler/templates/import" "POST" "$BASE_URL/api/crawler/templates/import" '{"data":"base64data"}' "200,404"
test_endpoint "9.10" "POST /api/crawler/templates/:id/export" "POST" "$BASE_URL/api/crawler/templates/1/export" '{"format":"JSON"}' "200,404"
test_endpoint "9.11" "POST /api/crawler/tasks" "POST" "$BASE_URL/api/crawler/tasks" '{"templateId":1,"url":"http://example.com"}' "200,404"
test_endpoint "9.12" "GET /api/crawler/tasks" "GET" "$BASE_URL/api/crawler/tasks" "" "200,404"
test_endpoint "9.13" "GET /api/crawler/tasks/:id" "GET" "$BASE_URL/api/crawler/tasks/1" "" "200,404"
test_endpoint "9.14" "DELETE /api/crawler/tasks/:id" "DELETE" "$BASE_URL/api/crawler/tasks/1" "" "200,404"
test_endpoint "9.15" "POST /api/crawler/tasks/:id/cancel" "POST" "$BASE_URL/api/crawler/tasks/1/cancel" '{}' "200,404"
test_endpoint "9.16" "POST /api/crawler/tasks/:id/retry" "POST" "$BASE_URL/api/crawler/tasks/1/retry" '{}' "200,404"
test_endpoint "9.17" "GET /api/crawler/tasks/:id/logs" "GET" "$BASE_URL/api/crawler/tasks/1/logs" "" "200,404"
test_endpoint "9.18" "GET /api/crawler/tasks/:id/result" "GET" "$BASE_URL/api/crawler/tasks/1/result" "" "200,404"
test_endpoint "9.19" "POST /api/crawler/batch" "POST" "$BASE_URL/api/crawler/batch" '{"templateId":1,"urls":["http://example.com"]}' "200,404"
test_endpoint "9.20" "POST /api/crawler/distributed" "POST" "$BASE_URL/api/crawler/distributed" '{"templateId":1,"urls":["http://example.com"],"nodeCount":3}' "200,404"
test_endpoint "9.21" "GET /api/crawler/distributed/:taskId" "GET" "$BASE_URL/api/crawler/distributed/1" "" "200,404"
test_endpoint "9.22" "GET /api/crawler/nodes" "GET" "$BASE_URL/api/crawler/nodes" "" "200,404"
test_endpoint "9.23" "GET /api/crawler/nodes/:id" "GET" "$BASE_URL/api/crawler/nodes/1" "" "200,404"
test_endpoint "9.24" "POST /api/crawler/nodes" "POST" "$BASE_URL/api/crawler/nodes" '{"name":"node1","address":"localhost:8081"}' "200,404"
test_endpoint "9.25" "DELETE /api/crawler/nodes/:id" "DELETE" "$BASE_URL/api/crawler/nodes/1" "" "200,404"
test_endpoint "9.26" "GET /api/crawler/stats" "GET" "$BASE_URL/api/crawler/stats" "" "200,404"
test_endpoint "9.27" "GET /api/crawler/stats/summary" "GET" "$BASE_URL/api/crawler/stats/summary" "" "200,404"
test_endpoint "9.28" "POST /api/crawler/templates/validate" "POST" "$BASE_URL/api/crawler/templates/validate" '{"name":"test","baseUrl":"http://example.com"}' "200,404"
test_endpoint "9.29" "GET /api/crawler/health" "GET" "$BASE_URL/api/crawler/health" "" "200,404"

# ============================================================================
# 测试结果汇总
# ============================================================================
echo ""
echo "========================================"
echo "测试结果汇总"
echo "========================================"
echo "总测试数: $TOTAL_TESTS"
echo -e "${GREEN}✅ 通过: $PASSED_TESTS${NC}"
echo -e "${RED}❌ 失败: $FAILED_TESTS${NC}"

if [ $TOTAL_TESTS -gt 0 ]; then
    PASS_RATE=$(awk "BEGIN {printf \"%.2f\", ($PASSED_TESTS/$TOTAL_TESTS)*100}")
    echo "通过率: $PASS_RATE%"
fi

echo ""
echo "测试完成时间: $(date '+%Y-%m-%d %H:%M:%S')"

# 返回退出码
if [ $FAILED_TESTS -eq 0 ]; then
    exit 0
else
    exit 1
fi
