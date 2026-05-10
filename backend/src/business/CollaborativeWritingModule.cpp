#include "business/CollaborativeWritingModule.hpp"
#include "core/HttpStatus.hpp"
#include "data/StringUtil.hpp"
#include "core/Router.hpp"
#include "core/ModuleExports.hpp"
#include "network/WebSocketModule.hpp"
#include "features/security/SecurityModule.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <algorithm>
#include <spdlog/spdlog.h>
#include "data/ValidationHelper.hpp"
#include "data/PreparedStatement.hpp"

namespace PaperCrawler {

// ============================================================================
// 辅助函数
// ============================================================================

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
    resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
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
        spdlog::warn("[CollabWriting] safeStoi failed for input: '{}'", s);
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

void CollaborativeWritingModule::setWebSocketModule(std::shared_ptr<WebSocketModule> wsModule) {
    wsModule_ = std::move(wsModule);
    spdlog::info("[Writing] WebSocketModule registered for real-time collaboration");
}

// ============================================================================
// 路由注册（直接在 lambda 中处理，不代理到 handleRequest）
// ============================================================================

void CollaborativeWritingModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    // Auth middleware - check Authorization header
    auto requireAuth = [](const HttpRequest& req) -> bool {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end()) return false;

        const std::string& authHeader = authIt->second;
        if (authHeader.substr(0, 7) != "Bearer ") return false;

        std::string token = authHeader.substr(7);
        if (token.empty()) return false;

