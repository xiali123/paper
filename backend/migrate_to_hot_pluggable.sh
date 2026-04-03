#!/bin/bash
# PaperCrawler 热插拔架构迁移脚本
# 此脚本帮助将现有代码迁移到热插拔架构

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_step() {
    echo -e "\n${GREEN}════════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}  $1${NC}"
    echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}\n"
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

backup_file() {
    local file=$1
    if [ -f "$file" ]; then
        local backup="${file}.bak.$(date +%Y%m%d_%H%M%S)"
        cp "$file" "$backup"
        print_info "已备份: $file -> $backup"
    fi
}

# ============================================================================
# 步骤1: 备份现有文件
# ============================================================================
step1_backup() {
    print_step "步骤1: 备份现有文件"

    backup_file "src/core/main.cpp"
    backup_file "include/core/IModule.hpp"
    backup_file "include/core/PluginManager.hpp"
    backup_file "src/core/PluginManager.cpp"
    backup_file "include/core/Router.hpp"
    backup_file "src/core/Router.cpp"

    print_success "文件备份完成"
}

# ============================================================================
# 步骤2: 更新IModule接口
# ============================================================================
step2_update_imodule() {
    print_step "步骤2: 更新IModule接口"

    print_info "更新 include/core/IModule.hpp"

    # 备份原文件
    backup_file "include/core/IModule.hpp"

    # 创建新的IModule.hpp
    cat > include/core/IModule.hpp << 'EOF'
#pragma once

#include "ModuleExports.hpp"
#include <string>
#include <map>
#include <memory>
#include <vector>

namespace PaperCrawler {

/**
 * @brief 模块依赖描述
 */
struct ModuleDependency {
    std::string moduleName;
    std::string minVersion;
    bool required;
};

/**
 * @brief 模块接口 - 增强版
 */
class IModule {
public:
    virtual ~IModule() = default;

    // ========== 基本信息 ==========
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual ModuleType getModuleType() const = 0;

    // ========== 路由配置 ==========
    /**
     * @brief 获取路由前缀
     * @return 路由前缀，如 "/api/auth"
     *
     * 默认实现：从模块名自动生成
     */
    virtual std::string getRoutePrefix() const;

    /**
     * @brief 注册模块路由
     */
    virtual void registerRoutes() = 0;

    // ========== 依赖管理 ==========
    /**
     * @brief 获取依赖的模块列表
     */
    virtual std::vector<ModuleDependency> getDependencies() const {
        return {};
    }

    /**
     * @brief 设置依赖注入
     */
    virtual void setDependencies(const std::map<std::string, IModule*>& dependencies) {
        // 默认空实现
    }

    // ========== 生命周期 ==========
    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual void cleanup() = 0;

    // ========== 状态查询 ==========
    virtual ModuleState getState() const {
        return state_;
    }

protected:
    ModuleState state_ = ModuleState::UNLOADED;
};

} // namespace PaperCrawler
EOF

    print_success "IModule.hpp 已更新"
}

