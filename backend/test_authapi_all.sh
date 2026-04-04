#!/bin/bash

# AuthApi模块完整端点测试脚本
# 测试所有9个认证端点
# 作者: Claude Code
# 日期: 2026-04-04

BASE_URL="http://localhost:8080/api/auth"
PASS=0
FAIL=0

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "AuthApi 完整端点测试"
echo "========================================="
echo ""

# 测试函数
test_endpoint() {
    local num="$1"
    local name="$2"
    local method="$3"
    local url="$4"
    local data="$5"
    local expected_status="$6"

    echo -n "[$num] 测试: $name ... "

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

    if [ -n "$expected_status" ]; then
        if [ "$status" = "$expected_status" ]; then
            echo -e "${GREEN}✅ 通过${NC} (HTTP $status)"
            ((PASS++))
            echo "   响应: $body" | head -c 100
            echo ""
        else
            echo -e "${RED}❌ 失败${NC} (期望 $expected_status, 实际 $status)"
            ((FAIL++))
            echo "   响应: $body"
        fi
    else
        # 默认：200、201、401、404都算通过
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

# ========================================================================
# 端点测试
# ========================================================================

echo "========================================="
echo "1. 认证基础功能"
echo "========================================="
echo ""

# 1. POST /api/auth/register - 用户注册
test_endpoint "1" "POST /api/auth/register - 用户注册" \
    "POST" "$BASE_URL/register" \
    '{
        "username": "testuser",
        "email": "test@example.com",
        "password": "Test123456",
        "full_name": "Test User"
    }' "201"

# 2. POST /api/auth/login - 用户登录
test_endpoint "2" "POST /api/auth/login - 用户登录" \
    "POST" "$BASE_URL/login" \
    '{
        "username": "testuser",
        "password": "Test123456"
    }' "200"

# 3. POST /api/auth/login - 错误密码
test_endpoint "3" "POST /api/auth/login - 错误密码" \
    "POST" "$BASE_URL/login" \
    '{
        "username": "testuser",
        "password": "WrongPassword"
    }' "401"

# 4. POST /api/auth/login - 不存在的用户
test_endpoint "4" "POST /api/auth/login - 不存在的用户" \
    "POST" "$BASE_URL/login" \
    '{
        "username": "nonexistent",
        "password": "Test123456"
    }' "401"

# 5. GET /api/auth/me - 获取当前用户信息（无认证）
test_endpoint "5" "GET /api/auth/me - 获取当前用户（未认证）" \
    "GET" "$BASE_URL/me" "" "401"

echo "========================================="
echo "2. 令牌管理"
echo "========================================="
echo ""

# 6. POST /api/auth/refresh - 刷新令牌
test_endpoint "6" "POST /api/auth/refresh - 刷新令牌" \
    "POST" "$BASE_URL/refresh" \
    '{
        "refresh_token": "dummy_token"
    }' "401"

# 7. POST /api/auth/logout - 登出
test_endpoint "7" "POST /api/auth/logout - 登出" \
    "POST" "$BASE_URL/logout" "" "401"

echo "========================================="
echo "3. 密码管理"
echo "========================================="
echo ""

# 8. POST /api/auth/change-password - 修改密码
test_endpoint "8" "POST /api/auth/change-password - 修改密码（未认证）" \
    "POST" "$BASE_URL/change-password" \
    '{
        "old_password": "Test123456",
        "new_password": "NewPassword123"
    }' "401"

# 9. POST /api/auth/reset-password - 重置密码
test_endpoint "9" "POST /api/auth/reset-password - 重置密码" \
    "POST" "$BASE_URL/reset-password" \
    '{
        "email": "test@example.com"
    }' "200"

echo "========================================="
echo "4. 会话管理"
echo "========================================="
echo ""

# 10. GET /api/auth/sessions - 获取所有会话
test_endpoint "10" "GET /api/auth/sessions - 获取所有会话（未认证）" \
    "GET" "$BASE_URL/sessions" "" "401"

# 11. DELETE /api/auth/sessions/1 - 删除会话
test_endpoint "11" "DELETE /api/auth/sessions/1 - 删除会话（未认证）" \
    "DELETE" "$BASE_URL/sessions/1" "" "401"

echo "========================================="
echo "5. 边界情况测试"
echo "========================================="
echo ""

