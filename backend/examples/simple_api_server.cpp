#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <thread>
#include <vector>
#include <ctime>

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

// Simple JSON response builder
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

std::string buildPapersJsonResponse(const std::string& papersJson, int total, int page, int pageSize) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Connection: close\r\n\r\n";

    response << "{\n";
    response << "  \"success\": true,\n";
    response << "  \"data\": {\n";
    response << "    \"papers\": " << papersJson << ",\n";
    response << "    \"total\": " << total << ",\n";
    response << "    \"page\": " << page << ",\n";
    response << "    \"pageSize\": " << pageSize << ",\n";
    response << "    \"totalPages\": " << ((total + pageSize - 1) / pageSize) << "\n";
    response << "  }\n";
    response << "}\n";

    return response.str();
}

std::string buildStatsJsonResponse(const std::map<std::string, std::string>& stats) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Connection: close\r\n\r\n";

    response << "{\n";
    response << "  \"success\": true,\n";
    response << "  \"data\": {\n";

    bool first = true;
    for (const auto& pair : stats) {
        if (!first) response << ",\n";
        first = false;
        response << "    \"" << pair.first << "\": " << pair.second;
    }

    response << "\n  }\n";
    response << "}\n";

    return response.str();
}

// Mock papers data
std::string getMockPapers() {
    return "[\n"
           "  {\n"
           "    \"id\": 1,\n"
           "    \"title\": \"Attention Is All You Need\",\n"
           "    \"authors\": \"Ashish Vaswani et al.\",\n"
           "    \"year\": 2023,\n"
           "    \"citation_count\": 150\n"
           "  },\n"
           "  {\n"
           "    \"id\": 2,\n"
           "    \"title\": \"BERT: Pre-training of Deep Bidirectional Transformers\",\n"
           "    \"authors\": \"Jacob Devlin et al.\",\n"
           "    \"year\": 2019,\n"
           "    \"citation_count\": 89000\n"
           "  },\n"
           "  {\n"
           "    \"id\": 3,\n"
           "    \"title\": \"Deep Residual Learning for Image Recognition\",\n"
           "    \"authors\": \"Kaiming He et al.\",\n"
           "    \"year\": 2016,\n"
           "    \"citation_count\": 150000\n"
           "  },\n"
           "  {\n"
           "    \"id\": 4,\n"
           "    \"title\": \"GPT-4 Technical Report\",\n"
           "    \"authors\": \"OpenAI\",\n"
           "    \"year\": 2023,\n"
           "    \"citation_count\": 5000\n"
           "  }\n"
           "]";
}

// Handle request
std::string handleRequest(const std::string& method, const std::string& path, const std::string& body) {
    std::cout << "[" << method << "] " << path << std::endl;

    // Health check
    if (path == "/health" || path == "/api/health") {
        return buildJsonResponse({
            {"status", "ok"},
            {"message", "PaperCrawler Backend Running"}
        });
    }

    // Get papers list
    if (path == "/api/papers" || path == "/api/papers/") {
        return buildPapersJsonResponse(getMockPapers(), 4, 1, 20);
    }

    // Get papers stats
    if (path == "/api/papers/stats") {
        return buildStatsJsonResponse({
            {"totalPapers", "4"},
            {"readPapers", "2"},
            {"unreadPapers", "2"},
            {"bookmarkedPapers", "2"}
        });
    }

    // Get single paper
    if (path.find("/api/papers/") == 0 && path != "/api/papers/" &&
        path != "/api/papers/stats" && path.find("/search") == std::string::npos) {

        // Extract paper ID
        size_t idStart = path.find_last_of('/');
        if (idStart != std::string::npos) {
            std::string idStr = path.substr(idStart + 1);

            // Mock paper details
            return buildPapersJsonResponse("[\n"
                   "  {\n"
                   "    \"id\": " + idStr + ",\n"
                   "    \"title\": \"Sample Paper\",\n"
                   "    \"authors\": \"Test Author\",\n"
                   "    \"year\": 2023,\n"
                   "    \"abstract\": \"This is a test abstract...\"\n"
                   "  }\n"
                   "]", 1, 1, 1);
        }
    }

    // 404 - Not found
    return buildJsonResponse({
        {"error", "Endpoint not found"},
        {"path", path}
    }, 404);
}

// Main server
int main() {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    // Set socket options
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    // Bind to port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        return 1;
    }

    // Listen
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "  PaperCrawler Backend Server" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Server running on http://localhost:8080" << std::endl;
    std::cout << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  GET  /health" << std::endl;
    std::cout << "  GET  /api/health" << std::endl;
    std::cout << "  GET  /api/papers" << std::endl;
    std::cout << "  GET  /api/papers/stats" << std::endl;
    std::cout << "  GET  /api/papers/:id" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Accept loop
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        // Receive request
        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';

            // Parse HTTP request
            std::istringstream request(buffer);
            std::string method, path, version;
            request >> method >> path >> version;

            // Handle request and send response
            std::string response = handleRequest(method, path, "");
            send(clientSocket, response.c_str(), response.length(), 0);
        }

        // Close client socket
#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
    }

    // Cleanup
#ifdef _WIN32
    closesocket(serverSocket);
    WSACleanup();
#else
    close(serverSocket);
#endif

    return 0;
}
