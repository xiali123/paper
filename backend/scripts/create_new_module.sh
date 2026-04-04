#!/bin/bash
# ============================================================================
# PaperCrawler Backend - New Module Creator
# ============================================================================
#
# This script creates a new module that conforms to the development standards
# defined in MODULE_DEVELOPMENT_STANDARDS.md
#
# Usage:
#   bash scripts/create_new_module.sh <ModuleName> [<Display Name>] [<Description>]
#
# Example:
#   bash scripts/create_new_module.sh ExportApiModule "Export API" "Export and download papers"
#
# Output:
#   - Creates include/business/<ModuleName>.hpp
#   - Creates src/business/<ModuleName>.cpp
#   - Updates CMakeLists.txt
#   - Runs compliance check
#
# ============================================================================

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Parse arguments
MODULE_CLASS_NAME=$1
MODULE_DISPLAY_NAME=${2:-"${MODULE_CLASS_NAME%Module}"}
MODULE_DESCRIPTION=${3:-"${MODULE_DISPLAY_NAME} module"}

if [ -z "$MODULE_CLASS_NAME" ]; then
    echo -e "${RED}❌ Error: Module class name not provided${NC}"
    echo ""
    echo "Usage: $0 <ModuleClassName> [\"Display Name\"] [\"Description\"]"
    echo ""
    echo "Example:"
    echo "  $0 ExportApiModule \"Export API\" \"Export and download papers\""
    exit 1
fi

# Validate module name format
if [[ ! $MODULE_CLASS_NAME =~ ^[A-Z][a-zA-Z0-9]*Module$ ]]; then
    echo -e "${RED}❌ Error: Invalid module class name format${NC}"
    echo ""
    echo "Module class name must:"
    echo "  - Start with uppercase letter"
    echo "  - Contain only alphanumeric characters"
    echo "  - End with 'Module'"
    echo ""
    echo "Valid examples: ExportApiModule, UserAuthModule, StatsModule"
    exit 1
fi

# ============================================================================
# Configuration
# ============================================================================

HEADER_FILE="include/business/${MODULE_CLASS_NAME}.hpp"
SOURCE_FILE="src/business/${MODULE_CLASS_NAME}.cpp"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODULE_API_NAME="${MODULE_CLASS_NAME%Module}"

# Convert CamelCase to lowercase for API routes
# Example: ExportApiModule -> exportapi
MODULE_ROUTE_NAME=$(echo "$MODULE_API_NAME" | sed 's/\([A-Z]\)/\L\1/g')

echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}🔨 PaperCrawler New Module Creator${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Module Class Name: ${MODULE_CLASS_NAME}"
echo -e "Display Name:      ${MODULE_DISPLAY_NAME}"
echo -e "Description:       ${MODULE_DESCRIPTION}"
echo -e "API Route Prefix:  /api/${MODULE_ROUTE_NAME}"
echo ""

# Change to project root
cd "$PROJECT_ROOT" || {
    echo -e "${RED}❌ Error: Cannot change to project root: ${PROJECT_ROOT}${NC}"
    exit 1
}

# ============================================================================
# Check if module already exists
# ============================================================================

if [ -f "$HEADER_FILE" ] || [ -f "$SOURCE_FILE" ]; then
    echo -e "${RED}❌ Error: Module already exists!${NC}"
    echo ""
    if [ -f "$HEADER_FILE" ]; then
        echo -e "  ${RED}✗${NC} Header file exists: ${HEADER_FILE}"
    fi
    if [ -f "$SOURCE_FILE" ]; then
        echo -e "  ${RED}✗${NC} Source file exists: ${SOURCE_FILE}"
    fi
    echo ""
    echo "Please choose a different module name or remove existing files."
    exit 1
fi

# ============================================================================
# Create Header File
# ============================================================================

echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Step 1: Creating Header File${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

cat > "$HEADER_FILE" <<EOF
#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <memory>

namespace PaperCrawler {

class ${MODULE_CLASS_NAME} : public BusinessModuleBase {
public:
    // 默认构造函数（用于DLL导出）
    ${MODULE_CLASS_NAME}();

    // 构造函数（可选：依赖注入）
    explicit ${MODULE_CLASS_NAME}(std::shared_ptr<IDatabase> database);

    ~${MODULE_CLASS_NAME}() override;

    // 模块元数据
    std::string getName() const override { return "${MODULE_API_NAME}"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "${MODULE_DESCRIPTION}";
    }

private:
    // 路由注册
    void registerRoutes() override;

    // 依赖注入
    std::shared_ptr<IDatabase> database_;
};

} // namespace PaperCrawler
EOF

echo -e "${GREEN}✅ Created:${NC} ${HEADER_FILE}"

# ============================================================================
# Create Source File
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Step 2: Creating Source File${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

cat > "$SOURCE_FILE" <<EOF
#include "business/${MODULE_CLASS_NAME}.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include <spdlog/spdlog.h>

namespace PaperCrawler {

// ============================================================================
// 构造函数和析构函数
// ============================================================================

// 默认构造函数
${MODULE_CLASS_NAME}::${MODULE_CLASS_NAME}()
    : ${MODULE_CLASS_NAME}(nullptr) {
    std::cout << "[${MODULE_API_NAME}] ${MODULE_CLASS_NAME} default constructor" << std::endl;
}

// 带参数的构造函数
${MODULE_CLASS_NAME}::${MODULE_CLASS_NAME}(std::shared_ptr<IDatabase> database)
    : database_(database) {
    // 初始化代码
    std::cout << "[${MODULE_API_NAME}] ${MODULE_CLASS_NAME} parameterized constructor" << std::endl;
}

// 析构函数
${MODULE_CLASS_NAME}::~${MODULE_CLASS_NAME}() = default;

// ============================================================================
// 路由注册
// ============================================================================

void ${MODULE_CLASS_NAME}::registerRoutes() {
    auto& router = Router::getInstance();

    std::cout << "[${MODULE_API_NAME}] Registering routes..." << std::endl;

    // GET /api/${MODULE_ROUTE_NAME}/list - 获取列表
    router.get("/api/${MODULE_ROUTE_NAME}/list", [this](const HttpRequest& req) {
        spdlog::info("[${MODULE_API_NAME}] Handling GET /api/${MODULE_ROUTE_NAME}/list");

        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({
            "status": "ok",
            "data": [],
            "message": "List retrieved successfully"
        })";

        return response;
    });

    // POST /api/${MODULE_ROUTE_NAME}/create - 创建新项
    router.post("/api/${MODULE_ROUTE_NAME}/create", [this](const HttpRequest& req) {
        spdlog::info("[${MODULE_API_NAME}] Handling POST /api/${MODULE_ROUTE_NAME}/create");

        HttpResponse response;
        response.statusCode = 201;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({
            "status": "ok",
            "message": "Item created successfully"
        })";

        return response;
    });

    // GET /api/${MODULE_ROUTE_NAME}/<id> - 获取详情
    router.get(R"(/api/${MODULE_ROUTE_NAME}/(\d+))", [this](const HttpRequest& req) {
        // 从路径中提取ID
        std::string id = req.matches[1];

        spdlog::info("[${MODULE_API_NAME}] Handling GET /api/${MODULE_ROUTE_NAME}/{}", id);

        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({
            "status": "ok",
            "data": {
                "id": ")" + id + R"(",
                "name": "Sample Item"
            },
            "message": "Item retrieved successfully"
        })";

        return response;
    });

    // PUT /api/${MODULE_ROUTE_NAME}/<id> - 更新
    router.put(R"(/api/${MODULE_ROUTE_NAME}/(\d+))", [this](const HttpRequest& req) {
        std::string id = req.matches[1];

        spdlog::info("[${MODULE_API_NAME}] Handling PUT /api/${MODULE_ROUTE_NAME}/{}", id);

        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({
            "status": "ok",
            "message": "Item updated successfully"
        })";

        return response;
    });

    // DELETE /api/${MODULE_ROUTE_NAME}/<id> - 删除
    router.del(R"(/api/${MODULE_ROUTE_NAME}/(\d+))", [this](const HttpRequest& req) {
        std::string id = req.matches[1];

        spdlog::info("[${MODULE_API_NAME}] Handling DELETE /api/${MODULE_ROUTE_NAME}/{}", id);

        HttpResponse response;
        response.statusCode = 200;
        response.headers["Content-Type"] = "application/json";
        response.body = R"({
            "status": "ok",
            "message": "Item deleted successfully"
        })";

        return response;
    });

    std::cout << "[${MODULE_API_NAME}] Routes registered successfully" << std::endl;
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    return new PaperCrawler::${MODULE_CLASS_NAME}();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::${MODULE_CLASS_NAME}*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}
EOF

echo -e "${GREEN}✅ Created:${NC} ${SOURCE_FILE}"

# ============================================================================
# Update CMakeLists.txt
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Step 3: Updating CMakeLists.txt${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

CMAKE_FILE="CMakeLists.txt"

# Check if module already in CMakeLists.txt
if grep -q "${MODULE_CLASS_NAME}" "$CMAKE_FILE"; then
    echo -e "${YELLOW}⚠️  Warning: Module already found in CMakeLists.txt${NC}"
    echo -e "${YELLOW}   Skipping CMakeLists.txt update${NC}"
else
    # Find the last add_dynamic_module line and insert after it
    if grep -q "add_dynamic_module" "$CMAKE_FILE"; then
        # Insert after the last add_dynamic_module
        sed -i "/add_dynamic_module.*/a\\
# ${MODULE_DISPLAY_NAME}\\
add_dynamic_module(${MODULE_CLASS_NAME}\\
    src/business/${MODULE_CLASS_NAME}.cpp\\
)" "$CMAKE_FILE"
    else
        # Append to end of file
        echo "" >> "$CMAKE_FILE"
        echo "# ${MODULE_DISPLAY_NAME}" >> "$CMAKE_FILE"
        echo "add_dynamic_module(${MODULE_CLASS_NAME}" >> "$CMAKE_FILE"
        echo "    src/business/${MODULE_CLASS_NAME}.cpp" >> "$CMAKE_FILE"
        echo ")" >> "$CMake_FILE"
    fi

    echo -e "${GREEN}✅ Updated:${NC} ${CMAKE_FILE}"
fi

# ============================================================================
# Run Compliance Check
# ============================================================================

echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Step 4: Running Compliance Check${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

if bash scripts/check_module_standards.sh "$MODULE_CLASS_NAME"; then
    echo ""
    echo -e "${GREEN}✅ Compliance check passed!${NC}"
else
    echo ""
    echo -e "${RED}❌ Compliance check failed!${NC}"
    echo ""
    echo "Please review the errors above and fix them manually."
    exit 1
fi

# ============================================================================
# Success Summary
# ============================================================================

echo ""
echo -e "${CYAN}═══════════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}✨ Module Created Successfully!${NC}"
echo -e "${CYAN}═══════════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Created Files:"
echo -e "  📄 ${GREEN}${HEADER_FILE}${NC}"
echo -e "  📄 ${GREEN}${SOURCE_FILE}${NC}"
echo ""
echo -e "Modified Files:"
echo -e "  📝 ${GREEN}${CMAKE_FILE}${NC}"
echo ""
echo -e "Next Steps:"
echo ""
echo -e "  1. ${CYAN}Review the generated files${NC} and customize your module logic"
echo -e "  2. ${CYAN}Build the module:${NC}"
echo -e "     cd backend/build"
echo -e "     cmake .. -DCMAKE_BUILD_TYPE=Release"
echo -e "     cmake --build . --config Release --target ${MODULE_CLASS_NAME}"
echo ""
echo -e "  3. ${CYAN}Add module configuration${NC} to config/modules_auto.json:"
echo -e     "{"
echo -e "       \"module\": \"${MODULE_API_NAME}\","
echo -e "       \"library\": \"./modules/dynamic/Release/lib${MODULE_CLASS_NAME}.dll\","
echo -e "       \"priority\": 50,"
echo -e "       \"enabled\": true,"
echo -e "       \"dependencies\": []"
echo -e     }"
echo ""
echo -e "  4. ${CYAN}Restart the server${NC} to load the new module"
echo ""
echo -e "${GREEN}🎉 Happy coding!${NC}"
echo ""
