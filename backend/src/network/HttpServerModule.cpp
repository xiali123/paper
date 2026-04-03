#include "network/HttpServerModule.hpp"
#include "core/Router.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <cstring>
#include <algorithm>

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
    typedef int SOCKET
#endif

namespace PaperCrawler {

class HttpServerModule::Impl {
public:
    SOCKET serverSocket_{INVALID_SOCKET};
    int port_{8080};
    bool running_{false};
    std::thread acceptThread_;
    HttpHandler routeHandler_;

    // 统计信息
    HttpServerModule::ServerStats stats_;

    bool initializeWinsock() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            spdlog::error("WSAStartup failed");
            return false;
        }
#endif
        return true;
    }

    bool createSocket() {
        serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket_ == INVALID_SOCKET) {
            spdlog::error("Socket creation failed");
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
            spdlog::error("Bind failed on port {}", port_);
            return false;
        }

        return true;
    }

    bool listenSocket() {
        if (listen(serverSocket_, SOMAXCONN) == SOCKET_ERROR) {
            spdlog::error("Listen failed");
            return false;
        }
        return true;
    }

    void acceptLoop() {
        spdlog::info("HTTP server accept loop started on port {}", port_);

        while (running_) {
            sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);

            SOCKET clientSocket = accept(serverSocket_, (sockaddr*)&clientAddr, &clientAddrLen);
            if (clientSocket == INVALID_SOCKET) {
                if (running_) {
                    spdlog::error("Accept failed");
                }
                continue;
            }

            // Increment active connections
            stats_.activeConnections++;

            // Handle client in a separate thread
            std::thread([this, clientSocket, clientAddr]() {
                handleClient(clientSocket, clientAddr);
            }).detach();
        }
    }

    void handleClient(SOCKET clientSocket, sockaddr_in clientAddr) {
        // First, read the initial buffer to get headers
        char buffer[8192];
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            stats_.totalBytesReceived += bytesRead;

            // Debug: Log raw request
            spdlog::debug("Raw request:\n{}", std::string(buffer, bytesRead));

            // Parse HTTP request headers first
            std::string requestStr(buffer);
            size_t headerEnd = requestStr.find("\r\n\r\n");

            if (headerEnd == std::string::npos) {
                spdlog::warn("Incomplete request headers received");
                return;
            }

            // Extract headers part
            std::string headersPart = requestStr.substr(0, headerEnd);

            // Check for Content-Length to see if we need to read more data
            size_t contentLengthPos = headersPart.find("Content-Length:");
            if (contentLengthPos != std::string::npos) {
                size_t colonPos = headersPart.find(":", contentLengthPos);
                size_t valueStart = headersPart.find_first_not_of(" \t", colonPos + 1);
                size_t valueEnd = headersPart.find("\r\n", valueStart);
                std::string contentLengthStr = headersPart.substr(valueStart, valueEnd - valueStart);
                int contentLength = std::stoi(contentLengthStr);

                spdlog::info("POST request with Content-Length: {}", contentLength);

                // Calculate how much body data we have already
                size_t bodyStart = headerEnd + 4;
                int currentBodyLength = bytesRead - bodyStart;

                // If we don't have all the body data yet, read more
                if (currentBodyLength < contentLength) {
                    int remainingBytes = contentLength - currentBodyLength;
                    spdlog::info("Need to read {} more bytes for body", remainingBytes);

                    int additionalBytes = recv(clientSocket, buffer + bytesRead, sizeof(buffer) - bytesRead - 1, 0);
                    if (additionalBytes > 0) {
                        bytesRead += additionalBytes;
                        buffer[bytesRead] = '\0';
                        stats_.totalBytesReceived += additionalBytes;
                        spdlog::info("Read {} additional bytes", additionalBytes);
                    }
                }
            }

            // Parse HTTP request
            std::string fullRequestStr(buffer, bytesRead);
            HttpRequest request = parseRequest(fullRequestStr);

            // Debug: Log parsed request
            spdlog::info("Request: {} {}", request.method, request.path);

            // Set client info
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
            request.remoteAddress = clientIP;
            request.remotePort = ntohs(clientAddr.sin_port);

            // Increment total requests
            stats_.totalRequests++;
            stats_.requestsByPath[request.path]++;

            // Route the request
            HttpResponse response;
            if (routeHandler_) {
                try {
                    response = routeHandler_(request);
                } catch (const std::exception& e) {
                    spdlog::error("Error handling request: {}", e.what());
                    response.statusCode = 500;
                    response.statusText = "Internal Server Error";
                    response.setError(500, "Internal server error");
                }
            } else {
                spdlog::warn("No route handler set, returning 404");
                response.statusCode = 404;
                response.statusText = "Not Found";
                response.setError(404, "No route handler configured");
            }

            // Send response
            std::string responseStr = formatResponse(response);
            send(clientSocket, responseStr.c_str(), responseStr.length(), 0);

            stats_.totalBytesSent += responseStr.length();
        }

        // Decrement active connections and close socket
        stats_.activeConnections--;
