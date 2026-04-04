#include "business/CollaborativeWritingModule.hpp"
#include "core/Router.hpp"
#include "core/EventDrivenIntegration.hpp"
#include "business/UnifiedAIWorkflow.hpp"
#include "modules/LoggingModule.hpp"
#include <sstream>
#include <algorithm>

namespace PaperCrawler {

class CollaborativeWritingModule::Impl {
public:
    std::shared_ptr<UnifiedAIWorkflow> aiWorkflow_;

    // 文档状态缓存（用于OT算法）
    std::map<int, std::string> documentContents_;
    std::map<int, std::vector<OTOperation>> pendingOperations_;

    // WebSocket连接管理
    std::map<int, std::vector<std::string>> documentSessions_;  // documentId -> socketIds
};

CollaborativeWritingModule::CollaborativeWritingModule(std::shared_ptr<IDatabase> database)
    : database_(database), impl_(std::make_unique<Impl>()) {

    // 解析依赖
    impl_->aiWorkflow_ = Services::resolve<UnifiedAIWorkflow>();
    websocketModule_ = Services::resolve<WebSocketModule>();
}

CollaborativeWritingModule::~CollaborativeWritingModule() = default;

void CollaborativeWritingModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    // 1. 文档管理
    router.post(prefix + "/documents", [this](const HttpRequest& req) {
        return handleRequest(req); // 代理到handleRequest
    });

    router.get(prefix + "/documents/:id", [this](const HttpRequest& req) {
        return handleRequest(req);
    });

    router.put(prefix + "/documents/:id", [this](const HttpRequest& req) {
        return handleRequest(req);
    });

    // 2. OT操作
    router.post(prefix + "/documents/:id/operations", [this](const HttpRequest& req) {
        return handleRequest(req);
    });

    // 3. AI建议
    router.get(prefix + "/documents/:id/suggestions", [this](const HttpRequest& req) {
        return handleRequest(req);
    });

    router.post(prefix + "/documents/:id/suggestions/generate", [this](const HttpRequest& req) {
        return handleRequest(req);
    });

    // 4. 版本控制
    router.get(prefix + "/documents/:id/versions", [this](const HttpRequest& req) {
        return handleRequest(req);
    });

    // 5. 评论
    router.post(prefix + "/documents/:id/comments", [this](const HttpRequest& req) {
        return handleRequest(req);
    });
}

// ============================================================================
// 1. 文档管理实现
// ============================================================================

std::optional<CollaborativeDocument> CollaborativeWritingModule::createDocument(
    int userId,
    const std::string& title,
    const std::string& documentType,
    int templateId) {

    try {
        // 如果使用了模板，从模板加载初始内容
        std::string initialContent = "";
        if (templateId > 0) {
            // TODO: 从document_templates表加载模板
            initialContent = "Template content"; // 占位符
        }

        // 插入新文档
        PreparedStatement stmt(database_,
            "INSERT INTO collaborative_documents "
            "(title, content, document_type, owner_id, template_id, word_count) "
            "VALUES (?, ?, ?, ?, ?, ?)"
        );

        stmt.bind(1, title);
        stmt.bind(2, initialContent);
        stmt.bind(3, documentType);
        stmt.bind(4, userId);
        stmt.bind(5, templateId);
        stmt.bind(6, 0); // word_count

        if (stmt.execute()) {
            int documentId = stmt.executeAndReturnId();

            // 缓存文档内容
            impl_->documentContents_[documentId] = initialContent;

            // 发布事件
            EventPublisher::documentCreated(documentId, userId, title);

            // 返回创建的文档
            return getDocument(documentId);
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to create document: " + std::string(e.what()));
        }
    }

    return std::nullopt;
}

