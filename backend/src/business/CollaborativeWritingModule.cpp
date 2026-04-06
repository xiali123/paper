#include "business/CollaborativeWritingModule.hpp"
#include "core/Router.hpp"
#include "core/ModuleExports.hpp"
#include <json.hpp>
#include <sstream>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace PaperCrawler {

// ============================================================================
// 辅助函数
// ============================================================================

static std::string escapeJson(const std::string& s) {
    std::string result;
    result.reserve(s.size() * 2);
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
        }
    }
    return result;
}

static std::string escapeSql(const std::string& s) {
    std::string result;
    result.reserve(s.size() * 2);
    for (char c : s) {
        switch (c) {
            case '\'': result += "''"; break;
            case '\\': result += "\\\\"; break;
            case '\0': result += "\\0"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            default:   result += c; break;
        }
    }
    return result;
}

static std::string buildJsonResponse(bool success, const std::string& message, const nlohmann::json& data = nullptr) {
    nlohmann::json resp;
    resp["success"] = success;
    resp["message"] = message;
    if (data != nullptr) resp["data"] = data;
    return resp.dump();
}

static std::string getParam(const std::map<std::string, std::string>& params, const std::string& key, const std::string& def = "") {
    auto it = params.find(key);
    return it != params.end() ? it->second : def;
}

static HttpResponse buildErrorResponse(int code, const std::string& message) {
    HttpResponse resp;
    resp.statusCode = code;
    resp.headers["Content-Type"] = "application/json";
    resp.body = buildJsonResponse(false, message);
    return resp;
}

// ============================================================================
// 辅助函数
// ============================================================================

// 安全地将字符串转换为整数，处理NULL和空字符串
static int safeStoi(const std::string& s, int defaultValue = 0) {
    if (s.empty() || s == "NULL") return defaultValue;
    try {
        return std::stoi(s);
    } catch (...) {
        return defaultValue;
    }
}

// ============================================================================
// Impl 类
// ============================================================================

class CollaborativeWritingModule::Impl {
public:
    std::map<int, std::string> documentContents_;
    std::map<int, std::vector<std::string>> documentSessions_;
};

// ============================================================================
// 构造/析构
// ============================================================================

CollaborativeWritingModule::CollaborativeWritingModule()
    : impl_(std::make_unique<Impl>()) {}

CollaborativeWritingModule::~CollaborativeWritingModule() = default;

// ============================================================================
// 路由注册（直接在 lambda 中处理，不代理到 handleRequest）
// ============================================================================

void CollaborativeWritingModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    // POST /api/writing/documents — 创建文档
    router.post(prefix + "/documents", [this](const HttpRequest& req) {
        try {
            auto json = nlohmann::json::parse(req.body);
            std::string title = json.value<std::string>("title", "Untitled");
            std::string docType = json.value<std::string>("document_type", "paper");
            int ownerId = json.value<int>("owner_id", 0);
            int templateId = json.value<int>("template_id", 0);

            auto doc = createDocument(ownerId, title, docType, templateId);
            if (doc.has_value()) {
                HttpResponse resp;
                resp.statusCode = 201;
                resp.headers["Content-Type"] = "application/json";
                nlohmann::json data;
                data["id"] = doc->id;
                data["title"] = doc->title;
                data["content"] = doc->content;
                data["document_type"] = doc->documentType;
                data["owner_id"] = doc->ownerId;
                data["status"] = doc->status;
                data["word_count"] = doc->wordCount;
                data["created_at"] = doc->createdAt;
                data["updated_at"] = doc->updatedAt;
                resp.body = buildJsonResponse(true, "Document created", data);
                return resp;
            }
            return buildErrorResponse(500, "Failed to create document");
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(400, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] createDocument error: {}", e.what());
            return buildErrorResponse(500, e.what());
        }
    });
    // GET /api/writing/documents — 获取文档列表
    router.get(prefix + "/documents", [this](const HttpRequest& req) {
        try {
            // 从查询参数获取
            int ownerId = 0;
            auto ownerIdIt = req.queryParams.find("owner_id");
            if (ownerIdIt != req.queryParams.end() && !ownerIdIt->second.empty()) {
                ownerId = std::stoi(ownerIdIt->second);
            }

            int page = 1;
            auto pageIt = req.queryParams.find("page");
            if (pageIt != req.queryParams.end() && !pageIt->second.empty()) {
                page = std::stoi(pageIt->second);
            }

            int limit = 20;
            auto limitIt = req.queryParams.find("limit");
            if (limitIt != req.queryParams.end() && !limitIt->second.empty()) {
                limit = std::stoi(limitIt->second);
            }

            auto docs = getDocuments(ownerId, page, limit);

            HttpResponse resp;
            resp.statusCode = 200;
            resp.headers["Content-Type"] = "application/json";
            nlohmann::json data;
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& doc : docs) {
                nlohmann::json item;
                item["id"] = doc.id;
                item["title"] = doc.title;
                item["content"] = doc.content;
                item["document_type"] = doc.documentType;
                item["owner_id"] = doc.ownerId;
                item["status"] = doc.status;
                item["word_count"] = doc.wordCount;
                item["created_at"] = doc.createdAt;
                item["updated_at"] = doc.updatedAt;
                arr.push_back(item);
            }
            data["documents"] = arr;
            data["total"] = docs.size();
            data["page"] = page;
            data["limit"] = limit;
            resp.body = buildJsonResponse(true, "", data);
            return resp;
        } catch (const std::exception& e) {
            spdlog::error("[Writing] getDocuments error: {}", e.what());
            return buildErrorResponse(500, e.what());
        }
    });
    // GET /api/writing/documents/:id — 获取文档
    router.get(prefix + "/documents/:id", [this](const HttpRequest& req) {
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto doc = getDocument(docId);
            if (doc.has_value()) {
                HttpResponse resp;
                resp.statusCode = 200;
                resp.headers["Content-Type"] = "application/json";
                nlohmann::json data;
                data["id"] = doc->id;
                data["title"] = doc->title;
                data["content"] = doc->content;
                data["document_type"] = doc->documentType;
                data["owner_id"] = doc->ownerId;
                data["status"] = doc->status;
                data["word_count"] = doc->wordCount;
                data["last_modified_by"] = doc->lastModifiedBy;
                data["created_at"] = doc->createdAt;
                data["updated_at"] = doc->updatedAt;
                resp.body = buildJsonResponse(true, "", data);
                return resp;
            }
            return buildErrorResponse(404, "Document not found");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] getDocument error: {}", e.what());
            return buildErrorResponse(500, e.what());
        }
    });

    // PUT /api/writing/documents/:id — 更新文档
    router.put(prefix + "/documents/:id", [this](const HttpRequest& req) {
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto json = nlohmann::json::parse(req.body);
            std::string content = json.value<std::string>("content", "");
            std::string title = json.value<std::string>("title", "");
            std::string status = json.value<std::string>("status", "");

            bool ok = updateDocument(docId, content, title, status);
            if (ok) {
                auto doc = getDocument(docId);
                HttpResponse resp;
                resp.statusCode = 200;
                resp.headers["Content-Type"] = "application/json";
                nlohmann::json data;
                if (doc.has_value()) {
                    data["id"] = doc->id;
                    data["title"] = doc->title;
                    data["content"] = doc->content;
                    data["status"] = doc->status;
                    data["word_count"] = doc->wordCount;
                    data["updated_at"] = doc->updatedAt;
                }
                resp.body = buildJsonResponse(true, "Document updated", data);
                return resp;
            }
            return buildErrorResponse(404, "Document not found or update failed");
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(400, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] updateDocument error: {}", e.what());
            return buildErrorResponse(500, e.what());
        }
    });

    // POST /api/writing/documents/:id/operations — 应用 OT 操作
    router.post(prefix + "/documents/:id/operations", [this](const HttpRequest& req) {
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto json = nlohmann::json::parse(req.body);

            OTOperation op;
            op.type = static_cast<OTOperationType>(json.value<int>("type", 0));
            op.position = json.value<int>("position", 0);
            op.length = json.value<int>("length", 0);
            op.content = json.value<std::string>("content", "");
            op.clientId = json.value<int>("client_id", 0);
            op.timestamp = json.value<int>("timestamp", 0);

            std::string newContent = applyOperation(docId, op);
            HttpResponse resp;
            resp.statusCode = 200;
            resp.headers["Content-Type"] = "application/json";
            nlohmann::json data;
            data["content"] = newContent;
            data["word_count"] = static_cast<int>(
                std::count_if(newContent.begin(), newContent.end(), [](unsigned char c) { return std::isprint(c) || c == '\n'; }));
            resp.body = buildJsonResponse(true, "Operation applied", data);
            return resp;
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(400, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] applyOperation error: {}", e.what());
            return buildErrorResponse(500, e.what());
        }
    });

    // GET /api/writing/documents/:id/suggestions — 获取 AI 建议
    router.get(prefix + "/documents/:id/suggestions", [this](const HttpRequest& req) {
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto suggestions = getWritingSuggestions(docId);

            HttpResponse resp;
            resp.statusCode = 200;
            resp.headers["Content-Type"] = "application/json";
            nlohmann::json data;
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& s : suggestions) {
                nlohmann::json item;
                item["id"] = s.id;
                item["document_id"] = s.documentId;
                item["suggestion_type"] = s.suggestionType;
                item["position_start"] = s.positionStart;
                item["position_end"] = s.positionEnd;
                item["original_text"] = s.originalText;
                item["suggested_text"] = s.suggestedText;
                item["confidence_score"] = s.confidenceScore;
                item["explanation"] = s.explanation;
                item["status"] = s.status;
                arr.push_back(item);
            }
            data["suggestions"] = arr;
            data["total"] = suggestions.size();
            resp.body = buildJsonResponse(true, "", data);
            return resp;
        } catch (const std::exception& e) {
            spdlog::error("[Writing] getSuggestions error: {}", e.what());
            return buildErrorResponse(500, e.what());
        }
    });

    // POST /api/writing/documents/:id/suggestions/generate — 生成 AI 建议
    router.post(prefix + "/documents/:id/suggestions/generate", [this](const HttpRequest& req) {
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto json = nlohmann::json::parse(req.body);
            std::string sugType = json.value<std::string>("suggestion_type", "content");
            int userId = json.value<int>("user_id", 0);
            int posStart = json.value<int>("position_start", 0);
            int posEnd = json.value<int>("position_end", 0);

            auto suggestion = generateSuggestion(docId, userId, sugType, posStart, posEnd);

            HttpResponse resp;
            resp.statusCode = 200;
            resp.headers["Content-Type"] = "application/json";
            nlohmann::json data;
            data["id"] = suggestion.id;
            data["document_id"] = suggestion.documentId;
            data["suggestion_type"] = suggestion.suggestionType;
            data["original_text"] = suggestion.originalText;
            data["suggested_text"] = suggestion.suggestedText;
            data["confidence_score"] = suggestion.confidenceScore;
            data["explanation"] = suggestion.explanation;
            data["status"] = suggestion.status;
            resp.body = buildJsonResponse(true, "Suggestion generated", data);
            return resp;
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(400, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] generateSuggestion error: {}", e.what());
            return buildErrorResponse(500, e.what());
        }
    });

    // GET /api/writing/documents/:id/versions — 获取版本历史
    router.get(prefix + "/documents/:id/versions", [this](const HttpRequest& req) {
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto versions = getVersions(docId);

            HttpResponse resp;
            resp.statusCode = 200;
            resp.headers["Content-Type"] = "application/json";
            nlohmann::json data;
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& v : versions) {
                nlohmann::json item;
                item["id"] = std::stoi(v.at("id"));
                item["version_number"] = std::stoi(v.at("version_number"));
                item["content"] = v.at("content");
                item["change_summary"] = v.at("change_summary");
                item["word_count"] = std::stoi(v.at("word_count"));
                item["created_by"] = std::stoi(v.at("created_by"));
                item["is_auto_save"] = v.at("is_auto_save") == "1";
                item["created_at"] = v.at("created_at");
                arr.push_back(item);
            }
            data["versions"] = arr;
            data["total"] = versions.size();
            resp.body = buildJsonResponse(true, "", data);
            return resp;
        } catch (const std::exception& e) {
            spdlog::error("[Writing] getVersions error: {}", e.what());
            return buildErrorResponse(500, e.what());
        }
    });

    // POST /api/writing/documents/:id/comments — 添加评论
    router.post(prefix + "/documents/:id/comments", [this](const HttpRequest& req) {
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto json = nlohmann::json::parse(req.body);
            int userId = json.value<int>("user_id", 0);
            std::string content = json.value<std::string>("content", "");
            int posStart = json.value<int>("position_start", -1);
            int posEnd = json.value<int>("position_end", -1);
            int parentId = json.value<int>("parent_id", 0);

            int commentId = addComment(docId, userId, content, posStart, posEnd, parentId);

            HttpResponse resp;
            resp.statusCode = 201;
            resp.headers["Content-Type"] = "application/json";
            nlohmann::json data;
            data["id"] = commentId;
            resp.body = buildJsonResponse(true, "Comment added", data);
            return resp;
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(400, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] addComment error: {}", e.what());
            return buildErrorResponse(500, e.what());
        }
    });
}

