#include "framework/PluginManager.hpp"
#include "modules/ApiGatewayModule.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <csignal>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

using namespace PaperCrawler;

// 全局变量用于信号处理
std::unique_ptr<ApiGatewayModule> g_gateway;
volatile bool g_running = true;

void signalHandler(int signal) {
    spdlog::info("Received signal {}, shutting down...", signal);
    g_running = false;
}

int main(int argc, char* argv[]) {
    // 初始化Windows Sockets
    #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock" << std::endl;
            return 1;
        }
    #endif

    // 设置信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "========================================" << std::endl;
    std::cout << "  PaperCrawler Modular Backend Server" << std::endl;
    std::cout << "  Version: 1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;

    spdlog::set_level(spdlog::level::info);
    spdlog::info("Starting PaperCrawler Backend...");

    try {
        // 1. 初始化插件管理器
        auto& pluginManager = PluginManager::getInstance();
        if (!pluginManager.initialize()) {
            spdlog::error("Failed to initialize PluginManager");
            return 1;
        }

        // 2. 创建并启动ApiGateway模块
        g_gateway = std::make_unique<ApiGatewayModule>();

        // 设置端口（可通过命令行参数修改）
        int port = 8080;
        if (argc > 1) {
            port = std::atoi(argv[1]);
        }
        g_gateway->setPort(port);

        // 初始化并启动
        if (!g_gateway->initialize()) {
            spdlog::error("Failed to initialize ApiGatewayModule");
            return 1;
        }

        if (!g_gateway->start()) {
            spdlog::error("Failed to start ApiGatewayModule");
            return 1;
        }

        spdlog::info("========================================");
        spdlog::info("Server started successfully!");
        spdlog::info("HTTP API: http://localhost:{}", port);
        spdlog::info("Health Check: http://localhost:{}/health", port);
        spdlog::info("API Modules: http://localhost:{}/api/modules", port);
        spdlog::info("========================================");
        spdlog::info("Press Ctrl+C to stop...");

        // 3. 主循环
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 4. 清理
        spdlog::info("Shutting down...");

        if (g_gateway) {
            g_gateway->stop();
            g_gateway->cleanup();
        }

        pluginManager.stopAllModules();

        spdlog::info("Shutdown complete");

        return 0;

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    #ifdef _WIN32
        WSACleanup();
    #endif
}
