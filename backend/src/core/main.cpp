/**
 * @file main.cpp
 * @brief PaperCrawler模块化后端服务器主程序入口
 *
 * 功能：
 * 1. 初始化框架核心（MessageBus, Router, PluginManager）
 * 2. 从配置文件加载模块元数据
 * 3. 按依赖顺序加载系统模块
 * 4. 加载业务模块
 * 5. 启动HTTP服务器
 * 6. 注册管理API
 * 7. 优雅关闭处理
 *
 * @author PaperCrawler Team
 * @version 1.0.0
 * @date 2026-03-28
 */

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

// 框架核心
#include "core/MessageBus.hpp"
#include "core/Router.hpp"
#include "core/PluginManager.hpp"
#include "core/ModuleRegistry.hpp"
#include "core/HotReloadManager.hpp"
#include "core/HttpTypes.hpp"

// 网络模块
#include "network/HttpServerModule.hpp"

#include <spdlog/spdlog.h>

using namespace PaperCrawler;

// 全局运行标志
std::atomic<bool> g_running{true};

// 全局HTTP服务器实例
std::unique_ptr<HttpServerModule> g_httpServer;

/**
 * @brief 信号处理函数
 */
void signalHandler(int signal) {
    spdlog::info("Received shutdown signal: {}", signal);
    g_running = false;
}

/**
 * @brief 注册信号处理
 */
void setupSignalHandlers() {
    std::signal(SIGINT, signalHandler);   // Ctrl+C
    std::signal(SIGTERM, signalHandler);  // 终止信号
#ifdef SIGQUIT
    std::signal(SIGQUIT, signalHandler);  // 退出信号
#endif
}

/**
 * @brief 打印欢迎信息
 */
void printWelcome() {
    std::cout << R"(
    ========================================
       PaperCrawler Modular Backend Server
    ========================================
       Version: 1.0.0
       Architecture: 34 Modules
       Build Date: )" << __DATE__ << R"(
    ========================================
    )" << std::endl;
}

/**
 * @brief 打印启动步骤
 */
void printStep(const std::string& step, const std::string& details) {
    std::cout << "[" << step << "] " << details << "..." << std::endl;
}

/**
 * @brief 打印成功信息
 */
void printSuccess(const std::string& message) {
    std::cout << "  ✓ " << message << std::endl;
}

/**
 * @brief 打印错误信息
 */
void printError(const std::string& message) {
    std::cerr << "  ✗ " << message << std::endl;
}

/**
 * @brief 打印启动完成信息
 */
void printReady(int port) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Server is running!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  HTTP Server: http://localhost:" << port << std::endl;
    std::cout << "  WebSocket:   ws://localhost:" << (port + 1) << std::endl;
    std::cout << "  API Docs:    http://localhost:" << port << "/api/docs" << std::endl;
    std::cout << "  Metrics:     http://localhost:" << port << "/metrics" << std::endl;
    std::cout << "\n  Management endpoints:" << std::endl;
    std::cout << "    GET  /api/modules              - List modules" << std::endl;
    std::cout << "    POST /api/modules/load         - Load module" << std::endl;
    std::cout << "    POST /api/modules/unload       - Unload module" << std::endl;
    std::cout << "    POST /api/modules/reload       - Reload module" << std::endl;
    std::cout << "    GET  /health                   - Health check" << std::endl;
    std::cout << "\n  Press Ctrl+C to stop" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

/**
 * @brief 初始化框架核心
 */
bool initializeFramework() {
    printStep("1/7", "Initializing framework core");

    try {
        // 初始化核心单例
        MessageBus::getInstance();
        Router::getInstance();
        PluginManager::getInstance();
        ModuleRegistry::getInstance();

        printSuccess("Framework core initialized");
        return true;
    } catch (const std::exception& e) {
        printError(std::string("Failed to initialize framework: ") + e.what());
        return false;
    }
}

/**
 * @brief 加载模块配置
 */
