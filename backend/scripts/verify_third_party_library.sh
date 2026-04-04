#!/bin/bash

# =============================================================================
# Third-Party Library Verification Script
# =============================================================================
#
# 验证第三方库是否遵循标准目录结构和配置
#
# 用法: bash scripts/verify_third_party_library.sh <library-name>
#
# 示例: bash scripts/verify_third_party_library.sh gumbo
#
# =============================================================================

set -e  # 遇到错误立即退出

LIBRARY_NAME=$1

if [ -z "$LIBRARY_NAME" ]; then
    echo "❌ 错误: 请提供库名称"
    echo "用法: $0 <library-name>"
    echo "示例: $0 gumbo"
    exit 1
fi

echo "🔍 Verifying third-party library: $LIBRARY_NAME"
echo "================================================"
echo ""

# 1. 检查库根目录
echo "📁 [1/7] Checking library directory..."
LIB_PATH="../core/external/$LIBRARY_NAME"

if [ ! -d "$LIB_PATH" ]; then
    echo "❌ Library directory not found: $LIB_PATH"
    echo "   Expected: core/external/$LIBRARY_NAME/"
    exit 1
fi
echo "✅ Found library directory: $LIB_PATH"
echo ""

# 2. 检查标准目录结构
echo "📂 [2/7] Checking standard directory structure..."
REQUIRED_DIRS=("include" "src")
OPTIONAL_DIRS=("win32" "docs" "tests" "examples")

for dir in "${REQUIRED_DIRS[@]}"; do
    if [ ! -d "$LIB_PATH/$dir" ]; then
        echo "❌ Missing required directory: $dir/"
        exit 1
    else
        echo "✅ Found required directory: $dir/"
    fi
done

for dir in "${OPTIONAL_DIRS[@]}"; do
    if [ -d "$LIB_PATH/$dir" ]; then
        echo "✅ Found optional directory: $dir/"
    fi
done
echo ""

# 3. 检查源文件
echo "📝 [3/7] Checking source files..."
SOURCE_COUNT=$(find "$LIB_PATH/src" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.cc" \) 2>/dev/null | wc -l)

if [ $SOURCE_COUNT -eq 0 ]; then
    echo "❌ No source files found in src/"
    echo "   Expected at least one .c or .cpp file"
    exit 1
fi

echo "✅ Found $SOURCE_COUNT source file(s) in src/"

# 列出前5个源文件
find "$LIB_PATH/src" -type f \( -name "*.c" -o -name "*.cpp" \) | head -5 | while read file; do
    echo "   - $(basename "$file")"
done

if [ $SOURCE_COUNT -gt 5 ]; then
    echo "   ... and $((SOURCE_COUNT - 5)) more"
fi
echo ""

# 4. 检查头文件
echo "📄 [4/7] Checking header files..."
HEADER_COUNT=$(find "$LIB_PATH/include" -type f \( -name "*.h" -o -name "*.hpp" \) 2>/dev/null | wc -l)

if [ $HEADER_COUNT -eq 0 ]; then
    echo "⚠️  No header files found in include/"
    echo "   This is unusual for a C/C++ library"
else
    echo "✅ Found $HEADER_COUNT header file(s) in include/"

    # 列出头文件
    find "$LIB_PATH/include" -type f \( -name "*.h" -o -name "*.hpp" \) | while read file; do
        echo "   - $(basename "$file")"
    done
fi
echo ""

# 5. 检查Windows兼容性
echo "🪟 [5/7] Checking Windows compatibility..."
if [ -f "$LIB_PATH/win32/strings.h" ]; then
    echo "✅ Win32 compatibility header found: win32/strings.h"
elif [ -f "$LIB_PATH/src/strings.h" ]; then
    echo "✅ Win32 compatibility header found: src/strings.h"
else
    echo "ℹ️  No strings.h replacement found"
    echo "   If the library uses Unix functions (strncasecmp, etc.),"
    echo "   consider creating win32/strings.h"
fi
echo ""

# 6. 检查build_info.txt
echo "📋 [6/7] Checking build_info.txt..."
if [ -f "$LIB_PATH/build_info.txt" ]; then
    echo "✅ build_info.txt found"
    echo "   Content:"
    head -5 "$LIB_PATH/build_info.txt" | sed 's/^/   /'
else
    echo "⚠️  build_info.txt not found (recommended)"
    echo "   Consider adding build_info.txt with:"
    echo "   Name: <library-name>"
    echo "   Version: <version>"
    echo "   Source: <URL>"
    echo "   License: <license>"
fi
echo ""

# 7. 检查README
echo "📖 [7/7] Checking README..."
if [ -f "$LIB_PATH/README.md" ] || [ -f "$LIB_PATH/README" ] || [ -f "$LIB_PATH/README.txt" ]; then
    echo "✅ README found"
else
    echo "⚠️  No README found (recommended)"
fi
echo ""

# 生成验证报告
echo "================================================"
echo "✅ Verification PASSED for $LIBRARY_NAME"
echo ""
echo "📊 Summary:"
echo "   - Source files: $SOURCE_COUNT"
echo "   - Header files: $HEADER_COUNT"
echo "   - Standard structure: ✅"
echo ""
echo "🚀 Next steps:"
echo "   1. Add to CMakeLists.txt:"
echo "      add_third_party_library($LIBRARY_NAME)"
echo ""
echo "   2. Link to SystemModules:"
echo "      target_link_libraries(SystemModules PUBLIC $LIBRARY_NAME)"
echo ""
echo "   3. Build and verify:"
echo "      cmake --build . --config Release"
echo ""

exit 0