// ============================================================================
// 1. 文档管理
// ============================================================================

std::optional<CollaborativeDocument> CollaborativeWritingModule::createDocument(
    int userId, const std::string& title, const std::string& documentType, int templateId) {

    try {
        std::string initialContent = "";
        if (templateId > 0) {
            auto results = database_->query(
                "SELECT content FROM document_templates WHERE id = " + std::to_string(templateId));
            if (!results.empty()) {
                initialContent = results[0]["content"];
            }
        }

        std::string sql = "INSERT INTO collaborative_documents "
                        "(id, title, content, document_type, owner_id, template_id, word_count, status) "
                        "VALUES (DEFAULT, '" + escapeSql(title) + "', '" + escapeSql(initialContent) + "', '"
                        + escapeSql(documentType) + "', " + std::to_string(userId) + ", "
                        + std::to_string(templateId) + ", 0, 'draft')";

        bool executeResult = database_->execute(sql);

        if (executeResult) {
            // 使用MAX(id)而不是LAST_INSERT_ID()
            auto maxIdResult = database_->query("SELECT MAX(id) as max_id FROM collaborative_documents");
            if (!maxIdResult.empty() && !maxIdResult[0]["max_id"].empty()) {
                int documentId = safeStoi(maxIdResult[0]["max_id"]);
                impl_->documentContents_[documentId] = initialContent;
                return getDocument(documentId);
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] createDocument error: {}", e.what());
    }
    return std::nullopt;
}

std::optional<CollaborativeDocument> CollaborativeWritingModule::getDocument(int documentId) {
    try {
        auto results = database_->query(
            "SELECT * FROM collaborative_documents WHERE id = " + std::to_string(documentId));

        if (!results.empty()) {
            CollaborativeDocument doc;
            doc.id = documentId;
            doc.title = results[0]["title"];
            doc.content = results[0]["content"];
            doc.documentType = results[0]["document_type"];
            doc.ownerId = safeStoi(results[0]["owner_id"]);
            doc.status = results[0]["status"];
            doc.wordCount = safeStoi(results[0]["word_count"]);
            doc.lastModifiedBy = safeStoi(results[0]["last_modified_by"]);
            doc.createdAt = results[0]["created_at"];
            doc.updatedAt = results[0]["updated_at"];
            impl_->documentContents_[documentId] = doc.content;
            return doc;
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] getDocument error: {}", e.what());
    }
    return std::nullopt;
}

bool CollaborativeWritingModule::updateDocument(int documentId, const std::string& content,
                                                const std::string& title, const std::string& status) {
    try {
        std::vector<std::string> sets;
        if (!content.empty()) sets.push_back("content = '" + escapeSql(content) + "'");
        if (!title.empty()) sets.push_back("title = '" + escapeSql(title) + "'");
        if (!status.empty()) sets.push_back("status = '" + escapeSql(status) + "'");

        if (sets.empty()) return true; // 没有更新内容

        int wordCount = static_cast<int>(
            std::count_if(content.begin(), content.end(), [](unsigned char c) { return std::isprint(c) || c == '\n'; }));
        sets.push_back("word_count = " + std::to_string(wordCount));
        sets.push_back("updated_at = NOW()");

        std::string sql = "UPDATE collaborative_documents SET " +
                        std::accumulate(sets.begin(), sets.end(), std::string(", ")) +
                        " WHERE id = " + std::to_string(documentId);

        return database_->execute(sql);
    } catch (const std::exception& e) {
        spdlog::error("[Writing] updateDocument error: {}", e.what());
        return false;
    }
}

bool CollaborativeWritingModule::deleteDocument(int documentId) {
    try {
        std::string sql = "DELETE FROM collaborative_documents WHERE id = " + std::to_string(documentId);
        bool ok = database_->execute(sql);
        if (ok) {
            impl_->documentContents_.erase(documentId);
        }
        return ok;
    } catch (const std::exception& e) {
        spdlog::error("[Writing] deleteDocument error: {}", e.what());
        return false;
    }
}

std::vector<CollaborativeDocument> CollaborativeWritingModule::getDocuments(int userId, int page, int limit) {
    std::vector<CollaborativeDocument> docs;
    try {
        int offset = (page - 1) * limit;
        auto results = database_->query(
            "SELECT * FROM collaborative_documents WHERE owner_id = " + std::to_string(userId) +
            " ORDER BY updated_at DESC LIMIT " + std::to_string(limit) +
            " OFFSET " + std::to_string(offset));

        for (auto& row : results) {
            CollaborativeDocument doc;
            doc.id = safeStoi(row["id"]);
            doc.title = row["title"];
            doc.content = row["content"];
            doc.documentType = row["document_type"];
            doc.ownerId = safeStoi(row["owner_id"]);
            doc.status = row["status"];
            doc.wordCount = safeStoi(row["word_count"]);
            doc.lastModifiedBy = safeStoi(row["last_modified_by"]);
            doc.createdAt = row["created_at"];
            doc.updatedAt = row["updated_at"];
            impl_->documentContents_[doc.id] = doc.content;
            docs.push_back(doc);
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] getDocuments error: {}", e.what());
    }
    return docs;
}

// ============================================================================
// 2. OT 算法
// ============================================================================

std::string CollaborativeWritingModule::applyOperation(int documentId, const OTOperation& operation) {
    try {
        std::string content = impl_->documentContents_[documentId];

        std::string newContent;
        switch (operation.type) {
            case OTOperationType::INSERT:
                if (operation.position >= 0 && operation.position <= (int)content.length()) {
                    newContent = content.substr(0, operation.position) +
                                  operation.content +
                                  content.substr(operation.position);
                }
                break;
            case OTOperationType::DELETE:
                if (operation.position >= 0 &&
                    operation.position + operation.length <= (int)content.length()) {
                    newContent = content.substr(0, operation.position) +
                                  content.substr(operation.position + operation.length);
                }
                break;
            default:
                newContent = content;
                break;
        }

        impl_->documentContents_[documentId] = newContent;

        // 记录操作
        std::string sql = "INSERT INTO document_operations "
                        "(document_id, user_id, operation_type, position, length, content) "
                        "VALUES (" + std::to_string(documentId) + ", "
                        + std::to_string(operation.clientId) + ", '"
                        + std::to_string(static_cast<int>(operation.type)) + "', "
                        + std::to_string(operation.position) + ", "
                        + std::to_string(operation.length) + ", '"
                        + escapeSql(operation.content) + "')";
        database_->execute(sql);

        // 更新文档
        int wordCount = static_cast<int>(
            std::count_if(newContent.begin(), newContent.end(), [](unsigned char c) { return std::isprint(c) || c == '\n'; }));
        std::string updateSql = "UPDATE collaborative_documents SET content = '" + escapeSql(newContent) +
                "', word_count = " + std::to_string(wordCount) + ", updated_at = NOW() WHERE id = " + std::to_string(documentId);
        database_->execute(updateSql);

        return newContent;
    } catch (const std::exception& e) {
        spdlog::error("[Writing] applyOperation error: {}", e.what());
        return "";
    }
}

OTOperation CollaborativeWritingModule::transformOperation(const OTOperation& clientOp, const OTOperation& serverOp) {
    if (clientOp.type == OTOperationType::INSERT) {
        if (serverOp.type == OTOperationType::INSERT) return transformInsertAgainstInsert(clientOp, serverOp);
        if (serverOp.type == OTOperationType::DELETE) return transformInsertAgainstDelete(clientOp, serverOp);
    } else if (clientOp.type == OTOperationType::DELETE) {
        if (serverOp.type == OTOperationType::INSERT) return transformDeleteAgainstInsert(clientOp, serverOp);
        if (serverOp.type == OTOperationType::DELETE) return transformDeleteAgainstDelete(clientOp, serverOp);
    }
    return clientOp;
}

OTOperation CollaborativeWritingModule::transformInsertAgainstInsert(const OTOperation& clientOp, const OTOperation& serverOp) {
    OTOperation transformed = clientOp;
    if (serverOp.position <= clientOp.position) {
        transformed.position = clientOp.position + (int)serverOp.content.length();
    }
    return transformed;
}

OTOperation CollaborativeWritingModule::transformInsertAgainstDelete(const OTOperation& clientOp, const OTOperation& serverOp) {
    OTOperation transformed = clientOp;
    if (serverOp.position < clientOp.position) {
        int offset = std::min(serverOp.length, clientOp.position - serverOp.position);
        transformed.position = clientOp.position - offset;
    } else if (serverOp.position < clientOp.position + (int)clientOp.content.length()) {
        int overlapStart = std::max(serverOp.position, clientOp.position);
        int overlapEnd = std::min(serverOp.position + serverOp.length,
                                clientOp.position + (int)clientOp.content.length());
        int overlapLength = overlapEnd - overlapStart;
        if (overlapLength > 0) {
            transformed.content = clientOp.content.substr(0, overlapStart - clientOp.position) +
                              clientOp.content.substr(overlapEnd - clientOp.position);
        }
    }
    return transformed;
}

OTOperation CollaborativeWritingModule::transformDeleteAgainstInsert(const OTOperation& clientOp, const OTOperation& serverOp) {
    OTOperation transformed = clientOp;
    if (serverOp.position <= clientOp.position) {
        transformed.position = clientOp.position + (int)serverOp.content.length();
    }
    return transformed;
}

OTOperation CollaborativeWritingModule::transformDeleteAgainstDelete(const OTOperation& clientOp, const OTOperation& serverOp) {
    OTOperation transformed = clientOp;
    if (serverOp.position < clientOp.position + clientOp.length &&
        serverOp.position + serverOp.length > clientOp.position) {
        int overlapStart = std::max(serverOp.position, clientOp.position);
        int overlapEnd = std::min(serverOp.position + serverOp.length, clientOp.position + clientOp.length);
        int removedByServer = overlapEnd - overlapStart;
        transformed.length = clientOp.length - removedByServer;
        if (transformed.length <= 0) {
            transformed.type = OTOperationType::RETAIN;
            transformed.length = 0;
        }
    }
    return transformed;
}

// ============================================================================
// 3. WebSocket（保留框架，实际发送 TODO）
// ============================================================================

void CollaborativeWritingModule::handleWebSocketConnection(int documentId, int userId, const std::string& socketId) {
    try {
        std::string sql = "INSERT INTO collaboration_sessions "
                        "(document_id, user_id, socket_id, is_active) "
                        "VALUES (" + std::to_string(documentId) + ", "
                        + std::to_string(userId) + ", '" + escapeSql(socketId) + "', TRUE)";
        if (database_->execute(sql)) {
            impl_->documentSessions_[documentId].push_back(socketId);
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] handleWebSocketConnection error: {}", e.what());
    }
}

void CollaborativeWritingModule::handleWebSocketDisconnection(const std::string& socketId) {
    try {
        database_->execute("UPDATE collaboration_sessions SET is_active = FALSE WHERE socket_id = '" + escapeSql(socketId) + "'");
        for (auto& [docId, sessions] : impl_->documentSessions_) {
            sessions.erase(std::remove(sessions.begin(), sessions.end(), socketId), sessions.end());
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] handleWebSocketDisconnection error: {}", e.what());
    }
}

void CollaborativeWritingModule::broadcastOperation(int documentId, const OTOperation& operation) {
    // TODO: 实际 WebSocket 广播
}

// ============================================================================
// 4. AI 写作建议
// ============================================================================

WritingSuggestion CollaborativeWritingModule::generateSuggestion(
    int documentId, int userId, const std::string& suggestionType, int positionStart, int positionEnd) {

    WritingSuggestion suggestion;
    suggestion.documentId = documentId;
    suggestion.userId = userId;
    suggestion.suggestionType = suggestionType;
    suggestion.positionStart = positionStart;
    suggestion.positionEnd = positionEnd;
    suggestion.status = "pending";
    suggestion.confidenceScore = 0.0f;

    try {
        auto docOpt = getDocument(documentId);
        if (!docOpt.has_value()) return suggestion;

        auto doc = docOpt.value();
        if (positionEnd <= positionStart || positionEnd > (int)doc.content.length()) {
            positionEnd = (int)doc.content.length();
        }

        std::string prompt = buildWritingPrompt(doc, positionStart, positionEnd);

        // 尝试 AI 调用，失败则返回基于规则的占位建议
        std::string suggestedText = "Consider improving clarity, structure, and academic tone.";
        bool aiSuccess = false;

        if (true) { // AI 服务暂时不可用，使用规则
            suggestedText = "[AI suggestion placeholder] Based on the selected text, consider: "
                             "1) Strengthening the thesis statement, "
                             "2) Adding supporting evidence, "
                             "3) Improving transition flow, "
                             "4) Checking academic tone consistency.";
            suggestion.confidenceScore = 0.5f;
            aiSuccess = true;
        }

        if (aiSuccess) {
            suggestion.originalText = doc.content.substr(positionStart, positionEnd - positionStart);
            suggestion.suggestedText = suggestedText;
            suggestion.explanation = "Rule-based writing improvement suggestion";

            std::string sql = "INSERT INTO ai_writing_suggestions "
                            "(document_id, user_id, suggestion_type, position_start, position_end, "
                            "original_text, suggested_text, confidence_score, explanation, status) "
                            "VALUES (" + std::to_string(documentId) + ", "
                            + std::to_string(userId) + ", '" + escapeSql(suggestionType) + "', "
                            + std::to_string(positionStart) + ", " + std::to_string(positionEnd) + ", '"
                            + escapeSql(suggestion.originalText) + "', '"
                            + escapeSql(suggestedText) + "', "
                            + std::to_string(suggestion.confidenceScore) + ", '"
                            + escapeSql(suggestion.explanation) + "', 'pending')";
            database_->execute(sql);

            auto results = database_->query("SELECT LAST_INSERT_ID() as id");
            if (!results.empty()) {
                suggestion.id = std::stoi(results[0]["id"]);
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] generateSuggestion error: {}", e.what());
    }
    return suggestion;
}

std::string CollaborativeWritingModule::buildWritingPrompt(const CollaborativeDocument& doc, int positionStart, int positionEnd) {
    std::ostringstream prompt;
    prompt << "You are an AI writing assistant. Help improve the following text:\n\n";
    prompt << "Document Title: " << doc.title << "\n";
    prompt << "Document Type: " << doc.documentType << "\n\n";

    int contextStart = std::max(0, positionStart - 100);
    int contextEnd = std::min(static_cast<int>(doc.content.length()), positionEnd + 100);
    std::string context = doc.content.substr(contextStart, contextEnd - contextStart);

    prompt << "Context:\n" << context << "\n\n";
    prompt << "Selected text to improve:\n" << doc.content.substr(positionStart, positionEnd - positionStart);
    prompt << "\n\nPlease provide:\n";
    prompt << "1. Improved version of the text\n";
    prompt << "2. Brief explanation of changes\n";
    prompt << "3. Suggestions for further improvement";
    return prompt.str();
}

std::vector<WritingSuggestion> CollaborativeWritingModule::getWritingSuggestions(int documentId) {
    std::vector<WritingSuggestion> suggestions;
    try {
        auto results = database_->query(
            "SELECT * FROM ai_writing_suggestions WHERE document_id = " + std::to_string(documentId) +
            " ORDER BY created_at DESC");

        for (auto& row : results) {
            WritingSuggestion s;
            s.id = std::stoi(row["id"]);
            s.documentId = documentId;
            s.userId = std::stoi(row["user_id"]);
            s.suggestionType = row["suggestion_type"];
            s.positionStart = std::stoi(row["position_start"]);
            s.positionEnd = std::stoi(row["position_end"]);
            s.originalText = row["original_text"];
            s.suggestedText = row["suggested_text"];
            s.confidenceScore = std::stof(row["confidence_score"]);
            s.explanation = row["explanation"];
            s.status = row["status"];
            suggestions.push_back(s);
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] getWritingSuggestions error: {}", e.what());
    }
    return suggestions;
}

bool CollaborativeWritingModule::acceptSuggestion(int suggestionId) {
    try {
        return database_->execute(
            "UPDATE ai_writing_suggestions SET status = 'accepted' WHERE id = " + std::to_string(suggestionId));
    } catch (const std::exception& e) {
        spdlog::error("[Writing] acceptSuggestion error: {}", e.what());
        return false;
    }
}

bool CollaborativeWritingModule::rejectSuggestion(int suggestionId) {
    try {
        return database_->execute(
            "UPDATE ai_writing_suggestions SET status = 'rejected' WHERE id = " + std::to_string(suggestionId));
    } catch (const std::exception& e) {
        spdlog::error("[Writing] rejectSuggestion error: {}", e.what());
        return false;
    }
}

// ============================================================================
// 5. 版本控制
// ============================================================================

bool CollaborativeWritingModule::createVersion(int documentId, int userId, const std::string& summary) {
    try {
        auto docOpt = getDocument(documentId);
        if (!docOpt.has_value()) return false;

        std::string content = docOpt->content;

        // 获取当前最大版本号
        auto verResults = database_->query(
            "SELECT COALESCE(MAX(version_number), 0) FROM document_versions WHERE document_id = "
            + std::to_string(documentId));
        int nextVersion = verResults.empty() ? 1 : std::stoi(verResults[0]["COALESCE(MAX(version_number), 0)"]) + 1;

        int wordCount = static_cast<int>(
            std::count_if(content.begin(), content.end(), [](unsigned char c) { return std::isprint(c) || c == '\n'; }));

        std::string sql = "INSERT INTO document_versions "
                        "(document_id, version_number, content, change_summary, word_count, created_by) "
                        "VALUES (" + std::to_string(documentId) + ", "
                        + std::to_string(nextVersion) + ", '"
                        + escapeSql(content) + "', '"
                        + escapeSql(summary) + "', "
                        + std::to_string(wordCount) + ", "
                        + std::to_string(userId) + ")";
        return database_->execute(sql);
    } catch (const std::exception& e) {
        spdlog::error("[Writing] createVersion error: {}", e.what());
        return false;
    }
}

std::vector<std::map<std::string, std::string>> CollaborativeWritingModule::getVersions(int documentId) {
    std::vector<std::map<std::string, std::string>> versions;
    try {
        auto results = database_->query(
            "SELECT * FROM document_versions WHERE document_id = " + std::to_string(documentId) +
            " ORDER BY version_number DESC");

        for (const auto& row : results) {
            versions.push_back(row);
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] getVersions error: {}", e.what());
    }
    return versions;
}

// ============================================================================
// 6. 评论
// ============================================================================

int CollaborativeWritingModule::addComment(int documentId, int userId, const std::string& content,
                                           int positionStart, int positionEnd, int parentId) {
    try {
        std::string sql = "INSERT INTO document_comments "
                        "(document_id, user_id, parent_id, position_start, position_end, content) "
                        "VALUES (" + std::to_string(documentId) + ", "
                        + std::to_string(userId) + ", "
                        + std::to_string(parentId) + ", "
                        + std::to_string(positionStart) + ", "
                        + std::to_string(positionEnd) + ", '"
                        + escapeSql(content) + "')";

        if (database_->execute(sql)) {
            auto results = database_->query("SELECT LAST_INSERT_ID() as id");
            if (!results.empty()) {
                return std::stoi(results[0]["id"]);
            }
        }
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("[Writing] addComment error: {}", e.what());
        return 0;
    }
}

std::vector<std::map<std::string, std::string>> CollaborativeWritingModule::getComments(int documentId) {
    std::vector<std::map<std::string, std::string>> comments;
    try {
        auto results = database_->query(
            "SELECT * FROM document_comments WHERE document_id = " + std::to_string(documentId) +
            " ORDER BY created_at ASC");

        for (const auto& row : results) {
            comments.push_back(row);
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] getComments error: {}", e.what());
    }
    return comments;
}

bool CollaborativeWritingModule::resolveComment(int commentId) {
    try {
        return database_->execute(
            "UPDATE document_comments SET is_resolved = TRUE, updated_at = NOW() WHERE id = "
            + std::to_string(commentId));
    } catch (const std::exception& e) {
        spdlog::error("[Writing] resolveComment error: {}", e.what());
        return false;
    }
}

// ============================================================================
// OTOperation 序列化
// ============================================================================

std::string OTOperation::toJSON() const {
    std::ostringstream json;
    json << "{\"type\":" << static_cast<int>(type) << ","
         << "\"position\":" << position << ","
         << "\"length\":" << length << ","
         << "\"content\":\"" << escapeJson(content) << "\","
         << "\"clientId\":" << clientId << ","
         << "\"timestamp\":" << timestamp << "}";
    return json.str();
}

} // namespace PaperCrawler

// ============================================================================
// DLL 导出函数
// ============================================================================

#define MODULE_EXPORT __attribute__((visibility("default")))

extern "C" {

MODULE_EXPORT void* createModule() {
    return new PaperCrawler::CollaborativeWritingModule();
}

MODULE_EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::CollaborativeWritingModule*>(ptr);
}

MODULE_EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}
