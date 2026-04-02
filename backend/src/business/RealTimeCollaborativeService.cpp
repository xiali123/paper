// 实时AI协作写作服务实现
// 文件位置：backend/src/business/RealTimeCollaborativeService.cpp

#include "business/CollaborativeWritingModule.hpp"
#include "business/AiApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <algorithm>

namespace PaperCrawler {
namespace Services {

// ============================================================================
// OT（Operational Transformation）引擎
// ============================================================================

/**
 * @brief 操作类型
 */
enum class OperationType {
    INSERT,   // 插入字符
    DELETE,   // 删除字符
    RETAIN    // 保留字符
};

/**
 * @brief 文档操作
 */
struct DocumentOperation {
    int userId;
    int clientId;  // 客户端ID
    int version;
    OperationType type;
    size_t position;
    std::string content;
    std::chrono::system_clock::time_point timestamp;

    /**
     * @brief 转换为JSON
     */
    nlohmann::json toJson() const {
        nlohmann::json j;
        j["user_id"] = userId;
        j["client_id"] = clientId;
        j["version"] = version;
        j["type"] = static_cast<int>(type);
        j["position"] = position;
        j["content"] = content;
        j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()
        ).count();
        return j;
    }

    static DocumentOperation fromJson(const nlohmann::json& j) {
        DocumentOperation op;
        op.userId = j["user_id"];
        op.clientId = j["client_id"];
        op.version = j["version"];
        op.type = static_cast<OperationType>(j["type"].get<int>());
        op.position = j["position"];
        op.content = j["content"];
        op.timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(j["timestamp"].get<long long>())
        );
        return op;
    }
};

/**
 * @brief OT引擎实现
 */
class OTEngine {
public:
    /**
     * @brief 转换操作（解决冲突）
     * @param operation 待转换操作
     * @param againstVersion 目标版本
     * @param history 历史操作链
     * @return 转换后的操作
     */
    DocumentOperation transform(
        const DocumentOperation& operation,
        int againstVersion,
        const std::vector<DocumentOperation>& history) {

        // 简化的OT实现（基于Insert-Delete冲突解决）
        DocumentOperation transformed = operation;

        for (const auto& op : history) {
            if (op.version <= againstVersion) {
                // 应用操作到position
                if (op.type == OperationType::INSERT) {
                    if (transformed.position >= op.position) {
                        transformed.position += op.content.length();
                    }
                } else if (op.type == OperationType::DELETE) {
                    if (transformed.position > op.position) {
                        transformed.position -= op.position;
                    } else if (transformed.position > op.position - op.content.length()) {
                        transformed.position = op.position;
                    }
                }
            }
        }

        return transformed;
    }

    /**
     * @brief 应用操作到文档
     * @param document 当前文档内容
     * @param operation 操作
     * @return 新文档内容
     */
    std::string apply(const std::string& document, const DocumentOperation& operation) {
        switch (operation.type) {
            case OperationType::INSERT:
                if (operation.position > document.length()) {
                    return document + operation.content;
                }
                return document.substr(0, operation.position) +
                       operation.content +
                       document.substr(operation.position);

            case OperationType::DELETE:
                if (operation.position >= document.length()) {
                    return document;
                }
                size_t deleteLength = std::min(
                    operation.position + operation.content.length(),
                    document.length()
                ) - operation.position;
                return document.substr(0, operation.position) +
                       document.substr(operation.position + deleteLength);

            case OperationType::RETAIN:
                return document;

            default:
                return document;
        }
    }
};

// ============================================================================
// 实时AI写作辅导
// ============================================================================

/**
 * @brief 写作建议
 */
struct WritingSuggestion {
    std::string type;          // 类型：grammar, style, structure, citation
    std::string message;       // 建议内容
    size_t position;          // 位置
    std::string original;     // 原文
    std::string suggested;    // 建议
    double confidence;        // 置信度 (0.0-1.0)
};

/**
 * @brief 实时AI写作辅导服务
 */