std::optional<CollaborativeDocument> CollaborativeWritingModule::getDocument(int documentId) {
    try {
        QueryBuilder queryBuilder(database_);
        queryBuilder.select()
            .from("collaborative_documents")
            .where("id", "=", documentId);

        auto rows = queryBuilder.query();

        if (!rows.empty()) {
            CollaborativeDocument doc;
            doc.id = documentId;
            doc.title = rows[0]["title"];
            doc.content = rows[0]["content"];
            doc.documentType = rows[0]["document_type"];
            doc.ownerId = std::stoi(rows[0]["owner_id"]);
            doc.status = rows[0]["status"];
            doc.wordCount = std::stoi(rows[0]["word_count"]);
            doc.lastModifiedBy = std::stoi(rows[0]["last_modified_by"]);
            doc.createdAt = rows[0]["created_at"];
            doc.updatedAt = rows[0]["updated_at"];

            // 更新缓存
            impl_->documentContents_[documentId] = doc.content;

            return doc;
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to get document: " + std::string(e.what()));
        }
    }

    return std::nullopt;
}

// ============================================================================
// 2. OT算法实现（核心）
// ============================================================================

std::string CollaborativeWritingModule::applyOperation(int documentId, const OTOperation& operation) {
    try {
        // 获取当前文档内容
        std::string content = impl_->documentContents_[documentId];

        // 应用操作
        std::string newContent;
        switch (operation.type) {
            case OTOperationType::INSERT:
                if (operation.position >= 0 && operation.position <= content.length()) {
                    newContent = content.substr(0, operation.position) +
                                  operation.content +
                                  content.substr(operation.position);
                }
                break;

            case OTOperationType::DELETE:
                if (operation.position >= 0 &&
                    operation.position + operation.length <= content.length()) {
                    newContent = content.substr(0, operation.position) +
                                  content.substr(operation.position + operation.length);
                }
                break;

            case OTOperationType::RETAIN:
                newContent = content;
                break;

            default:
                newContent = content;
                break;
        }

        // 更新文档内容和缓存
        impl_->documentContents_[documentId] = newContent;

        // 记录操作到数据库
        PreparedStatement stmt(database_,
            "INSERT INTO document_operations "
            "(document_id, user_id, operation_type, position, length, content) "
            "VALUES (?, ?, ?, ?, ?, ?)"
        );

        stmt.bind(1, documentId);
        stmt.bind(2, operation.clientId);
        stmt.bind(3, static_cast<int>(operation.type));
        stmt.bind(4, operation.position);
        stmt.bind(5, operation.length);
        stmt.bind(6, operation.content);

        stmt.execute();

        // 更新数据库中的文档
        PreparedStatement updateStmt(database_,
            "UPDATE collaborative_documents SET content = ?, word_count = ?, updated_at = NOW() WHERE id = ?"
        );

        updateStmt.bind(1, newContent);
        updateStmt.bind(2, static_cast<int>(std::count_if(newContent.begin(), newContent.end(), ::isprint)));
        updateStmt.bind(3, documentId);

        updateStmt.execute();

        // 广播操作到所有连接的客户端
        broadcastOperation(documentId, operation);

        return newContent;

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to apply operation: " + std::string(e.what()));
        }
        return "";
    }
}

OTOperation CollaborativeWritingModule::transformOperation(
    const OTOperation& clientOp,
    const OTOperation& serverOp) {

    // 基于操作类型调用相应的转换函数
    if (clientOp.type == OTOperationType::INSERT) {
        if (serverOp.type == OTOperationType::INSERT) {
            return transformInsertAgainstInsert(clientOp, serverOp);
        } else if (serverOp.type == OTOperationType::DELETE) {
            return transformInsertAgainstDelete(clientOp, serverOp);
        }
    } else if (clientOp.type == OTOperationType::DELETE) {
        if (serverOp.type == OTOperationType::INSERT) {
            return transformDeleteAgainstInsert(clientOp, serverOp);
        } else if (serverOp.type == OTOperationType::DELETE) {
            return transformDeleteAgainstDelete(clientOp, serverOp);
        }
    }

    // 默认：不转换
    return clientOp;
}

OTOperation CollaborativeWritingModule::transformInsertAgainstInsert(
    const OTOperation& clientOp,
    const OTOperation& serverOp) {

    // Operational Transformation (OT)算法
    // 如果服务器操作在客户端操作之前插入，需要调整客户端操作的位置

    OTOperation transformed = clientOp;

    if (serverOp.position <= clientOp.position) {
        // 服务器操作在客户端操作之前
        // 客户端操作位置需要向后偏移服务器操作内容的长度
        transformed.position = clientOp.position + serverOp.content.length();
    }

    return transformed;
}