# ============================================================================
# 步骤3: 更新Router
# ============================================================================
step3_update_router() {
    print_step "步骤3: 更新Router"

    # 备份原文件
    backup_file "include/core/Router.hpp"
    backup_file "src/core/Router.cpp"

    print_info "更新 include/core/Router.hpp"

    cat > include/core/Router.hpp << 'EOF'
#pragma once

#include <string>
#include <map>
#include <functional>
#include <vector>
#include "core/HttpTypes.hpp"
#include "core/IModule.hpp"

namespace PaperCrawler {

typedef std::function<HttpResponse(const HttpRequest&)> RouteHandler;

struct RouteValue {
    RouteHandler handler;
    std::string moduleName;
    std::string description;
};

class Router {
public:
    static Router& getInstance();

    void get(const std::string& path, RouteHandler handler,
             const std::string& moduleName = "", const std::string& description = "");
    void post(const std::string& path, RouteHandler handler,
              const std::string& moduleName = "", const std::string& description = "");
    void put(const std::string& path, RouteHandler handler,
             const std::string& moduleName = "", const std::string& description = "");
    void del(const std::string& path, RouteHandler handler,
             const std::string& moduleName = "", const std::string& description = "");
    void patch(const std::string& path, RouteHandler handler,
               const std::string& moduleName = "", const std::string& description = "");
    void options(const std::string& path, RouteHandler handler,
                 const std::string& moduleName = "", const std::string& description = "");

    HttpResponse route(const HttpRequest& request);
    void registerModuleRoutes(const std::string& prefix, IModule* module);
    void printRoutes() const;

private:
    Router() = default;
    ~Router() = default;
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;

    struct RouteKey {
        std::string method;
        std::string pattern;

        bool operator<(const RouteKey& other) const {
            if (method != other.method) return method < other.method;
            return pattern < other.pattern;
        }
    };

    bool matchPattern(const std::string& pattern,
                     const std::string& path,
                     std::map<std::string, std::string>& pathParams) const;

    std::map<RouteKey, RouteValue> routes_;
};

} // namespace PaperCrawler
EOF

    print_info "更新 src/core/Router.cpp"

    cat > src/core/Router.cpp << 'EOF'
#include "core/Router.hpp"
#include "core/IModule.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <vector>

namespace PaperCrawler {

Router& Router::getInstance() {
    static Router instance;
    return instance;
}

void Router::get(const std::string& path, RouteHandler handler,
                 const std::string& moduleName, const std::string& description) {
    routes_[RouteKey{"GET", path}] = {handler, moduleName, description};
    spdlog::debug("Registered GET route: {} (module: {})", path, moduleName);
}

void Router::post(const std::string& path, RouteHandler handler,
                  const std::string& moduleName, const std::string& description) {
    routes_[RouteKey{"POST", path}] = {handler, moduleName, description};
    spdlog::debug("Registered POST route: {} (module: {})", path, moduleName);
}

void Router::put(const std::string& path, RouteHandler handler,
                 const std::string& moduleName, const std::string& description) {
    routes_[RouteKey{"PUT", path}] = {handler, moduleName, description};
    spdlog::debug("Registered PUT route: {} (module: {})", path, moduleName);
}

void Router::del(const std::string& path, RouteHandler handler,
                 const std::string& moduleName, const std::string& description) {
    routes_[RouteKey{"DELETE", path}] = {handler, moduleName, description};
    spdlog::debug("Registered DELETE route: {} (module: {})", path, moduleName);
}

void Router::patch(const std::string& path, RouteHandler handler,
                   const std::string& moduleName, const std::string& description) {
    routes_[RouteKey{"PATCH", path}] = {handler, moduleName, description};
    spdlog::debug("Registered PATCH route: {} (module: {})", path, moduleName);
}

void Router::options(const std::string& path, RouteHandler handler,
                     const std::string& moduleName, const std::string& description) {
    routes_[RouteKey{"OPTIONS", path}] = {handler, moduleName, description};
    spdlog::debug("Registered OPTIONS route: {} (module: {})", path, moduleName);
}

bool Router::matchPattern(const std::string& pattern,
                         const std::string& path,
                         std::map<std::string, std::string>& pathParams) const {
    if (pattern == path) {
        return true;
    }

    std::vector<std::string> patternParts;
    std::vector<std::string> pathParts;
    std::stringstream ssPattern(pattern);
    std::stringstream ssPath(path);
    std::string item;

    while (std::getline(ssPattern, item, '/')) {
        if (!item.empty()) patternParts.push_back(item);
    }
    while (std::getline(ssPath, item, '/')) {
        if (!item.empty()) pathParts.push_back(item);
    }

    if (patternParts.size() != pathParts.size()) {
        return false;
    }

    for (size_t i = 0; i < patternParts.size(); ++i) {
        const std::string& patternPart = patternParts[i];
        const std::string& pathPart = pathParts[i];

        if (patternPart[0] == ':') {
            std::string paramName = patternPart.substr(1);
            pathParams[paramName] = pathPart;
        } else if (patternPart != pathPart) {
            return false;
        }
    }

    return true;
}

HttpResponse Router::route(const HttpRequest& request) {
    spdlog::info("Routing: {} {}", request.method, request.path);

    for (const auto& pair : routes_) {
        if (pair.first.method == request.method) {
            if (pair.first.pattern.find(':') == std::string::npos) {
                if (pair.first.pattern == request.path) {
                    try {
                        return pair.second.handler(request);
                    } catch (const std::exception& e) {
                        spdlog::error("Route handler error: {}", e.what());
                        HttpResponse errorResponse;
                        errorResponse.statusCode = 500;
                        errorResponse.statusText = "Internal Server Error";
                        errorResponse.headers["Content-Type"] = "application/json";
                        errorResponse.body = "{\"error\":\"" + std::string(e.what()) + "\"}";
                        return errorResponse;
                    }
                }
            }
        }
    }

    for (const auto& pair : routes_) {
        if (pair.first.method == request.method) {
            if (pair.first.pattern.find(':') != std::string::npos) {
                std::map<std::string, std::string> pathParams;

                if (matchPattern(pair.first.pattern, request.path, pathParams)) {
                    HttpRequest requestWithParams = request;
                    requestWithParams.pathParams = pathParams;

                    try {
                        return pair.second.handler(requestWithParams);
                    } catch (const std::exception& e) {
                        spdlog::error("Route handler error: {}", e.what());
                        HttpResponse errorResponse;
                        errorResponse.statusCode = 500;
                        errorResponse.statusText = "Internal Server Error";
                        errorResponse.headers["Content-Type"] = "application/json";
                        errorResponse.body = "{\"error\":\"" + std::string(e.what()) + "\"}";
                        return errorResponse;
                    }
                }
            }
        }
    }

    spdlog::warn("Route not found: {} {}", request.method, request.path);

    HttpResponse notFoundResponse;
    notFoundResponse.statusCode = 404;
    notFoundResponse.statusText = "Not Found";
    notFoundResponse.headers["Content-Type"] = "application/json";
    notFoundResponse.body = "{\"error\":\"Route not found\"}";
    return notFoundResponse;
}

void Router::registerModuleRoutes(const std::string& prefix, IModule* module) {
    if (!module) {
        spdlog::error("Cannot register routes for null module");
        return;
    }

    spdlog::info("Module {} registering routes with prefix: {}",
                 module->getName(), prefix);
}

void Router::printRoutes() const {
    std::cout << "\n  Registered routes:" << std::endl;
    for (const auto& pair : routes_) {
        std::cout << "    " << pair.first.method << "    " << pair.first.pattern;
        if (!pair.second.moduleName.empty()) {
            std::cout << " (" << pair.second.moduleName << ")";
        }
        std::cout << std::endl;
    }
}

} // namespace PaperCrawler
EOF

    print_success "Router 已更新"
}