class RealTimeWritingAssistant {
public:
    RealTimeWritingAssistant(
        std::shared_ptr<AiApiClient> aiClient,
        std::shared_ptr<ICache> cache)
        : aiClient_(aiClient), cache_(cache) {}

    /**
     * @brief 分析文本并提供建议
     * @param text 文本内容
     * @return 写作建议列表
     */
    std::vector<WritingSuggestion> analyze(const std::string& text) {
        auto logger = spdlog::get("CollaborativeWriting");

        logger->debug("Analyzing text of length: {}", text.length());

        // 1. 检查缓存
        std::string cacheKey = "writing:suggestion:" + std::to_string(std::hash<std::string>{}(text));
        auto cached = cache_->get(cacheKey);
        if (cached) {
            return deserializeSuggestions(*cached);
        }

        // 2. 构建AI prompt
        std::ostringstream prompt;
        prompt << "Analyze the following academic text and provide suggestions:\n\n"
               << "Text:\n" << text << "\n\n"
               << "Provide suggestions for:\n"
               << "1. Grammar corrections\n"
               << "2. Style improvements (academic tone)\n"
               << "3. Structure recommendations\n"
               << "4. Citation formatting\n\n"
               << "Format as JSON array with fields: type, message, position, original, suggested, confidence.";

        // 3. 调用AI分析
        auto aiResponse = aiClient_->complete(prompt.str());

        // 4. 解析响应
        auto suggestions = parseSuggestions(aiResponse);

        // 5. 缓存结果（5分钟）
        cache_->set(cacheKey, serializeSuggestions(suggestions), 300);

        return suggestions;
    }

    /**
     * @brief 实时语法检查（去抖）
     * @param userId 用户ID
     * @param text 文本内容
     * @return 写作建议
     */
    std::vector<WritingSuggestion> checkGrammarWithDebounce(
        int userId,
        const std::string& text) {

        auto now = std::chrono::steady_clock::now();

        // 检查去抖（上次检查不到1秒）
        auto lastCheck = lastCheckTime_[userId];
        if (lastCheck.has_value() &&
            now - *lastCheck < std::chrono::milliseconds(1000)) {
            return {};  // 跳过
        }

        lastCheckTime_[userId] = now;
        return analyze(text);
    }

    /**
     * @brief 引用推荐（基于上下文）
     * @param text 当前文本
     * @param position 光标位置
     * @return 推荐的论文列表
     */
    std::vector<PaperDto> suggestCitations(const std::string& text, size_t position) {
        // 提取上下文（前后100字符）
        size_t start = position > 100 ? position - 100 : 0;
        size_t end = std::min(position + 100, text.length());
        std::string context = text.substr(start, end - start);

        // 构建AI prompt
        std::ostringstream prompt;
        prompt << "Based on the following context from an academic paper:\n\n"
               << "Context: \"" << context << "\"\n\n"
               << "Suggest 3 relevant papers that should be cited.\n"
               << "For each paper, provide: title, authors, year, venue.\n"
               << "Format as JSON array.";

        auto aiResponse = aiClient_->complete(prompt.str());

        // 解析响应并搜索论文数据库
        auto suggestedPapers = parseAndSearchPapers(aiResponse);

        return suggestedPapers;
    }

private:
    std::vector<WritingSuggestion> parseSuggestions(const std::string& aiResponse) {
        std::vector<WritingSuggestion> suggestions;

        try {
            auto json = nlohmann::json::parse(aiResponse);

            for (const auto& item : json) {
                WritingSuggestion suggestion;
                suggestion.type = item["type"];
                suggestion.message = item["message"];
                suggestion.position = item["position"];
                suggestion.original = item["original"];
                suggestion.suggested = item["suggested"];
                suggestion.confidence = item["confidence"];

                suggestions.push_back(suggestion);
            }
        } catch (const std::exception& e) {
            spdlog::get("CollaborativeWriting")->error(
                "Failed to parse suggestions: {}", e.what()
            );
        }

        return suggestions;
    }

