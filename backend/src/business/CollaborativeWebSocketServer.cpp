#include "business/CollaborativeWritingEnhanced.hpp"
#include "network/WebSocketModule.hpp"
#include "modules/LoggingModule.hpp"
#include "data/IDatabase.hpp"
#include <sstream>
#include <functional>
#include <memory>

namespace PaperCrawler {

/**
 * @brief WebSocket协作服务器
 *
 * 集成CollaborationSessionManager、RealTimeCollaborationEditor和AIWritingAssistant
 * 提供完整的WebSocket实时协作功能
 */
class CollaborativeWebSocketServer {
public:
    CollaborativeWebSocketServer(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<WebSocketModule> websocketModule)
        : database_(database)
        , websocketModule_(websocketModule)
        , sessionManager_(websocketModule)
        , editor_(database)
        , aiAssistant_(nullptr) {  // 后续从ServiceContainer解析AIWritingAssistant

        setupWebSocketHandlers();
    }

    ~CollaborativeWebSocketServer() = default;

    /**
     * @brief 启动WebSocket服务器
     */
    bool start() {
        try {
            auto logging = Services::resolve<LoggingModule>();
            if (logging) {
                logging->info("Starting Collaborative WebSocket Server...");
            }

            // 设置WebSocket处理器
            websocketModule_->setMessageHandler([this](const WebSocketMessage& message) {
                handleWebSocketMessage(message);
            });

            websocketModule_->setOpenHandler([this](const WebSocketConnection& connection) {
                handleConnectionOpen(connection);
            });

            websocketModule_->setCloseHandler([this](const WebSocketConnection& connection) {
                handleConnectionClose(connection);
            });

            if (logging) {
                logging->info("Collaborative WebSocket Server started successfully");
            }

            return true;

        } catch (const std::exception& e) {
            auto logging = Services::resolve<LoggingModule>();
            if (logging) {
                logging->error("Failed to start WebSocket server: " + std::string(e.what()));
            }
            return false;
        }
    }

    /**
     * @brief 停止WebSocket服务器
     */
    void stop() {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->info("Stopping Collaborative WebSocket Server...");
        }

        // 清理资源（关闭WebSocket连接和释放会话）
    }

private:
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<WebSocketModule> websocketModule_;
    CollaborationSessionManager sessionManager_;
    RealTimeCollaborationEditor editor_;
    std::unique_ptr<AIWritingAssistant> aiAssistant_;

    /**
     * @brief 设置WebSocket消息处理器
     */
    void setupWebSocketHandlers() {
        // 处理器已在start()中设置
    }

    /**
     * @brief 处理WebSocket连接打开
     */
    void handleConnectionOpen(const WebSocketConnection& connection) {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->info("WebSocket connection opened: " + connection.connectionId);
        }

        // 发送欢迎消息
        std::ostringstream welcomeMsg;
        welcomeMsg << "{";
        welcomeMsg << "\"type\":\"connected\",";
        welcomeMsg << "\"connectionId\":\"" << connection.connectionId << "\",";
        welcomeMsg << "\"timestamp\":\"" << getCurrentTimestamp() << "\"";
        welcomeMsg << "}";