#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
    }

    HttpRequest parseRequest(const std::string& requestStr) {
        HttpRequest request;
        std::istringstream iss(requestStr);
        std::string line;

        // Parse request line
        if (std::getline(iss, line)) {
            std::istringstream lineStream(line);
            lineStream >> request.method >> request.path >> request.version;
            spdlog::info("Parsed request: {} {} {}", request.method, request.path, request.version);
        }

        // Parse headers
        while (std::getline(iss, line) && !line.empty()) {
            if (line.back() == '\r') {
                line.pop_back();
            }

            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);

                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                request.headers[key] = value;
            }
        }

        // Parse query parameters
        size_t queryPos = request.path.find('?');
        if (queryPos != std::string::npos) {
            std::string queryString = request.path.substr(queryPos + 1);
            request.path = request.path.substr(0, queryPos);

            // Parse query string
            std::istringstream queryStream(queryString);
            std::string pair;
            while (std::getline(queryStream, pair, '&')) {
                size_t equalPos = pair.find('=');
                if (equalPos != std::string::npos) {
                    std::string key = pair.substr(0, equalPos);
                    std::string value = pair.substr(equalPos + 1);
                    request.queryParams[key] = value;
                }
            }
        }

        // Parse body (if any) - everything after the empty line following headers
        // Get the remaining content from the stream
        std::string body;
        std::string remaining;
        while (std::getline(iss, line)) {
            if (!line.empty() || body.empty()) {
                // Only add newline if it's not the first empty line after headers
                body += line + "\n";
            }
        }

        // Trim trailing newlines from body
        while (!body.empty() && (body.back() == '\n' || body.back() == '\r')) {
            body.pop_back();
        }

        // Fallback: If body parsing failed but we know there should be a body,
        // extract it directly from the original request string
        if (body.empty()) {
            size_t contentLengthPos = requestStr.find("Content-Length:");
            if (contentLengthPos != std::string::npos) {
                // Find the body start (after \r\n\r\n)
                size_t bodyStart = requestStr.find("\r\n\r\n");
                if (bodyStart != std::string::npos) {
                    bodyStart += 4;
                    if (bodyStart < requestStr.length()) {
                        body = requestStr.substr(bodyStart);
                        spdlog::info("Extracted body directly from request string, {} bytes", body.length());
                    }
                }
            }
        }

        if (!body.empty()) {
            request.body = body;
            spdlog::info("Parsed body with {} bytes: '{}'", body.length(), body.substr(0, std::min(size_t(100), body.length())));
        } else {
            spdlog::warn("Body is empty after parsing");
        }

        return request;
    }

    std::string formatResponse(const HttpResponse& response) {
        std::ostringstream oss;

        // Status line
        oss << "HTTP/1.1 " << response.statusCode << " " << response.statusText << "\r\n";

        // Headers
        for (const auto& header : response.headers) {
            oss << header.first << ": " << header.second << "\r\n";
        }

        // Content-Length
        oss << "Content-Length: " << response.body.length() << "\r\n";

        // End of headers
        oss << "\r\n";

        // Body
        oss << response.body;

        return oss.str();
    }
};

// HttpServerModule implementation

HttpServerModule::HttpServerModule(uint16_t port)
    : port_(port), impl_(std::make_unique<Impl>()) {
    impl_->port_ = port;
}

HttpServerModule::~HttpServerModule() {
    stop();
    cleanup();
}

bool HttpServerModule::initialize() {
    spdlog::info("Initializing HTTP server on port {}", port_);

    if (!impl_->initializeWinsock()) {
        spdlog::error("Failed to initialize Winsock");
        return false;
    }

    if (!impl_->createSocket()) {
        spdlog::error("Failed to create socket");
        return false;
    }

    if (!impl_->bindSocket()) {
        spdlog::error("Failed to bind socket");
        return false;
    }

    if (!impl_->listenSocket()) {
        spdlog::error("Failed to listen on socket");
        return false;
    }

    spdlog::info("HTTP server initialized successfully");
    return true;
}

bool HttpServerModule::start() {
    if (running_) {
        spdlog::warn("HTTP server already running");
        return true;
    }

    spdlog::info("Starting HTTP server...");

    impl_->running_ = true;
    impl_->acceptThread_ = std::thread([this]() {
        impl_->acceptLoop();
    });

    running_ = true;
    spdlog::info("HTTP server started on port {}", port_);
    return true;
}

bool HttpServerModule::stop() {
    if (!running_) {
        return true;
    }

    spdlog::info("Stopping HTTP server...");
    impl_->running_ = false;

#ifdef _WIN32
    closesocket(impl_->serverSocket_);
#else
    close(impl_->serverSocket_);
#endif

    if (impl_->acceptThread_.joinable()) {
        impl_->acceptThread_.join();
    }

    running_ = false;
    spdlog::info("HTTP server stopped");
    return true;
}

void HttpServerModule::cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void HttpServerModule::setRouteHandler(HttpHandler handler) {
    impl_->routeHandler_ = handler;
    spdlog::info("Route handler set");
}

HttpServerModule::ServerStats HttpServerModule::getStats() const {
    return impl_->stats_;
}

void HttpServerModule::resetStats() {
    impl_->stats_ = HttpServerModule::ServerStats{};
}

} // namespace PaperCrawler