# ============================================================================
# 步骤4: 更新PluginManager
# ============================================================================
step4_update_plugin_manager() {
    print_step "步骤4: 更新PluginManager"

    # 备份原文件
    backup_file "include/core/PluginManager.hpp"
    backup_file "src/core/PluginManager.cpp"

    print_info "更新 include/core/PluginManager.hpp"

    cat > include/core/PluginManager.hpp << 'EOF'
#pragma once

#include "IModule.hpp"
#include "ModuleExports.hpp"
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

namespace PaperCrawler {

class PluginManager {
public:
    static PluginManager& getInstance();

    bool initialize();
    bool loadModule(const std::string& moduleName, const std::string& modulePath);
    bool unloadModule(const std::string& moduleName);
    IModule* getModule(const std::string& moduleName);
    std::vector<IModule*> getAllModules();
    std::vector<IModule*> getBusinessModules();
    std::vector<IModule*> getServerModules();
    bool startAllModules();
    bool stopAllModules();
    std::vector<std::string> getLoadedModules() const;
    bool scanAndLoadModules(const std::string& modulesDir);
    void registerAllModuleRoutes();
    std::vector<std::string> resolveDependencies(const std::vector<std::string>& moduleNames) const;

private:
    PluginManager() = default;
    ~PluginManager();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    std::string extractModuleName(const std::string& filename) const;

    std::map<std::string, std::unique_ptr<IModule>> modules_;
    std::map<std::string, ModuleHandle> handles_;
    mutable std::recursive_mutex mutex_;
};

} // namespace PaperCrawler
EOF

    print_info "更新 src/core/PluginManager.cpp (在scanAndLoadModules末尾添加路由注册)"

    # 在scanAndLoadModules方法末尾添加路由注册调用
    # 注意：这需要手动编辑，或者使用sed命令

    print_success "PluginManager 已更新"
    print_warning "请手动在 scanAndLoadModules() 方法末尾添加: registerAllModuleRoutes()"
}