        websocketModule_->send(connection.connectionId, welcomeMsg.str());
    }

    /**
     * @brief 处理WebSocket连接关闭
     */
    void handleConnectionClose(const WebSocketConnection& connection) {
        auto logging = Services::resolve<LoggingModule>();
        if (logging) {
            logging->info("WebSocket connection closed: " + connection.connectionId);
        }

        // 从会话管理器中移除
        sessionManager_.removeConnection(connection.connectionId);
    }

    /**
     * @brief 处理WebSocket消息
     */
    void handleWebSocketMessage(const WebSocketMessage& message) {
        try {
            auto logging = Services::resolve<LoggingModule>();
            if (logging) {
                logging->debug("Received WebSocket message from: " + message.connectionId);
            }

            // 解析消息类型
            std::string messageType = extractMessageType(message.data);

            if (messageType == "join_document") {
                handleJoinDocument(message);
            } else if (messageType == "operation") {
                handleDocumentOperation(message);
            } else if (messageType == "cursor_update") {
                handleCursorUpdate(message);
            } else if (messageType == "request_suggestion") {
                handleSuggestionRequest(message);
            } else if (messageType == "heartbeat") {
                handleHeartbeat(message);
            } else {
                if (logging) {
                    logging->warn("Unknown message type: " + messageType);
                }
            }

        } catch (const std::exception& e) {
            auto logging = Services::resolve<LoggingModule>();
            if (logging) {
                logging->error("Failed to handle WebSocket message: " + std::string(e.what()));
            }
        }
    }

    /**
     * @brief 处理加入文档请求
     */
    void handleJoinDocument(const WebSocketMessage& message) {
        // 解析JSON获取documentId和userId（当前使用示例数据）
        int documentId = 1;  // 示例
        int userId = 1;      // 示例

        // 添加到会话管理器
        sessionManager_.addConnection(documentId, userId, message.connectionId);

        // 获取当前文档内容
        std::string content = editor_.getDocumentContent(documentId);

        // 发送文档内容给新连接的客户端
        std::ostringstream response;
        response << "{";
        response << "\"type\":\"document_init\",";
        response << "\"documentId\":" << documentId << ",";
        response << "\"content\":\"" << escapeJson(content) << "\",";
        response << "\"timestamp\":\"" << getCurrentTimestamp() << "\"";
        response << "}";

        websocketModule_->send(message.connectionId, response.str());
    }

    /**
     * @brief 处理文档操作
     */
    void handleDocumentOperation(const WebSocketMessage& message) {
        // 解析操作数据（当前使用示例数据）
        OTOperation operation;
        operation.type = OTOperationType::INSERT;
        operation.position = 0;
        operation.content = "test";
        operation.clientId = 1;
        operation.timestamp = 0;

        int documentId = 1;

        // 应用操作
        std::string newContent = editor_.applyOperation(documentId, operation);

        // 广播操作到所有连接的客户端
        std::ostringstream broadcastMsg;
        broadcastMsg << "{";
        broadcastMsg << "\"type\":\"operation_applied\",";
        broadcastMsg << "\"documentId\":" << documentId << ",";
        broadcastMsg << "\"operation\":" << operation.toJSON() << ",";
        broadcastMsg << "\"newContentLength\":" << newContent.length();
        broadcastMsg << "}";

        sessionManager_.broadcastOperation(documentId, broadcastMsg.str());
    }

    /**
     * @brief 处理光标更新
     */
    void handleCursorUpdate(const WebSocketMessage& message) {
        // 解析光标位置（当前使用示例数据）
        int documentId = 1;
        int userId = 1;
        int position = 0;

        sessionManager_.updateCursor(documentId, userId, position);
    }

    /**
     * @brief 处理AI建议请求
     */
    void handleSuggestionRequest(const WebSocketMessage& message) {
        if (!aiAssistant_) {
            sendError(message.connectionId, "AI assistant not available");
            return;
        }

        // 解析请求参数（当前使用示例数据）
        int documentId = 1;
        std::string content = "sample content";
        int position = 0;
        std::string suggestionType = "grammar";

        // 生成建议
        auto suggestion = aiAssistant_->generateSuggestion(
            documentId, content, position, suggestionType
        );

        // 发送建议给客户端
        std::ostringstream response;
        response << "{";
        response << "\"type\":\"suggestion\",";
        response << "\"documentId\":" << suggestion.documentId << ",";
        response << "\"suggestionType\":\"" << suggestion.suggestionType << "\",";
        response << "\"originalText\":\"" << escapeJson(suggestion.originalText) << "\",";
        response << "\"suggestedText\":\"" << escapeJson(suggestion.suggestedText) << "\",";
        response << "\"confidenceScore\":" << suggestion.confidenceScore << ",";
        response << "\"explanation\":\"" << escapeJson(suggestion.explanation) << "\",";
        response << "\"status\":\"" << suggestion.status << "\"";
        response << "}";

        websocketModule_->send(message.connectionId, response.str());
    }

    /**
     * @brief 处理心跳
     */
    void handleHeartbeat(const WebSocketMessage& message) {
        std::ostringstream pong;
        pong << "{\"type\":\"pong\",\"timestamp\":\"" << getCurrentTimestamp() << "\"}";
        websocketModule_->send(message.connectionId, pong.str());
    }

    /**
     * @brief 发送错误消息
     */
    void sendError(const std::string& connectionId, const std::string& errorMessage) {
        std::ostringstream error;
        error << "{";
        error << "\"type\":\"error\",";
        error << "\"message\":\"" << escapeJson(errorMessage) << "\",";
        error << "\"timestamp\":\"" << getCurrentTimestamp() << "\"";
        error << "}";

        websocketModule_->send(connectionId, error.str());
    }

    /**
     * @brief 提取消息类型
     */
    std::string extractMessageType(const std::string& message) {
        // 简化实现：假设消息格式为 {"type":"message_type", ...}
        // 后续迁移至JsonUtils统一解析
        size_t typePos = message.find("\"type\":");
        if (typePos != std::string::npos) {
            size_t start = message.find("\"", typePos + 7);
            if (start != std::string::npos) {
                size_t end = message.find("\"", start + 1);
                if (end != std::string::npos) {
                    return message.substr(start + 1, end - start - 1);
                }
            }
        }
        return "";
    }

    /**
     * @brief 转义JSON字符串
     */
    std::string escapeJson(const std::string& str) {
        std::string escaped;
        escaped.reserve(str.length() * 2);

        for (char c : str) {
            switch (c) {
                case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (c < 32) {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", c);
                        escaped += buf;
                    } else {
                        escaped += c;
                    }
                    break;
            }
        }

        return escaped;
    }

    /**
     * @brief 获取当前时间戳
     */
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};

} // namespace PaperCrawler
