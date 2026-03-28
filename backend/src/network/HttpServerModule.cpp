#include "network/HttpServerModule.hpp"
#include <iostream>
#include <sstream>
#include <thread>
#include <cstring>

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
    typedef int SOCKET;
#endif

namespace PaperCrawler {

class HttpServerModule::Impl {
public:
    SOCKET serverSocket_{INVALID_SOCKET};
    int port_{8080};
    bool running_{false};
    std::thread acceptThread_;

    // HTTP请求解析
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string version;
        std::map<std::string, std::string> headers;
        std::string body;
    };

    bool initializeWinsock() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return false;
        }
#endif
        return true;
    }

    bool createSocket() {
        serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket_ == INVALID_SOCKET) {
            std::cerr << "Socket creation failed" << std::endl;
            return false;
        }

        // Set socket options
        int opt = 1;
        setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        return true;
    }

    bool bindSocket() {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port_);

        if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed on port " << port_ << std::endl;
            return false;
        }

        return true;
    }

    bool listenSocket() {
        if (listen(serverSocket_, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed" << std::endl;
            return false;
        }
        return true;
    }

    void acceptLoop() {
        while (running_) {
            sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);

            SOCKET clientSocket = accept(serverSocket_, (sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSocket == INVALID_SOCKET) {
                if (running_) {
                    std::cerr << "Accept failed" << std::endl;
                }
                continue;
            }

            // Handle client in a separate thread
            std::thread([this, clientSocket]() {
                handleClient(clientSocket);
            }).detach();
        }
    }

    void handleClient(SOCKET clientSocket) {
        char buffer[8192];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';

            // Parse HTTP request
            HttpRequest request = parseRequest(buffer);

            // Handle request through router
            std::string response = handleRequest(request);

            // Send response
            send(clientSocket, response.c_str(), response.length(), 0);
        }

        // Close client socket
#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
    }

    HttpRequest parseRequest(const std::string& rawData) {
        HttpRequest request;
        std::istringstream iss(rawData);

        // Parse request line
        iss >> request.method >> request.path >> request.version;

        // Parse headers (simplified)
        std::string line;
        std::getline(iss, line); // consume remainder of request line
        while (std::getline(iss, line) && line != "\r") {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                // Trim whitespace
                size_t start = value.find_first_not_of(" \r");
                size_t end = value.find_last_not_of(" \r");
                if (start != std::string::npos && end != std::string::npos) {
                    value = value.substr(start, end - start + 1);
                }
                request.headers[key] = value;
            }
        }

        // Parse body (if any)
        std::string body;
        while (std::getline(iss, line)) {
            body += line + "\n";
        }
        request.body = body;

        return request;
    }

    std::string handleRequest(const HttpRequest& request) {
        // TODO: 通过Router处理请求
        // 这里暂时返回简单的响应
        return buildJsonResponse({
            {"status", "ok"},
            {"message", "HttpServerModule running"}
        });
    }

    std::string buildJsonResponse(const std::map<std::string, std::string>& data, int statusCode = 200) {
        std::ostringstream response;
        response << "HTTP/1.1 " << statusCode;
        switch (statusCode) {
            case 200: response << " OK"; break;
            case 201: response << " Created"; break;
            case 400: response << " Bad Request"; break;
            case 404: response << " Not Found"; break;
            case 500: response << " Internal Server Error"; break;
            default: response << " Unknown"; break;
        }
        response << "\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Connection: close\r\n\r\n";

        response << "{\n";
        bool first = true;
        for (const auto& pair : data) {
            if (!first) response << ",\n";
            first = false;
            response << "  \"" << pair.first << "\": \"" << pair.second << "\"";
        }
        response << "\n}\n";

        return response.str();
    }

    void cleanupWinsock() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

// ============================================================================
// HttpServerModule Implementation
// ============================================================================

HttpServerModule::HttpServerModule()
    : impl_(std::make_unique<Impl>()) {
}

HttpServerModule::~HttpServerModule() {
    if (impl_->serverSocket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(impl_->serverSocket_);
#else
        close(impl_->serverSocket_);
#endif
    }
}

std::string HttpServerModule::getName() const {
    return "HttpServer";
}

std::string HttpServerModule::getVersion() const {
    return "1.0.0";
}

std::string HttpServerModule::getDescription() const {
    return "HTTP server module";
}

ModuleType HttpServerModule::getModuleType() const {
    return ModuleType::SERVER;
}

bool HttpServerModule::initialize() {
    if (!impl_->initializeWinsock()) {
        return false;
    }

    if (!impl_->createSocket()) {
        return false;
    }

    if (!impl_->bindSocket()) {
        return false;
    }

    return true;
}

bool HttpServerModule::start() {
    if (!impl_->listenSocket()) {
        return false;
    }

    impl_->running_ = true;

    // Start accept loop in separate thread
    impl_->acceptThread_ = std::thread([this]() {
        impl_->acceptLoop();
    });

    std::cout << "HttpServer started on port " << impl_->port_ << std::endl;
    return true;
}

bool HttpServerModule::stop() {
    impl_->running_ = false;

    // Close server socket to unblock accept()
    if (impl_->serverSocket_ != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(impl_->serverSocket_);
#else
        close(impl_->serverSocket_);
#endif
        impl_->serverSocket_ = INVALID_SOCKET;
    }

    // Wait for accept thread to finish
    if (impl_->acceptThread_.joinable()) {
        impl_->acceptThread_.join();
    }

    std::cout << "HttpServer stopped" << std::endl;
    return true;
}

void HttpServerModule::cleanup() {
    impl_->cleanupWinsock();
}

void HttpServerModule::setPort(int port) {
    impl_->port_ = port;
}

int HttpServerModule::getPort() const {
    return impl_->port_;
}

bool HttpServerModule::isRunning() const {
    return impl_->running_;
}

} // namespace PaperCrawler
