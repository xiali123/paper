#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <iomanip>

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

// PaperCrawler API
#include "core/PaperCrawlerAPI.hpp"
#include "core/Exception.hpp"

using namespace PaperCrawler;

// Simple JSON response builder
std::string buildResponse(const std::string& content, const std::string& contentType = "application/json") {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << contentType << "; charset=utf-8\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response << "Access-Control-Allow-Headers: Content-Type\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "\r\n";
    response << content;
    return response.str();
}

std::string buildErrorResponse(int code, const std::string& message) {
    std::ostringstream response;
    response << "HTTP/1.1 " << code << " Error\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "\r\n";
    response << "{\"error\":\"" << message << "\"}";
    return response.str();
}

// Global API instance
PaperCrawlerAPI* g_api = nullptr;

// Helper function to escape JSON strings
std::string escapeJsonString(const std::string& str) {
    std::ostringstream escaped;
    for (char c : str) {
        switch (c) {
            case '"':  escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (c < '\x20') {
                    escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    escaped << c;
                }
        }
    }
    return escaped.str();
}

std::string searchPapers(const std::string& keyword, int maxResults) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API not initialized");
    }

    try {
        SearchRequest request;
        request.keyword = keyword;
        request.maxResults = maxResults;

        SearchResult result = g_api->search(request);

        std::ostringstream json;
        json << "{\n";
        json << "  \"papers\": [\n";
        for (size_t i = 0; i < result.papers.size(); ++i) {
            const auto& paper = result.papers[i];
            json << "    {\n";
            json << "      \"id\": " << paper.getId() << ",\n";
            json << "      \"title\": \"" << escapeJsonString(paper.getTitle()) << "\",\n";
            json << "      \"journal\": \"" << escapeJsonString(paper.getJournalShort()) << "\",\n";
            json << "      \"year\": \"" << paper.getYear() << "\",\n";
            json << "      \"level\": \"" << paper.getLevel() << "\"\n";
            json << "    }" << (i < result.papers.size() - 1 ? ",\n" : "\n");
        }
        json << "  ],\n";
        json << "  \"total\": " << result.totalCount << ",\n";
        json << "  \"keyword\": \"" << escapeJsonString(result.keyword) << "\",\n";
        json << "  \"duration\": " << result.durationSeconds << "\n";
        json << "}";
        return json.str();
    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(500, "Database error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Error searching papers: " << e.what() << std::endl;
        return buildErrorResponse(500, std::string(e.what()));
    }
}

std::string getStatistics() {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API not initialized");
    }

    try {
        Statistics stats = g_api->getStatistics();

        std::ostringstream json;
        json << "{\n";
        json << "  \"totalPapers\": " << stats.totalPapers << ",\n";
        json << "  \"totalJournals\": " << stats.totalJournals << ",\n";
        json << "  \"topTierPapers\": " << stats.topTierPapers << ",\n";
        json << "  \"papersLastYear\": " << stats.papersLastYear << ",\n";
        json << "  \"mostActiveJournal\": \"" << escapeJsonString(stats.mostActiveJournal) << "\"\n";
        json << "}";
        return json.str();
    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(500, "Database error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Error getting statistics: " << e.what() << std::endl;
        return buildErrorResponse(500, std::string(e.what()));
    }
}

std::string exportToCSV(const std::string& keyword) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API not initialized");
    }

    try {
        std::vector<Paper> papers = g_api->getPapers(keyword, 0, 100);
        return g_api->exportToCSV(papers);
    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(500, "Database error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Error exporting to CSV: " << e.what() << std::endl;
        return buildErrorResponse(500, std::string(e.what()));
    }
}

