#pragma once

#include "modules/WebSocketModule.hpp"
#include "business/CollaborativeWritingModule.hpp"
#include <string>
#include <map>
#include <memory>
#include <functional>
#include <mutex>

namespace PaperCrawler {

/**
 * @brief WebSocket连接信息
 */
struct WebSocketConnection {
    std::string socketId;
    int userId;
    int documentId;
    bool isActive;
    std::string lastHeartbeat;
};

/**
 * @brief 协作会话管理器
 *
 * 管理文档的WebSocket连接、广播、心跳检测
 */
class CollaborationSessionManager {
public:
    CollaborationSessionManager(std::shared_ptr<WebSocketModule> websocketModule);
    ~CollaborationSessionManager() = default;

    /**
     * @brief 添加连接
     */
    void addConnection(int documentId, int userId, const std::string& socketId);

    /**
     * @brief 移除连接
     */
    void removeConnection(const std::string& socketId);

    /**
     * @brief 广播操作到文档的所有连接
     */
    void broadcastOperation(int documentId, const std::string& message);

    /**
     * @brief 发送消息给特定用户
     */
    void sendToUser(int userId, const std::string& message);

    /**
     * @brief 更新用户光标位置
     */
    void updateCursor(int documentId, int userId, int position);

    /**
     * @brief 广播光标位置
     */
    void broadcastCursorUpdate(int documentId, int userId, int position);

    /**
     * @brief 心跳检测
     */
    void heartbeatCheck();

    /**
     * @brief 获取活跃连接数
     */
    int getActiveConnections(int documentId);

private:
    std::shared_ptr<WebSocketModule> websocketModule_;
    std::map<std::string, WebSocketConnection> connections_;
    std::map<int, std::vector<std::string>> documentConnections_;  // documentId -> socketIds
    std::map<int, std::map<int, int>> cursorPositions_;  // documentId -> userId -> position
    std::mutex mutex_;

    std::string getCurrentTimestamp();
};

/**
 * @brief 实时协作编辑器
 *
 * 管理文档的实时编辑、操作转换、冲突解决
 */
class RealTimeCollaborationEditor {
public:
    RealTimeCollaborationEditor(std::shared_ptr<IDatabase> database);

    /**
     * @brief 应用操作
     */
    std::string applyOperation(int documentId, const OTOperation& operation);

    /**
     * @brief 转换操作（OT算法）
     */
    OTOperation transformClientOperation(const OTOperation& clientOp, int documentId);

    /**
     * @brief 获取文档当前内容
     */
    std::string getDocumentContent(int documentId);

    /**
     * @brief 保存文档快照
     */
    bool saveSnapshot(int documentId, const std::string& content, int userId);

private:
    std::shared_ptr<IDatabase> database_;
    std::map<int, std::string> documentContents_;
    std::map<int, std::vector<OTOperation>> operationHistory_;
    std::mutex mutex_;
};

/**
 * @brief AI写作助手
 *
 * 提供实时AI写作建议、语法检查、风格改进
 */
class AIWritingAssistant {
public:
    AIWritingAssistant(std::shared_ptr<UnifiedAIWorkflow> aiWorkflow);

    /**
     * @brief 生成写作建议
     */
    WritingSuggestion generateSuggestion(
        int documentId,
        const std::string& content,
        int position,
        const std::string& suggestionType
    );

    /**
     * @brief 语法检查
     */
    std::vector<std::map<std::string, std::string>> checkGrammar(const std::string& text);

    /**
     * @brief 风格改进建议
     */
    std::vector<std::string> improveStyle(const std::string& text, const std::string& style);

    /**
     * @brief 引用推荐
     */
    std::vector<std::map<std::string, std::string>> recommendCitations(
        const std::string& context,
        int maxCount = 5
    );

    /**
     * @brief 自动完成建议
     */
    std::vector<std::string> suggestAutocompletion(
        const std::string& prefix,
        const std::string& documentContext
    );

private:
    std::shared_ptr<UnifiedAIWorkflow> aiWorkflow_;
};

} // namespace PaperCrawler