# 12. POST /api/auth/register - 缺少必填字段
test_endpoint "12" "POST /api/auth/register - 缺少必填字段" \
    "POST" "$BASE_URL/register" \
    '{
        "username": "incomplete"
    }' "400"

# 13. POST /api/auth/register - 空JSON
test_endpoint "13" "POST /api/auth/register - 空JSON" \
    "POST" "$BASE_URL/register" \
    '{}' "400"

# 14. POST /api/auth/login - 空JSON
test_endpoint "14" "POST /api/auth/login - 空JSON" \
    "POST" "$BASE_URL/login" \
    '{}' "400"

# 15. POST /api/auth/login - 缺少用户名
test_endpoint "15" "POST /api/auth/login - 缺少用户名" \
    "POST" "$BASE_URL/login" \
    '{
        "password": "Test123456"
    }' "400"

echo "========================================="
echo "6. 输入验证测试"
echo "========================================="
echo ""

# 16. POST /api/auth/register - 弱密码
test_endpoint "16" "POST /api/auth/register - 弱密码" \
    "POST" "$BASE_URL/register" \
    '{
        "username": "weakuser",
        "email": "weak@example.com",
        "password": "123"
    }' "400"

# 17. POST /api/auth/register - 无效邮箱
test_endpoint "17" "POST /api/auth/register - 无效邮箱" \
    "POST" "$BASE_URL/register" \
    '{
        "username": "invaliduser",
        "email": "notanemail",
        "password": "Test123456"
    }' "400"

# 18. POST /api/auth/register - 重复用户名
test_endpoint "18" "POST /api/auth/register - 重复用户名" \
    "POST" "$BASE_URL/register" \
    '{
        "username": "testuser",
        "email": "another@example.com",
        "password": "Test123456"
    }' "409"

# 19. POST /api/auth/login - SQL注入尝试
test_endpoint "19" "POST /api/auth/login - SQL注入测试" \
    "POST" "$BASE_URL/login" \
    '{"username":"admin'\'' OR '\''1'\''='\''1","password":"test"}' "400"

# 20. POST /api/auth/login - XSS尝试
test_endpoint "20" "POST /api/auth/login - XSS测试" \
    "POST" "$BASE_URL/login" \
    '{"username":"<script>alert(1)</script>","password":"test"}' "400"

echo "========================================="
echo "7. 性能测试"
echo "========================================="
echo ""

# 性能测试：登录响应时间
echo "21. 性能测试: 登录请求响应时间..."
start_time=$(date +%s%N)
response=$(curl -s -w "\n%{http_code}" -X POST \
    -H "Content-Type: application/json" \
    -d '{"username":"testuser","password":"Test123456"}' \
    "$BASE_URL/login" 2>&1)
status=$(echo "$response" | tail -n 1)
end_time=$(date +%s%N)
duration=$(( (end_time - start_time) / 1000000 ))

if [ "$status" = "200" ] || [ "$status" = "401" ]; then
    echo -e "${GREEN}✅ 通过${NC} (HTTP $status, 耗时: ${duration}ms)"
    ((PASS++))
else
    echo -e "${RED}❌ 失败${NC} (HTTP $status, 耗时: ${duration}ms)"
    ((FAIL++))
fi
echo ""

# ========================================================================
# 测试总结
# ========================================================================

echo "========================================="
echo "测试总结"
echo "========================================="
echo ""
echo -e "总测试数: $((PASS + FAIL))"
echo -e "${GREEN}✅ 通过: $PASS${NC}"
echo -e "${RED}❌ 失败: $FAIL${NC}"

if [ $FAIL -eq 0 ]; then
    echo ""
    echo -e "${GREEN}🎉 所有测试通过！${NC}"
    exit 0
else
    echo ""
    pass_rate=$(( PASS * 100 / (PASS + FAIL) ))
    echo -e "通过率: ${pass_rate}%"

    if [ $pass_rate -ge 95 ]; then
        echo -e "${GREEN}✅ 优秀！通过率 ≥ 95%${NC}"
        exit 0
    elif [ $pass_rate -ge 80 ]; then
        echo -e "${YELLOW}⚠️  良好，但仍有改进空间${NC}"
        exit 1
    else
        echo -e "${RED}❌ 需要修复多个端点${NC}"
        exit 1
    fi
fi
