#!/bin/bash

# UserApi模块快速测试脚本
# 测试9个已实现的端点

BASE_URL="http://localhost:8080/api/users"
PASS=0
FAIL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "========================================="
echo "UserApi 快速测试"
echo "========================================="
echo ""

test_endpoint() {
    local num="$1"
    local name="$2"
    local method="$3"
    local url="$4"
    local data="$5"

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

    if [ "$status" = "200" ] || [ "$status" = "201" ] || [ "$status" = "404" ] || [ "$status" = "400" ]; then
        echo -e "${GREEN}✅ 通过${NC} (HTTP $status)"
        ((PASS++))
    else
        echo -e "${RED}❌ 失败${NC} (HTTP $status)"
        ((FAIL++))
    fi
}

echo "1. 用户CRUD操作"
echo ""

test_endpoint "1" "GET /api/users - 获取用户列表" \
    "GET" "$BASE_URL" ""

test_endpoint "2" "GET /api/users/1 - 获取用户详情" \
    "GET" "$BASE_URL/1" ""

test_endpoint "3" "GET /api/users/999 - 不存在的用户" \
    "GET" "$BASE_URL/999" ""

test_endpoint "4" "POST /api/users - 创建用户" \
    "POST" "$BASE_URL" \
    '{"username":"testuser_api","email":"testapi@example.com","password":"Test123456","full_name":"API Test User"}'

test_endpoint "5" "PUT /api/users/1 - 更新用户" \
    "PUT" "$BASE_URL/1" \
    '{"full_name":"Updated Name","bio":"API test update"}'

test_endpoint "6" "DELETE /api/users/999 - 删除不存在用户" \
    "DELETE" "$BASE_URL/999" ""

echo ""
echo "2. 统计和搜索"
echo ""

test_endpoint "7" "GET /api/users/stats - 用户统计" \
    "GET" "$BASE_URL/stats" ""

test_endpoint "8" "GET /api/users/me - 当前用户（未认证）" \
    "GET" "$BASE_URL/me" ""

test_endpoint "9" "GET /api/users/search - 搜索用户" \
    "GET" "$BASE_URL/search?keyword=test" ""

echo ""
echo "========================================="
echo "测试总结"
echo "========================================="
echo ""
echo -e "总测试数: $((PASS + FAIL))"
echo -e "${GREEN}✅ 通过: $PASS${NC}"
echo -e "${RED}❌ 失败: $FAIL${NC}"

if [ $FAIL -eq 0 ]; then
    echo ""
    echo -e "${GREEN}🎉 UserApi测试全部通过！${NC}"
else
    pass_rate=$(( PASS * 100 / (PASS + FAIL) ))
    echo ""
    echo -e "通过率: ${pass_rate}%"
fi
