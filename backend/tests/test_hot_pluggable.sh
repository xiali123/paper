#!/bin/bash
# PaperCrawler 热插拔架构测试脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 测试结果
TESTS_PASSED=0
TESTS_FAILED=0

# 打印函数
print_header() {
    echo -e "\n${GREEN}╔═══════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║ $1${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════════════════════╝${NC}\n"
}

print_test() {
    echo -e "${YELLOW}[TEST]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
    ((TESTS_PASSED++))
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
    ((TESTS_FAILED++))
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# ============================================================================
# 测试1: 检查main.cpp行数
# ============================================================================
test_main_cpp_size() {
    print_test "检查main.cpp行数"

    if [ -f "src/core/main.cpp" ]; then
        LINES=$(wc -l < src/core/main.cpp)
        print_info "main.cpp当前行数: $LINES"

        if [ $LINES -lt 300 ]; then
            print_success "main.cpp行数 < 300 (实际: $LINES)"
        else
            print_error "main.cpp行数 >= 300 (实际: $LINES)"
        fi
    else
        print_error "main.cpp文件不存在"
    fi
}

# ============================================================================
# 测试2: 检查模块DLL文件
# ============================================================================
test_module_dlls() {
    print_test "检查模块DLL文件"

    MODULES_DIR="build/Release/modules/dynamic/Release"
    EXPECTED_MODULES=(
        "libAuthApiModule.dll"
        "libPaperApiModule.dll"
        "libUserApiModule.dll"
        "libAiApiModule.dll"
        "libSearchApiModule.dll"
        "libExportApiModule.dll"
    )

    for module in "${EXPECTED_MODULES[@]}"; do
        if [ -f "$MODULES_DIR/$module" ]; then
            print_success "找到模块: $module"
        else
            print_error "缺失模块: $module"
        fi
    done
}

# ============================================================================
# 测试3: 检查接口定义
# ============================================================================
test_interface_definitions() {
    print_test "检查IModule接口定义"

    # 检查getRoutePrefix方法
    if grep -q "getRoutePrefix" include/core/IModule.hpp; then
        print_success "IModule::getRoutePrefix() 已定义"
    else
        print_error "IModule::getRoutePrefix() 未定义"
    fi

    # 检查registerRoutes方法
    if grep -q "registerRoutes" include/core/IModule.hpp; then
        print_success "IModule::registerRoutes() 已定义"
    else
        print_error "IModule::registerRoutes() 未定义"
    fi

    # 检查getDependencies方法
    if grep -q "getDependencies" include/core/IModule.hpp; then
        print_success "IModule::getDependencies() 已定义"
    else
        print_error "IModule::getDependencies() 未定义"
    fi
}

# ============================================================================
# 测试4: 编译测试
# ============================================================================
test_compilation() {
    print_test "编译项目"

    print_info "开始编译..."

    if cd build && cmake --build . --config Release 2>&1 | tee build.log; then
        print_success "项目编译成功"
        cd ..
    else
        print_error "项目编译失败"
        cd ..
        return 1
    fi
}

# ============================================================================
# 测试5: 运行服务器测试
# ============================================================================
test_server_startup() {
    print_test "服务器启动测试"

    print_info "启动服务器..."

    # 后台启动服务器
    cd build/Release
    timeout 10 ./src/core/main.exe > server.log 2>&1 &
    SERVER_PID=$!
    cd ../..

    # 等待服务器启动
    sleep 3

    # 检查进程是否运行
    if ps -p $SERVER_PID > /dev/null; then
        print_success "服务器启动成功 (PID: $SERVER_PID)"

        # 检查日志
        if grep -q "Server Started Successfully" build/Release/server.log; then
            print_success "服务器启动日志正常"
        else
            print_error "服务器启动日志异常"
        fi

        # 停止服务器
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    else
        print_error "服务器启动失败"
        cat build/Release/server.log
    fi
}

# ============================================================================
# 测试6: API路由测试
# ============================================================================
test_api_routes() {
    print_test "API路由测试"

    # 启动服务器
    cd build/Release
    ./src/core/main.exe > server.log 2>&1 &
    SERVER_PID=$!
    cd ../..

    sleep 3

    # 测试健康检查
    print_info "测试 GET /health"
    if curl -s http://localhost:8080/health | grep -q "healthy"; then
        print_success "GET /health 正常"
    else
        print_error "GET /health 失败"
    fi

    # 测试认证路由
    print_info "测试 POST /api/auth/login"
    RESPONSE=$(curl -s -X POST http://localhost:8080/api/auth/login \
        -H "Content-Type: application/json" \
        -d '{"username":"test","password":"test"}')

    if echo "$RESPONSE" | grep -q "access_token\|error"; then
        print_success "POST /api/auth/login 正常响应"
    else
        print_error "POST /api/auth/login 响应异常: $RESPONSE"
    fi

    # 测试论文路由
    print_info "测试 GET /api/papers"
    if curl -s http://localhost:8080/api/papers | grep -q "\[\]"; then
        print_success "GET /api/papers 正常响应"
    else
        print_error "GET /api/papers 响应异常"
    fi

    # 停止服务器
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
}

# ============================================================================
# 测试7: 模块加载测试
# ============================================================================
test_module_loading() {
    print_test "模块加载测试"

    # 启动服务器
    cd build/Release
    ./src/core/main.exe > server.log 2>&1 &
    SERVER_PID=$!
    cd ../..

    sleep 3

    # 检查日志中的模块加载信息
    if grep -q "Loaded.*module" build/Release/server.log; then
        print_success "模块加载日志正常"

        # 统计加载的模块数量
        MODULE_COUNT=$(grep -o "Loaded [0-9]* module" build/Release/server.log | grep -o "[0-9]*")
        print_info "加载的模块数量: $MODULE_COUNT"

        if [ "$MODULE_COUNT" -ge 6 ]; then
            print_success "模块数量符合预期 (>= 6)"
        else
            print_error "模块数量不足 (< 6)"
        fi
    else
        print_error "未找到模块加载日志"
    fi

    # 停止服务器
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
}

# ============================================================================
# 测试8: 路由注册测试
# ============================================================================
test_route_registration() {
    print_test "路由注册测试"

    # 启动服务器
    cd build/Release
    ./src/core/main.exe > server.log 2>&1 &
    SERVER_PID=$!
    cd ../..

    sleep 3

    # 检查日志中的路由注册信息
    if grep -q "Registering routes" build/Release/server.log; then
        print_success "路由注册日志正常"

        # 统计注册的路由数量
        ROUTE_COUNT=$(grep -c "POST\|GET\|PUT\|DELETE" build/Release/server.log || echo "0")
        print_info "注册的路由数量: $ROUTE_COUNT"

        if [ "$ROUTE_COUNT" -ge 10 ]; then
            print_success "路由数量符合预期 (>= 10)"
        else
            print_error "路由数量不足 (< 10)"
        fi
    else
        print_error "未找到路由注册日志"
    fi

    # 停止服务器
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
}

# ============================================================================
# 测试9: 性能测试
# ============================================================================
test_performance() {
    print_test "性能测试"

    print_info "准备性能测试..."

    # 启动服务器
    cd build/Release
    ./src/core/main.exe > server.log 2>&1 &
    SERVER_PID=$!
    cd ../..

    sleep 3

    # 测试启动时间
    START_TIME=$(grep "Server Started Successfully" build/Release/server.log | head -1 | awk '{print $1}' | cut -d'[' -f2 | cut -d']' -f1)
    print_info "服务器启动时间: $START_TIME"

    # 测试响应时间
    print_info "测试API响应时间..."

    TOTAL_TIME=0
    REQUESTS=10

    for i in $(seq 1 $REQUESTS); do
        START=$(date +%s%3N)
        curl -s http://localhost:8080/health > /dev/null
        END=$(date +%s%3N)
        ELAPSED=$((END - START))
        TOTAL_TIME=$((TOTAL_TIME + ELAPSED))
    done

    AVG_TIME=$((TOTAL_TIME / REQUESTS))
    print_info "平均响应时间: ${AVG_TIME}ms"

    if [ $AVG_TIME -lt 200 ]; then
        print_success "响应时间符合预期 (< 200ms)"
    else
        print_error "响应时间过长 (>= 200ms)"
    fi

    # 停止服务器
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
}

# ============================================================================
# 主函数
# ============================================================================
main() {
    print_header "PaperCrawler 热插拔架构测试套件"

    print_info "开始时间: $(date)"
    print_info "工作目录: $(pwd)"

    # 检查环境
    if [ ! -d "build" ]; then
        print_error "build目录不存在，请先运行CMake配置"
        exit 1
    fi

    # 运行测试
    test_main_cpp_size
    test_module_dlls
    test_interface_definitions
    test_compilation
    test_server_startup
    test_module_loading
    test_route_registration
    test_api_routes
    test_performance

    # 打印结果
    print_header "测试结果汇总"

    TOTAL_TESTS=$((TESTS_PASSED + TESTS_FAILED))
    SUCCESS_RATE=0

    if [ $TOTAL_TESTS -gt 0 ]; then
        SUCCESS_RATE=$((TESTS_PASSED * 100 / TOTAL_TESTS))
    fi

    echo -e "总测试数: $TOTAL_TESTS"
    echo -e "${GREEN}通过: $TESTS_PASSED${NC}"
    echo -e "${RED}失败: $TESTS_FAILED${NC}"
    echo -e "成功率: $SUCCESS_RATE%"

    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "\n${GREEN}🎉 所有测试通过！${NC}\n"
        exit 0
    else
        echo -e "\n${RED}⚠️  部分测试失败${NC}\n"
        exit 1
    fi
}

# 运行主函数
main
