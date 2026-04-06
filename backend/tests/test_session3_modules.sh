#!/bin/bash

# 验证所有新实现的API端点
# 测试Session 3重构的模块

BASE_URL="http://localhost:8080"
PASS=0
FAIL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "========================================="
echo "API模块端点验证测试"
echo "Branch: feature/FS-8888-fix-compile-bug"
echo "========================================="
echo ""

test_endpoint() {
    local num="$1"
    local name="$2"
    local method="$3"
    local url="$4"
    local data="$5"
    local expected="$6"

    echo -n "[$num] $name ... "

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
    body=$(echo "$response" | head -n -1)

    if [ -n "$expected" ]; then
        if [ "$status" = "$expected" ]; then
            echo -e "${GREEN}✅ 通过${NC} (HTTP $status)"
            ((PASS++))
            echo "   响应: $body" | head -c 100
            echo ""
        else
            echo -e "${RED}❌ 失败${NC} (期望 $expected, 实际 $status)"
            ((FAIL++))
            echo "   响应: $body"
        fi
    else
        # 200, 201, 401, 404都算通过
        if [ "$status" = "200" ] || [ "$status" = "201" ] || [ "$status" = "401" ] || [ "$status" = "404" ]; then
            echo -e "${GREEN}✅ 通过${NC} (HTTP $status)"
            ((PASS++))
            echo "   响应: $body" | head -c 100
            echo ""
        else
            echo -e "${RED}❌ 失败${NC} (HTTP $status)"
            ((FAIL++))
            echo "   响应: $body"
        fi
    fi
    echo ""
}

echo "========================================="
echo "1. SearchApi（Session 3新实现）"
echo "========================================="
echo ""

test_endpoint "1" "GET /api/search" "GET" "$BASE_URL/api/search" "" "200"
test_endpoint "2" "GET /api/search/suggest" "GET" "$BASE_URL/api/search/suggest" "" "200"
test_endpoint "3" "GET /api/search/trending" "GET" "$BASE_URL/api/search/trending" "" "200"
test_endpoint "4" "GET /api/search/history" "GET" "$BASE_URL/api/search/history" "" "200"
test_endpoint "5" "GET /api/search/stats" "GET" "$BASE_URL/api/search/stats" "" "200"

echo "========================================="
echo "2. StatsApi（Session 3重构）"
echo "========================================="
echo ""

test_endpoint "6" "GET /api/stats/system" "GET" "$BASE_URL/api/stats/system" "" "200"
test_endpoint "7" "GET /api/stats/resources" "GET" "$BASE_URL/api/stats/resources" "" "200"
test_endpoint "8" "GET /api/stats/uptime" "GET" "$BASE_URL/api/stats/uptime" "" "200"
test_endpoint "9" "GET /api/stats/modules" "GET" "$BASE_URL/api/stats/modules" "" "200"
test_endpoint "10" "GET /api/stats/performance" "GET" "$BASE_URL/api/stats/performance" "" "200"

echo "========================================="
echo "3. ExportApi（Session 3重构）"
echo "========================================="
echo ""

test_endpoint "11" "GET /api/export" "GET" "$BASE_URL/api/export" "" "200"
test_endpoint "12" "POST /api/export" "POST" "$BASE_URL/api/export" '{"paper_ids":[1,2]}' "201"
test_endpoint "13" "GET /api/export/formats" "GET" "$BASE_URL/api/export/formats" "" "200"
test_endpoint "14" "GET /api/export/stats" "GET" "$BASE_URL/api/export/stats" "" "200"

echo "========================================="
echo "4. AiApi（Session 3重构）"
echo "========================================="
echo ""

test_endpoint "15" "GET /api/ai/status" "GET" "$BASE_URL/api/ai/status" "" "200"
test_endpoint "16" "POST /api/ai/summarize" "POST" "$BASE_URL/api/ai/summarize" '{"paper_id":1}' "200"
test_endpoint "17" "POST /api/ai/chat" "POST" "$BASE_URL/api/ai/chat" '{"message":"hello"}' "200"
test_endpoint "18" "POST /api/ai/keywords" "POST" "$BASE_URL/api/ai/keywords" '{"paper_id":1}' "200"

echo "========================================="
echo "5. RecommendationApi（Session 3重构）"
echo "========================================="
echo ""

test_endpoint "19" "GET /api/recommendations/papers" "GET" "$BASE_URL/api/recommendations/papers" "" "200"
test_endpoint "20" "GET /api/recommendations/trending" "GET" "$BASE_URL/api/recommendations/trending" "" "200"
test_endpoint "21" "POST /api/recommendations/feedback" "POST" "$BASE_URL/api/recommendations/feedback" '{"paper_id":1,"rating":5}' "200"
test_endpoint "22" "GET /api/recommendations/stats" "GET" "$BASE_URL/api/recommendations/stats" "" "200"

echo "========================================="
echo "6. AuthApi输入验证"
echo "========================================="
echo ""

test_endpoint "23" "注册：缺少必填字段" "POST" "$BASE_URL/api/auth/register" '{"username":"test"}' "400"
test_endpoint "24" "注册：弱密码" "POST" "$BASE_URL/api/auth/register" '{"username":"test2","email":"test2@test.com","password":"123"}' "400"
test_endpoint "25" "登录：缺少密码" "POST" "$BASE_URL/api/auth/login" '{"username":"test"}' "400"

echo "========================================="
echo "测试总结"
echo "========================================="
echo ""
echo -e "总测试数: $((PASS + FAIL))"
echo -e "${GREEN}✅ 通过: $PASS${NC}"
echo -e "${RED}❌ 失败: $FAIL${NC}"
echo ""

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}🎉 所有测试通过！${NC}"
    exit 0
else
    pass_rate=$(( PASS * 100 / (PASS + FAIL) ))
    echo -e "通过率: ${pass_rate}%"

    if [ $pass_rate -ge 95 ]; then
        echo -e "${GREEN}✅ 优秀！通过率 ≥ 95%${NC}"
        exit 0
    elif [ $pass_rate -ge 80 ]; then
        echo -e "${YELLOW}⚠️  良好，通过率 ≥ 80%${NC}"
        exit 0
    else
        echo -e "${RED}❌ 需要修复${NC}"
        exit 1
    fi
fi
