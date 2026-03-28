#include "features/infrastructure/ResponseHandlerModule.hpp"
#include "features/infrastructure/ResponseQueueModule.hpp"
#include <sstream>
#include <thread>
#include <chrono>

namespace PaperCrawler {

ResponseHandlerModule::ResponseHandlerModule()
    : running_(false) {}

ResponseHandlerModule::~ResponseHandlerModule() {
    stop();
}

bool ResponseHandlerModule::initialize() {
    std::cout << "ResponseHandlerModule initialized" << std::endl;
    return true;
}

bool ResponseHandlerModule::start() {
    running_ = true;

    // 启动工作线程
    size_t numWorkers = 4;
    for (size_t i = 0; i < numWorkers; ++i) {
        workerThreads_.emplace_back(&ResponseHandlerModule::responseWorkerLoop, this);
    }

    std::cout << "ResponseHandlerModule started with " << numWorkers << " workers" << std::endl;
    return true;
}

bool ResponseHandlerModule::stop() {
    running_ = false;

    // 等待所有工作线程结束
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads_.clear();

    std::cout << "ResponseHandlerModule stopped" << std::endl;
    return true;
}

void ResponseHandlerModule::cleanup() {
    stop();
}

void ResponseHandlerModule::responseWorkerLoop() {
    auto& responseQueue = ResponseQueueModule::getInstance();

    while (running_) {
        // 1. 从返回队列获取响应
        ResponseQueueItem item;
        if (!responseQueue.tryDequeue(item, std::chrono::milliseconds(100))) {
            continue;
        }

        try {
            // 2. 格式化为 HTTP 响应
            std::string httpResponse = formatHttpResponse(item.response, item.connectionId);

            // 3. 发送给客户端
            sendToClient(item.connectionId, httpResponse);

        } catch (const std::exception& e) {
            // 4. 错误处理
            std::string errorResponse = formatErrorResponse(e.what(), 500);
            // 尝试发送错误响应
            sendToClient(item.connectionId, errorResponse);
        }
    }
}

std::string ResponseHandlerModule::formatHttpResponse(const MessageResponse& moduleResponse,
                                                       const std::string& connectionId) {
    std::ostringstream oss;

    int statusCode = moduleResponse.success ? 200 : 500;
    std::string body;

    try {
        if (moduleResponse.data.has_value()) {
            body = std::any_cast<std::string>(moduleResponse.data);
        } else {
            body = "{}";
        }
    } catch (...) {
        body = "{}";
    }

    // 构建HTTP响应
    oss << "HTTP/1.1 " << statusCode << " " << getStatusText(statusCode) << "\r\n";
    oss << buildHttpHeaders(statusCode, body.size(), "application/json");
    oss << "\r\n";
    oss << body;

    return oss.str();
}

bool ResponseHandlerModule::sendToClient(const std::string& connectionId,
                                          const std::string& responseData) {
    // TODO: 实际发送到客户端（通过ConnectionManager）
    std::cout << "[ResponseHandler] Sending to " << connectionId
              << " (" << responseData.size() << " bytes)" << std::endl;
    return true;
}

std::string ResponseHandlerModule::formatErrorResponse(const std::string& errorMessage, int statusCode) {
    std::ostringstream oss;

    std::string body = R"({"success":false,"error":")" + errorMessage + R"("})";

    oss << "HTTP/1.1 " << statusCode << " " << getStatusText(statusCode) << "\r\n";
    oss << buildHttpHeaders(statusCode, body.size(), "application/json");
    oss << "\r\n";
    oss << body;

    return oss.str();
}

std::string ResponseHandlerModule::buildHttpHeaders(int statusCode, size_t contentLength,
                                                        const std::string& contentType) {
    std::ostringstream oss;

    oss << "Content-Type: " << contentType << "; charset=utf-8\r\n";
    oss << "Content-Length: " << contentLength << "\r\n";
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Connection: close\r\n";

    return oss.str();
}

std::string ResponseHandlerModule::getStatusText(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}

// ============================================================================
// 辅助函数（从 simple_api_server.cpp 提取）
// ============================================================================

/**
 * @brief 构建简单JSON响应
 * @param data 键值对数据
 * @param statusCode HTTP状态码
 * @return 完整HTTP响应字符串
 */
std::string ResponseHandlerModule::buildJsonResponse(const std::map<std::string, std::string>& data,
                                                    int statusCode) {
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

/**
 * @brief 构建论文列表JSON响应
 * @param papersJson 论文JSON数组
 * @param total 总数
 * @param page 当前页
 * @param pageSize 每页大小
 * @return 完整HTTP响应字符串
 */
std::string ResponseHandlerModule::buildPapersJsonResponse(const std::string& papersJson,
                                                          int total, int page, int pageSize) {
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

/**
 * @brief 构建统计信息JSON响应
 * @param stats 统计数据键值对
 * @return 完整HTTP响应字符串
 */
std::string ResponseHandlerModule::buildStatsJsonResponse(const std::map<std::string, std::string>& stats) {
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

} // namespace PaperCrawler
