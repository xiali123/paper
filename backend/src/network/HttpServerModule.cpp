#include "network/HttpServerModule.hpp"
#include "core/Router.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <cerrno>

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
    HttpHandler routeHandler_;

    // 统计信息
    HttpServerModule::ServerStats stats_;

    // Case-insensitive string search (HTTP headers are case-insensitive)
    static std::string toLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    static size_t findHeaderCI(const std::string& haystack, const std::string& needle) {
        std::string lowerHay = toLower(haystack);
        std::string lowerNeedle = toLower(needle);
        return lowerHay.find(lowerNeedle);
    }

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
#ifdef _WIN32
                int error = WSAGetLastError();
                if (running_ && error != WSAEINTR) {
                    spdlog::error("Accept failed: WSA error {}", error);
                }
#else
                if (running_ && errno != EINTR) {
                    spdlog::error("Accept failed: {}", strerror(errno));
                }
#endif
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
            spdlog::info("Raw request ({} bytes):\n{}", bytesRead, std::string(buffer, bytesRead));

            // Parse HTTP request headers first
            std::string requestStr(buffer);
            size_t headerEnd = requestStr.find("\r\n\r\n");

            if (headerEnd == std::string::npos) {
                spdlog::warn("Incomplete request headers received");
                return;
            }

            // Extract headers part
            std::string headersPart = requestStr.substr(0, headerEnd);

            // Check for Transfer-Encoding: chunked or Content-Length (case-insensitive)
            size_t bodyStart = headerEnd + 4;
            size_t contentLengthPos = findHeaderCI(headersPart, "Content-Length:");
            size_t chunkedPos = findHeaderCI(headersPart, "Transfer-Encoding:");

            if (chunkedPos != std::string::npos) {
                std::string teValue = headersPart.substr(chunkedPos + 18);
                size_t teEnd = teValue.find("\r\n");
                if (teEnd != std::string::npos) teValue = teValue.substr(0, teEnd);
                // Trim spaces
                while (!teValue.empty() && teValue[0] == ' ') teValue.erase(0, 1);

                if (teValue.find("chunked") != std::string::npos) {
                    spdlog::info("Chunked transfer encoding detected, reading chunks...");

                    // Read remaining data already in buffer
                    std::string accumulated(buffer + bodyStart, bytesRead - bodyStart);
                    std::string decodedBody;

                    // Decode chunks from accumulated data
                    while (true) {
                        // Find chunk size line
                        size_t chunkSizeEnd = accumulated.find("\r\n");
                        if (chunkSizeEnd == std::string::npos) {
                            // Need more data
                            char tmpBuf[4096];
                            int n = recv(clientSocket, tmpBuf, sizeof(tmpBuf) - 1, 0);
                            if (n > 0) {
                                tmpBuf[n] = '\0';
                                accumulated += tmpBuf;
                                stats_.totalBytesReceived += n;
                                continue;
                            }
                            break;
                        }

                        std::string chunkSizeStr = accumulated.substr(0, chunkSizeEnd);
                        // Parse hex chunk size (strip extensions after semicolon)
                        size_t extPos = chunkSizeStr.find(';');
                        if (extPos != std::string::npos) chunkSizeStr = chunkSizeStr.substr(0, extPos);
                        while (!chunkSizeStr.empty() && chunkSizeStr[0] == ' ') chunkSizeStr.erase(0, 1);

                        int chunkSize = 0;
                        try { chunkSize = std::stoi(chunkSizeStr, nullptr, 16); } catch (...) { break; }

                        if (chunkSize == 0) break; // Last chunk

                        accumulated.erase(0, chunkSizeEnd + 2); // Skip size line + \r\n

                        // Read chunk data
                        while (accumulated.length() < (size_t)chunkSize + 2) {
                            char tmpBuf[4096];
                            int n = recv(clientSocket, tmpBuf, sizeof(tmpBuf) - 1, 0);
                            if (n > 0) {
                                tmpBuf[n] = '\0';
                                accumulated += tmpBuf;
                                stats_.totalBytesReceived += n;
                            } else break;
                        }

                        if (accumulated.length() >= (size_t)chunkSize) {
                            decodedBody += accumulated.substr(0, chunkSize);
                            accumulated.erase(0, chunkSize + 2); // Skip data + \r\n
                        }
                    }

                    spdlog::info("Decoded chunked body: {} bytes", decodedBody.length());

                    // Replace buffer with a synthetic Content-Length request
                    std::string newRequest = requestStr.substr(0, headerEnd + 4) + decodedBody;
                    size_t copyLen = std::min(newRequest.length(), sizeof(buffer) - 1);
                    memcpy(buffer, newRequest.c_str(), copyLen);
                    bytesRead = copyLen;
                    buffer[bytesRead] = '\0';
                }
            } else if (contentLengthPos != std::string::npos) {
                size_t colonPos = headersPart.find(":", contentLengthPos);
                size_t valueStart = headersPart.find_first_not_of(" \t", colonPos + 1);
                size_t valueEnd = headersPart.find("\r\n", valueStart);
                std::string contentLengthStr = headersPart.substr(valueStart, valueEnd - valueStart);
                int contentLength = std::stoi(contentLengthStr);

                spdlog::info("POST request with Content-Length: {}", contentLength);

                // Calculate how much body data we have already
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

            // Handle OPTIONS preflight requests for CORS
            if (request.method == "OPTIONS") {
                spdlog::info("Handling OPTIONS preflight request");
                HttpResponse optionsResponse;
                optionsResponse.statusCode = 200;
                optionsResponse.setHeader("Access-Control-Allow-Origin", "*");
                optionsResponse.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                optionsResponse.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
                optionsResponse.setHeader("Access-Control-Max-Age", "86400");
                std::string responseStr = formatResponse(optionsResponse);
                send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
                return;
            }

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

        // Parse headers (normalize keys to Title-Case for case-insensitive matching)
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

                // Normalize header key to Title-Case (HTTP headers are case-insensitive)
                std::string normKey;
                bool nextUpper = true;
                for (char c : key) {
                    if (c == '-') {
                        nextUpper = true;
                        normKey += c;
                    } else if (nextUpper) {
                        normKey += std::toupper(static_cast<unsigned char>(c));
                        nextUpper = false;
                    } else {
                        normKey += std::tolower(static_cast<unsigned char>(c));
                    }
                }

                request.headers[normKey] = value;
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

        // Parse body: extract directly from requestStr after \r\n\r\n
        // This is more reliable than std::getline which can miss content without trailing \n
        std::string body;
        size_t bodyStart = requestStr.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            bodyStart += 4;
            if (bodyStart < requestStr.length()) {
                body = requestStr.substr(bodyStart);
                // Trim trailing whitespace
                while (!body.empty() && (body.back() == '\r' || body.back() == '\n' || body.back() == ' ')) {
                    body.pop_back();
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

        // Add CORS headers if not already present
        if (response.headers.find("Access-Control-Allow-Origin") == response.headers.end()) {
            oss << "Access-Control-Allow-Origin: *\r\n";
            oss << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
            oss << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
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