    std::vector<PaperDto> parseAndSearchPapers(const std::string& aiResponse) {
        std::vector<PaperDto> papers;

        try {
            auto json = nlohmann::json::parse(aiResponse);

            for (const auto& item : json) {
                std::string title = item["title"];
                std::string authors = item["authors"];
                int year = item["year"];

                // 从数据库搜索匹配的论文
                // （简化实现，实际应该使用全文搜索）
                auto paper = searchPaper(title, authors, year);
                if (paper) {
                    papers.push_back(*paper);
                }
            }
        } catch (const std::exception& e) {
            spdlog::get("CollaborativeWriting")->error(
                "Failed to parse paper suggestions: {}", e.what()
            );
        }

        return papers;
    }

    std::optional<PaperDto> searchPaper(
        const std::string& title,
        const std::string& authors,
        int year) {

        // 简化实现：应该使用全文搜索
        std::ostringstream sql;
        sql << "SELECT * FROM papers "
             << "WHERE title LIKE '%" << title << "%' "
             << "AND year = " << year << " "
             << "LIMIT 1";

        // （需要数据库连接）
        // auto results = database_->query(sql.str());
        // ...

        return std::nullopt;
    }

    std::string serializeSuggestions(const std::vector<WritingSuggestion>& suggestions) {
        nlohmann::json j = suggestions;
        return j.dump();
    }

    std::vector<WritingSuggestion> deserializeSuggestions(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        return j.get<std::vector<WritingSuggestion>>();
    }

private:
    std::shared_ptr<AiApiClient> aiClient_;
    std::shared_ptr<ICache> cache_;

    std::map<int, std::optional<std::chrono::steady_clock::time_point>> lastCheckTime_;
};

// ============================================================================
// 协作文档管理
// ============================================================================

/**
 * @brief 协作文档
 */
struct CollaborativeDocument {
    int id;
    std::string title;
    std::string content;
    int ownerId;
    std::string documentType;
    int version;
    std::chrono::system_clock::time_point lastModified;
    std::map<int, bool> activeUsers;  // userId -> isConnected
};

/**
 * @brief 协作服务
 */
class CollaborativeService {
public:
    CollaborativeService(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<OTEngine> otEngine,
        std::shared_ptr<RealTimeWritingAssistant> assistant)
        : database_(database), otEngine_(otEngine), assistant_(assistant) {

        // 启动后台同步线程
        syncThread_ = std::thread([this]() { syncThreadLoop(); });
    }

    ~CollaborativeService() {
        running_ = false;
        if (syncThread_.joinable()) {
            syncThread_.join();
        }
    }

    /**
     * @brief 应用操作到文档
     * @param documentId 文档ID
     * @param operation 操作
     * @return 应用后的文档版本
     */
    int applyOperation(int documentId, const DocumentOperation& operation) {
        std::unique_lock<std::shared_mutex> lock(documentMutex_);

        // 1. 获取文档
        auto doc = getDocument(documentId);
        if (!doc) {
            throw Errors::NotFound("Document not found");
        }

        // 2. 转换操作
        auto transformed = otEngine_->transform(
            operation,
            doc->version,
            getOperationHistory(documentId)
        );

        // 3. 应用操作
        doc->content = otEngine_->apply(doc->content, transformed);
        doc->version++;
        doc->lastModified = std::chrono::system_clock::now();

        // 4. 保存到数据库
        saveDocument(*doc);

        // 5. 保存操作历史
        saveOperation(documentId, transformed);

        // 6. 通知其他用户
        broadcastOperation(documentId, transformed);

        return doc->version;
    }

    /**
     * @brief 获取文档操作历史
     * @param documentId 文档ID
     * @return 操作列表
     */
    std::vector<DocumentOperation> getOperationHistory(int documentId) {
        std::unique_lock<std::shared_mutex> lock(documentMutex_);

        std::ostringstream sql;
        sql << "SELECT * FROM document_operations "
             << "WHERE document_id = " << documentId << " "
             << "ORDER BY timestamp ASC";

        // （从数据库加载）
        // auto results = database_->query(sql.str());
        // ...

        return {};
    }

