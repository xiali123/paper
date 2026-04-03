/**
 * @file main.cpp
 * @brief PaperCrawler模块化后端服务器主程序入口（重构版）
 *
 * 功能：
 * 1. 初始化框架核心（MessageBus, Router, ModuleLoader）
 * 2. 自动加载所有业务模块
 * 3. 启动HTTP服务器
 * 4. 注册管理和监控API
 * 5. 优雅关闭处理
 *
 * 特性：
 * - 完全自动化的模块加载和路由注册
 * - 模块健康检查和故障隔离
 * - 模块热重载支持
 * - 生产级别的日志记录
 *
 * @author PaperCrawler Team
 * @version 2.0.0
 * @date 2026-04-04
 */

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <filesystem>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

// 框架核心
#include "core/MessageBus.hpp"
#include "core/Router.hpp"
#include "core/ModuleLoader.hpp"
#include "core/HttpTypes.hpp"
#include "core/ConfigManager.hpp"
#include "core/ServiceContainer.hpp"

// 网络模块
#include "network/HttpServerModule.hpp"

// JSON library
#include <spdlog/spdlog.h>
#include "../../core/external/nlohmann/json.hpp"

using namespace PaperCrawler;
using json = nlohmann::json;

// ============================================================================
// 全局变量
// ============================================================================

std::atomic<bool> g_running{true};
std::unique_ptr<HttpServerModule> g_httpServer;

// ============================================================================
// 信号处理
// ============================================================================

void signalHandler(int signal) {
    spdlog::info("Received signal: {}", signal);
    g_running.store(false);
}

void setupSignalHandlers() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    #ifdef _WIN32
        std::signal(SIGBREAK, signalHandler);
    #else
        std::signal(SIGHUP, signalHandler);
    #endif
}

// ============================================================================
// 管理API端点
// ============================================================================

void registerManagementAPIs() {
    auto& router = Router::getInstance();
    auto& loader = ModuleLoader::getInstance();

    // 模块列表API
    router.get("/api/modules", [&loader](const HttpRequest& req) {
        try {
            auto modules = loader.getAllModulesMetadata();
            json response = json::array();

            for (const auto& metadata : modules) {
                response.push_back(json::parse(metadata.toJson()));
            }

            HttpResponse httpResponse;
            httpResponse.statusCode = 200;
            httpResponse.headers["Content-Type"] = "application/json";
            httpResponse.body = response.dump(2);
            return httpResponse;

        } catch (const std::exception& e) {
            HttpResponse httpResponse;
            httpResponse.statusCode = 500;
            httpResponse.headers["Content-Type"] = "application/json";
            httpResponse.body = json{{"error", e.what()}}.dump();
            return httpResponse;
        }
    });

    // 模块详情API
    router.get("/api/modules/:name", [&loader](const HttpRequest& req) {
        try {
            std::string moduleName = req.pathParams.at("name");
            auto* metadata = loader.getModuleMetadata(moduleName);

            if (!metadata) {
                HttpResponse httpResponse;
                httpResponse.statusCode = 404;
                httpResponse.headers["Content-Type"] = "application/json";
                httpResponse.body = json{{"error", "Module not found"}}.dump();
                return httpResponse;
            }

            HttpResponse httpResponse;
            httpResponse.statusCode = 200;
            httpResponse.headers["Content-Type"] = "application/json";
            httpResponse.body = metadata->toJson();
            return httpResponse;

        } catch (const std::exception& e) {
            HttpResponse httpResponse;
            httpResponse.statusCode = 500;
            httpResponse.headers["Content-Type"] = "application/json";
            httpResponse.body = json{{"error", e.what()}}.dump();
            return httpResponse;
        }
    });

    // 模块重载API
    router.post("/api/modules/:name/reload", [&loader](const HttpRequest& req) {
        try {
            std::string moduleName = req.pathParams.at("name");

            if (loader.reloadModule(moduleName)) {
                HttpResponse httpResponse;
                httpResponse.statusCode = 200;
                httpResponse.headers["Content-Type"] = "application/json";
                httpResponse.body = json{{"message", "Module reloaded successfully"}}.dump();
                return httpResponse;
            } else {
                HttpResponse httpResponse;
                httpResponse.statusCode = 500;
                httpResponse.headers["Content-Type"] = "application/json";
                httpResponse.body = json{{"error", "Failed to reload module"}}.dump();
                return httpResponse;
            }

        } catch (const std::exception& e) {
            HttpResponse httpResponse;
            httpResponse.statusCode = 500;
            httpResponse.headers["Content-Type"] = "application/json";
            httpResponse.body = json{{"error", e.what()}}.dump();
            return httpResponse;
        }
    });

    // 健康检查API
    router.get("/api/health", [&loader](const HttpRequest& req) {
        try {
            auto modules = loader.getAllModulesMetadata();
            json health = json::object();

            int healthyCount = 0;
            int unhealthyCount = 0;

            for (const auto& metadata : modules) {
                json moduleHealth;
                moduleHealth["status"] = (metadata.healthStatus == ModuleHealthStatus::HEALTHY) ? "healthy" : "unhealthy";
                moduleHealth["uptime"] = metadata.getUptimeSeconds();
                moduleHealth["errorRate"] = metadata.getErrorRate();
                moduleHealth["requestCount"] = metadata.requestCount;
                moduleHealth["errorCount"] = metadata.errorCount;

                health[metadata.name] = moduleHealth;

                if (metadata.isHealthy()) {
                    healthyCount++;
                } else {
                    unhealthyCount++;
                }
            }

            health["summary"]["total"] = modules.size();
            health["summary"]["healthy"] = healthyCount;
            health["summary"]["unhealthy"] = unhealthyCount;

            HttpResponse httpResponse;
            httpResponse.statusCode = (unhealthyCount == 0) ? 200 : 503;
            httpResponse.headers["Content-Type"] = "application/json";
            httpResponse.body = health.dump(2);
            return httpResponse;

        } catch (const std::exception& e) {
            HttpResponse httpResponse;
            httpResponse.statusCode = 500;
            httpResponse.headers["Content-Type"] = "application/json";
            httpResponse.body = json{{"error", e.what()}}.dump();
            return httpResponse;
        }
    });

    // 系统信息API
    router.get("/api/system/info", [](const HttpRequest& req) {
        try {
            json systemInfo;
            systemInfo["version"] = "2.0.0";
            systemInfo["name"] = "PaperCrawler Backend";
            systemInfo["description"] = "Modular backend with automatic module loading";
            systemInfo["platform"] =
                #ifdef _WIN32
                    "Windows"
                #else
                    "Linux"
                #endif
            ;
            systemInfo["architecture"] =
                #ifdef _WIN32
                    #ifdef _WIN64
                        "x64"
                    #else
                        "x86"
                    #endif
                #else
                    "x64"
                #endif
            ;

            HttpResponse httpResponse;
            httpResponse.statusCode = 200;
            httpResponse.headers["Content-Type"] = "application/json";
            httpResponse.body = systemInfo.dump(2);
            return httpResponse;

        } catch (const std::exception& e) {
            HttpResponse httpResponse;
            httpResponse.statusCode = 500;
            httpResponse.headers["Content-Type"] = "application/json";
            httpResponse.body = json{{"error", e.what()}}.dump();
            return httpResponse;
        }
    });

    spdlog::info("Management APIs registered successfully");
}

