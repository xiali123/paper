#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>

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

// Demo papers data
struct Paper {
    int id;
    std::string title;
    std::string journal;
    std::string year;
    std::string level;
};

std::vector<Paper> getDemoPapers(const std::string& keyword, int count) {
    std::vector<Paper> papers;
    for (int i = 0; i < count; ++i) {
        Paper p;
        p.id = i + 1;
        p.title = "Paper " + std::to_string(i + 1) + ": Deep Learning for " + keyword;
        p.journal = "CVPR " + std::to_string(2024 - i % 5);
        p.year = std::to_string(2024 - i % 5);
        p.level = (i % 3 == 0) ? "A" : (i % 3 == 1) ? "B" : "C";
        papers.push_back(p);
    }
    return papers;
}

std::string searchPapers(const std::string& keyword, int maxResults) {
    auto papers = getDemoPapers(keyword, std::min(10, maxResults));

    std::ostringstream json;
    json << "{\n";
    json << "  \"papers\": [\n";
    for (size_t i = 0; i < papers.size(); ++i) {
        json << "    {\n";
        json << "      \"id\": " << papers[i].id << ",\n";
        json << "      \"title\": \"" << papers[i].title << "\",\n";
        json << "      \"journal\": \"" << papers[i].journal << "\",\n";
        json << "      \"year\": \"" << papers[i].year << "\",\n";
        json << "      \"level\": \"" << papers[i].level << "\"\n";
        json << "    }" << (i < papers.size() - 1 ? ",\n" : "\n");
    }
    json << "  ],\n";
    json << "  \"total\": 50,\n";
    json << "  \"keyword\": \"" << keyword << "\",\n";
    json << "  \"duration\": 1.5\n";
    json << "}";
    return json.str();
}

std::string getStatistics() {
    std::ostringstream json;
    json << "{\n";
    json << "  \"totalPapers\": 1000,\n";
    json << "  \"totalJournals\": 50,\n";
    json << "  \"topTierPapers\": 300,\n";
    json << "  \"papersLastYear\": 150,\n";
    json << "  \"mostActiveJournal\": \"CVPR\"\n";
    json << "}";
    return json.str();
}

std::string exportToCSV(const std::string& keyword) {
    auto papers = getDemoPapers(keyword, 10);

    std::ostringstream csv;
    csv << "ID,Title,Journal,Year,Level\n";
    for (const auto& p : papers) {
        csv << p.id << ",\"" << p.title << "\",\"" << p.journal << "\",\"" << p.year << "\",\"" << p.level << "\"\n";
    }
    return csv.str();
}

std::string handleRequest(const std::string& path, const std::string& method) {
    std::cout << "Request: " << method << " " << path << std::endl;

    if (path == "/health" || path == "/") {
        return buildResponse("{"
            "\"status\":\"ok\","
            "\"service\":\"PaperCrawler API\","
            "\"version\":\"1.0.0\","
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
        auto papers = getDemoPapers("demo", 10);
        std::ostringstream json;
        json << "[\n";
        for (size_t i = 0; i < papers.size(); ++i) {
            json << "  {\n";
            json << "    \"id\":" << papers[i].id << ",\n";
            json << "    \"title\":\"" << papers[i].title << "\",\n";
            json << "    \"journal\":\"" << papers[i].journal << "\",\n";
            json << "    \"year\":\"" << papers[i].year << "\",\n";
            json << "    \"level\":\"" << papers[i].level << "\"\n";
            json << "  }" << (i < papers.size() - 1 ? ",\n" : "\n");
        }
        json << "]\n";

        std::string jsonStr = json.str();
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Content-Disposition: attachment; filename=papers.json\r\n";
        response << "Content-Length: " << jsonStr.length() << "\r\n";
        response << "\r\n";
        response << jsonStr;
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

#ifdef _WIN32
    closesocket(serverSocket);
    WSACleanup();
#else
    close(serverSocket);
#endif

    return 0;
}
