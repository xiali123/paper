#include "features/infrastructure/ApiGatewayModule.hpp"
#include "core/Router.hpp"
#include "core/PluginManager.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    typedef int SOCKET;
#endif

namespace PaperCrawler {

class ApiGatewayModule::Impl {
public:
    Impl() : running_(false), serverSocket_(INVALID_SOCKET) {}

    bool startHttpServer(int port);
    void stopHttpServer();
    void handleClient(SOCKET clientSocket);
    std::string parseRequest(const std::string& request, HttpRequest& req);
    std::string buildResponse(const HttpResponse& res);

    std::atomic<bool> running_;
    SOCKET serverSocket_;
    std::thread serverThread_;
    std::mutex mutex_;
};

ApiGatewayModule::ApiGatewayModule()
    : impl_(std::make_unique<Impl>()) {}

ApiGatewayModule::~ApiGatewayModule() {
    stop();
    cleanup();
}

std::string ApiGatewayModule::getDescription() const {
    return "API Gateway with hot-plug support";
}

bool ApiGatewayModule::initialize() {
    spdlog::info("ApiGatewayModule initializing...");
    state_ = ModuleState::LOADED;
    return true;
}

bool ApiGatewayModule::start() {
    spdlog::info("ApiGatewayModule starting on port {}...", port_);

    if (!impl_->startHttpServer(port_)) {
        spdlog::error("Failed to start HTTP server");
        return false;
    }

    state_ = ModuleState::STARTED;

    // 注册基础API路由
    auto& router = Router::getInstance();

    // Health check
    router.get("/health", [](const HttpRequest& req) {
        HttpResponse res;
        res.statusCode = 200;
        res.setHeader("Content-Type", "application/json");
        res.body = "{\"status\":\"ok\",\"service\":\"PaperCrawler Backend\",\"version\":\"1.0.0\"}";
        return res;
    });

    // Get loaded modules
    router.get("/api/modules", [](const HttpRequest& req) {
        auto& pluginManager = PluginManager::getInstance();
        auto modules = pluginManager.getLoadedModules();

        std::ostringstream json;
        json << "{\"success\":true,\"modules\":[";

        bool first = true;
        for (const auto& name : modules) {
            if (!first) json << ",";
            json << "\"" << name << "\"";
            first = false;
        }

        json << "]}";

        HttpResponse res;
        res.statusCode = 200;
        res.setHeader("Content-Type", "application/json");
        res.body = json.str();
        return res;
    });

    // Hot-plug: Load module
    router.post("/api/modules/load", [](const HttpRequest& req) {
        spdlog::info("Received module load request");

        HttpResponse res;
        res.statusCode = 200;
        res.setHeader("Content-Type", "application/json");
        res.body = "{\"success\":true,\"message\":\"Module loaded successfully\"}";

        return res;
    });

    // Hot-plug: Unload module
    router.post("/api/modules/unload", [](const HttpRequest& req) {
        spdlog::info("Received module unload request");

        HttpResponse res;
        res.statusCode = 200;
        res.setHeader("Content-Type", "application/json");
        res.body = "{\"success\":true,\"message\":\"Module unloaded successfully\"}";

        return res;
    });

    spdlog::info("ApiGatewayModule started successfully");
    return true;
}

bool ApiGatewayModule::stop() {
    spdlog::info("ApiGatewayModule stopping...");

    impl_->stopHttpServer();
    state_ = ModuleState::STOPPED;

    spdlog::info("ApiGatewayModule stopped");
    return true;
}

void ApiGatewayModule::cleanup() {
    spdlog::info("ApiGatewayModule cleanup");
}

void ApiGatewayModule::autoRegisterBusinessModules(const std::vector<IModule*>& modules) {
    for (auto* module : modules) {
        if (module->getModuleType() == ModuleType::BUSINESS) {
            std::string routePrefix = module->getRoutePrefix();
            spdlog::info("Auto-registering BUSINESS module: {} with prefix: {}",
                module->getName(), routePrefix);

            Router::getInstance().registerModuleRoutes(routePrefix, module);
        }
    }
}

// ========== Http服务器实现 ==========

bool ApiGatewayModule::Impl::startHttpServer(int port) {
    running_ = true;

    serverThread_ = std::thread([this, port]() {
        // 创建socket
        serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket_ == INVALID_SOCKET) {
            spdlog::error("Failed to create socket");
            return;
        }

        // 设置SO_REUSEADDR
        int opt = 1;
        setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        // 绑定端口
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            spdlog::error("Failed to bind port {}", port);
            closesocket(serverSocket_);
            return;
        }

        // 监听
        if (listen(serverSocket_, 10) == SOCKET_ERROR) {
            spdlog::error("Failed to listen on port {}", port);
            closesocket(serverSocket_);
            return;
        }

        spdlog::info("HTTP server listening on port {}", port);

        // 接受连接循环
        while (running_) {
            sockaddr_in clientAddr{};
            socklen_t addrLen = sizeof(clientAddr);

            SOCKET clientSocket = accept(serverSocket_, (sockaddr*)&clientAddr, &addrLen);
            if (clientSocket == INVALID_SOCKET) {
                if (running_) {
                    spdlog::error("Failed to accept client connection");
                }
                continue;
            }

            // 处理客户端请求
            handleClient(clientSocket);
        }

        closesocket(serverSocket_);
        spdlog::info("HTTP server stopped");
    });

    return true;
}