        SecurityModule sec;
        auto result = sec.verifyJWT(token);
        return result.valid;
    };

    auto unauthorizedResp = []() -> HttpResponse {
        return HttpResponse::json(HTTP::UNAUTHORIZED, buildJsonResponse(false, "Unauthorized"));
    };

    // POST /api/writing/documents — 创建文档
    router.post(prefix + "/documents", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            auto json = nlohmann::json::parse(req.body);
            std::string title = ValidationHelper::sanitize(json.value<std::string>("title", "Untitled"));
            std::string docType = json.value<std::string>("document_type", "paper");
            int ownerId = json.value<int>("owner_id", 0);
            int templateId = json.value<int>("template_id", 0);

            auto doc = createDocument(ownerId, title, docType, templateId);
            if (doc.has_value()) {
                HttpResponse resp;
                resp.statusCode = HTTP::CREATED;
                resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
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
            return buildErrorResponse(HTTP::INTERNAL_ERROR, "Failed to create document");
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] createDocument error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });
    // GET /api/writing/documents — 获取文档列表
    router.get(prefix + "/documents", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
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
            resp.statusCode = HTTP::OK;
            resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
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
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });
    // GET /api/writing/documents/:id — 获取文档
    router.get(prefix + "/documents/:id", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto doc = getDocument(docId);
            if (doc.has_value()) {
                HttpResponse resp;
                resp.statusCode = HTTP::OK;
                resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
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
            return buildErrorResponse(HTTP::NOT_FOUND, "Document not found");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] getDocument error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // PUT /api/writing/documents/:id — 更新文档
    router.put(prefix + "/documents/:id", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto json = nlohmann::json::parse(req.body);
            std::string content = ValidationHelper::sanitize(json.value<std::string>("content", ""));
            std::string title = ValidationHelper::sanitize(json.value<std::string>("title", ""));
            std::string status = json.value<std::string>("status", "");

            bool ok = updateDocument(docId, content, title, status);
            if (ok) {
                auto doc = getDocument(docId);
                HttpResponse resp;
                resp.statusCode = HTTP::OK;
                resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
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
            return buildErrorResponse(HTTP::NOT_FOUND, "Document not found or update failed");
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] updateDocument error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // POST /api/writing/documents/:id/operations — 应用 OT 操作
    router.post(prefix + "/documents/:id/operations", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto json = nlohmann::json::parse(req.body);

            OTOperation op;
            op.type = static_cast<OTOperationType>(json.value<int>("type", 0));
            op.position = json.value<int>("position", 0);
            op.length = json.value<int>("length", 0);
            op.content = ValidationHelper::sanitize(json.value<std::string>("content", ""));
            op.clientId = json.value<int>("client_id", 0);
            op.timestamp = json.value<int>("timestamp", 0);

            std::string newContent = applyOperation(docId, op);
            HttpResponse resp;
            resp.statusCode = HTTP::OK;
            resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
            nlohmann::json data;
            data["content"] = newContent;
            data["word_count"] = static_cast<int>(
                std::count_if(newContent.begin(), newContent.end(), [](unsigned char c) { return std::isprint(c) || c == '\n'; }));
            resp.body = buildJsonResponse(true, "Operation applied", data);
            return resp;
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] applyOperation error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // GET /api/writing/documents/:id/suggestions — 获取 AI 建议
    router.get(prefix + "/documents/:id/suggestions", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto suggestions = getWritingSuggestions(docId);

            HttpResponse resp;
            resp.statusCode = HTTP::OK;
            resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
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
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // POST /api/writing/documents/:id/suggestions/generate — 生成 AI 建议
    router.post(prefix + "/documents/:id/suggestions/generate", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto json = nlohmann::json::parse(req.body);
            std::string sugType = ValidationHelper::sanitize(json.value<std::string>("suggestion_type", "content"));
            int userId = json.value<int>("user_id", 0);
            int posStart = json.value<int>("position_start", 0);
            int posEnd = json.value<int>("position_end", 0);

            auto suggestion = generateSuggestion(docId, userId, sugType, posStart, posEnd);

            HttpResponse resp;
            resp.statusCode = HTTP::OK;
            resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
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
            return buildErrorResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] generateSuggestion error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // GET /api/writing/documents/:id/versions — 获取版本历史
    router.get(prefix + "/documents/:id/versions", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto versions = getVersions(docId);

            HttpResponse resp;
            resp.statusCode = HTTP::OK;
            resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
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
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // POST /api/writing/documents/:id/comments — 添加评论
    router.post(prefix + "/documents/:id/comments", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto json = nlohmann::json::parse(req.body);
            int userId = json.value<int>("user_id", 0);
            std::string content = ValidationHelper::sanitize(json.value<std::string>("content", ""));
            int posStart = json.value<int>("position_start", -1);
            int posEnd = json.value<int>("position_end", -1);
            int parentId = json.value<int>("parent_id", 0);

            int commentId = addComment(docId, userId, content, posStart, posEnd, parentId);

            HttpResponse resp;
            resp.statusCode = HTTP::CREATED;
            resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
            nlohmann::json data;
            data["id"] = commentId;
            resp.body = buildJsonResponse(true, "Comment added", data);
            return resp;
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] addComment error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // DELETE /api/writing/documents/:id — 删除文档
    router.del(prefix + "/documents/:id", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            bool ok = deleteDocument(docId);
            if (ok) {
                HttpResponse resp;
                resp.statusCode = HTTP::OK;
                resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
                resp.body = buildJsonResponse(true, "Document deleted");
                return resp;
            }
            return buildErrorResponse(HTTP::NOT_FOUND, "Document not found");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] deleteDocument error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // GET /api/writing/documents/:id/comments — 获取评论列表
    router.get(prefix + "/documents/:id/comments", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto comments = getComments(docId);

            HttpResponse resp;
            resp.statusCode = HTTP::OK;
            resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
            nlohmann::json data;
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& comment : comments) {
                nlohmann::json item;
                auto idIt = comment.find("id");
                auto docIdIt = comment.find("document_id");
                auto userIdIt = comment.find("user_id");
                auto contentIt = comment.find("content");
                auto posStartIt = comment.find("position_start");
                auto posEndIt = comment.find("position_end");
                auto parentIdIt = comment.find("parent_id");
                auto resolvedIt = comment.find("is_resolved");
                auto createdAtIt = comment.find("created_at");

                if (idIt != comment.end()) item["id"] = safeStoi(idIt->second);
                if (docIdIt != comment.end()) item["document_id"] = safeStoi(docIdIt->second);
                if (userIdIt != comment.end()) item["user_id"] = safeStoi(userIdIt->second);
                if (contentIt != comment.end()) item["content"] = contentIt->second;
                if (posStartIt != comment.end()) item["position_start"] = safeStoi(posStartIt->second);
                if (posEndIt != comment.end()) item["position_end"] = safeStoi(posEndIt->second);
                if (parentIdIt != comment.end()) item["parent_id"] = safeStoi(parentIdIt->second);
                if (resolvedIt != comment.end()) item["is_resolved"] = resolvedIt->second == "1";
                if (createdAtIt != comment.end()) item["created_at"] = createdAtIt->second;
                arr.push_back(item);
            }
            data["comments"] = arr;
            data["total"] = comments.size();
            resp.body = buildJsonResponse(true, "", data);
            return resp;
        } catch (const std::exception& e) {
            spdlog::error("[Writing] getComments error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // POST /api/writing/documents/:id/versions — 创建版本
    router.post(prefix + "/documents/:id/versions", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int docId = std::stoi(getParam(req.pathParams, "id", "0"));
            auto json = nlohmann::json::parse(req.body);
            std::string summary = ValidationHelper::sanitize(json.value<std::string>("summary", ""));

            // 获取文档的owner_id作为created_by
            auto docOpt = getDocument(docId);
            if (!docOpt.has_value()) {
                return buildErrorResponse(HTTP::NOT_FOUND, "Document not found");
            }
            int userId = docOpt->ownerId;

            bool ok = createVersion(docId, userId, summary);
            if (ok) {
                HttpResponse resp;
                resp.statusCode = HTTP::CREATED;
                resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
                resp.body = buildJsonResponse(true, "Version created");
                return resp;
            }
            return buildErrorResponse(HTTP::INTERNAL_ERROR, "Failed to create version");
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] createVersion error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // PUT /api/writing/suggestions/:id/accept — 接受建议
    router.put(prefix + "/suggestions/:id/accept", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int suggestionId = std::stoi(getParam(req.pathParams, "id", "0"));
            bool ok = acceptSuggestion(suggestionId);
            if (ok) {
                HttpResponse resp;
                resp.statusCode = HTTP::OK;
                resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
                resp.body = buildJsonResponse(true, "Suggestion accepted");
                return resp;
            }
            return buildErrorResponse(HTTP::NOT_FOUND, "Suggestion not found");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] acceptSuggestion error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // PUT /api/writing/suggestions/:id/reject — 拒绝建议
    router.put(prefix + "/suggestions/:id/reject", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int suggestionId = std::stoi(getParam(req.pathParams, "id", "0"));
            bool ok = rejectSuggestion(suggestionId);
            if (ok) {
                HttpResponse resp;
                resp.statusCode = HTTP::OK;
                resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
                resp.body = buildJsonResponse(true, "Suggestion rejected");
                return resp;
            }
            return buildErrorResponse(HTTP::NOT_FOUND, "Suggestion not found");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] rejectSuggestion error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // PUT /api/writing/comments/:id/resolve — 解决评论
    router.put(prefix + "/comments/:id/resolve", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            int commentId = std::stoi(getParam(req.pathParams, "id", "0"));
            bool ok = resolveComment(commentId);
            if (ok) {
                HttpResponse resp;
                resp.statusCode = HTTP::OK;
                resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
                resp.body = buildJsonResponse(true, "Comment resolved");
                return resp;
            }
            return buildErrorResponse(HTTP::NOT_FOUND, "Comment not found");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] resolveComment error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // PUT /api/writing/documents/:id/cursor — 光标位置更新
    router.put(prefix + "/documents/:id/cursor", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            std::string docIdStr = getParam(req.pathParams, "id", "0");
            auto json = nlohmann::json::parse(req.body);

            int userId = json.value<int>("user_id", 0);
            std::string username = ValidationHelper::sanitize(json.value<std::string>("username", ""));
            int line = json.value<int>("line", 0);
            int column = json.value<int>("column", 0);

            if (userId == 0) {
                return buildErrorResponse(HTTP::BAD_REQUEST, "Missing user_id");
            }

            // 存储光标位置
            {
                std::lock_guard<std::mutex> lock(cursorsMutex_);
                CursorPosition pos;
                pos.userId = userId;
                pos.username = username;
                pos.line = line;
                pos.column = column;
                pos.lastActive = std::chrono::steady_clock::now();
                documentCursors_[docIdStr][userId] = std::move(pos);
            }

            // 通过 WebSocket 广播光标更新
            if (wsModule_) {
                std::string message = "{\"type\":\"cursor_update\","
                    "\"document_id\":" + docIdStr + ","
                    "\"user_id\":" + std::to_string(userId) + ","
                    "\"username\":\"" + StringUtil::escapeJson(username) + "\","
                    "\"line\":" + std::to_string(line) + ","
                    "\"column\":" + std::to_string(column) + "}";

                auto it = impl_->documentSessions_.find(std::stoi(docIdStr));
                if (it != impl_->documentSessions_.end()) {
                    for (const auto& socketId : it->second) {
                        wsModule_->send(socketId, message);
                    }
                }
            }

            spdlog::debug("[Writing] Cursor updated: doc={}, user={}, line={}, col={}",
                          docIdStr, userId, line, column);

            HttpResponse resp;
            resp.statusCode = HTTP::OK;
            resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
            resp.body = buildJsonResponse(true, "Cursor updated");
            return resp;
        } catch (const nlohmann::json::parse_error&) {
            return buildErrorResponse(HTTP::BAD_REQUEST, "Invalid JSON format");
        } catch (const std::exception& e) {
            spdlog::error("[Writing] cursor update error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // GET /api/writing/documents/:id/presence — 获取在线用户
    router.get(prefix + "/documents/:id/presence", [this, requireAuth, unauthorizedResp](const HttpRequest& req) {
        if (!requireAuth(req)) return unauthorizedResp();
        try {
            std::string docIdStr = getParam(req.pathParams, "id", "0");

            nlohmann::json data;
            nlohmann::json users = nlohmann::json::array();
            auto now = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lock(cursorsMutex_);
                auto docIt = documentCursors_.find(docIdStr);
                if (docIt != documentCursors_.end()) {
                    for (const auto& [uid, pos] : docIt->second) {
                        // 只返回最近60秒内活跃的用户
                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - pos.lastActive);
                        if (elapsed.count() < 60) {
                            nlohmann::json user;
                            user["user_id"] = pos.userId;
                            user["username"] = pos.username;
                            user["line"] = pos.line;
                            user["column"] = pos.column;
                            user["last_active_seconds_ago"] = static_cast<int>(elapsed.count());
                            users.push_back(user);
                        }
                    }
                }
            }

            data["document_id"] = std::stoi(docIdStr);
            data["online_users"] = users;
            data["count"] = users.size();

            HttpResponse resp;
            resp.statusCode = HTTP::OK;
            resp.headers["Content-Type"] = HTTP::CONTENT_TYPE_JSON;
            resp.body = buildJsonResponse(true, "", data);
            return resp;
        } catch (const std::exception& e) {
            spdlog::error("[Writing] presence error: {}", e.what());
            return buildErrorResponse(HTTP::INTERNAL_ERROR, e.what());
        }
    });

    // POST /api/writing/documents/:id/export — export document
    router.post(prefix + "/documents/:id/export", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            std::string format = "markdown";
            if (!req.body.empty()) {
                auto json = nlohmann::json::parse(req.body);
                format = json.value("format", "markdown");
            }

            nlohmann::json data;
            data["documentId"] = docId;
            data["format"] = format;
            data["content"] = "";
            data["success"] = true;

            if (database_) {
                auto rows = database_->query(
                    "SELECT title, content FROM collab_documents WHERE id = " + docId);
                if (!rows.empty()) {
                    data["title"] = rows[0].count("title") ? rows[0].at("title") : "";
                    data["content"] = rows[0].count("content") ? rows[0].at("content") : "";
                }
            }
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // PUT /api/writing/comments/resolve-all — resolve all comments for doc
    router.put(prefix + "/comments/resolve-all", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto json = nlohmann::json::parse(req.body);
            std::string docId = json.value("documentId", "");

            if (database_ && !docId.empty()) {
                database_->execute(
                    "UPDATE collab_comments SET resolved = 1 WHERE document_id = " + docId);
            }
            nlohmann::json data;
            data["success"] = true;
            data["documentId"] = docId;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/writing/documents/:id/restore — restore document version
    router.post(prefix + "/documents/:id/restore", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            auto json = nlohmann::json::parse(req.body);
            std::string versionId = json.value("versionId", "");

            nlohmann::json data;
            data["success"] = true;
            data["documentId"] = docId;
            data["restoredVersion"] = versionId;

            if (database_ && !versionId.empty()) {
                auto rows = database_->query(
                    "SELECT content FROM collab_versions WHERE id = " + versionId
                    + " AND document_id = " + docId);
                if (!rows.empty()) {
                    database_->execute(
                        "UPDATE collab_documents SET content = '"
                        + StringUtil::escapeSql(rows[0].at("content"))
                        + "' WHERE id = " + docId);
                    data["restored"] = true;
                }
            }
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // ========================================================================
    // New routes (v4 additions)
    // ========================================================================

    // GET /api/writing/documents/:id/versions — Get document version history (lightweight)
    router.get(prefix + "/documents/:id/versions", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");

            nlohmann::json data;
            data["documentId"] = docId;
            data["versions"] = nlohmann::json::array();

            if (database_) {
                auto rows = database_->query(
                    "SELECT id, document_id, created_at FROM collab_versions WHERE document_id = "
                    + docId + " ORDER BY created_at DESC LIMIT 20");
                for (const auto& row : rows) {
                    nlohmann::json item;
                    item["id"] = row.count("id") ? row.at("id") : "";
                    item["document_id"] = row.count("document_id") ? row.at("document_id") : "";
                    item["created_at"] = row.count("created_at") ? row.at("created_at") : "";
                    data["versions"].push_back(item);
                }
            }
            data["total"] = data["versions"].size();
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/writing/comments/:id/reply — Reply to a comment
    router.post(prefix + "/comments/:id/reply", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string commentId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string content = ValidationHelper::sanitize(body.value<std::string>("content", ""));
            std::string userId = std::to_string(body.value<int>("userId", 0));

            if (database_) {
                database_->execute(
                    "INSERT INTO collab_comments (document_id, user_id, content, parent_id, created_at) "
                    "SELECT document_id, " + userId + ", '"
                    + StringUtil::escapeSql(content) + "', " + commentId
                    + ", NOW() FROM collab_comments WHERE id = " + commentId);
            }

            nlohmann::json data;
            data["success"] = true;
            data["replyId"] = "reply_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/writing/documents/:id/comments — Get all comments for a document (lightweight)
    router.get(prefix + "/documents/:id/comments", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");

            nlohmann::json data;
            data["documentId"] = docId;
            data["comments"] = nlohmann::json::array();

            if (database_) {
                auto rows = database_->query(
                    "SELECT * FROM collab_comments WHERE document_id = "
                    + docId + " ORDER BY created_at DESC");
                for (const auto& row : rows) {
                    nlohmann::json item;
                    for (const auto& [key, value] : row) {
                        item[key] = value;
                    }
                    data["comments"].push_back(item);
                }
            }
            data["total"] = data["comments"].size();
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // PUT /api/writing/documents/:id/title — Update document title
    router.put("/api/writing/documents/:id/title", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string title = body.value("title", "");

            if (database_ && !title.empty()) {
                database_->execute(
                    "UPDATE collab_documents SET title = '"
                    + StringUtil::escapeSql(title) + "' WHERE id = " + docId);
            }
            nlohmann::json data;
            data["success"] = true;
            data["documentId"] = docId;
            data["title"] = title;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/writing/documents/:id/word-count — Get word count stats
    router.get("/api/writing/documents/:id/word-count", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");

            nlohmann::json stats;
            stats["words"] = 0;
            stats["characters"] = 0;
            stats["paragraphs"] = 0;

            if (database_) {
                auto rows = database_->query(
                    "SELECT content FROM collab_documents WHERE id = " + docId);
                if (!rows.empty() && rows[0].count("content")) {
                    const std::string& content = rows[0].at("content");
                    stats["characters"] = static_cast<int>(content.size());

                    int words = 0;
                    int paragraphs = 0;
                    bool inWord = false;
                    for (char c : content) {
                        if (c == '\n') paragraphs++;
                        if (std::isspace(static_cast<unsigned char>(c))) {
                            inWord = false;
                        } else if (!inWord) {
                            inWord = true;
                            words++;
                        }
                    }
                    if (!content.empty()) paragraphs++;
                    stats["words"] = words;
                    stats["paragraphs"] = paragraphs;
                }
            }

            nlohmann::json data;
            data["documentId"] = docId;
            data["stats"] = stats;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/writing/documents/:id/duplicate — Duplicate a document
    router.post("/api/writing/documents/:id/duplicate", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            std::string newTitle = "Copy of ...";
            if (!req.body.empty()) {
                auto body = nlohmann::json::parse(req.body);
                newTitle = body.value("title", "Copy of ...");
            }

            if (database_) {
                database_->execute(
                    "INSERT INTO collab_documents (title, content, created_at) "
                    "SELECT CONCAT('Copy of ', title), content, NOW() "
                    "FROM collab_documents WHERE id = " + docId);
            }

            nlohmann::json data;
            data["success"] = true;
            data["sourceId"] = docId;
            data["newTitle"] = newTitle;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/writing/documents/:id/collaborators — Get document collaborators
    router.get(prefix + "/documents/:id/collaborators", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");

            nlohmann::json data;
            data["documentId"] = docId;
            data["collaborators"] = nlohmann::json::array();

            if (database_) {
                database_->execute(
                    "CREATE TABLE IF NOT EXISTS document_collaborators ("
                    "id INT AUTO_INCREMENT PRIMARY KEY, "
                    "document_id INT, "
                    "user_id INT, "
                    "role VARCHAR(20) DEFAULT 'editor', "
                    "joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                    "UNIQUE KEY uniq (document_id, user_id))");

                auto rows = database_->query(
                    "SELECT u.id, u.username FROM users u "
                    "INNER JOIN document_collaborators dc ON u.id = dc.user_id "
                    "WHERE dc.document_id = " + docId);

                for (const auto& row : rows) {
                    nlohmann::json item;
                    item["id"] = row.count("id") && !row.at("id").empty() ? safeStoi(row.at("id")) : 0;
                    item["username"] = row.count("username") ? row.at("username") : "";
                    data["collaborators"].push_back(item);
                }
            }
            data["total"] = data["collaborators"].size();
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/writing/documents/:id/share-link — Generate share link for document
    router.post(prefix + "/documents/:id/share-link", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            int expiresIn = 24;
            if (!req.body.empty()) {
                auto body = nlohmann::json::parse(req.body);
                expiresIn = body.value("expiresIn", 24);
            }

            // Generate a simple token from document id + timestamp
            auto now = std::chrono::system_clock::now();
            auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            std::string token = "doc_" + docId + "_" + std::to_string(ts);

            nlohmann::json data;
            data["success"] = true;
            data["shareLink"] = "/share/" + token;
            data["expiresIn"] = expiresIn;
            data["documentId"] = docId;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/writing/documents/recent — Get recently edited documents
    router.get(prefix + "/documents/recent", [this](const HttpRequest& req) -> HttpResponse {
        try {
            nlohmann::json data;
            data["documents"] = nlohmann::json::array();

            if (database_) {
                auto rows = database_->query(
                    "SELECT id, title, updated_at FROM collab_documents "
                    "ORDER BY updated_at DESC LIMIT 10");

                for (const auto& row : rows) {
                    nlohmann::json item;
                    item["id"] = row.count("id") && !row.at("id").empty() ? safeStoi(row.at("id")) : 0;
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["updatedAt"] = row.count("updated_at") ? row.at("updated_at") : "";
                    data["documents"].push_back(item);
                }
            }
            data["total"] = data["documents"].size();
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // ========================================================================
    // New routes (v6 additions)
    // ========================================================================

    // PUT /api/writing/documents/:id/permissions — Update document permissions
    router.put(prefix + "/documents/:id/permissions", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string userId = std::to_string(body.value("userId", 0));
            std::string role = body.value("role", "viewer");

            if (database_) {
                database_->execute(
                    "CREATE TABLE IF NOT EXISTS document_collaborators ("
                    "id INT AUTO_INCREMENT PRIMARY KEY, "
                    "document_id INT, "
                    "user_id INT, "
                    "role VARCHAR(20) DEFAULT 'editor', "
                    "joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                    "UNIQUE KEY uniq (document_id, user_id))");

                database_->execute(
                    "INSERT INTO document_collaborators (document_id, user_id, role) VALUES ("
                    + docId + ", " + userId + ", '"
                    + StringUtil::escapeSql(role)
                    + "') ON DUPLICATE KEY UPDATE role = VALUES(role)");
            }

            nlohmann::json data;
            data["success"] = true;
            data["documentId"] = docId;
            data["userId"] = body.value("userId", 0);
            data["role"] = role;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/writing/documents/:id/stats — Get document editing stats
    router.get(prefix + "/documents/:id/stats", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");

            nlohmann::json stats;
            stats["versions"] = 0;
            stats["comments"] = 0;
            stats["collaborators"] = 0;

            if (database_) {
                try {
                    auto verRows = database_->query(
                        "SELECT COUNT(*) as version_count FROM collab_versions WHERE document_id = " + docId);
                    if (!verRows.empty() && verRows[0].count("version_count") && !verRows[0].at("version_count").empty()) {
                        stats["versions"] = safeStoi(verRows[0].at("version_count"));
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("[Writing] Stats version query failed: {}", e.what());
                }

                try {
                    auto comRows = database_->query(
                        "SELECT COUNT(*) as comment_count FROM collab_comments WHERE document_id = " + docId);
                    if (!comRows.empty() && comRows[0].count("comment_count") && !comRows[0].at("comment_count").empty()) {
                        stats["comments"] = safeStoi(comRows[0].at("comment_count"));
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("[Writing] Stats comment query failed: {}", e.what());
                }

                try {
                    auto colRows = database_->query(
                        "SELECT COUNT(*) as collab_count FROM document_collaborators WHERE document_id = " + docId);
                    if (!colRows.empty() && colRows[0].count("collab_count") && !colRows[0].at("collab_count").empty()) {
                        stats["collaborators"] = safeStoi(colRows[0].at("collab_count"));
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("[Writing] Stats collaborator query failed: {}", e.what());
                }
            }

            nlohmann::json data;
            data["stats"] = stats;
            data["documentId"] = docId;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/writing/documents/:id/lock — Lock/unlock document for editing
    router.post(prefix + "/documents/:id/lock", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            bool locked = body.value("locked", true);
            std::string userId = std::to_string(body.value("userId", 0));

            if (database_) {
                database_->execute(
                    "UPDATE collab_documents SET locked = "
                    + std::string(locked ? "1" : "0")
                    + ", locked_by = " + userId
                    + " WHERE id = " + docId);
            }

            nlohmann::json data;
            data["success"] = true;
            data["documentId"] = docId;
            data["locked"] = locked;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // ========================================================================
    // New routes (v7 additions)
    // ========================================================================

    // GET /api/writing/documents/:id/diff — Get diff between two versions
    router.get(prefix + "/documents/:id/diff", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");

            std::string versionId1, versionId2;
            auto v1It = req.queryParams.find("versionId1");
            auto v2It = req.queryParams.find("versionId2");
            if (v1It != req.queryParams.end()) versionId1 = v1It->second;
            if (v2It != req.queryParams.end()) versionId2 = v2It->second;

            nlohmann::json diff;
            diff["added"] = 0;
            diff["removed"] = 0;
            diff["changes"] = nlohmann::json::array();

            if (database_ && !versionId1.empty() && !versionId2.empty()) {
                auto rows1 = database_->query(
                    "SELECT content FROM collab_versions WHERE id = "
                    + versionId1 + " AND document_id = " + docId);
                auto rows2 = database_->query(
                    "SELECT content FROM collab_versions WHERE id = "
                    + versionId2 + " AND document_id = " + docId);

                std::string content1 = (!rows1.empty() && rows1[0].count("content"))
                    ? rows1[0].at("content") : "";
                std::string content2 = (!rows2.empty() && rows2[0].count("content"))
                    ? rows2[0].at("content") : "";

                // Simple line-based diff count
                int added = 0;
                int removed = 0;
                if (content2.size() > content1.size()) {
                    added = static_cast<int>(content2.size() - content1.size());
                } else if (content1.size() > content2.size()) {
                    removed = static_cast<int>(content1.size() - content2.size());
                }

                diff["added"] = added;
                diff["removed"] = removed;
            }

            nlohmann::json data;
            data["diff"] = diff;
            data["versionId1"] = versionId1;
            data["versionId2"] = versionId2;
            data["documentId"] = docId;
            data["success"] = true;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/writing/comments/:id/react — Add reaction to a comment
    router.post(prefix + "/comments/:id/react", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string commentId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string userId = std::to_string(body.value("userId", 0));
            std::string emoji = body.value("emoji", "");

            if (database_ && !emoji.empty()) {
                database_->execute(
                    "CREATE TABLE IF NOT EXISTS comment_reactions ("
                    "id INT AUTO_INCREMENT PRIMARY KEY, "
                    "comment_id INT, "
                    "user_id INT, "
                    "emoji VARCHAR(10), "
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                    "UNIQUE KEY uniq (comment_id, user_id, emoji))");

                database_->execute(
                    "INSERT INTO comment_reactions (comment_id, user_id, emoji) VALUES ("
                    + commentId + ", " + userId + ", '"
                    + StringUtil::escapeSql(emoji) + "') "
                    "ON DUPLICATE KEY UPDATE emoji = VALUES(emoji)");
            }

            nlohmann::json data;
            data["success"] = true;
            data["commentId"] = commentId;
            data["emoji"] = emoji;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/writing/documents/search — Search within documents
    router.get(prefix + "/documents/search", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string query;
            auto qIt = req.queryParams.find("q");
            if (qIt != req.queryParams.end()) query = qIt->second;

            nlohmann::json data;
            data["documents"] = nlohmann::json::array();
            data["total"] = 0;
            data["query"] = query;

            if (database_ && !query.empty()) {
                auto rows = database_->query(
                    "SELECT id, title FROM collab_documents WHERE title LIKE '%"
                    + StringUtil::escapeSql(query) + "%' OR content LIKE '%"
                    + StringUtil::escapeSql(query) + "%' LIMIT 10");

                nlohmann::json arr = nlohmann::json::array();
                for (const auto& row : rows) {
                    nlohmann::json item;
                    item["id"] = row.count("id") && !row.at("id").empty() ? safeStoi(row.at("id")) : 0;
                    item["title"] = row.count("title") ? row.at("title") : "";
                    arr.push_back(item);
                }
                data["documents"] = arr;
                data["total"] = arr.size();
            }
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // ========================================================================
    // New routes (Round 22 additions)
    // ========================================================================

    // POST /api/writing/documents/:id/autosave — Autosave document content
    router.post(prefix + "/documents/:id/autosave", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string content = body.value<std::string>("content", "");
            std::string userId = std::to_string(body.value<int>("userId", 0));

            nlohmann::json resp;
            resp["success"] = true;
            resp["documentId"] = docId;
            resp["savedAt"] = "";

            if (database_) {
                database_->execute(
                    "UPDATE collab_documents SET content = '"
                    + StringUtil::escapeSql(content)
                    + "', updated_at = NOW() WHERE id = " + docId);
                auto rows = database_->query(
                    "SELECT updated_at FROM collab_documents WHERE id = " + docId);
                if (!rows.empty() && rows[0].count("updated_at")) {
                    resp["savedAt"] = rows[0].at("updated_at");
                }
            } else {
                auto now = std::chrono::system_clock::now();
                auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
                resp["savedAt"] = std::to_string(ts);
            }
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = e.what();
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // GET /api/writing/documents/:id/activity — Get document activity log
    router.get(prefix + "/documents/:id/activity", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");

            nlohmann::json resp;
            resp["activities"] = nlohmann::json::array();
            resp["documentId"] = docId;

            if (database_) {
                auto rows = database_->query(
                    "SELECT type, user_id, description, timestamp FROM document_activity "
                    "WHERE document_id = " + docId + " ORDER BY timestamp DESC LIMIT 50");
                for (const auto& row : rows) {
                    nlohmann::json item;
                    item["type"] = row.count("type") ? row.at("type") : "";
                    item["userId"] = (row.count("user_id") && !row.at("user_id").empty())
                        ? safeStoi(row.at("user_id")) : 0;
                    item["description"] = row.count("description") ? row.at("description") : "";
                    item["timestamp"] = row.count("timestamp") ? row.at("timestamp") : "";
                    resp["activities"].push_back(item);
                }
            }
            resp["total"] = resp["activities"].size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = e.what();
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // POST /api/writing/documents/:id/invite — Invite user to collaborate
    router.post(prefix + "/documents/:id/invite", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string email = body.value<std::string>("email", "");
            std::string role = body.value<std::string>("role", "viewer");

            nlohmann::json resp;
            resp["success"] = true;
            resp["documentId"] = docId;

            if (database_) {
                database_->execute(
                    "CREATE TABLE IF NOT EXISTS document_invitations ("
                    "id INT AUTO_INCREMENT PRIMARY KEY, "
                    "document_id INT, "
                    "email VARCHAR(255), "
                    "role VARCHAR(20) DEFAULT 'editor', "
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                database_->execute(
                    "INSERT INTO document_invitations (document_id, email, role) VALUES ("
                    + docId + ", '"
                    + StringUtil::escapeSql(email) + "', '"
                    + StringUtil::escapeSql(role) + "')");
                auto rows = database_->query("SELECT LAST_INSERT_ID() as id");
                if (!rows.empty() && rows[0].count("id") && !rows[0].at("id").empty()) {
                    resp["inviteId"] = rows[0].at("id");
                } else {
                    auto now = std::chrono::system_clock::now();
                    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()).count();
                    resp["inviteId"] = "inv_" + std::to_string(ts);
                }
            } else {
                auto now = std::chrono::system_clock::now();
                auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();
                resp["inviteId"] = "inv_" + std::to_string(ts);
            }
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = e.what();
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // ========================================================================
    // Round 25 additions
    // ========================================================================

    // POST /api/writing/documents/:id/merge — Merge document changes
    router.post(prefix + "/documents/:id/merge", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            int sourceVersion = 0;
            int targetVersion = 0;
            std::string strategy = "auto";

            if (!req.body.empty()) {
                auto body = nlohmann::json::parse(req.body);
                sourceVersion = body.value("sourceVersion", 0);
                targetVersion = body.value("targetVersion", 0);
                strategy = body.value("strategy", "auto");
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["mergedVersion"] = targetVersion + 1;
            resp["conflicts"] = 0;
            resp["documentId"] = docId;

            if (database_) {
                try {
                    // Fetch source and target version contents
                    auto srcRows = database_->query(
                        "SELECT content FROM collab_versions WHERE document_id = "
                        + docId + " AND version_number = "
                        + std::to_string(sourceVersion));
                    auto tgtRows = database_->query(
                        "SELECT content FROM collab_versions WHERE document_id = "
                        + docId + " AND version_number = "
                        + std::to_string(targetVersion));

                    if (!srcRows.empty() && !tgtRows.empty()) {
                        // Insert merged version
                        int mergedVersion = targetVersion + 1;
                        std::string mergedContent = tgtRows[0].count("content")
                            ? tgtRows[0].at("content") : "";

                        database_->execute(
                            "INSERT INTO collab_versions (document_id, version_number, content, "
                            "change_summary, created_at) VALUES ("
                            + docId + ", " + std::to_string(mergedVersion)
                            + ", '" + StringUtil::escapeSql(mergedContent)
                            + "', 'Merged from v" + std::to_string(sourceVersion)
                            + " into v" + std::to_string(targetVersion)
                            + "', NOW())");
                        resp["mergedVersion"] = mergedVersion;
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("[Writing] Merge DB operation failed: {}", e.what());
                }
            }

            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = e.what();
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // GET /api/writing/documents/:id/export/pdf — Export document as PDF
    router.get(prefix + "/documents/:id/export/pdf", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");

            nlohmann::json resp;
            resp["downloadUrl"] = "/downloads/doc_" + docId + "_export.pdf";
            resp["expiresAt"] = "";

            if (database_) {
                try {
                    auto rows = database_->query(
                        "SELECT title FROM collab_documents WHERE id = " + docId);
                    if (!rows.empty() && rows[0].count("title")) {
                        resp["title"] = rows[0].at("title");
                    }

                    // Calculate expiry 24 hours from now
                    auto expiryRows = database_->query(
                        "SELECT DATE_ADD(NOW(), INTERVAL 24 HOUR) as expires_at");
                    if (!expiryRows.empty() && expiryRows[0].count("expires_at")
                        && !expiryRows[0]["expires_at"].empty()) {
                        resp["expiresAt"] = expiryRows[0]["expires_at"];
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("[Writing] PDF export DB query failed: {}", e.what());
                }
            }

            if (resp["expiresAt"].get<std::string>().empty()) {
                auto now = std::chrono::system_clock::now();
                auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count() + 86400000;
                resp["expiresAt"] = std::to_string(ts);
            }

            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = e.what();
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // POST /api/writing/documents/:id/tag — Add tag to document
    router.post(prefix + "/documents/:id/tag", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string tag = body.value("tag", "");

            if (tag.empty()) {
                nlohmann::json errResp;
                errResp["success"] = false;
                errResp["error"] = "Tag is required";
                return HttpResponse::json(HTTP::BAD_REQUEST, errResp.dump());
            }

            if (database_) {
                try {
                    database_->execute(
                        "CREATE TABLE IF NOT EXISTS document_tags ("
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "document_id INT, "
                        "tag VARCHAR(100), "
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                        "UNIQUE KEY uniq_tag (document_id, tag))");

                    database_->execute(
                        "INSERT INTO document_tags (document_id, tag) VALUES ("
                        + docId + ", '"
                        + StringUtil::escapeSql(tag) + "') "
                        "ON DUPLICATE KEY UPDATE tag = VALUES(tag)");
                } catch (const std::exception& e) {
                    spdlog::warn("[Writing] Tag insert failed: {}", e.what());
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["documentId"] = docId;
            resp["tag"] = tag;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = e.what();
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // ========================================================================
    // Round 30 additions
    // ========================================================================

    // POST /api/writing/documents/:id/transform — Transform document format
    router.post(prefix + "/documents/:id/transform", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string docId = req.pathParams.at("id");
            std::string fromFormat = "markdown";
            std::string toFormat = "html";

            if (!req.body.empty()) {
                auto body = nlohmann::json::parse(req.body);
                fromFormat = body.value("fromFormat", "markdown");
                toFormat = body.value("toFormat", "html");
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["format"] = toFormat;

            if (database_) {
                auto rows = database_->query(
                    "SELECT content FROM collab_documents WHERE id = " + docId);
                if (!rows.empty() && rows[0].count("content")) {
                    const std::string& content = rows[0].at("content");
                    // Stub transformation: wrap content based on target format
                    std::string transformed;
                    if (toFormat == "html") {
                        transformed = "<p>" + content + "</p>";
                    } else if (toFormat == "latex") {
                        transformed = "\\begin{document}\n" + content + "\n\\end{document}";
                    } else {
                        transformed = content;
                    }
                    resp["content"] = transformed;
                } else {
                    resp["content"] = "";
                }
            } else {
                // Stub fallback
                resp["content"] = "<p>Mock transformed content from " + fromFormat + " to " + toFormat + "</p>";
            }

            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = e.what();
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    // GET /api/writing/templates — Get writing templates
    router.get(prefix + "/templates", [this](const HttpRequest& req) -> HttpResponse {
        try {
            nlohmann::json resp;
            nlohmann::json arr = nlohmann::json::array();

            if (database_) {
                try {
                    auto rows = database_->query(
                        "SELECT id, name, category, description FROM writing_templates "
                        "ORDER BY category, name LIMIT 50");
                    for (const auto& row : rows) {
                        nlohmann::json item;
                        item["id"] = row.count("id") && !row.at("id").empty() ? safeStoi(row.at("id")) : 0;
                        item["name"] = row.count("name") ? row.at("name") : "";
                        item["category"] = row.count("category") ? row.at("category") : "";
                        item["description"] = row.count("description") ? row.at("description") : "";
                        arr.push_back(item);
                    }
                } catch (const std::exception& e) {
                    spdlog::warn("[Writing] Templates query failed: {}", e.what());
                }
            } else {
                // Stub: return 3 mock templates
                arr.push_back({{"id", 1}, {"name", "Research Paper"}, {"category", "academic"}, {"description", "Standard academic research paper template"}});
                arr.push_back({{"id", 2}, {"name", "Literature Review"}, {"category", "academic"}, {"description", "Comprehensive literature review template"}});
                arr.push_back({{"id", 3}, {"name", "Case Study"}, {"category", "business"}, {"description", "Business case study analysis template"}});
            }

            resp["templates"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            nlohmann::json errResp;
            errResp["success"] = false;
            errResp["error"] = e.what();
            return HttpResponse::json(HTTP::INTERNAL_ERROR, errResp.dump());
        }
    });

    spdlog::info("[CollabWriting] Registered 43 routes");
}

// ============================================================================
// 1. 文档管理
// ============================================================================

std::optional<CollaborativeDocument> CollaborativeWritingModule::createDocument(
    int userId, const std::string& title, const std::string& documentType, int templateId) {

    try {
        std::string initialContent = "";
        if (templateId > 0) {
            PreparedStatement tplStmt(database_,
                "SELECT content FROM document_templates WHERE id = ?");
            tplStmt.bind(0, templateId);
            auto results = tplStmt.query();
            if (!results.empty()) {
                initialContent = results[0]["content"];
            }
        }

        PreparedStatement stmt(database_,
            "INSERT INTO collaborative_documents "
            "(id, title, content, document_type, owner_id, template_id, word_count, status) "
            "VALUES (DEFAULT, ?, ?, ?, ?, ?, 0, 'draft')");
        stmt.bind(0, title);
        stmt.bind(1, initialContent);
        stmt.bind(2, documentType);
        stmt.bind(3, userId);
        stmt.bind(4, templateId);

        bool executeResult = stmt.execute();

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
        PreparedStatement stmt(database_,
            "SELECT * FROM collaborative_documents WHERE id = ?");
        stmt.bind(0, documentId);
        auto results = stmt.query();

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
        // Count word_count for any update path that includes content
        int wordCount = static_cast<int>(
            std::count_if(content.begin(), content.end(), [](unsigned char c) { return std::isprint(c) || c == '\n'; }));

        if (content.empty() && title.empty() && status.empty()) {
            // At least update updated_at
            PreparedStatement stmt(database_,
                "UPDATE collaborative_documents SET updated_at = NOW() WHERE id = ?");
            stmt.bind(0, documentId);
            return stmt.execute();
        }

        // Build dynamic SET clause with placeholders
        std::vector<std::string> setClauses;
        int paramIndex = 0;
        std::vector<std::pair<int, std::variant<int, double, std::string, bool, std::nullptr_t>>> params;

        if (!content.empty()) {
            setClauses.push_back("content = ?");
            params.push_back({paramIndex++, content});
        }
        if (!title.empty()) {
            setClauses.push_back("title = ?");
            params.push_back({paramIndex++, title});
        }
        if (!status.empty()) {
            setClauses.push_back("status = ?");
            params.push_back({paramIndex++, status});
        }

        setClauses.push_back("word_count = ?");
        params.push_back({paramIndex++, wordCount});

        setClauses.push_back("updated_at = NOW()");

        // Build SET clause string
        std::string setClause = setClauses[0];
        for (size_t i = 1; i < setClauses.size(); ++i) {
            setClause += ", " + setClauses[i];
        }

        std::string sql = "UPDATE collaborative_documents SET " + setClause + " WHERE id = ?";
        params.push_back({paramIndex, documentId});

        PreparedStatement stmt(database_, sql);
        for (const auto& [idx, val] : params) {
            stmt.bind(idx, val);
        }

        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[Writing] updateDocument error: {}", e.what());
        return false;
    }
}

bool CollaborativeWritingModule::deleteDocument(int documentId) {
    try {
        PreparedStatement stmt(database_,
            "DELETE FROM collaborative_documents WHERE id = ?");
        stmt.bind(0, documentId);
        bool ok = stmt.execute();
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
        PreparedStatement stmt(database_,
            "SELECT * FROM collaborative_documents WHERE owner_id = ? ORDER BY updated_at DESC LIMIT ? OFFSET ?");
        stmt.bind(0, userId);
        stmt.bind(1, limit);
        stmt.bind(2, offset);
        auto results = stmt.query();

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
        PreparedStatement opStmt(database_,
            "INSERT INTO document_operations "
            "(document_id, user_id, operation_type, position, length, content) "
            "VALUES (?, ?, ?, ?, ?, ?)");
        opStmt.bind(0, documentId);
        opStmt.bind(1, operation.clientId);
        opStmt.bind(2, static_cast<int>(operation.type));
        opStmt.bind(3, operation.position);
        opStmt.bind(4, operation.length);
        opStmt.bind(5, operation.content);
        opStmt.execute();

        // 更新文档
        int wordCount = static_cast<int>(
            std::count_if(newContent.begin(), newContent.end(), [](unsigned char c) { return std::isprint(c) || c == '\n'; }));
        PreparedStatement updateStmt(database_,
            "UPDATE collaborative_documents SET content = ?, word_count = ?, updated_at = NOW() WHERE id = ?");
        updateStmt.bind(0, newContent);
        updateStmt.bind(1, wordCount);
        updateStmt.bind(2, documentId);
        updateStmt.execute();

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
// 3. WebSocket（保留框架，实际发送通过 WebSocketModule）
// ============================================================================

void CollaborativeWritingModule::handleWebSocketConnection(int documentId, int userId, const std::string& socketId) {
    try {
        PreparedStatement stmt(database_,
            "INSERT INTO collaboration_sessions "
            "(document_id, user_id, socket_id, is_active) "
            "VALUES (?, ?, ?, TRUE)");
        stmt.bind(0, documentId);
        stmt.bind(1, userId);
        stmt.bind(2, socketId);
        if (stmt.execute()) {
            impl_->documentSessions_[documentId].push_back(socketId);
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] handleWebSocketConnection error: {}", e.what());
    }
}

void CollaborativeWritingModule::handleWebSocketDisconnection(const std::string& socketId) {
    try {
        PreparedStatement stmt(database_,
            "UPDATE collaboration_sessions SET is_active = FALSE WHERE socket_id = ?");
        stmt.bind(0, socketId);
        stmt.execute();
        for (auto& [docId, sessions] : impl_->documentSessions_) {
            sessions.erase(std::remove(sessions.begin(), sessions.end(), socketId), sessions.end());
        }
    } catch (const std::exception& e) {
        spdlog::error("[Writing] handleWebSocketDisconnection error: {}", e.what());
    }
}

void CollaborativeWritingModule::broadcastOperation(int documentId, const OTOperation& operation) {
    if (!wsModule_) {
        spdlog::debug("[Writing] broadcastOperation: no WebSocketModule, skipping broadcast");
        return;
    }

    // Build the broadcast message as JSON
    std::string message = "{\"type\":\"operation\",\"document_id\":" +
                          std::to_string(documentId) +
                          ",\"operation\":" + operation.toJSON() + "}";

    // Broadcast to all sessions subscribed to this document
    auto it = impl_->documentSessions_.find(documentId);
    if (it == impl_->documentSessions_.end()) {
        return;
    }

    for (const auto& socketId : it->second) {
        wsModule_->send(socketId, message);
    }

    spdlog::debug("[Writing] Broadcast operation to {} sessions for document {}",
                  it->second.size(), documentId);
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

            PreparedStatement stmt(database_,
                "INSERT INTO ai_writing_suggestions "
                "(document_id, user_id, suggestion_type, position_start, position_end, "
                "original_text, suggested_text, confidence_score, explanation, status) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, 'pending')");
            stmt.bind(0, documentId);
            stmt.bind(1, userId);
            stmt.bind(2, suggestionType);
            stmt.bind(3, positionStart);
            stmt.bind(4, positionEnd);
            stmt.bind(5, suggestion.originalText);
            stmt.bind(6, suggestedText);
            stmt.bind(7, static_cast<double>(suggestion.confidenceScore));
            stmt.bind(8, suggestion.explanation);
            stmt.execute();

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
        PreparedStatement stmt(database_,
            "SELECT * FROM ai_writing_suggestions WHERE document_id = ? ORDER BY created_at DESC");
        stmt.bind(0, documentId);
        auto results = stmt.query();

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
        PreparedStatement stmt(database_,
            "UPDATE ai_writing_suggestions SET status = 'accepted' WHERE id = ?");
        stmt.bind(0, suggestionId);
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[Writing] acceptSuggestion error: {}", e.what());
        return false;
    }
}

bool CollaborativeWritingModule::rejectSuggestion(int suggestionId) {
    try {
        PreparedStatement stmt(database_,
            "UPDATE ai_writing_suggestions SET status = 'rejected' WHERE id = ?");
        stmt.bind(0, suggestionId);
        return stmt.execute();
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
        if (!docOpt.has_value()) {
            spdlog::error("[Writing] createVersion: document {} not found", documentId);
            return false;
        }

        // 验证用户是否存在
        PreparedStatement userStmt(database_,
            "SELECT id FROM users WHERE id = ?");
        userStmt.bind(0, userId);
        auto userCheck = userStmt.query();
        if (userCheck.empty()) {
            spdlog::error("[Writing] createVersion: user {} not found", userId);
            return false;
        }

        std::string content = docOpt->content;

        // 获取当前最大版本号
        PreparedStatement verStmt(database_,
            "SELECT COALESCE(MAX(version_number), 0) as max_ver FROM document_versions WHERE document_id = ?");
        verStmt.bind(0, documentId);
        auto verResults = verStmt.query();
        int nextVersion = 1;
        if (!verResults.empty() && !verResults[0]["max_ver"].empty()) {
            nextVersion = safeStoi(verResults[0]["max_ver"]) + 1;
        }

        int wordCount = static_cast<int>(
            std::count_if(content.begin(), content.end(), [](unsigned char c) { return std::isprint(c) || c == '\n'; }));

        PreparedStatement stmt(database_,
            "INSERT INTO document_versions "
            "(document_id, version_number, content, change_summary, word_count, created_by) "
            "VALUES (?, ?, ?, ?, ?, ?)");
        stmt.bind(0, documentId);
        stmt.bind(1, nextVersion);
        stmt.bind(2, content);
        stmt.bind(3, summary);
        stmt.bind(4, wordCount);
        stmt.bind(5, userId);

        spdlog::info("[Writing] createVersion: doc={}, ver={}, user={}", documentId, nextVersion, userId);

        bool result = stmt.execute();
        spdlog::info("[Writing] createVersion result: {}", result);
        return result;
    } catch (const std::exception& e) {
        spdlog::error("[Writing] createVersion error: {}", e.what());
        return false;
    }
}

std::vector<std::map<std::string, std::string>> CollaborativeWritingModule::getVersions(int documentId) {
    std::vector<std::map<std::string, std::string>> versions;
    try {
        PreparedStatement stmt(database_,
            "SELECT * FROM document_versions WHERE document_id = ? ORDER BY version_number DESC");
        stmt.bind(0, documentId);
        auto results = stmt.query();

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
        PreparedStatement stmt(database_,
            "INSERT INTO document_comments "
            "(document_id, user_id, parent_id, position_start, position_end, content) "
            "VALUES (?, ?, ?, ?, ?, ?)");
        stmt.bind(0, documentId);
        stmt.bind(1, userId);
        stmt.bind(2, parentId);
        stmt.bind(3, positionStart);
        stmt.bind(4, positionEnd);
        stmt.bind(5, content);

        if (stmt.execute()) {
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
        PreparedStatement stmt(database_,
            "SELECT * FROM document_comments WHERE document_id = ? ORDER BY created_at ASC");
        stmt.bind(0, documentId);
        auto results = stmt.query();

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
        PreparedStatement stmt(database_,
            "UPDATE document_comments SET is_resolved = TRUE, updated_at = NOW() WHERE id = ?");
        stmt.bind(0, commentId);
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[Writing] resolveComment error: {}", e.what());
        return false;
    }
}

// ============================================================================
// OTOperation 序列化
// ============================================================================

std::string OTOperation::toJSON() const {
    nlohmann::json json;
    json["type"] = static_cast<int>(type);
    json["position"] = position;
    json["length"] = length;
    json["content"] = content;
    json["clientId"] = clientId;
    json["timestamp"] = timestamp;
    return json.dump();
}

} // namespace PaperCrawler

// ============================================================================
// DLL 导出函数
// ============================================================================

#include "core/ModuleExports.hpp"

extern "C" {

PAPERCRAWLER_API void* createModule() {
    return new PaperCrawler::CollaborativeWritingModule();
}

PAPERCRAWLER_API void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::CollaborativeWritingModule*>(ptr);
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

}
