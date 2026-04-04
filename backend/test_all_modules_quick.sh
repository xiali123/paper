#!/bin/bash

# 快速检测所有API模块状态
# 测试所有已知模块的端点是否工作

BASE_URL="http://localhost:8080"
PASS=0
FAIL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "========================================="
echo "所有API模块快速检测"
echo "========================================="
echo ""

# 测试模块端点
test_module() {
    local module_name="$1"
    local endpoint="$2"
    local description="$3"

    echo -n "[$module_name] $description ... "

    response=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL$endpoint" 2>&1)
    status=$(echo "$response" | tail -n 1 | tr -d '\r')
    body=$(echo "$response" | head -n -1)

    if [ "$status" = "200" ] || [ "$status" = "404" ] || [ "$status" = "401" ]; then
        if echo "$body" | grep -q "Route not found"; then
            echo -e "${RED}❌ 未注册${NC}"
            ((FAIL++))
        else
            echo -e "${GREEN}✅ 已注册${NC} (HTTP $status)"
            ((PASS++))
        fi
    else
        echo -e "${YELLOW}⚠️ 异常${NC} (HTTP $status)"
        ((FAIL++))
    fi
}

echo "1. 核心认证模块"
echo ""
test_module "AuthApi" "/api/auth/me" "获取当前用户"
test_module "AuthApi" "/api/auth/register" "用户注册端点"
echo ""

echo "2. 用户管理模块"
echo ""
test_module "UserApi" "/api/users" "用户列表"
test_module "UserApi" "/api/users/1" "用户详情"
echo ""

echo "3. 论文管理模块"
echo ""
test_module "PaperApi" "/api/papers" "论文列表"
test_module "PaperApi" "/api/papers/1" "论文详情"
test_module "PaperApi" "/api/papers/export" "导出功能"
echo ""

echo "4. 搜索模块"
echo ""
test_module "SearchApi" "/api/search" "搜索端点"
test_module "SearchApi" "/api/search/papers" "论文搜索"
echo ""

echo "5. 统计分析模块"
echo ""
test_module "StatsApi" "/api/stats" "统计端点"
test_module "StatsApi" "/api/stats/overview" "概览统计"
echo ""

echo "6. 导出模块"
echo ""
test_module "ExportApi" "/api/export" "导出端点"
test_module "ExportApi" "/api/export/papers" "论文导出"
echo ""

echo "7. AI功能模块"
echo ""
test_module "AiApi" "/api/ai" "AI端点"
test_module "AiApi" "/api/ai/summarize" "摘要生成"
echo ""

echo "8. 推荐模块"
echo ""
test_module "RecommendationApi" "/api/recommendations" "推荐端点"
test_module "RecommendationApi" "/api/recommendations/papers" "论文推荐"
echo ""

echo "9. 爬虫模块"
echo ""
test_module "CrawlerApi" "/api/crawler" "爬虫端点"
test_module "CrawlerApi" "/api/crawler/templates" "模板列表"
echo ""

echo "========================================="
echo "检测结果总结"
echo "========================================="
echo ""
echo -e "总测试数: $((PASS + FAIL))"
echo -e "${GREEN}✅ 已注册: $PASS${NC}"
echo -e "${RED}❌ 未注册/异常: $FAIL${NC}"
echo ""

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}🎉 所有模块端点均已注册！${NC}"
    exit 0
else
    pass_rate=$(( PASS * 100 / (PASS + FAIL) ))
    echo -e "注册率: ${pass_rate}%"
    echo ""
    echo -e "${YELLOW}需要修复的模块：${NC}"

    # 重新检测并列出未注册的模块
    for module in AuthApi UserApi PaperApi SearchApi StatsApi ExportApi AiApi RecommendationApi CrawlerApi; do
        response=$(curl -s -w "\n%{http_code}" -X GET "$BASE_URL/api/${module,,}/test" 2>&1)
        body=$(echo "$response" | head -n -1)
        if echo "$body" | grep -q "Route not found"; then
            echo -e "  - ${RED}$module${NC}"
        fi
    done
fi
echo ""