OTOperation CollaborativeWritingModule::transformInsertAgainstDelete(
    const OTOperation& clientOp,
    const OTOperation& serverOp) {

    OTOperation transformed = clientOp;

    if (serverOp.position < clientOp.position) {
        // 服务器删除在客户端插入之前
        // 客户端操作位置需要向前偏移
        int offset = std::min(serverOp.length, clientOp.position - serverOp.position);
        transformed.position = clientOp.position - offset;
    } else if (serverOp.position < clientOp.position + clientOp.content.length()) {
        // 服务器删除与客户端插入重叠
        // 缩短客户端插入的内容
        int overlapStart = std::max(serverOp.position, clientOp.position);
        int overlapEnd = std::min(serverOp.position + serverOp.length,
                                clientOp.position + static_cast<int>(clientOp.content.length()));
        int overlapLength = overlapEnd - overlapStart;

        if (overlapLength > 0) {
            transformed.content = clientOp.content.substr(0, overlapStart - clientOp.position) +
                                  clientOp.content.substr(overlapEnd - clientOp.position);
        }
    }

    return transformed;
}

OTOperation CollaborativeWritingModule::transformDeleteAgainstInsert(
    const OTOperation& clientOp,
    const OTOperation& serverOp) {

    OTOperation transformed = clientOp;

    if (serverOp.position <= clientOp.position) {
        // 服务器插入在客户端删除之前
        // 客户端删除位置需要向后偏移
        transformed.position = clientOp.position + static_cast<int>(serverOp.content.length());
    }

    return transformed;
}

OTOperation CollaborativeWritingModule::transformDeleteAgainstDelete(
    const OTOperation& clientOp,
    const OTOperation& serverOp) {

    OTOperation transformed = clientOp;

    // 简化实现：如果操作重叠，保留第一个操作
    if (serverOp.position < clientOp.position + clientOp.length &&
        serverOp.position + serverOp.length > clientOp.position) {
        // 重叠：缩短客户端删除的长度
        int overlapStart = std::max(serverOp.position, clientOp.position);
        int overlapEnd = std::min(serverOp.position + serverOp.length,
                                clientOp.position + clientOp.length);
        int removedByServer = overlapEnd - overlapStart;

        transformed.length = clientOp.length - removedByServer;
        if (transformed.length <= 0) {
            // 完全被服务器操作覆盖
            transformed.type = OTOperationType::RETAIN;
            transformed.length = 0;
        }
    }

    return transformed;
}

// ============================================================================
// 3. WebSocket实时协作
// ============================================================================

void CollaborativeWritingModule::handleWebSocketConnection(
    int documentId, int userId, const std::string& socketId) {

    // 记录会话
    PreparedStatement stmt(database_,
        "INSERT INTO collaboration_sessions "
        "(document_id, user_id, socket_id, is_active) "
        "VALUES (?, ?, ?, TRUE)"
    );

    stmt.bind(1, documentId);
    stmt.bind(2, userId);
    stmt.bind(3, socketId);

    if (stmt.execute()) {
        impl_->documentSessions_[documentId].push_back(socketId);

        // 通知其他用户
        // TODO: 通过WebSocket发送用户加入通知
    }
}

void CollaborativeWritingModule::handleWebSocketDisconnection(const std::string& socketId) {
    // 更新会话状态
    PreparedStatement stmt(database_,
        "UPDATE collaboration_sessions SET is_active = FALSE WHERE socket_id = ?"
    );

    stmt.bind(1, socketId);
    stmt.execute();

    // 从内存中移除
    for (auto& [docId, sessions] : impl_->documentSessions_) {
        sessions.erase(
            std::remove(sessions.begin(), sessions.end(), socketId),
            sessions.end()
        );
    }
}

void CollaborativeWritingModule::broadcastOperation(int documentId, const OTOperation& operation) {
    // 广播操作到所有连接的客户端
    if (websocketModule_) {
        std::string message = operation.toJSON();

        for (const auto& socketId : impl_->documentSessions_[documentId]) {
            // TODO: 通过WebSocket发送消息
            // websocketModule_->send(socketId, message);
        }
    }
}

