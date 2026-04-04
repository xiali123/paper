#!/bin/bash
# API测试工具库
# 提供通用的测试函数

# 服务器配置
SERVER_HOST="localhost"
SERVER_PORT="8080"
BASE_URL="http://${SERVER_HOST}:${SERVER_PORT}"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 测试统计
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试结果存储
declare -a TEST_RESULTS

# 打印带颜色的消息
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# 等待服务器启动
wait_for_server() {
    local max_attempts=30
    local attempt=1

    print_info "等待服务器启动..."

    while [ $attempt -le $max_attempts ]; do
        if curl -s "${BASE_URL}/api/health" > /dev/null 2>&1; then
            print_success "服务器已就绪"
            return 0
        fi

        echo -n "."
        sleep 1
        ((attempt++))
    done

    echo ""
    print_error "服务器启动超时"
    return 1
}

# 执行HTTP请求并检查响应
test_endpoint() {
    local test_name="$1"
    local method="$2"
    local endpoint="$3"
    local expected_status="${4:-200}"
    local data="$5"

    ((TOTAL_TESTS++))

    print_info "测试: $test_name"

    # 执行请求
    local response
    local status_code
    local body

    if [ -n "$data" ]; then
        response=$(curl -s -w $'\n%{http_code}' -X "${method}" \
            -H 'Content-Type: application/json' \
            -d "$data" \
            "${BASE_URL}${endpoint}")
    else
        response=$(curl -s -w $'\n%{http_code}' -X "${method}" \
            "${BASE_URL}${endpoint}")
    fi

    # 分离响应体和状态码
    body=$(echo "$response" | head -n -1)
    status_code=$(echo "$response" | tail -n 1 | tr -d '\r')

    # 检查状态码
    if [ "$status_code" = "$expected_status" ]; then
        print_success "$test_name - HTTP $status_code"

        # 检查是否返回有效的JSON（如果不是404）
        if [ "$status_code" != "404" ] && echo "$body" | jq empty > /dev/null 2>&1; then
            print_success "$test_name - JSON格式正确"
        elif [ "$status_code" != "404" ]; then
            print_warning "$test_name - 响应不是JSON格式"
            echo "$body" | head -c 100
            echo ""
        fi

        # 记录成功
        TEST_RESULTS+=("{\"test\":\"$test_name\",\"status\":\"pass\",\"http_code\":$status_code}")
        ((PASSED_TESTS++))
        return 0
    else
        print_error "$test_name - HTTP $status_code (期望 $expected_status)"

        # 记录失败
        TEST_RESULTS+=("{\"test\":\"$test_name\",\"status\":\"fail\",\"http_code\":$status_code,\"expected\":$expected_status}")
        ((FAILED_TESTS++))
        return 1
    fi
}

# 打印测试摘要
print_summary() {
    echo ""
    echo "=========================================="
    echo "测试摘要"
    echo "=========================================="
    echo -e "总测试数: ${BLUE}${TOTAL_TESTS}${NC}"
    echo -e "通过: ${GREEN}${PASSED_TESTS}${NC}"
    echo -e "失败: ${RED}${FAILED_TESTS}${NC}"
    echo "=========================================="

    local success_rate=0
    if [ $TOTAL_TESTS -gt 0 ]; then
        success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi

    echo -e "通过率: ${BLUE}${success_rate}%${NC}"
    echo "=========================================="

    # 返回0表示全部通过，1表示有失败
    if [ $FAILED_TESTS -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# 保存测试结果到JSON文件
save_results() {
    local output_file="$1"
    local module_name="$2"

    # 生成JSON结果
    echo "{" > "$output_file"
    echo "  \"module\": \"$module_name\"," >> "$output_file"
    echo "  \"timestamp\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\"," >> "$output_file"
    echo "  \"total_tests\": $TOTAL_TESTS," >> "$output_file"
    echo "  \"passed\": $PASSED_TESTS," >> "$output_file"
    echo "  \"failed\": $FAILED_TESTS," >> "$output_file"
    echo "  \"success_rate\": $((PASSED_TESTS * 100 / (TOTAL_TESTS > 0 ? TOTAL_TESTS : 1)))," >> "$output_file"
    echo "  \"tests\": [" >> "$output_file"

    local first=true
    for result in "${TEST_RESULTS[@]}"; do
        if [ "$first" = true ]; then
            echo "    $result" >> "$output_file"
            first=false
        else
            echo "    ,$result" >> "$output_file"
        fi
    done

    echo "  ]" >> "$output_file"
    echo "}" >> "$output_file"

    print_info "测试结果已保存到: $output_file"
}

# 重置测试统计
reset_stats() {
    TOTAL_TESTS=0
    PASSED_TESTS=0
    FAILED_TESTS=0
    TEST_RESULTS=()
}