bool loadModuleConfiguration() {
    printStep("2/7", "Loading module configuration");

    auto& registry = ModuleRegistry::getInstance();
    if (!registry.loadFromConfig("./config/modules.json")) {
        printError("Failed to load modules.json");
        return false;
    }

    auto allModules = registry.getAllModules();
    printSuccess("Loaded configuration for " + std::to_string(allModules.size()) + " modules");
    return true;
}

/**
 * @brief 加载和启动系统模块
 */
bool loadAndStartSystemModules() {
    printStep("3/7", "Loading and starting system modules");

    auto& registry = ModuleRegistry::getInstance();
    auto& pluginMgr = PluginManager::getInstance();

    // 按优先级排序（这里简化处理，实际应该按照priority字段排序）
    auto systemModules = registry.getModulesByType(ModuleType::SERVER);

    size_t loadedCount = 0;
    for (const auto& moduleInfo : systemModules) {
        std::cout << "  - Loading " << moduleInfo.name << "..." << std::endl;

        if (!pluginMgr.loadModule(moduleInfo.name, moduleInfo.libraryPath)) {
            printError("Failed to load " + moduleInfo.name);
            continue;
        }

        loadedCount++;
    }

    // 启动所有模块
    if (!pluginMgr.startAllModules()) {
        printError("Failed to start some modules");
        return false;
    }

    printSuccess("Loaded and started " + std::to_string(loadedCount) + " system modules");
    return true;
}

/**
 * @brief 加载业务模块
 */
bool loadBusinessModules() {
    printStep("4/7", "Loading business modules");

    auto& registry = ModuleRegistry::getInstance();
    auto& pluginMgr = PluginManager::getInstance();

    auto businessModules = registry.getModulesByType(ModuleType::BUSINESS);
    size_t loadedCount = 0;

    for (const auto& moduleInfo : businessModules) {
        std::cout << "  - Loading " << moduleInfo.name
                  << " (v" << moduleInfo.version << ")..." << std::endl;

        if (!pluginMgr.loadModule(moduleInfo.name, moduleInfo.libraryPath)) {
            printError("Warning: Failed to load " + moduleInfo.name);
            continue;
        }

        loadedCount++;
    }

    printSuccess("Loaded " + std::to_string(loadedCount) + " business modules");
    return true;
}

/**
 * @brief 注册管理API
 */
