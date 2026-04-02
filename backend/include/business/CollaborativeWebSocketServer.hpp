#pragma once

#include "network/WebSocketModule.hpp"
#include "business/CollaborativeWritingEnhanced.hpp"
#include "data/IDatabase.hpp"
#include <memory>

namespace PaperCrawler {

/**
 * @brief WebSocket协作服务器
 *
 * 集成CollaborationSessionManager、RealTimeCollaborationEditor和AIWritingAssistant
 * 提供完整的WebSocket实时协作功能
 *
 * 支持的WebSocket消息类型：
 * - join_document: 加入文档协作
 * - operation: 应用OT操作
 * - cursor_update: 更新光标位置
 * - request_suggestion: 请求AI写作建议
 * - heartbeat: 心跳检测
 *
 * 使用示例：
 * ```cpp
 * CollaborativeWebSocketServer server(database, websocketModule);
 * server.start();
 * ```
 */
class CollaborativeWebSocketServer {
public:
    CollaborativeWebSocketServer(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<WebSocketModule> websocketModule);

    ~CollaborativeWebSocketServer();

    /**
     * @brief 启动WebSocket服务器
     * @return true 启动成功
     */
    bool start();

    /**
     * @brief 停止WebSocket服务器
     */
    void stop();

    /**
     * @brief 获取会话管理器
     */
    CollaborationSessionManager& getSessionManager() {
        return sessionManager_;
    }

    /**
     * @brief 获取协作编辑器
     */
    RealTimeCollaborationEditor& getEditor() {
        return editor_;
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
    void setupWebSocketHandlers();

    /**
     * @brief 处理WebSocket连接打开
     */
    void handleConnectionOpen(const WebSocketConnection& connection);

    /**
     * @brief 处理WebSocket连接关闭
     */
    void handleConnectionClose(const WebSocketConnection& connection);

    /**
     * @brief 处理WebSocket消息
     */
    void handleWebSocketMessage(const WebSocketMessage& message);

    /**
     * @brief 处理加入文档请求
     */
    void handleJoinDocument(const WebSocketMessage& message);

    /**
     * @brief 处理文档操作
     */
    void handleDocumentOperation(const WebSocketMessage& message);

    /**
     * @brief 处理光标更新
     */
    void handleCursorUpdate(const WebSocketMessage& message);

    /**
     * @brief 处理AI建议请求
     */
    void handleSuggestionRequest(const WebSocketMessage& message);

    /**
     * @brief 处理心跳
     */
    void handleHeartbeat(const WebSocketMessage& message);

    /**
     * @brief 发送错误消息
     */
    void sendError(const std::string& connectionId, const std::string& errorMessage);

    /**
     * @brief 提取消息类型
     */
    std::string extractMessageType(const std::string& message);

    /**
     * @brief 转义JSON字符串
     */
    std::string escapeJson(const std::string& str);

    /**
     * @brief 获取当前时间戳
     */
    std::string getCurrentTimestamp();
};

} // namespace PaperCrawler
