#!/bin/bash
# ============================================================================
# PaperCrawler 后端构建脚本（优化版）
# ============================================================================()

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# ============================================================================
# 配置
# ============================================================================
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="build"
CLEAN_BUILD="${CLEAN_BUILD:-false}"
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc)}"

# ============================================================================
# 辅助函数
# ============================================================================()

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# ============================================================================
# 清理构建目录
# ============================================================================
clean_build() {
    print_info "Cleaning build directory..."
    rm -rf ${BUILD_DIR}
    mkdir -p ${BUILD_DIR}
    print_success "Build directory cleaned"
}

# ============================================================================
# 配置CMake
# ============================================================================
configure_cmake() {
    print_info "Configuring CMake..."

    cd ${BUILD_DIR}

    cmake .. \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -G "Unix Makefiles"

    cd ..

    print_success "CMake configured"
}

# ============================================================================
# 编译核心库
# ============================================================================
build_core() {
    print_info "Building core library..."

    cd ${BUILD_DIR}

    cmake --build . \
        --config ${BUILD_TYPE} \
        --target PaperCrawlerCore \
        --parallel ${PARALLEL_JOBS}

    cd ..

    print_success "Core library built"
}

# ============================================================================
# 编译所有业务模块
# ============================================================================
build_modules() {
    print_info "Building business modules..."

    cd ${BUILD_DIR}

    # 获取所有模块目标
    MODULES=$(cmake --build . --target help 2>/dev/null | grep -E "^\.\.\. [a-zA-Z]+Module" | awk '{print $2}' || echo "")

    if [ -z "$MODULES" ]; then
        print_warning "No modules found to build"
        return 0
    fi

    # 编译所有模块
    for module in $MODULES; do
        print_info "Building module: ${module}"

        cmake --build . \
            --config ${BUILD_TYPE} \
            --target ${module} \
            --parallel ${PARALLEL_JOBS} || {
                print_error "Failed to build module: ${module}"
                return 1
            }
    done

    cd ..

    print_success "All modules built"
}

# ============================================================================
# 编译主程序
# ============================================================================
build_executable() {
    print_info "Building main executable..."

    cd ${BUILD_DIR}

    cmake --build . \
        --config ${BUILD_TYPE} \
        --target PaperCrawlerServerHotPlug \
        --parallel ${PARALLEL_JOBS}

    cd ..

    print_success "Main executable built"
}

# ============================================================================
# 复制配置文件
# ============================================================================
copy_configs() {
    print_info "Copying configuration files..."

    mkdir -p ${BUILD_DIR}/${BUILD_TYPE}/modules/config

    if [ -f "config/modules_auto.json" ]; then
        cp config/modules_auto.json ${BUILD_DIR}/${BUILD_TYPE}/modules/config/
        print_success "Configuration files copied"
    else
        print_warning "config/modules_auto.json not found"
    fi
}

# ============================================================================
# 显示构建结果
# ============================================================================
show_results() {
    print_info "Build completed!"

    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Build Output Structure${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "${BUILD_DIR}/${BUILD_TYPE}/"
    echo "├── bin/"
    echo "│   └── PaperCrawlerServerHotPlug"
    echo "├── lib/"
    echo "│   ├── core/"
    echo "│   │   └── libPaperCrawlerCore.a"
    echo "│   └── modules/"
    echo "│       ├── libAuthApiModule.so"
    echo "│       ├── libUserApiModule.so"
    echo "│       └── ..."
    echo "└── modules/"
    echo "    └── config/"
    echo "        └── modules.json"
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo ""
}

# ============================================================================
# 主函数
# ============================================================================
main() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}  PaperCrawler Backend Build Script${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""

    # 显示配置
    print_info "Configuration:"
    echo "  Build Type: ${BUILD_TYPE}"
    echo "  Build Directory: ${BUILD_DIR}"
    echo "  Parallel Jobs: ${PARALLEL_JOBS}"
    echo "  Clean Build: ${CLEAN_BUILD}"
    echo ""

    # 清理（如果需要）
    if [ "${CLEAN_BUILD}" = "true" ]; then
        clean_build
    fi

    # 创建构建目录
    mkdir -p ${BUILD_DIR}

    # 执行构建步骤
    configure_cmake
    build_core
    build_modules
    build_executable
    copy_configs

    # 显示结果
    show_results

    print_success "All done! 🚀"
}

# ============================================================================
# 执行主函数
# ============================================================================
main "$@"