std::string exportToJSON(const std::string& keyword) {
    if (!g_api || !g_api->isInitialized()) {
        return buildErrorResponse(500, "API not initialized");
    }

    try {
        std::vector<Paper> papers = g_api->getPapers(keyword, 0, 100);
        return g_api->exportToJSON(papers);
    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
        return buildErrorResponse(500, "Database error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Error exporting to JSON: " << e.what() << std::endl;
        return buildErrorResponse(500, std::string(e.what()));
    }
}

std::string handleRequest(const std::string& path, const std::string& method) {
    std::cout << "Request: " << method << " " << path << std::endl;

    if (path == "/health" || path == "/") {
        std::string apiStatus = (g_api && g_api->isInitialized()) ? "connected" : "disconnected";
        return buildResponse("{"
            "\"status\":\"ok\","
            "\"service\":\"PaperCrawler API\","
            "\"version\":\"1.0.0\","
            "\"database\":\"" + apiStatus + "\","
            "\"timestamp\":" + std::to_string(std::time(nullptr)) +
        "}");
    }

    if (path.find("/api/search?q=") == 0) {
        std::string keyword = path.substr(path.find("q=") + 2);
        return buildResponse(searchPapers(keyword, 100));
    }

    if (path == "/api/stats/overview") {
        return buildResponse(getStatistics());
    }

    if (path == "/api/export/csv") {
        std::string csv = exportToCSV("demo");
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/csv\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Content-Disposition: attachment; filename=papers.csv\r\n";
        response << "Content-Length: " << csv.length() << "\r\n";
        response << "\r\n";
        response << csv;
        return response.str();
    }

    if (path == "/api/export/json") {
        std::string json = exportToJSON("all");
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Content-Disposition: attachment; filename=papers.json\r\n";
        response << "Content-Length: " << json.length() << "\r\n";
        response << "\r\n";
        response << json;
        return response.str();
    }

    return buildErrorResponse(404, "Not found");
}

void handleClient(SOCKET clientSocket) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::string request(buffer);

        // Parse HTTP request
        size_t pathStart = request.find(" ");
        size_t pathEnd = request.find(" ", pathStart + 1);
        std::string path = request.substr(pathStart + 1, pathEnd - pathStart - 1);

        size_t methodEnd = request.find(" ");
        std::string method = request.substr(0, methodEnd);

        // Handle OPTIONS preflight
        if (method == "OPTIONS") {
            std::ostringstream response;
            response << "HTTP/1.1 204 No Content\r\n";
            response << "Access-Control-Allow-Origin: *\r\n";
            response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
            response << "Access-Control-Allow-Headers: Content-Type\r\n";
            response << "\r\n";
            send(clientSocket, response.str().c_str(), response.str().length(), 0);
        } else {
            std::string response = handleRequest(path, method);
            send(clientSocket, response.c_str(), response.length(), 0);
            std::cout << "Response sent (" << response.length() << " bytes)" << std::endl;
        }
    }

#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  PaperCrawler REST API Server v1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Initialize PaperCrawler API
    std::cout << "Initializing PaperCrawler API..." << std::endl;
    try {
        g_api = &PaperCrawlerAPI::getInstance();
        g_api->initialize("config/config.json");
        std::cout << "✓ API initialized successfully" << std::endl;
        std::cout << "✓ Database connected" << std::endl;
    } catch (const ConfigException& e) {
        std::cerr << "✗ Configuration error: " << e.what() << std::endl;
        return 1;
    } catch (const DatabaseException& e) {
        std::cerr << "✗ Database connection error: " << e.what() << std::endl;
        std::cerr << "Please ensure MySQL is running and config/config.json is correct" << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "✗ Initialization error: " << e.what() << std::endl;
        return 1;
    }
    std::cout << std::endl;

    std::cout << "Server starting on port 8080..." << std::endl;
    std::cout << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  GET /health" << std::endl;
    std::cout << "  GET /api/search?q=keyword" << std::endl;
    std::cout << "  GET /api/stats/overview" << std::endl;
    std::cout << "  GET /api/export/csv" << std::endl;
    std::cout << "  GET /api/export/json" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    std::cout << std::endl;

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

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        return 1;
    }

    std::cout << "Server is running on http://localhost:8080" << std::endl;
    std::cout << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSocket != INVALID_SOCKET) {
            std::cout << "\nNew connection accepted" << std::endl;
            handleClient(clientSocket);
        }
    }

    // Cleanup
    if (g_api) {
        std::cout << std::endl << "Shutting down API..." << std::endl;
        g_api->shutdown();
    }

#ifdef _WIN32
    closesocket(serverSocket);
    WSACleanup();
#else
    close(serverSocket);
#endif

    return 0;
}
