#!/bin/bash
# ============================================================================
# 热插拔架构自动化测试脚本
# ============================================================================()

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 计数器
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试结果数组
declare -a TEST_RESULTS

# ============================================================================
# 辅助函数
# ============================================================================()

print_header() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
}

print_test() {
    echo -e "${NC}[TEST] ${NC}$1"
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((PASSED_TESTS++))
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((FAILED_TESTS++))
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# ============================================================================
# 测试执行函数
# ============================================================================()

run_test() {
    local test_name="$1"
    local test_func="$2"

    ((TOTAL_TESTS++))
    print_test "$test_name"

    if $test_func; then
        print_pass "$test_name"
        TEST_RESULTS+=("PASS:$test_name")
        return 0
    else
        print_fail "$test_name"
        TEST_RESULTS+=("FAIL:$test_name")
        return 1
    fi
}

# ============================================================================
# 测试1: 配置文件解析
# ============================================================================()

test_config_parsing() {
    local config_file="$1"

    if [ ! -f "$config_file" ]; then
        print_warning "配置文件不存在: $config_file"
        return 1
    fi

    # 检查JSON格式
    if command -v python &> /dev/null; then
        python -c "import json; json.load(open('$config_file'))" 2>/dev/null
        if [ $? -eq 0 ]; then
            print_info "JSON格式验证通过"

            # 显示配置内容
            print_info "模块数量: $(python -c "import json; data=json.load(open('$config_file')); print(len(data.get('modules', [])))")"
            return 0
        else
            print_fail "JSON格式验证失败"
            return 1
        fi
    else
        print_warning "Python未安装，跳过JSON验证"
    fi
}

# ============================================================================
# 测试2: DLL文件检查
# ============================================================================()

test_dll_files() {
    local dll_dir="$1"
    local required_modules=(
        "libAuthApiModule.dll"
        "libUserApiModule.dll"
        "libSearchApiModule.dll"
        "libExportApiModule.dll"
        "libAiApiModule.dll"
        "libRecommendationApiModule.dll"
    )

    local found_count=0
    local total_count=${#required_modules[@]}

    for module in "${required_modules[@]}"; do
        if [ -f "$dll_dir/$module" ]; then
            print_info "找到模块: $module"
            ((found_count++))
        else
            print_warning "缺失模块: $module"
        fi
    done

    print_info "模块发现率: $found_count/$total_count"

    if [ $found_count -eq $total_count ]; then
        return 0
    else
        return 1
    fi
}

# ============================================================================
# 测试3: 可执行文件检查
# ============================================================================()

test_executable() {
    local exe="$1"

    if [ ! -f "$exe" ]; then
        print_fail "可执行文件不存在: $exe"
        return 1
    fi

    # 检查文件大小
    local size=$(stat -f%z "$exe" 2>/dev/null || stat -f%s "$exe" 2>/dev/null)
    print_info "文件大小: $size bytes"

    if [ $size -gt 100000 ]; then
        print_info "文件大小合理 (>100KB)"
        return 0
    else
        print_warning "文件大小过小"
        return 1
    fi
}

# ============================================================================
# 测试4: 依赖DLL检查
# ============================================================================()

test_dependencies() {
    local dll_dir="$1"
    local dependencies=(
        "libcurl-x64.dll"
        "libcrypto*.dll"
        "libssl*.dll"
        "libz*.dll"
    )

    local missing_deps=0

    for dep in "${dependencies[@]}"; do
        # 使用通配符查找
        found=0
        for file in $dll_dir/$dep; do
            if [ -f "$file" ]; then
                print_info "找到依赖: $(basename $file)"
                found=1
                break
            fi
        done

        if [ $found -eq 0 ]; then
            print_warning "缺失依赖: $dep"
            ((missing_deps++))
        fi
    done

    if [ $missing_deps -eq 0 ]; then
        return 0
    else
        print_warning "缺失 $missing_deps 个依赖文件"
        return 1
    fi
}

# ============================================================================
# 测试5: 配置文件内容验证
# ============================================================================()

test_config_content() {
    local config_file="$1"

    # 检查必需的配置项
    local required_keys=(
        "modulesDirectory"
        "healthCheckInterval"
        "modules"
    )

    for key in "${required_keys[@]}"; do
        if grep -q "\"$key\"" "$config_file"; then
            print_info "配置项存在: $key"
        else
            print_fail "配置项缺失: $key"
            return 1
        fi
    done

    return 0
}

# ============================================================================
# 测试6: 模块路径验证
# ============================================================================()

test_module_paths() {
    local config_file="$1"
    local base_dir="$2"

    # 提取模块路径
    local paths=$(grep -oP '"libraryPath":\s*"\K[^"]+' "$config_file" | head -3)

    local valid_paths=0
    local total_paths=$(echo "$paths" | wc -w)

    print_info "检查 $total_paths 个模块路径..."

    for path in $paths; do
        local full_path="$base_dir/$(echo $path | sed 's|^\./||')"
        if [ -f "$full_path" ]; then
            print_info "路径有效: $(basename $path)"
            ((valid_paths++))
        else
            print_warning "路径无效: $full_path"
        fi
    done

    print_info "路径验证: $valid_paths/$total_paths"

    if [ $valid_paths -eq $total_paths ] && [ $total_paths -gt 0 ]; then
        return 0
    else
        return 1
    fi
}

# ============================================================================
# 主测试函数
# ============================================================================()

main() {
    print_header "热插拔架构自动化测试"

    # 配置
    BACKEND_DIR="../backend"
    BUILD_DIR="$BACKEND_DIR/build/Release"
    CONFIG_FILE="$BACKEND_DIR/config/modules_auto.json"
    EXE_FILE="$BUILD_DIR/PaperCrawlerServerHotPlug.exe"
    DLL_DIR="$BUILD_DIR/modules/dynamic/Release"

    # 检查目录
    if [ ! -d "$BUILD_DIR" ]; then
        print_fail "构建目录不存在: $BUILD_DIR"
        exit 1
    fi

    # 执行测试
    echo -e "${YELLOW}开始测试...${NC}\n"

    # 测试组1: 配置文件解析
    print_header "测试组1: 配置文件解析"
    run_test "配置文件存在性" "test -f $CONFIG_FILE"
    run_test "配置文件格式" "test_config_parsing $CONFIG_FILE"
    run_test "配置内容验证" "test_config_content $CONFIG_FILE"

    # 测试组2: DLL文件检查
    print_header "测试组2: DLL文件检查"
    run_test "可执行文件" "test_executable $EXE_FILE"
    run_test "业务模块" "test_dll_files $DLL_DIR"
    run_test "依赖库" "test_dependencies $DLL_DIR"

    # 测试组3: 路径验证
    print_header "测试组3: 路径验证"
    run_test "模块路径" "test_module_paths $CONFIG_FILE $BUILD_DIR"

    # 测试总结
    print_header "测试总结"
    echo -e "${BLUE}总测试数:${NC} $TOTAL_TESTS"
    echo -e "${GREEN}通过:${NC} $PASSED_TESTS"
    echo -e "${RED}失败:${NC} $FAILED_TESTS"
    echo ""

    # 通过率
    local pass_rate=0
    if [ $TOTAL_TESTS -gt 0 ]; then
        pass_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    fi

    echo -e "${BLUE}通过率:${NC} $pass_rate%"
    echo ""

    # 显示详细结果
    if [ ${#TEST_RESULTS[@]} -gt 0 ]; then
        echo "详细结果:"
        for result in "${TEST_RESULTS[@]}"; do
            status=$(echo $result | cut -d: -f1)
            name=$(echo $result | cut -d: -f2)

            if [ "$status" = "PASS" ]; then
                echo -e "  ${GREEN}✓${NC} $name"
            else
                echo -e "  ${RED}✗${NC} $name"
            fi
        done
        echo ""
    fi

    # 最终结果
    if [ $FAILED_TESTS -eq 0 ]; then
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}  所有测试通过！✅${NC}"
        echo -e "${GREEN}========================================${NC}"
        echo ""
        echo "下一步: 启动服务器进行功能测试"
        echo "  cd $BUILD_DIR"
        echo "  ./PaperCrawlerServerHotPlug.exe ../../config/modules_auto.json"
        echo ""
        return 0
    else
        echo -e "${RED}========================================${NC}"
        echo -e "${RED}  有测试失败！❌${NC}"
        echo -e "${RED}========================================${NC}"
        echo ""
        echo "请检查上述失败的测试项"
        echo ""
        return 1
    fi
}

# ============================================================================
# 执行主函数
# ============================================================================()

cd "$(dirname "$0")"
main "$@"