// ============================================================================
// 4. 实时AI写作辅导
// ============================================================================

WritingSuggestion CollaborativeWritingModule::generateSuggestion(
    int documentId,
    int userId,
    const std::string& suggestionType,
    int positionStart,
    int positionEnd) {

    WritingSuggestion suggestion;
    suggestion.documentId = documentId;
    suggestion.userId = userId;
    suggestion.suggestionType = suggestionType;
    suggestion.positionStart = positionStart;
    suggestion.positionEnd = positionEnd;
    suggestion.status = "pending";

    try {
        // 获取文档内容
        auto docOpt = getDocument(documentId);
        if (!docOpt.has_value()) {
            return suggestion;
        }

        auto doc = docOpt.value();

        // 提取选中的文本
        std::string selectedText = doc.content.substr(positionStart, positionEnd - positionStart);

        // 构建AI提示词
        std::string prompt = buildWritingPrompt(doc, positionStart, positionEnd);

        // 调用AI
        if (impl_->aiWorkflow_) {
            auto aiResult = impl_->aiWorkflow_->executeAIRequest(
                prompt,
                AIModelType::GPT_4_MINI
            );

            if (aiResult.success) {
                suggestion.originalText = selectedText;
                suggestion.suggestedText = aiResult.content;
                suggestion.confidenceScore = 0.85f; // TODO: 从AI响应解析
                suggestion.explanation = "AI-powered writing improvement suggestion";

                // 保存到数据库
                PreparedStatement stmt(database_,
                    "INSERT INTO ai_writing_suggestions "
                    "(document_id, user_id, suggestion_type, position_start, position_end, "
                    "original_text, suggested_text, confidence_score, explanation) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
                );

                stmt.bind(1, documentId);
                stmt.bind(2, userId);
                stmt.bind(3, suggestionType);
                stmt.bind(4, positionStart);
                stmt.bind(5, positionEnd);
                stmt.bind(6, selectedText);
                stmt.bind(7, aiResult.content);
                stmt.bind(8, 0.85f);
                stmt.bind(9, "AI-generated suggestion");

                if (stmt.execute()) {
                    suggestion.id = stmt.executeAndReturnId();
                }
            }
        }

    } catch (const std::exception& e) {
        if (auto logging = Services::resolve<LoggingModule>()) {
            logging->error("Failed to generate suggestion: " + std::string(e.what()));
        }
    }

    return suggestion;
}

std::string CollaborativeWritingModule::buildWritingPrompt(
    const CollaborativeDocument& doc, int positionStart, int positionEnd) {

    std::ostringstream prompt;

    prompt << "You are an AI writing assistant. Help improve the following text:\n\n";
    prompt << "Document Title: " << doc.title << "\n";
    prompt << "Document Type: " << doc.documentType << "\n\n";

    // 包含上下文（前后各100个字符）
    int contextStart = std::max(0, positionStart - 100);
    int contextEnd = std::min(static_cast<int>(doc.content.length()), positionEnd + 100);
    std::string context = doc.content.substr(contextStart, contextEnd - contextStart);

    prompt << "Context:\n" << context << "\n\n";
    prompt << "Selected text to improve:\n";
    prompt << doc.content.substr(positionStart, positionEnd - positionStart);
    prompt << "\n\nPlease provide:\n";
    prompt << "1. Improved version of the text\n";
    prompt << "2. Brief explanation of changes\n";
    prompt << "3. Suggestions for further improvement";

    return prompt.str();
}

// ============================================================================
// OTOperation序列化
// ============================================================================

std::string OTOperation::toJSON() const {
    std::ostringstream json;

    json << "{";
    json << "\"type\":" << static_cast<int>(type) << ",";
    json << "\"position\":" << position << ",";
    json << "\"length\":" << length << ",";
    json << "\"content\":\"" << escapeJson(content) << "\",";
    json << "\"clientId\":" << clientId << ",";
    json << "\"timestamp\":" << timestamp;
    json << "}";

    return json.str();
}

} // namespace PaperCrawler