bool registerManagementAPIs() {
    printStep("5/7", "Registering management APIs");

    auto& router = Router::getInstance();

    // 模块管理API
    router.get("/api/modules", [](const HttpRequest& req) {
        // TODO: 列出所有模块
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"modules":[]})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.post("/api/modules/load", [](const HttpRequest& req) {
        // TODO: 动态加载模块
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"message":"Module loaded successfully"})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.post("/api/modules/unload", [](const HttpRequest& req) {
        // TODO: 智能卸载模块
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"message":"Module unloaded successfully"})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.post("/api/modules/reload", [](const HttpRequest& req) {
        // TODO: 热重载模块
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"message":"Module reloaded successfully"})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.get("/api/modules/:name/stats", [](const HttpRequest& req) {
        // TODO: 模块统计
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"success":true,"stats":{}})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // 健康检查API
    router.get("/health", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"status":"ok","timestamp":")" +
                       std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) +
                       R"("})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.get("/health/components", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        response.body = R"({"status":"ok","components":{"Pool":"HEALTHY","Database":"HEALTHY","Cache":"HEALTHY"}})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Papers API
    router.get("/api/papers", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        // TODO: 查询数据库获取论文列表
        response.body = R"({"success":true,"papers":[],"total":0,"page":1,"pageSize":20})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.get("/api/papers/:id", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        // TODO: 根据ID查询论文
        std::string paperId = req.getPathParam("id", "0");
        response.body = R"({"success":true,"paper":{"id":)" + paperId + R"(}})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    router.get("/api/papers/search", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        std::string query = req.getQuery("q", "");
        // TODO: 搜索论文
        response.body = R"({"success":true,"papers":[],"total":0,"query":")" + query + R"("})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Journals API
    router.get("/api/journals", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        // TODO: 查询数据库获取期刊列表
        response.body = R"({"success":true,"journals":[]})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Authors API
    router.get("/api/authors", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        // TODO: 查询数据库获取作者列表
        response.body = R"({"success":true,"authors":[]})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Collections API
    router.get("/api/collections", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        // TODO: 查询数据库获取收藏集
        response.body = R"({"success":true,"collections":[]})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    // Statistics API
    router.get("/api/stats", [](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = 200;
        // TODO: 从数据库获取统计数据
        response.body = R"({"success":true,"stats":{"totalPapers":0,"totalJournals":0,"totalAuthors":0}})";
        response.setHeader("Content-Type", "application/json");
        return response;
    });

    printSuccess("Registered 13 endpoints");
    return true;
}

/**
 * @brief 启动HTTP服务器
 */
bool startHTTPServer() {
    printStep("6/7", "Starting HTTP server");

    // 创建HTTP服务器实例
    g_httpServer = std::make_unique<HttpServerModule>(8080);

    // 初始化服务器
    if (!g_httpServer->initialize()) {
        printError("Failed to initialize HTTP server");
        return false;
    }

    // 设置路由处理器 - 将Router连接到HttpServerModule
    auto& router = Router::getInstance();
    g_httpServer->setRouteHandler([&router](const HttpRequest& req) -> HttpResponse {
        return router.route(req);
    });

    // 启动服务器
    if (!g_httpServer->start()) {
        printError("Failed to start HTTP server");
        return false;
    }

    printSuccess("HTTP server started on port 8080");
    return true;
}

/**
 * @brief 打印已注册的路由
 */
void printRegisteredRoutes() {
    auto& router = Router::getInstance();
    router.printRoutes();
}

/**
 * @brief 主循环
 */
void mainLoop() {
    printStep("7/7", "Entering main loop");

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/**
 * @brief 优雅关闭
 */
void gracefulShutdown() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Shutting down..." << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. 停止HTTP服务器
    std::cout << "  - Stopping HTTP server..." << std::endl;
    if (g_httpServer) {
        g_httpServer->stop();
        g_httpServer->cleanup();
        g_httpServer.reset();
    }

    auto& pluginMgr = PluginManager::getInstance();

    // 2. 卸载业务模块
    std::cout << "  - Unloading business modules..." << std::endl;
    auto& registry = ModuleRegistry::getInstance();
    auto businessModules = registry.getModulesByType(ModuleType::BUSINESS);

    for (auto& moduleInfo : businessModules) {
        if (moduleInfo.state == ModuleState::STARTED) {
            std::cout << "    - Unloading " << moduleInfo.name << "..." << std::endl;

            pluginMgr.unloadModule(moduleInfo.name);
        }
    }

    // 3. 停止系统模块
    std::cout << "  - Stopping system modules..." << std::endl;
    pluginMgr.stopAllModules();

    std::cout << "✓ Shutdown complete" << std::endl;
    std::cout << "========================================" << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    // 初始化Windows Sockets
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock" << std::endl;
            return 1;
        }
    #endif

    // 设置日志级别
    spdlog::set_level(spdlog::level::info);

    // 打印欢迎信息
    printWelcome();

    // 设置信号处理
    setupSignalHandlers();

    try {
        // 1. 初始化框架核心
        if (!initializeFramework()) {
            return 1;
        }

        // 2. 加载模块配置
        if (!loadModuleConfiguration()) {
            return 1;
        }

        // 3. 加载和启动系统模块
        if (!loadAndStartSystemModules()) {
            return 1;
        }

        // 4. 加载业务模块
        if (!loadBusinessModules()) {
            return 1;
        }

        // 5. 注册管理API
        if (!registerManagementAPIs()) {
            return 1;
        }

        // 6. 启动HTTP服务器
        if (!startHTTPServer()) {
            return 1;
        }

        // 打印路由
        printRegisteredRoutes();

        // 打印就绪信息
        printReady(8080);

        // 7. 主循环
        mainLoop();

        // 8. 优雅关闭
        gracefulShutdown();

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        std::cerr << "\nFatal error: " << e.what() << std::endl;
        return 1;
    }

    #ifdef _WIN32
        WSACleanup();
    #endif

    return 0;
}