# ============================================================================
# 步骤5: 创建新的main.cpp
# ============================================================================
step5_create_main() {
    print_step "步骤5: 创建新的main.cpp"

    # 备份原文件
    backup_file "src/core/main.cpp"

    print_info "创建精简的 main.cpp"

    cat > src/core/main.cpp << 'EOF'
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

// 核心模块
#include "core/PluginManager.hpp"
#include "core/Router.hpp"
#include "network/HttpServerModule.hpp"

// 全局变量
std::atomic<bool> g_running{true};
std::unique_ptr<HttpServerModule> g_httpServer;

// 信号处理
void signalHandler(int signal) {
    spdlog::info("Received signal {}, shutting down...", signal);
    g_running = false;
}

// 日志初始化
void initializeLogging() {
    auto console = spdlog::stdout_color_mt("console");
    console->set_level(spdlog::level::info);
    spdlog::set_default_logger(console);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
}

// 主函数
int main(int argc, char* argv[]) {
    // 1. 初始化日志
    initializeLogging();

    spdlog::info("╔═══════════════════════════════════════════════════════╗");
    spdlog::info("║     PaperCrawler Backend - Hot Pluggable Architecture ║");
    spdlog::info("╚═══════════════════════════════════════════════════════╝");

    // 2. 设置信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // 3. 初始化插件管理器
    auto& pluginManager = PluginManager::getInstance();
    if (!pluginManager.initialize()) {
        spdlog::error("Failed to initialize PluginManager");
        return 1;
    }
    spdlog::info("✓ PluginManager initialized");

    // 4. 扫描并加载所有模块
    std::string modulesDir = "modules/dynamic";
    if (argc > 1) {
        modulesDir = argv[1];
    }

    spdlog::info("Loading modules from: {}", modulesDir);

    if (!pluginManager.scanAndLoadModules(modulesDir)) {
        spdlog::warn("Some modules failed to load, continuing...");
    }

    // 5. 打印已加载模块
    auto loadedModules = pluginManager.getLoadedModules();
    spdlog::info("✓ Loaded {} module(s):", loadedModules.size());
    for (const auto& name : loadedModules) {
        spdlog::info("  - {}", name);
    }

    // 6. 打印所有路由
    spdlog::info("╔═══════════════════════════════════════════════════════╗");
    spdlog::info("║                    Registered Routes                   ║");
    spdlog::info("╚═══════════════════════════════════════════════════════╝");
    Router::getInstance().printRoutes();

    // 7. 启动所有模块
    if (!pluginManager.startAllModules()) {
        spdlog::error("Failed to start some modules");
        return 1;
    }
    spdlog::info("✓ All modules started");

    // 8. 创建并启动HTTP服务器
    g_httpServer = std::make_unique<HttpServerModule>();
    if (!g_httpServer->initialize()) {
        spdlog::error("Failed to initialize HTTP server");
        return 1;
    }

    if (!g_httpServer->start()) {
        spdlog::error("Failed to start HTTP server");
        return 1;
    }

    spdlog::info("╔═══════════════════════════════════════════════════════╗");
    spdlog::info("║          Server Started Successfully! 🚀               ║");
    spdlog::info("║          Listening on http://localhost:8080            ║");
    spdlog::info("╚═══════════════════════════════════════════════════════╝");

    // 9. 主循环
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 10. 优雅关闭
    spdlog::info("╔═══════════════════════════════════════════════════════╗");
    spdlog::info("║                    Shutting Down...                    ║");
    spdlog::info("╚═══════════════════════════════════════════════════════╝");

    if (g_httpServer) {
        g_httpServer->stop();
        spdlog::info("✓ HTTP Server stopped");
    }

    pluginManager.stopAllModules();
    spdlog::info("✓ All modules stopped");

    spdlog::info("Goodbye! 👋");

    return 0;
}
EOF

    print_success "main.cpp 已创建 (行数: $(wc -l < src/core/main.cpp))"

    if [ $(wc -l < src/core/main.cpp) -lt 300 ]; then
        print_success "main.cpp 行数符合要求 (< 300行)"
    else
        print_warning "main.cpp 行数超标 (>= 300行)"
    fi
}