    /**
     * @brief 智能合并版本
     * @param documentId 文档ID
     * @param branchVersion1 分支版本1
     * @param branchVersion2 分支版本2
     * @return 合并后的版本
     */
    int mergeVersions(int documentId, int branchVersion1, int branchVersion2) {
        std::unique_lock<std::shared_mutex> lock(documentMutex_);

        // 1. 获取两个版本的操作历史
        auto history1 = getOperationHistoryBefore(documentId, branchVersion1);
        auto history2 = getOperationHistoryBefore(documentId, branchVersion2);

        // 2. 使用AI辅助合并
        auto merged = aiAssistedMerge(history1, history2);

        // 3. 应用合并后的操作
        auto doc = getDocument(documentId);
        for (const auto& op : merged) {
            doc->content = otEngine_->apply(doc->content, op);
        }

        doc->version++;
        saveDocument(*doc);

        return doc->version;
    }

private:
    void syncThreadLoop() {
        while (running_) {
            // 定期同步文档到数据库
            std::this_thread::sleep_for(std::chrono::seconds(10));

            std::unique_lock<std::shared_mutex> lock(documentMutex_);
            for (const auto& [docId, doc] : documents_) {
                if (doc->version % 10 == 0) {  // 每10个版本保存一次
                    saveDocument(*doc);
                }
            }
        }
    }

    void broadcastOperation(int documentId, const DocumentOperation& operation) {
        // 通过WebSocket广播操作到所有连接的客户端
        // （需要WebSocketModule集成）
    }

    std::vector<DocumentOperation> getOperationHistoryBefore(int documentId, int version) {
        std::vector<DocumentOperation> history;

        std::ostringstream sql;
        sql << "SELECT * FROM document_operations "
             << "WHERE document_id = " << documentId << " "
             << "AND version <= " << version << " "
             << "ORDER BY timestamp ASC";

        // （从数据库加载）
        // ...

        return history;
    }

    std::vector<DocumentOperation> aiAssistedMerge(
        const std::vector<DocumentOperation>& history1,
        const std::vector<DocumentOperation>& history2) {

        // 使用AI智能合并冲突操作
        std::ostringstream prompt;
        prompt << "Merge the following two sequences of document operations:\n\n"
               << "Sequence 1:\n";
        for (const auto& op : history1) {
            prompt << op.toJson() << "\n";
        }

        prompt << "\nSequence 2:\n";
        for (const auto& op : history2) {
            prompt << op.toJson() << "\n";
        }

        prompt << "\nProvide a merged sequence that preserves both users' intent.\n"
               << "Format as JSON array.";

        // 调用AI合并
        // auto aiResponse = aiClient_->complete(prompt.str());
        // ...

        // 简化实现：选择历史较长的版本
        return history1.size() > history2.size() ? history1 : history2;
    }

    std::optional<CollaborativeDocument> getDocument(int documentId) {
        // 从内存或数据库加载
        // ...
        return std::nullopt;
    }

    void saveDocument(const CollaborativeDocument& doc) {
        // 保存到数据库
        // ...
    }

    void saveOperation(int documentId, const DocumentOperation& op) {
        std::ostringstream sql;
        sql << "INSERT INTO document_operations "
             << "(document_id, user_id, operation_data) VALUES ("
             << documentId << ", "
             << op.userId << ", "
             << "'" << op.toJson().dump() << "')";

        // database_->execute(sql.str());
    }

private:
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<OTEngine> otEngine_;
    std::shared_ptr<RealTimeWritingAssistant> assistant_;

    std::shared_mutex documentMutex_;
    std::map<int, CollaborativeDocument> documents_;

    std::thread syncThread_;
    std::atomic<bool> running_{true};
};

} // namespace Services
} // namespace PaperCrawler
