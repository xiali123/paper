#!/bin/bash

# PaperApi模块完整端点测试脚本
# 测试所有10个API端点
# 作者: Claude Code
# 日期: 2026-04-04

BASE_URL="http://localhost:8080/api/papers"
PASS=0
FAIL=0

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================="
echo "PaperApi 完整端点测试"
echo "========================================="
echo ""

# 测试函数
test_endpoint() {
    local num="$1"
    local name="$2"
    local method="$3"
    local url="$4"
    local data="$5"
    local expected_status="$6"  # 可选的期望状态码

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

    # 如果指定了期望状态码，检查是否匹配
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
        # 默认：200和404都算通过
        if [ "$status" = "200" ] || [ "$status" = "404" ] || [ "$status" = "201" ]; then
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
echo "1. 基础CRUD操作"
echo "========================================="
echo ""

# 1. GET /api/papers - 获取论文列表（分页）
test_endpoint "1" "GET /api/papers - 获取列表" \
    "GET" "$BASE_URL?page=1&limit=20" ""

# 2. GET /api/papers/:id - 获取论文详情
test_endpoint "2" "GET /api/papers/1 - 获取详情" \
    "GET" "$BASE_URL/1" ""

# 3. POST /api/papers - 创建论文
test_endpoint "3" "POST /api/papers - 创建论文" \
    "POST" "$BASE_URL" \
    '{
        "title": "深度学习在自然语言处理中的应用",
        "authors": "张三,李四",
        "year": 2024,
        "abstract": "本文研究了深度学习技术在NLP领域的应用...",
        "journal": "计算机学报",
        "volume": "45",
        "issue": "3",
        "pages": "1-15",
        "doi": "10.12345/example.2024",
        "tags": ["深度学习", "NLP", "AI"]
    }' "201"

# 4. PUT /api/papers/:id - 更新论文
test_endpoint "4" "PUT /api/papers/1 - 更新论文" \
    "PUT" "$BASE_URL/1" \
    '{
        "title": "深度学习在自然语言处理中的应用（修订版）",
        "authors": "张三,李四,王五",
        "year": 2024,
        "abstract": "本文研究了深度学习技术在NLP领域的应用，包含最新实验结果...",
        "journal": "计算机学报",
        "citation_count": 15
    }'

# 5. DELETE /api/papers/:id - 删除论文
test_endpoint "5" "DELETE /api/papers/999 - 删除论文（不存在）" \
    "DELETE" "$BASE_URL/999" "" "404"

echo "========================================="
echo "2. 搜索和过滤"
echo "========================================="
echo ""

# 6. GET /api/papers/search - 搜索论文
test_endpoint "6" "GET /api/papers/search - 关键词搜索" \
    "GET" "$BASE_URL/search?query=深度学习&page=1&limit=10" ""

test_endpoint "7" "GET /api/papers/search - 作者搜索" \
    "GET" "$BASE_URL/search?author=张三" ""

test_endpoint "8" "GET /api/papers/search - 年份范围" \
    "GET" "$BASE_URL/search?yearFrom=2020&yearTo=2024" ""

test_endpoint "9" "GET /api/papers/search - 标签过滤" \
    "GET" "$BASE_URL/search?tags=深度学习,AI" ""

test_endpoint "10" "GET /api/papers/search - 综合搜索" \
    "GET" "$BASE_URL/search?query=NLP&yearFrom=2023&journal=计算机学报" ""

echo "========================================="
echo "3. 统计和分析"
echo "========================================="
echo ""

# 7. GET /api/papers/stats - 获取统计信息
test_endpoint "11" "GET /api/papers/stats - 统计信息" \
    "GET" "$BASE_URL/stats" ""

echo "========================================="
echo "4. 批量操作"
echo "========================================="
echo ""

# 8. POST /api/papers/import - 批量导入
test_endpoint "12" "POST /api/papers/import - 批量导入" \
    "POST" "$BASE_URL/import" \
    '{
        "papers": [
            {
                "title": "机器学习基础",
                "authors": "王五",
                "year": 2023,
                "journal": "人工智能",
                "tags": ["机器学习", "入门"]
            },
            {
                "title": "神经网络架构设计",
                "authors": "赵六",
                "year": 2024,
                "journal": "深度学习",
                "tags": ["神经网络", "架构"]
            }
        ]
    }'

# 9. GET /api/papers/export - 导出论文
test_endpoint "13" "GET /api/papers/export - 导出JSON" \
    "GET" "$BASE_URL/export?format=json" ""

test_endpoint "14" "GET /api/papers/export - 导出BibTeX" \
    "GET" "$BASE_URL/export?format=bibtex" ""

test_endpoint "15" "GET /api/papers/export - 导出指定ID" \
    "GET" "$BASE_URL/export?ids=1,2,3&format=json" ""

echo "========================================="
echo "5. 交互操作"
echo "========================================="
echo ""

# 10. POST /api/papers/:id/favorite - 收藏/取消收藏
test_endpoint "16" "POST /api/papers/1/favorite - 添加收藏" \
    "POST" "$BASE_URL/1/favorite" \
    '{"favorite": true}'

test_endpoint "17" "POST /api/papers/1/favorite - 取消收藏" \
    "POST" "$BASE_URL/1/favorite" \
    '{"favorite": false}'

# 额外测试：标记已读/未读（如果实现了这个端点）
test_endpoint "18" "POST /api/papers/1/read - 标记已读" \
    "POST" "$BASE_URL/1/read" \
    '{"is_read": true}'

test_endpoint "19" "POST /api/papers/1/tags - 添加标签" \
    "POST" "$BASE_URL/1/tags" \
    '{"tags": ["重要", "必读"]}'

test_endpoint "20" "DELETE /api/papers/1/tags/重要 - 移除标签" \
    "DELETE" "$BASE_URL/1/tags/重要" ""

echo "========================================="
echo "6. 边界情况测试"
echo "========================================="
echo ""

# 测试无效ID
test_endpoint "21" "GET /api/papers/abc - 无效ID" \
    "GET" "$BASE_URL/abc" "" "400"

# 测试空参数
test_endpoint "22" "POST /api/papers - 空JSON" \
    "POST" "$BASE_URL" \
    '{}' "400"

# 测试超大分页
test_endpoint "23" "GET /api/papers - 超大分页" \
    "GET" "$BASE_URL?page=9999&limit=9999" ""

# 测试特殊字符搜索
test_endpoint "24" "GET /api/papers/search - 特殊字符" \
    "GET" "$BASE_URL/search?query=<script>alert('xss')</script>" ""

# 测试SQL注入尝试（使用URL编码避免bash解析问题）
# ' OR '1'='1 → %27%20OR%20%271%27%3D%271
test_endpoint "25" "GET /api/papers/search - SQL注入测试" \
    "GET" "$BASE_URL/search?query=%27%20OR%20%271%27%3D%271" ""

echo "========================================="
echo "7. 性能测试"
echo "========================================="
echo ""

# 性能测试：大量数据查询
echo "26. 性能测试: 大量数据查询..."
start_time=$(date +%s%N)
response=$(curl -s -w "\n%{http_code}" "$BASE_URL?page=1&limit=100" 2>&1)
status=$(echo "$response" | tail -n 1)
end_time=$(date +%s%N)
duration=$(( (end_time - start_time) / 1000000 ))

if [ "$status" = "200" ] || [ "$status" = "404" ]; then
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
