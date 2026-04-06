#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <functional>

namespace PaperCrawler {

/**
 * @brief OT操作类型
 */
enum class OTOperationType {
    INSERT,  // 插入文本
    DELETE,  // 删除文本
    RETAIN,  // 保留（无操作）
    FORMAT   // 格式化
};

/**
 * @brief OT操作
 */
struct OTOperation {
    OTOperationType type;
    int position;
    int length;  // 对于DELETE操作
    std::string content;  // 对于INSERT操作
    int clientId;  // 客户端ID
    int timestamp;

    std::string toJSON() const;
};

/**
 * @brief 文档信息
 */
struct CollaborativeDocument {
    int id;
    std::string title;
    std::string content;
    std::string documentType;
    int ownerId;
    std::string status;
    int wordCount;
    int lastModifiedBy;
    std::string createdAt;
    std::string updatedAt;
};

/**
 * @brief AI写作建议
 */
struct WritingSuggestion {
    int id;
    int documentId;
    int userId;
    std::string suggestionType;
    int positionStart;
    int positionEnd;
    std::string originalText;
    std::string suggestedText;
    float confidenceScore;
    std::string explanation;
    std::string status;
};

/**
 * @brief 协作会话信息
 */
struct CollaborationSession {
    int id;
    int documentId;
    int userId;
    std::string socketId;
    int cursorPosition;
    bool isActive;
    std::string lastHeartbeat;
};

/**
 * @brief 实时AI协作写作模块
 *
 * 功能：
 * 1. 多人实时协作编辑
 * 2. OT算法（冲突解决）
 * 3. 实时AI写作辅导
 * 4. 版本控制
 * 5. 协作评论和批注
 *
 * ROI: 9.0/10
 * 开发周期: 6-8周
 */
class CollaborativeWritingModule : public BusinessModuleBase {
public:
    CollaborativeWritingModule();
    explicit CollaborativeWritingModule(std::shared_ptr<IDatabase> database);
    ~CollaborativeWritingModule() override;

    std::string getName() const override { return "CollaborativeWriting"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Real-time AI collaborative writing platform";
    }

    // ========================================================================
    // 1. 文档管理
    // ========================================================================

    /**
     * @brief 创建文档
     * POST /api/writing/documents
     */
    std::optional<CollaborativeDocument> createDocument(
        int userId,
        const std::string& title,
        const std::string& documentType,
        int templateId = 0
    );

    /**
     * @brief 获取文档
     * GET /api/writing/documents/:id
     */
    std::optional<CollaborativeDocument> getDocument(int documentId);

    /**
     * @brief 更新文档
     * PUT /api/writing/documents/:id
     */
    bool updateDocument(int documentId, const std::string& content,
                        const std::string& title, const std::string& status);

    /**
     * @brief 删除文档
     * DELETE /api/writing/documents/:id
     */
    bool deleteDocument(int documentId);

    /**
     * @brief 获取用户的文档列表
     * GET /api/writing/documents
     */
    std::vector<CollaborativeDocument> getDocuments(int userId, int page = 1, int limit = 20);

    // ========================================================================
    // 2. OT算法和实时协作
    // ========================================================================

    /**
     * @brief 应用OT操作
     * POST /api/writing/documents/:id/operations
     */
    std::string applyOperation(int documentId, const OTOperation& operation);

    /**
     * @brief 转换操作（客户端到服务器）
     */
    OTOperation transformOperation(const OTOperation& clientOp, const OTOperation& serverOp);

    /**
     * @brief 批量应用操作
     */
    std::string applyOperations(int documentId, const std::vector<OTOperation>& operations);

    // ========================================================================
    // 3. WebSocket实时协作
    // ========================================================================

    /**
     * @brief 处理WebSocket连接
     */
    void handleWebSocketConnection(int documentId, int userId, const std::string& socketId);

    /**
     * @brief 处理WebSocket断开
     */
    void handleWebSocketDisconnection(const std::string& socketId);

    /**
     * @brief 广播操作到所有连接的客户端
     */
    void broadcastOperation(int documentId, const OTOperation& operation);

    /**
     * @brief 发送光标位置更新
     */
    void updateCursorPosition(int documentId, int userId, int position);

    // ========================================================================
    // 4. 实时AI写作辅导
    // ========================================================================

    /**
     * @brief 获取AI写作建议
     * GET /api/writing/documents/:id/suggestions
     */
    std::vector<WritingSuggestion> getWritingSuggestions(int documentId);

    /**
     * @brief 生成AI写作建议
     * POST /api/writing/documents/:id/suggestions/generate
     */
    WritingSuggestion generateSuggestion(
        int documentId,
        int userId,
        const std::string& suggestionType,
        int positionStart,
        int positionEnd
    );

    /**
     * @brief 接受建议
     * PUT /api/writing/suggestions/:id/accept
     */
    bool acceptSuggestion(int suggestionId);

    /**
     * @brief 拒绝建议
     * PUT /api/writing/suggestions/:id/reject
     */
    bool rejectSuggestion(int suggestionId);

    // ========================================================================
    // 5. 版本控制
    // ========================================================================

    /**
     * @brief 创建版本
     * POST /api/writing/documents/:id/versions
     */
    bool createVersion(int documentId, int userId, const std::string& summary = "");

    /**
     * @brief 获取版本历史
     * GET /api/writing/documents/:id/versions
     */
    std::vector<std::map<std::string, std::string>> getVersions(int documentId);

    /**
     * @brief 恢复到特定版本
     * POST /api/writing/documents/:id/versions/:versionId/restore
     */
    bool restoreToVersion(int documentId, int versionId, int userId);

    // ========================================================================
    // 6. 协作评论
    // ========================================================================

    /**
     * @brief 添加评论
     * POST /api/writing/documents/:id/comments
     */
    int addComment(
        int documentId,
        int userId,
        const std::string& content,
        int positionStart,
        int positionEnd,
        int parentId = 0
    );

    /**
     * @brief 获取评论列表
     * GET /api/writing/documents/:id/comments
     */
    std::vector<std::map<std::string, std::string>> getComments(int documentId);

    /**
     * @brief 解决评论
     * PUT /api/writing/comments/:id/resolve
     */
    bool resolveComment(int commentId);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<IDatabase> database_;

    void registerRoutes() override;

    // OT算法辅助方法
    OTOperation transformInsertAgainstInsert(const OTOperation& clientOp, const OTOperation& serverOp);
    OTOperation transformInsertAgainstDelete(const OTOperation& clientOp, const OTOperation& serverOp);
    OTOperation transformDeleteAgainstInsert(const OTOperation& clientOp, const OTOperation& serverOp);
    OTOperation transformDeleteAgainstDelete(const OTOperation& clientOp, const OTOperation& serverOp);

    // AI辅助方法
    std::string buildWritingPrompt(const CollaborativeDocument& doc, int positionStart, int positionEnd);
    std::string applySuggestionToContent(const std::string& content, const WritingSuggestion& suggestion);
};

} // namespace PaperCrawler