// ============================================================================
// 模块事件监听器
// ============================================================================

void setupModuleEventListeners() {
    auto& loader = ModuleLoader::getInstance();

    // 监听模块加载事件
    loader.registerEventListener("module_loaded", [](const std::string& moduleName, const std::string& message) {
        spdlog::info("[Event] Module loaded: {} - {}", moduleName, message);
    });

    // 监听模块失败事件
    loader.registerEventListener("module_failed", [](const std::string& moduleName, const std::string& message) {
        spdlog::error("[Event] Module failed: {} - {}", moduleName, message);
    });

    // 监听模块健康状态变化
    loader.registerEventListener("module_unhealthy", [](const std::string& moduleName, const std::string& message) {
        spdlog::warn("[Event] Module unhealthy: {} - {}", moduleName, message);
    });

    // 监听模块重载事件
    loader.registerEventListener("module_reloaded", [](const std::string& moduleName, const std::string& message) {
        spdlog::info("[Event] Module reloaded: {} - {}", moduleName, message);
    });

    spdlog::info("Module event listeners registered");
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char* argv[]) {
    // 初始化日志
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    spdlog::info("========================================");
    spdlog::info("PaperCrawler Backend v2.0.0");
    spdlog::info("Modular Architecture with Auto-Loading");
    spdlog::info("========================================");

    // 设置信号处理
    setupSignalHandlers();

    // 初始化核心组件
    spdlog::info("Initializing core components...");

    MessageBus::getInstance();
    Router::getInstance();

    // 初始化模块加载器
    auto& loader = ModuleLoader::getInstance();

    std::string configPath = (argc > 1) ? argv[1] : "config/modules.json";
    if (!loader.initialize(configPath)) {
        spdlog::warn("Failed to initialize module loader with config, using defaults");
    }

    // 注册模块事件监听器
    setupModuleEventListeners();

    // 加载所有模块
    spdlog::info("Loading modules...");
    if (!loader.loadAllModules()) {
        spdlog::warn("Some modules failed to load, continuing...");
    }

    // 启动所有模块
    spdlog::info("Starting modules...");
    if (!loader.startAllModules()) {
        spdlog::error("Failed to start all modules");
        return 1;
    }

    // 注册管理API
    registerManagementAPIs();

    // 打印所有路由
    Router::getInstance().printRoutes();

    // 启动HTTP服务器
    spdlog::info("Starting HTTP server...");
    g_httpServer = std::make_unique<HttpServerModule>();
    g_httpServer->initialize();

    // 设置路由处理器
    auto& router = Router::getInstance();
    g_httpServer->setRouteHandler([&router](const HttpRequest& req) -> HttpResponse {
        return router.route(req);
    });

    // 启动服务器（端口8080）
    if (!g_httpServer->start()) {
        spdlog::error("Failed to start HTTP server");
        return 1;
    }

    spdlog::info("HTTP server started on port 8080");

    // 启动健康检查线程
    loader.startHealthCheckThread(30);  // 每30秒检查一次

    spdlog::info("========================================");
    spdlog::info("Server is running");
    spdlog::info("Management APIs:");
    spdlog::info("  GET    /api/modules         - List all modules");
    spdlog::info("  GET    /api/modules/:name   - Get module details");
    spdlog::info("  POST   /api/modules/:name/reload - Reload module");
    spdlog::info("  GET    /api/health           - Health check");
    spdlog::info("  GET    /api/system/info      - System information");
    spdlog::info("========================================");

    // 主循环
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 优雅关闭
    spdlog::info("Shutting down gracefully...");

    // 停止健康检查
    loader.stopHealthCheckThread();

    // 停止HTTP服务器
    if (g_httpServer) {
        g_httpServer->stop();
        g_httpServer->cleanup();
    }

    // 停止所有模块
    loader.stopAllModules();

    spdlog::info("Shutdown complete");
    return 0;
}