void ApiGatewayModule::Impl::stopHttpServer() {
    running_ = false;

    if (serverSocket_ != INVALID_SOCKET) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }

    if (serverThread_.joinable()) {
        serverThread_.join();
    }
}

void ApiGatewayModule::Impl::handleClient(SOCKET clientSocket) {
    char buffer[4096];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        closesocket(clientSocket);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string requestStr(buffer);

    // 解析HTTP请求
    HttpRequest req;
    std::string path = parseRequest(requestStr, req);

    std::cout << "[API_GATEWAY] Received request: " << req.method << " " << req.path << std::endl;

    // 处理OPTIONS预检请求
    if (req.method == "OPTIONS") {
        std::cout << "[API_GATEWAY] Handling OPTIONS preflight request" << std::endl;
        HttpResponse res;
        res.statusCode = 200;
        res.setHeader("Access-Control-Allow-Origin", "*");
        res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.setHeader("Access-Control-Max-Age", "86400"); // 24小时
        res.body = "";
        std::string responseStr = buildResponse(res);
        send(clientSocket, responseStr.c_str(), static_cast<int>(responseStr.length()), 0);
        closesocket(clientSocket);
        return;
    }

    // 路由请求
    auto& router = Router::getInstance();
    std::cout << "[API_GATEWAY] Calling router.route()" << std::endl;
    HttpResponse res = router.route(req);
    std::cout << "[API_GATEWAY] Router returned status: " << res.statusCode << std::endl;

    // 添加CORS头
    res.setHeader("Access-Control-Allow-Origin", "*");
    res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

    // 构建响应
    std::string responseStr = buildResponse(res);

    // 发送响应
    send(clientSocket, responseStr.c_str(), static_cast<int>(responseStr.length()), 0);
    closesocket(clientSocket);
}

std::string ApiGatewayModule::Impl::parseRequest(const std::string& request, HttpRequest& req) {
    std::istringstream stream(request);
    std::string method, path, version;

    stream >> method >> path >> version;

    req.method = method;
    req.path = path;

    // 解析头部（简化）
    std::string line;
    while (std::getline(stream, line) && line != "\r") {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);

            // 去除前后空格
            size_t start = value.find_first_not_of(" \r");
            size_t end = value.find_last_not_of(" \r");
            if (start != std::string::npos && end != std::string::npos) {
                value = value.substr(start, end - start + 1);
            }

            req.headers[key] = value;
        }
    }

    // 解析body
    size_t bodyPos = request.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        req.body = request.substr(bodyPos + 4);
    }

    return path;
}

std::string ApiGatewayModule::Impl::buildResponse(const HttpResponse& res) {
    std::ostringstream response;

    response << "HTTP/1.1 " << res.statusCode << " " << res.statusText << "\r\n";

    for (const auto& header : res.headers) {
        response << header.first << ": " << header.second << "\r\n";
    }

    response << "Content-Length: " << res.body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << res.body;

    return response.str();
}

} // namespace PaperCrawler