# ============================================================================
# 步骤6: 更新模块实现
# ============================================================================
step6_update_modules() {
    print_step "步骤6: 更新模块实现"

    print_info "需要手动更新以下模块的 .cpp 文件:"
    print_info "  - AuthApiModule"
    print_info "  - PaperApiModule"
    print_info "  - UserApiModule"
    print_info "  - AiApiModule"
    print_info "  - SearchApiModule"
    print_info "  - ExportApiModule"

    print_info ""
    print_info "每个模块需要添加 registerRoutes() 方法，示例:"
    cat << 'EXAMPLE'

void YourModule::registerRoutes() {
    auto& router = Router::getInstance();
    auto prefix = getRoutePrefix();  // 自动生成: /api/yourmodule

    router.post(
        prefix + "/endpoint",
        [this](const HttpRequest& req) { return handleEndpoint(req); },
        getName(),
        "Description"
    );
}
EXAMPLE

    print_success "模块更新指南已显示"
}

# ============================================================================
# 步骤7: 编译测试
# ============================================================================
step7_compile() {
    print_step "步骤7: 编译测试"

    print_info "开始编译项目..."

    if [ ! -d "build" ]; then
        print_info "创建build目录并配置CMake..."
        mkdir -p build
        cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release
        cd ..
    fi

    cd build
    if cmake --build . --config Release; then
        print_success "项目编译成功"
        cd ..
        return 0
    else
        print_error "项目编译失败"
        cd ..
        return 1
    fi
}

# ============================================================================
# 主函数
# ============================================================================
main() {
    print_info "PaperCrawler 热插拔架构迁移脚本"
    print_info "开始时间: $(date)"
    print_info "工作目录: $(pwd)"

    echo ""
    read -p "是否继续? (y/n) " -n 1 -r
    echo ""

    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_info "取消操作"
        exit 0
    fi

    # 执行迁移步骤
    step1_backup
    step2_update_imodule
    step3_update_router
    step4_update_plugin_manager
    step5_create_main
    step6_update_modules

    print_step "迁移完成"

    print_success "文件迁移完成！"
    print_warning "接下来需要:"
    print_info "  1. 手动更新每个模块的 .cpp 文件，添加 registerRoutes() 方法"
    print_info "  2. 在 PluginManager.cpp 的 scanAndLoadModules() 末尾添加路由注册调用"
    print_info "  3. 运行编译测试: step7_compile"

    echo ""
    read -p "是否立即编译? (y/n) " -n 1 -r
    echo ""

    if [[ $REPLY =~ ^[Yy]$ ]]; then
        step7_compile
    fi

    print_info "完成时间: $(date)"
}

# 运行主函数
main
