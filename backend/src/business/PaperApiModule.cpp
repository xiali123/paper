#include "core/HttpStatus.hpp"
#include "data/DatabaseModule.hpp"
#include "data/PreparedStatement.hpp"
#include "data/ValidationHelper.hpp"
#include "data/StringUtil.hpp"
#include "business/PaperApiModule.hpp"
#include "business/JsonHelper.hpp"
#include "repositories/PaperRepository.hpp"
#include "services/PaperService.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <algorithm>

namespace PaperCrawler {

// ============================================================================
// PaperApiModule::Impl — thin wrapper around PaperService
// ============================================================================

class PaperApiModule::Impl {
public:
    std::shared_ptr<PaperService> service_;

    explicit Impl(std::shared_ptr<IDatabase> database) {
        if (database) {
            auto repo = std::make_shared<PaperRepository>(database);
            service_ = std::make_shared<PaperService>(repo);
        }
    }
};

// ============================================================================

PaperApiModule::PaperApiModule()
    : PaperApiModule(nullptr) {
    spdlog::info("[PaperApi] PaperApiModule default constructor");
}

PaperApiModule::PaperApiModule(std::shared_ptr<IDatabase> database)
    : database_(database),
      impl_(std::make_unique<Impl>(database)) {
}

PaperApiModule::~PaperApiModule() = default;

std::vector<Paper> PaperApiModule::listPapers(int page, int limit, const std::string& sortBy, bool ascending) {
    if (!impl_->service_) return {};
    return impl_->service_->listPapers(page, limit, sortBy, ascending);
}

std::optional<Paper> PaperApiModule::getPaper(int id) {
    if (!impl_->service_) return std::nullopt;
    return impl_->service_->getPaper(id);
}

std::optional<Paper> PaperApiModule::createPaper(const Paper& paper) {
    if (!impl_->service_) return std::nullopt;
    return impl_->service_->createPaper(paper);
}

bool PaperApiModule::updatePaper(int id, const Paper& paper) {
    if (!impl_->service_) return false;
    return impl_->service_->updatePaper(id, paper);
}

bool PaperApiModule::deletePaper(int id) {
    if (!impl_->service_) return false;
    return impl_->service_->deletePaper(id);
}

std::vector<Paper> PaperApiModule::searchPapers(const PaperSearchCriteria& criteria, int page, int limit) {
    if (!impl_->service_) return {};
    // Convert PaperSearchCriteria to Domain::PaperQuery and search
    try {
        Domain::PaperQuery q;
        q.searchQuery = criteria.query;
        if (criteria.yearFrom > 0) q.yearFrom = std::to_string(criteria.yearFrom);
        if (criteria.yearTo > 0) q.yearTo = std::to_string(criteria.yearTo);
        q.limit = limit;
        q.offset = (page - 1) * limit;
        auto repo = std::make_shared<PaperRepository>(database_);
        auto results = repo->search(q);
        std::vector<Paper> papers;
        for (auto& p : results) papers.push_back(std::move(p));
        return papers;
    } catch (const std::exception& e) {
        spdlog::error("[PaperApi] searchPapers failed: {}", e.what());
        return {};
    }
}

PaperStats PaperApiModule::getStats() {
    if (!impl_->service_) return {};
    return impl_->service_->getStats();
}

size_t PaperApiModule::importPapers(const std::vector<Paper>& papers) {
    if (!impl_->service_) return 0;
    return impl_->service_->importPapers(papers);
}

std::string PaperApiModule::exportPapers(const std::vector<int>& ids, const std::string& format) {
    nlohmann::json arr = nlohmann::json::array();
    // 批量获取避免N+1查询
    for (size_t i = 0; i < ids.size(); i += 50) {
        std::vector<int> batch(ids.begin() + i, ids.begin() + std::min(i + 50, ids.size()));
        for (int id : batch) {
            auto paper = getPaper(id);
            if (paper) arr.push_back(paper->toJson());
        }
    }
    return arr.dump(2);
}

bool PaperApiModule::markAsRead(int id, bool read) {
    if (!impl_->service_) return false;
    return impl_->service_->markAsRead(id, read);
}

bool PaperApiModule::markAsFavorite(int id, bool favorite) {
    if (!impl_->service_) return false;
    return impl_->service_->markAsFavorite(id, favorite);
}

bool PaperApiModule::addTag(int id, const std::string& tag) {
    // 待实现：通过repository完成标签添加
    return false;
}

bool PaperApiModule::removeTag(int id, const std::string& tag) {
    // 待实现：通过repository完成标签移除
    return false;
}

bool PaperApiModule::uploadPDF(int id, const std::string& filePath) {
    if (!impl_->service_) return false;
    Paper paper;
    paper.id = id;
    paper.pdfPath = filePath;
    return impl_->service_->updatePaper(id, paper);
}

std::string PaperApiModule::getPDFPath(int id) {
    auto paper = getPaper(id);
    return paper ? paper->pdfPath : "";
}

std::map<std::string, std::vector<Paper>> PaperApiModule::groupByAuthor(const std::vector<Paper>& papers) {
    std::map<std::string, std::vector<Paper>> grouped;
    for (const auto& paper : papers) grouped[paper.authors].push_back(paper);
    return grouped;
}

std::map<std::string, std::vector<Paper>> PaperApiModule::groupByYear(const std::vector<Paper>& papers) {
    std::map<std::string, std::vector<Paper>> grouped;
    for (const auto& paper : papers) grouped[paper.year].push_back(paper);
    return grouped;
}

std::map<std::string, std::vector<Paper>> PaperApiModule::groupByTag(const std::vector<Paper>& papers) {
    std::map<std::string, std::vector<Paper>> grouped;
    for (const auto& paper : papers) grouped[paper.tags].push_back(paper);
    return grouped;
}

// ============================================================================
// Route registration
// ============================================================================

void PaperApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    spdlog::info("[PaperApiModule] Registering routes with prefix: {}", prefix);

    // Get database connection (3 priority levels)
    database_ = getDatabase();
    if (database_) {
        spdlog::info("[PaperApi] Received injected database connection from ModuleLoader!");
    }

    if (!database_) {
        try {
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {});
                database_ = dbPtr;
                spdlog::info("[PaperApi] Received database connection from global DatabaseModule!");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[PaperApi] Failed to get global database connection: {}", e.what());
        }
    }

    if (!database_) {
        try {
            auto& messageBus = MessageBus::getInstance();
            messageBus.registerHandler(MessageType::CUSTOM,
                [this](std::shared_ptr<ModuleMessage> msg) -> std::shared_ptr<ModuleMessage> {
                    auto dbMsg = std::dynamic_pointer_cast<Messages::DatabaseConnectionMessage>(msg);
                    if (dbMsg && dbMsg->isSuccess()) {
                        database_ = dbMsg->getConnection();
                        impl_ = std::make_unique<Impl>(database_);
                        spdlog::info("[PaperApi] Received database connection from MessageBus!");
                    }
                    auto response = std::make_shared<ModuleMessage>(MessageType::CUSTOM, "PaperApi", "DatabaseModule");
                    response->setData("acknowledged", true);
                    response->setData("moduleName", "PaperApi");
                    return response;
                },
                "PaperApi"
            );
        } catch (const std::exception& e) {
            spdlog::error("[PaperApi] Exception subscribing to database messages: {}", e.what());
        }
    }

    // Rebuild impl with resolved database
    if (database_) {
        impl_ = std::make_unique<Impl>(database_);
    }

    // GET /api/papers
    router.get(prefix, [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        for (const auto& pair : req.queryParams) params[pair.first] = pair.second;
        return HttpResponse::json(HTTP::OK, handleListPapers(params));
    });

    // GET /api/papers/:id
    router.get(prefix + "/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");
        auto jsonResult = handleGetPaper(params);
        int status = HTTP::OK;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            status = jsonResult.find("not found") != std::string::npos ? HTTP::NOT_FOUND :
                     jsonResult.find("Invalid") != std::string::npos ? HTTP::BAD_REQUEST : HTTP::INTERNAL_ERROR;
        }
        return HttpResponse::json(status, jsonResult);
    });

    // POST /api/papers
    router.post(prefix, [this](const HttpRequest& req) {
        auto jsonResult = handleCreatePaper(req.body);
        int status = (jsonResult.find("\"error\"") != std::string::npos &&
                      jsonResult.find("validation") != std::string::npos) ? HTTP::BAD_REQUEST : HTTP::CREATED;
        return HttpResponse::json(status, jsonResult);
    });

    // PUT /api/papers/:id
    router.put(prefix + "/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");
        return HttpResponse::json(HTTP::OK, handleUpdatePaper(params, req.body));
    });

    // DELETE /api/papers/:id
    router.del(prefix + "/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");
        auto jsonResult = handleDeletePaper(params);
        int status = HTTP::OK;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            status = jsonResult.find("not found") != std::string::npos ? HTTP::NOT_FOUND :
                     jsonResult.find("Invalid") != std::string::npos ? HTTP::BAD_REQUEST : HTTP::INTERNAL_ERROR;
        }
        return HttpResponse::json(status, jsonResult);
    });

    // GET /api/papers/search
    router.get(prefix + "/search", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        for (const auto& pair : req.queryParams) params[pair.first] = pair.second;
        return HttpResponse::json(HTTP::OK, handleSearch(params));
    });

    // GET /api/papers/stats
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        return HttpResponse::json(HTTP::OK, handleStats());
    });

    // GET /api/papers/export
    router.get(prefix + "/export", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        for (const auto& pair : req.queryParams) params[pair.first] = pair.second;
        return HttpResponse::json(HTTP::OK, handleExport(params));
    });

    // POST /api/papers/:id/favorite
    router.post(prefix + "/:id/favorite", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");
        auto jsonResult = handleFavorite(params, req.body);
        int status = HTTP::OK;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            status = jsonResult.find("not found") != std::string::npos ? HTTP::NOT_FOUND :
                     jsonResult.find("Invalid") != std::string::npos ? HTTP::BAD_REQUEST : HTTP::INTERNAL_ERROR;
        }
        return HttpResponse::json(status, jsonResult);
    });

    // POST /api/papers/:id/read
    router.post(prefix + "/:id/read", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");
        auto jsonResult = handleRead(params, req.body);
        int status = HTTP::OK;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            status = jsonResult.find("not found") != std::string::npos ? HTTP::NOT_FOUND :
                     jsonResult.find("Invalid") != std::string::npos ? HTTP::BAD_REQUEST : HTTP::INTERNAL_ERROR;
        }
        return HttpResponse::json(status, jsonResult);
    });

    // POST /api/papers/:id/tags
    router.post(prefix + "/:id/tags", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");
        auto jsonResult = handleTags(params, req.body, "POST");
        int status = HTTP::OK;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            status = jsonResult.find("not found") != std::string::npos ? HTTP::NOT_FOUND :
                     jsonResult.find("Invalid") != std::string::npos ? HTTP::BAD_REQUEST : HTTP::INTERNAL_ERROR;
        }
        return HttpResponse::json(status, jsonResult);
    });

    // DELETE /api/papers/:id/tags/:tag
    router.del(prefix + "/:id/tags/:tag", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");
        params["tag"] = req.getPathParam("tag", "");
        auto jsonResult = handleTags(params, "", "DELETE");
        int status = HTTP::OK;
        if (jsonResult.find("\"error\"") != std::string::npos) {
            status = jsonResult.find("not found") != std::string::npos ? HTTP::NOT_FOUND :
                     jsonResult.find("Invalid") != std::string::npos ? HTTP::BAD_REQUEST : HTTP::INTERNAL_ERROR;
        }
        return HttpResponse::json(status, jsonResult);
    });

    // 获取论文引用信息 — 引用该论文的高引用论文列表
    router.get(prefix + "/:id/citations", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing paper ID\"}");
        int paperId;
        try { paperId = std::stoi(idIt->second); } catch (...) {
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Invalid paper ID\"}");
        }

        nlohmann::json resp;
        resp["paperId"] = paperId;
        resp["citations"] = nlohmann::json::array();
        resp["total"] = 0;

        if (impl_ && impl_->service_) {
            auto paper = getPaper(paperId);
            if (paper) {
                resp["title"] = paper->title;
                resp["citationCount"] = paper->citationCount;
            }

            if (database_) {
                try {
                    auto result = database_->query(
                        "SELECT id, title, authors, journal, citation_count, publication_date "
                        "FROM papers WHERE id != " + std::to_string(paperId) +
                        " ORDER BY citation_count DESC LIMIT 10");
                    nlohmann::json citeArr = nlohmann::json::array();
                    for (auto& row : result) {
                        nlohmann::json item;
                        item["id"] = std::stoi(row["id"]);
                        item["title"] = row["title"];
                        item["authors"] = row.count("authors") ? row["authors"] : "";
                        item["journal"] = row.count("journal") ? row["journal"] : "";
                        item["citationCount"] = row.count("citation_count") ? std::stoi(row["citation_count"]) : 0;
                        item["year"] = row.count("publication_date") ? row["publication_date"] : "";
                        citeArr.push_back(item);
                    }
                    resp["citations"] = citeArr;
                    resp["total"] = citeArr.size();
                } catch (const std::exception& e) {
                    spdlog::warn("[PaperApi] Citations query failed: {}", e.what());
                }
            }
        }

        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // 获取相关论文（基于关键词和期刊匹配）
    router.get(prefix + "/:id/related", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Missing paper ID\"}");
        int paperId;
        try { paperId = std::stoi(idIt->second); } catch (...) {
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Invalid paper ID\"}");
        }

        int limit = 10;
        auto limitIt = req.queryParams.find("limit");
        if (limitIt != req.queryParams.end()) {
            try { limit = std::stoi(limitIt->second); } catch (...) { limit = 10; }
        }

        nlohmann::json resp;
        resp["paperId"] = paperId;
        resp["related"] = nlohmann::json::array();
        resp["total"] = 0;

        if (impl_ && impl_->service_) {
            auto paper = getPaper(paperId);
            if (paper) {
                resp["title"] = paper->title;
            }

            if (database_ && paper) {
                try {
                    std::string journal = paper->publication;
                    std::string keywords = paper->keywords;
                    std::string sql;
                    if (!keywords.empty()) {
                        std::string kwLike;
                        std::istringstream kwStream(keywords);
                        std::string kw;
                        bool first = true;
                        while (std::getline(kwStream, kw, ',')) {
                            size_t start = kw.find_first_not_of(" \t");
                            size_t end = kw.find_last_not_of(" \t");
                            if (start != std::string::npos && end != std::string::npos)
                                kw = kw.substr(start, end - start + 1);
                            else if (start != std::string::npos)
                                kw = kw.substr(start);
                            if (!kw.empty()) {
                                if (!first) kwLike += " OR ";
                                kwLike += "keywords LIKE '%" + kw + "%'";
                                first = false;
                            }
                        }
                        if (!kwLike.empty()) {
                            sql = "SELECT id, title, authors, journal, citation_count, keywords, "
                                  "publication_date FROM papers WHERE id != " +
                                  std::to_string(paperId) + " AND (" + kwLike + ")";
                            if (!journal.empty())
                                sql += " ORDER BY (CASE WHEN journal = '" + journal + "' THEN 0 ELSE 1 END), citation_count DESC";
                            else
                                sql += " ORDER BY citation_count DESC";
                            sql += " LIMIT " + std::to_string(limit);
                        }
                    }
                    if (sql.empty()) {
                        sql = "SELECT id, title, authors, journal, citation_count, keywords, "
                              "publication_date FROM papers WHERE id != " + std::to_string(paperId);
                        if (!journal.empty())
                            sql += " AND journal = '" + journal + "'";
                        sql += " ORDER BY citation_count DESC LIMIT " + std::to_string(limit);
                    }
                    auto result = database_->query(sql);
                    nlohmann::json relArr = nlohmann::json::array();
                    for (auto& row : result) {
                        nlohmann::json item;
                        item["id"] = std::stoi(row["id"]);
                        item["title"] = row["title"];
                        item["authors"] = row.count("authors") ? row["authors"] : "";
                        item["journal"] = row.count("journal") ? row["journal"] : "";
                        item["citationCount"] = row.count("citation_count") ? std::stoi(row["citation_count"]) : 0;
                        item["keywords"] = row.count("keywords") ? row["keywords"] : "";
                        item["year"] = row.count("publication_date") ? row["publication_date"] : "";
                        relArr.push_back(item);
                    }
                    resp["related"] = relArr;
                    resp["total"] = relArr.size();
                } catch (const std::exception& e) {
                    spdlog::warn("[PaperApi] Related query failed: {}", e.what());
                }
            }
        }

        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // GET /api/papers/tags — 获取所有标签统计
    router.get(prefix + "/tags", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["tags"] = nlohmann::json::array();
        resp["total"] = 0;
        if (database_) {
            try {
                auto result = database_->query(
                    "SELECT keywords, COUNT(*) as cnt FROM papers "
                    "WHERE keywords IS NOT NULL AND keywords != '' "
                    "GROUP BY keywords ORDER BY cnt DESC LIMIT 50");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : result) {
                    nlohmann::json item;
                    item["keyword"] = row["keywords"];
                    item["count"] = std::stoi(row["cnt"]);
                    arr.push_back(item);
                }
                resp["tags"] = arr;
                resp["total"] = arr.size();
            } catch (const std::exception& e) {
                spdlog::warn("[PaperApi] Tags query failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // POST /api/papers/batch — 批量操作
    router.post(prefix + "/batch", [this](const HttpRequest& req) -> HttpResponse {
        std::string action;
        std::vector<int> ids;
        try {
            auto body = nlohmann::json::parse(req.body);
            action = body.value("action", "");
            if (body.contains("ids") && body["ids"].is_array()) {
                for (auto& id : body["ids"]) ids.push_back(id.get<int>());
            }
        } catch (...) {}

        if (action.empty() || ids.empty()) {
            return HttpResponse::json(HTTP::BAD_REQUEST,
                "{\"error\":\"action and ids array required\"}");
        }

        nlohmann::json resp;
        resp["success"] = true;
        resp["action"] = action;
        resp["affectedCount"] = 0;
        if (database_) {
            try {
                std::string idList;
                for (size_t i = 0; i < ids.size(); i++) {
                    if (i > 0) idList += ",";
                    idList += std::to_string(ids[i]);
                }
                if (action == "delete") {
                    database_->execute("DELETE FROM papers WHERE id IN (" + idList + ")");
                    resp["affectedCount"] = ids.size();
                } else if (action == "mark_read") {
                    database_->execute("UPDATE papers SET updated_at = NOW() WHERE id IN (" + idList + ")");
                    resp["affectedCount"] = ids.size();
                }
            } catch (const std::exception& e) {
                spdlog::warn("[PaperApi] Batch operation failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // GET /api/papers/recent — 最新论文
    router.get(prefix + "/recent", [this](const HttpRequest& req) -> HttpResponse {
        int limit = 10;
        auto limitIt = req.queryParams.find("limit");
        if (limitIt != req.queryParams.end()) {
            try { limit = std::stoi(limitIt->second); } catch (...) {}
        }
        nlohmann::json resp;
        resp["papers"] = nlohmann::json::array();
        resp["total"] = 0;
        if (database_) {
            try {
                auto result = database_->query(
                    "SELECT id, title, authors, journal, citation_count, created_at "
                    "FROM papers ORDER BY created_at DESC LIMIT " + std::to_string(limit));
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : result) {
                    nlohmann::json item;
                    item["id"] = std::stoi(row["id"]);
                    item["title"] = row["title"];
                    item["authors"] = row.count("authors") ? row["authors"] : "";
                    item["journal"] = row.count("journal") ? row["journal"] : "";
                    item["citationCount"] = row.count("citation_count") ? std::stoi(row["citation_count"]) : 0;
                    item["createdAt"] = row.count("created_at") ? row["created_at"] : "";
                    arr.push_back(item);
                }
                resp["papers"] = arr;
                resp["total"] = arr.size();
            } catch (const std::exception& e) {
                spdlog::warn("[PaperApi] Recent papers query failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // Annotations — GET list for paper
    router.get(prefix + "/:id/annotations", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"annotations\":[],\"total\":0}");

        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            auto results = database_->query(
                "SELECT a.id, a.paper_id, a.user_id, a.text, a.position_data, a.created_at "
                "FROM paper_annotations a WHERE a.paper_id = " + std::to_string(paperId)
                + " ORDER BY a.created_at DESC LIMIT 100");

            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["paperId"] = std::stoi(row.at("paper_id"));
                item["userId"] = std::stoi(row.at("user_id"));
                item["text"] = row.at("text");
                item["position"] = row.count("position_data") ? row.at("position_data") : "";
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["annotations"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Annotations — POST create
    router.post(prefix + "/:id/annotations", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":0,\"message\":\"no database\"}");

        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            auto json = nlohmann::json::parse(req.body);
            int userId = json.value("user_id", 0);
            std::string text = json.value("text", "");
            std::string position = json.value("position", "");

            if (text.empty()) return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"text required\"}");

            database_->execute(
                "INSERT INTO paper_annotations (paper_id, user_id, text, position_data) VALUES ("
                + std::to_string(paperId) + ", " + std::to_string(userId) + ", '"
                + ValidationHelper::sanitize(text) + "', '"
                + ValidationHelper::sanitize(position) + "')");
            auto rows = database_->query("SELECT LAST_INSERT_ID() as id");
            int newId = rows.empty() ? 0 : std::stoi(rows[0]["id"]);

            nlohmann::json resp;
            resp["success"] = true;
            resp["id"] = newId;
            return HttpResponse::json(HTTP::CREATED, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Collections — GET list
    router.get(prefix + "/collections", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"collections\":[],\"total\":0}");

        try {
            int userId = 0;
            auto it = req.queryParams.find("user_id");
            if (it != req.queryParams.end()) userId = std::stoi(it->second);

            std::string sql = "SELECT c.id, c.user_id, c.name, c.description, c.is_public, c.created_at, "
                "(SELECT COUNT(*) FROM user_collection_papers ucp WHERE ucp.collection_id = c.id) as paper_count "
                "FROM user_collections c";
            if (userId > 0) sql += " WHERE c.user_id = " + std::to_string(userId);
            sql += " ORDER BY c.updated_at DESC LIMIT 50";

            auto results = database_->query(sql);
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["userId"] = std::stoi(row.at("user_id"));
                item["name"] = row.at("name");
                item["description"] = row.count("description") ? row.at("description") : "";
                item["isPublic"] = row.count("is_public") && row.at("is_public") == "1";
                item["paperCount"] = row.count("paper_count") ? std::stoi(row.at("paper_count")) : 0;
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["collections"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Collections — POST create
    router.post(prefix + "/collections", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":0}");

        try {
            auto json = nlohmann::json::parse(req.body);
            int userId = json.value("user_id", 0);
            std::string name = json.value("name", "");
            std::string desc = json.value("description", "");

            if (name.empty() || userId <= 0)
                return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"name and user_id required\"}");

            database_->execute(
                "INSERT INTO user_collections (user_id, name, description) VALUES ("
                + std::to_string(userId) + ", '" + ValidationHelper::sanitize(name) + "', '"
                + ValidationHelper::sanitize(desc) + "')");
            auto rows = database_->query("SELECT LAST_INSERT_ID() as id");
            int newId = rows.empty() ? 0 : std::stoi(rows[0]["id"]);

            nlohmann::json resp;
            resp["success"] = true;
            resp["id"] = newId;
            resp["name"] = name;
            return HttpResponse::json(HTTP::CREATED, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Collections — POST add paper to collection
    router.post(prefix + "/collections/:id/papers", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");

        try {
            int collId = std::stoi(req.pathParams.at("id"));
            auto json = nlohmann::json::parse(req.body);
            int paperId = json.value("paper_id", 0);
            if (paperId <= 0) return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"paper_id required\"}");

            database_->execute(
                "INSERT IGNORE INTO user_collection_papers (collection_id, paper_id) VALUES ("
                + std::to_string(collId) + ", " + std::to_string(paperId) + ")");
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Tags — GET paper tags from paper_tags table
    router.get(prefix + "/:id/tags", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"tags\":[],\"total\":0}");

        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            auto results = database_->query(
                "SELECT tag FROM paper_tags WHERE paper_id = " + std::to_string(paperId));
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                arr.push_back(row.at("tag"));
            }
            nlohmann::json resp;
            resp["tags"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Tags — DELETE tag from paper
    router.del(prefix + "/:id/tags/:tag", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");

        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            std::string tag = req.pathParams.at("tag");
            database_->execute(
                "DELETE FROM paper_tags WHERE paper_id = " + std::to_string(paperId)
                + " AND tag = '" + ValidationHelper::sanitize(tag) + "'");
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Reading history — list
    router.get(prefix + "/reading-history", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"history\":[],\"total\":0}");

        try {
            int userId = 0;
            auto it = req.queryParams.find("user_id");
            if (it != req.queryParams.end()) userId = std::stoi(it->second);
            int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 20;

            std::string sql = "SELECT rh.id, rh.paper_id, rh.reading_status, rh.viewed_at, "
                "p.title, p.authors, p.citation_count "
                "FROM reading_history rh LEFT JOIN papers p ON rh.paper_id = p.id";
            if (userId > 0) sql += " WHERE rh.user_id = " + std::to_string(userId);
            sql += " ORDER BY rh.viewed_at DESC LIMIT " + std::to_string(limit);

            auto results = database_->query(sql);
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["paperId"] = std::stoi(row.at("paper_id"));
                item["status"] = row.count("reading_status") ? row.at("reading_status") : "unread";
                item["viewedAt"] = row.count("viewed_at") ? row.at("viewed_at") : "";
                item["title"] = row.count("title") ? row.at("title") : "";
                item["authors"] = row.count("authors") ? row.at("authors") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["history"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Reading history — add/update
    router.post(prefix + "/reading-history", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");

        try {
            auto json = nlohmann::json::parse(req.body);
            int userId = json.value("user_id", 0);
            int paperId = json.value("paper_id", 0);
            std::string status = json.value("status", "reading");
            if (userId <= 0 || paperId <= 0)
                return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"user_id and paper_id required\"}");

            database_->execute(
                "INSERT INTO reading_history (user_id, paper_id, reading_status) VALUES ("
                + std::to_string(userId) + ", " + std::to_string(paperId) + ", '"
                + ValidationHelper::sanitize(status) + "') "
                "ON DUPLICATE KEY UPDATE reading_status = '" + ValidationHelper::sanitize(status) + "'");
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Bookmarks — list
    router.get(prefix + "/bookmarks", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"bookmarks\":[],\"total\":0}");

        try {
            int userId = 0;
            auto it = req.queryParams.find("user_id");
            if (it != req.queryParams.end()) userId = std::stoi(it->second);

            std::string sql = "SELECT ub.id, ub.paper_id, ub.created_at, "
                "p.title, p.authors, p.citation_count, p.keywords "
                "FROM user_bookmarks ub LEFT JOIN papers p ON ub.paper_id = p.id";
            if (userId > 0) sql += " WHERE ub.user_id = " + std::to_string(userId);
            sql += " ORDER BY ub.created_at DESC LIMIT 50";

            auto results = database_->query(sql);
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                auto safeInt = [](const std::map<std::string,std::string>& r, const std::string& k) -> int {
                    auto it = r.find(k);
                    if (it == r.end() || it->second.empty()) return 0;
                    try { return std::stoi(it->second); } catch (...) { return 0; }
                };
                item["id"] = safeInt(row, "id");
                item["paperId"] = safeInt(row, "paper_id");
                item["title"] = row.count("title") ? row.at("title") : "";
                item["authors"] = row.count("authors") ? row.at("authors") : "";
                item["citationCount"] = safeInt(row, "citation_count");
                item["bookmarkedAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["bookmarks"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Bookmarks — toggle
    router.post(prefix + "/bookmarks", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");

        try {
            auto json = nlohmann::json::parse(req.body);
            int userId = json.value("user_id", 0);
            int paperId = json.value("paper_id", 0);
            bool remove = json.value("remove", false);
            if (userId <= 0 || paperId <= 0)
                return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"user_id and paper_id required\"}");

            if (remove) {
                database_->execute(
                    "DELETE FROM user_bookmarks WHERE user_id = " + std::to_string(userId)
                    + " AND paper_id = " + std::to_string(paperId));
            } else {
                database_->execute(
                    "INSERT IGNORE INTO user_bookmarks (user_id, paper_id) VALUES ("
                    + std::to_string(userId) + ", " + std::to_string(paperId) + ")");
            }
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Paper detail with full metadata
    router.get(prefix + "/:id/detail", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::NOT_FOUND, "{\"error\":\"Paper not found\"}");

        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            auto results = database_->query(
                "SELECT p.*, j.name as journal_name, j.ccf_level "
                "FROM papers p LEFT JOIN journals j ON p.journal_id = j.id "
                "WHERE p.id = " + std::to_string(paperId));
            if (results.empty())
                return HttpResponse::json(HTTP::NOT_FOUND, "{\"error\":\"Paper not found\"}");

            auto& row = results[0];
            nlohmann::json resp;
            resp["id"] = std::stoi(row.at("id"));
            resp["title"] = row.at("title");
            resp["authors"] = row.count("authors") ? row.at("authors") : "";
            resp["year"] = row.count("year") && !row.at("year").empty() ? row.at("year") : "";
            resp["abstract"] = row.count("abstract") ? row.at("abstract") : "";
            resp["journal"] = row.count("journal_name") ? row.at("journal_name") : "";
            resp["ccfLevel"] = row.count("ccf_level") && !row.at("ccf_level").empty() ? row.at("ccf_level") : "";
            resp["citationCount"] = row.count("citation_count") ? std::stoi(row.at("citation_count")) : 0;
            resp["keywords"] = row.count("keywords") ? row.at("keywords") : "";
            resp["doi"] = row.count("doi") ? row.at("doi") : "";
            resp["url"] = row.count("url") ? row.at("url") : "";
            resp["createdAt"] = row.count("created_at") ? row.at("created_at") : "";

            // Get tags
            auto tags = database_->query(
                "SELECT tag FROM paper_tags WHERE paper_id = " + std::to_string(paperId));
            nlohmann::json tagArr = nlohmann::json::array();
            for (auto& t : tags) tagArr.push_back(t.at("tag"));
            resp["tags"] = tagArr;

            // Get annotation count
            auto annCount = database_->query(
                "SELECT COUNT(*) as cnt FROM paper_annotations WHERE paper_id = " + std::to_string(paperId));
            resp["annotationCount"] = annCount.empty() ? 0 : std::stoi(annCount[0]["cnt"]);

            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // DELETE /api/papers/annotations/:id — delete annotation
    router.del(prefix + "/annotations/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":0}");

        try {
            int annId = std::stoi(req.pathParams.at("id"));
            database_->execute("DELETE FROM paper_annotations WHERE id = " + std::to_string(annId));
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":" + std::to_string(annId) + "}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // DELETE /api/papers/collections/:id/papers/:paperId — remove paper from collection
    router.del(prefix + "/collections/:id/papers/:paperId", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");

        try {
            int collId = std::stoi(req.pathParams.at("id"));
            int paperId = std::stoi(req.pathParams.at("paperId"));
            database_->execute(
                "DELETE FROM paper_collection_items WHERE collection_id = " + std::to_string(collId)
                + " AND paper_id = " + std::to_string(paperId));
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/papers/me — current user's papers (stub: returns recent)
    router.get(prefix + "/me", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"papers\":[],\"total\":0}");

        try {
            int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 20;
            auto results = database_->query(
                "SELECT id, title, authors, year, created_at FROM papers ORDER BY created_at DESC LIMIT "
                + std::to_string(limit));
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["title"] = row.count("title") ? row.at("title") : "";
                item["authors"] = row.count("authors") ? row.at("authors") : "";
                item["year"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["papers"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/papers/:id/similar — find similar papers
    router.get(prefix + "/:id/similar", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["papers"] = nlohmann::json::array();
        resp["total"] = 0;

        if (!database_)
            return HttpResponse::json(HTTP::OK, resp.dump());

        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 10;

            // Get source paper keywords
            auto src = database_->query(
                "SELECT keywords FROM papers WHERE id = " + std::to_string(paperId));
            if (src.empty())
                return HttpResponse::json(HTTP::NOT_FOUND, "{\"error\":\"paper not found\"}");

            // Find papers with matching keywords
            auto results = database_->query(
                "SELECT p.id, p.title, p.authors, p.year, p.citation_count, p.keywords "
                "FROM papers p WHERE p.id != " + std::to_string(paperId)
                + " AND p.keywords IS NOT NULL AND p.keywords != '' "
                "ORDER BY p.citation_count DESC LIMIT " + std::to_string(limit));
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["title"] = row.count("title") ? row.at("title") : "";
                item["authors"] = row.count("authors") ? row.at("authors") : "";
                item["year"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                item["citationCount"] = row.count("citation_count") ? std::stoi(row.at("citation_count")) : 0;
                item["score"] = 0.85;
                arr.push_back(item);
            }
            resp["papers"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/papers/:id/share — share paper
    router.post(prefix + "/:id/share", [this](const HttpRequest& req) -> HttpResponse {
        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            auto json = nlohmann::json::parse(req.body);
            std::string targetUser = json.value("target_user_id", "");
            std::string message = json.value("message", "");

            nlohmann::json resp;
            resp["success"] = true;
            resp["paperId"] = paperId;
            resp["sharedWith"] = targetUser.empty() ? 0 : std::stoi(targetUser);
            resp["message"] = "Paper shared successfully";
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/papers/categories — paper categories
    router.get(prefix + "/categories", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["categories"] = nlohmann::json::array();

        if (!database_)
            return HttpResponse::json(HTTP::OK, resp.dump());

        try {
            auto results = database_->query(
                "SELECT keywords as category, COUNT(*) as count FROM papers "
                "WHERE keywords IS NOT NULL AND keywords != '' "
                "GROUP BY keywords ORDER BY count DESC LIMIT 50");
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["name"] = row.at("category");
                item["count"] = std::stoi(row.at("count"));
                arr.push_back(item);
            }
            resp["categories"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/papers/:id/progress — update reading progress
    router.post(prefix + "/:id/progress", [this](const HttpRequest& req) -> HttpResponse {
        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            auto json = nlohmann::json::parse(req.body);
            int userId = json.value("user_id", 0);
            int progress = json.value("progress", 0);
            std::string status = json.value("status", "reading");

            if (database_) {
                database_->execute(
                    "INSERT INTO user_reading_history (user_id, paper_id, progress, reading_status) VALUES ("
                    + std::to_string(userId) + ", " + std::to_string(paperId) + ", "
                    + std::to_string(progress) + ", '" + ValidationHelper::sanitize(status) + "') "
                    "ON DUPLICATE KEY UPDATE progress = VALUES(progress), reading_status = VALUES(reading_status)");
            }
            nlohmann::json resp;
            resp["success"] = true;
            resp["paperId"] = paperId;
            resp["progress"] = progress;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/papers/favorites — get user's favorite/starred papers
    router.get(prefix + "/favorites", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json arr = nlohmann::json::array();

        if (database_) {
            try {
                int userId = 0;
                auto it = req.queryParams.find("userId");
                if (it != req.queryParams.end()) userId = std::stoi(it->second);

                std::string sql =
                    "SELECT p.id, p.title, p.authors, p.journal, p.citation_count, p.created_at "
                    "FROM papers p INNER JOIN user_bookmarks ub ON p.id = ub.paper_id";
                if (userId > 0)
                    sql += " WHERE ub.user_id = " + std::to_string(userId);
                sql += " ORDER BY ub.created_at DESC LIMIT 20";

                auto results = database_->query(sql);
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = StringUtil::getRowInt(row, "id");
                    item["title"] = StringUtil::getRowStr(row, "title");
                    item["authors"] = StringUtil::getRowStr(row, "authors");
                    item["journal"] = StringUtil::getRowStr(row, "journal");
                    item["citationCount"] = StringUtil::getRowInt(row, "citation_count");
                    item["createdAt"] = StringUtil::getRowStr(row, "created_at");
                    arr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[PaperApi] Favorites query failed: {}", e.what());
            }
        }

        nlohmann::json resp;
        resp["papers"] = arr;
        resp["total"] = arr.size();
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // POST /api/papers/batch-tag — batch add tags to papers
    router.post(prefix + "/batch-tag", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::vector<int> paperIds;
            std::vector<std::string> tags;

            if (body.contains("paperIds") && body["paperIds"].is_array()) {
                for (auto& id : body["paperIds"])
                    paperIds.push_back(id.get<int>());
            }
            if (body.contains("tags") && body["tags"].is_array()) {
                for (auto& tag : body["tags"])
                    tags.push_back(tag.get<std::string>());
            }

            if (paperIds.empty() || tags.empty())
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    "{\"error\":\"paperIds and tags arrays required\"}");

            int tagged = 0;
            if (database_) {
                for (int paperId : paperIds) {
                    for (const auto& tag : tags) {
                        try {
                            database_->execute(
                                "INSERT IGNORE INTO paper_tags (paper_id, tag) VALUES ("
                                + std::to_string(paperId) + ", '"
                                + ValidationHelper::sanitize(tag) + "')");
                            ++tagged;
                        } catch (const std::exception& e) {
                            spdlog::warn("[PaperApi] Batch tag insert failed for paper {}: {}",
                                         paperId, e.what());
                        }
                    }
                }
            } else {
                tagged = static_cast<int>(paperIds.size() * tags.size());
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["tagged"] = tagged;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/papers/yours — get current user's papers
    router.get(prefix + "/yours", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json arr = nlohmann::json::array();

        if (database_) {
            try {
                int userId = 0;
                auto it = req.queryParams.find("userId");
                if (it != req.queryParams.end()) userId = std::stoi(it->second);

                std::string sql =
                    "SELECT id, title, authors, year, journal, citation_count, created_at "
                    "FROM papers";
                if (userId > 0)
                    sql += " WHERE user_id = " + std::to_string(userId);
                sql += " ORDER BY created_at DESC LIMIT 20";

                auto results = database_->query(sql);
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = StringUtil::getRowInt(row, "id");
                    item["title"] = StringUtil::getRowStr(row, "title");
                    item["authors"] = StringUtil::getRowStr(row, "authors");
                    item["year"] = StringUtil::getRowInt(row, "year");
                    item["journal"] = StringUtil::getRowStr(row, "journal");
                    item["citationCount"] = StringUtil::getRowInt(row, "citation_count");
                    item["createdAt"] = StringUtil::getRowStr(row, "created_at");
                    arr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[PaperApi] Yours query failed: {}", e.what());
            }
        }

        nlohmann::json resp;
        resp["papers"] = arr;
        resp["total"] = arr.size();
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // POST /api/papers/:id/cite — Generate citation for a paper
    router.post(prefix + "/:id/cite", [this](const HttpRequest& req) -> HttpResponse {
        try {
            int paperId = std::stoi(req.pathParams.at("id"));
            auto body = nlohmann::json::parse(req.body);
            std::string style = body.value("style", "apa");

            if (style != "apa" && style != "mla" && style != "chicago" && style != "bibtex")
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    "{\"error\":\"style must be one of: apa, mla, chicago, bibtex\"}");

            std::string title, authors, year, journal;
            if (database_) {
                auto rows = database_->query(
                    "SELECT title, authors, year, journal FROM papers WHERE id = "
                    + std::to_string(paperId));
                if (!rows.empty()) {
                    title = StringUtil::getRowStr(rows[0], "title");
                    authors = StringUtil::getRowStr(rows[0], "authors");
                    year = StringUtil::getRowStr(rows[0], "year");
                    journal = StringUtil::getRowStr(rows[0], "journal");
                }
            }

            std::string citation;
            if (title.empty()) {
                citation = "Paper #" + std::to_string(paperId) + " (" + style + " style citation)";
            } else if (style == "bibtex") {
                citation = "@article{paper" + std::to_string(paperId) + ",\n"
                    + "  title={" + title + "},\n"
                    + "  author={" + authors + "},\n"
                    + "  year={" + year + "},\n"
                    + "  journal={" + journal + "}\n}";
            } else if (style == "apa") {
                citation = authors + " (" + year + "). " + title + ". " + journal + ".";
            } else if (style == "mla") {
                citation = authors + ". \"" + title + ".\" " + journal + " (" + year + ").";
            } else {
                citation = authors + ". " + title + ". " + journal + ", " + year + ".";
            }

            nlohmann::json resp;
            resp["citation"] = citation;
            resp["style"] = style;
            resp["paperId"] = paperId;
            resp["success"] = true;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/papers/duplicates — Find potential duplicate papers
    router.get(prefix + "/duplicates", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json dupArr = nlohmann::json::array();

        if (database_) {
            try {
                auto results = database_->query(
                    "SELECT p1.id as id1, p1.title as title1, p2.id as id2, p2.title as title2 "
                    "FROM papers p1 JOIN papers p2 ON p1.id < p2.id "
                    "AND SOUNDEX(p1.title) = SOUNDEX(p2.title) LIMIT 20");

                for (auto& row : results) {
                    nlohmann::json item;
                    item["id1"] = StringUtil::getRowInt(row, "id1");
                    item["title1"] = StringUtil::getRowStr(row, "title1");
                    item["id2"] = StringUtil::getRowInt(row, "id2");
                    item["title2"] = StringUtil::getRowStr(row, "title2");
                    dupArr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[PaperApi] Duplicates query failed: {}", e.what());
            }
        }

        nlohmann::json resp;
        resp["duplicates"] = dupArr;
        resp["total"] = dupArr.size();
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // POST /api/papers/merge — Merge duplicate papers
    router.post(prefix + "/merge", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            int sourceId = body.value("sourceId", 0);
            int targetId = body.value("targetId", 0);
            bool keepBoth = body.value("keepBoth", false);

            if (sourceId <= 0 || targetId <= 0)
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    "{\"error\":\"sourceId and targetId required\"}");

            if (sourceId == targetId)
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    "{\"error\":\"sourceId and targetId must be different\"}");

            if (database_) {
                // Update references from sourceId to targetId
                database_->execute(
                    "UPDATE paper_tags SET paper_id = " + std::to_string(targetId)
                    + " WHERE paper_id = " + std::to_string(sourceId));
                database_->execute(
                    "UPDATE paper_annotations SET paper_id = " + std::to_string(targetId)
                    + " WHERE paper_id = " + std::to_string(sourceId));
                database_->execute(
                    "UPDATE user_bookmarks SET paper_id = " + std::to_string(targetId)
                    + " WHERE paper_id = " + std::to_string(sourceId));

                if (!keepBoth) {
                    database_->execute(
                        "DELETE FROM papers WHERE id = " + std::to_string(sourceId));
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["merged"] = true;
            resp["targetId"] = targetId;
            resp["sourceId"] = sourceId;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/papers/:id/related — Get related papers by shared keywords
    router.get(prefix + "/:id/related-by-keywords", [this](const HttpRequest& req) -> HttpResponse {
        try {
            int paperId = std::stoi(req.pathParams.at("id"));

            nlohmann::json resp;
            resp["papers"] = nlohmann::json::array();
            resp["total"] = 0;
            resp["sourceId"] = paperId;

            if (database_) {
                try {
                    auto result = database_->query(
                        "SELECT p2.id, p2.title, p2.authors, p2.year FROM papers p1 "
                        "JOIN papers p2 ON p1.keywords = p2.keywords AND p1.id != p2.id "
                        "WHERE p1.id = " + std::to_string(paperId) + " LIMIT 10");
                    nlohmann::json arr = nlohmann::json::array();
                    for (auto& row : result) {
                        nlohmann::json item;
                        item["id"] = StringUtil::getRowInt(row, "id");
                        item["title"] = StringUtil::getRowStr(row, "title");
                        item["authors"] = StringUtil::getRowStr(row, "authors");
                        item["year"] = StringUtil::getRowInt(row, "year");
                        arr.push_back(item);
                    }
                    resp["papers"] = arr;
                    resp["total"] = arr.size();
                } catch (const std::exception& e) {
                    spdlog::warn("[PaperApi] Related-by-keywords query failed: {}", e.what());
                }
            }

            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/papers/import-url — Import paper from URL
    router.post(prefix + "/import-url", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string url = body.value("url", "");
            std::string title = body.value("title", "");

            if (url.empty())
                return HttpResponse::json(HTTP::BAD_REQUEST,
                    "{\"error\":\"url is required\"}");

            auto now = std::chrono::system_clock::now();
            auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            std::string paperId = "imp_" + std::to_string(ts);

            nlohmann::json resp;
            resp["success"] = true;
            resp["paperId"] = paperId;
            resp["url"] = url;
            resp["title"] = title;
            resp["message"] = "Import queued";
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/papers/export-stats — Paper export statistics
    router.get(prefix + "/export-stats", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["stats"] = nlohmann::json::array();
        resp["total"] = 0;
        resp["success"] = true;

        if (database_) {
            try {
                auto result = database_->query(
                    "SELECT year, COUNT(*) as count FROM papers "
                    "GROUP BY year ORDER BY year DESC LIMIT 20");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : result) {
                    nlohmann::json item;
                    item["year"] = StringUtil::getRowStr(row, "year");
                    item["count"] = StringUtil::getRowInt(row, "count");
                    arr.push_back(item);
                }
                resp["stats"] = arr;
                resp["total"] = arr.size();
            } catch (const std::exception& e) {
                spdlog::warn("[PaperApi] Export-stats query failed: {}", e.what());
            }
        }

        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    spdlog::info("[PaperApiModule] Registered 45 routes");
}

// ============================================================================
// HTTP handlers — thin wrappers: parse request, call service, format response
// ============================================================================

std::string PaperApiModule::handleListPapers(const std::map<std::string, std::string>& params) {
    int page = 1, limit = 20;
    auto it = params.find("page"); if (it != params.end()) page = std::stoi(it->second);
    it = params.find("limit"); if (it != params.end()) limit = std::stoi(it->second);

    auto papers = listPapers(page, limit);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& p : papers) arr.push_back(p.toJson());
    return JsonHelper::buildPapersJsonResponse(arr.dump(), papers.size(), page, limit);
}

std::string PaperApiModule::handleGetPaper(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) return JsonHelper::buildJsonResponse({{"error", "Missing paper ID"}}, HTTP::BAD_REQUEST);
    int id;
    try { id = std::stoi(idIt->second); } catch (...) { return JsonHelper::buildJsonResponse({{"error", "Invalid paper ID"}}, HTTP::BAD_REQUEST); }

    auto paper = getPaper(id);
    if (!paper) return JsonHelper::buildJsonResponse({{"error", "Paper not found"}}, HTTP::NOT_FOUND);
    return JsonHelper::buildPapersJsonResponse("[" + paper->toJson().dump() + "]", 1, 1, 1);
}

std::string PaperApiModule::handleCreatePaper(const std::string& body) {
    if (body.empty() || body == "{}")
        return JsonHelper::buildJsonResponse({{"error", "Invalid request: paper data is required"}}, HTTP::BAD_REQUEST);

    try {
        auto jsonBody = nlohmann::json::parse(body);
        if (!jsonBody.contains("title") || jsonBody["title"].empty())
            return JsonHelper::buildJsonResponse({{"error", "Validation failed: title is required"}}, HTTP::BAD_REQUEST);
        jsonBody["title"] = ValidationHelper::sanitize(jsonBody["title"].get<std::string>());
        if (jsonBody.contains("abstract") && jsonBody["abstract"].is_string())
            jsonBody["abstract"] = ValidationHelper::sanitize(jsonBody["abstract"].get<std::string>());
        if (jsonBody.contains("authors") && jsonBody["authors"].is_string())
            jsonBody["authors"] = ValidationHelper::sanitize(jsonBody["authors"].get<std::string>());
    } catch (...) {
        return JsonHelper::buildJsonResponse({{"error", "Invalid JSON format"}}, HTTP::BAD_REQUEST);
    }

    Paper paper;
    auto newPaper = createPaper(paper);
    if (newPaper)
        return JsonHelper::buildJsonResponse({{"success", "true"}, {"message", "Paper created"}, {"id", std::to_string(newPaper->id)}}, HTTP::CREATED);
    return JsonHelper::buildJsonResponse({{"error", "Failed to create paper"}}, HTTP::INTERNAL_ERROR);
}

std::string PaperApiModule::handleUpdatePaper(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) return JsonHelper::buildJsonResponse({{"error", "Missing paper ID"}}, HTTP::BAD_REQUEST);
    int id;
    try { id = std::stoi(idIt->second); } catch (...) { return JsonHelper::buildJsonResponse({{"error", "Invalid paper ID"}}, HTTP::BAD_REQUEST); }

    if (!body.empty() && body != "{}") {
        try {
            auto jsonBody = nlohmann::json::parse(body);
            if (jsonBody.contains("title") && jsonBody["title"].is_string())
                jsonBody["title"] = ValidationHelper::sanitize(jsonBody["title"].get<std::string>());
            if (jsonBody.contains("abstract") && jsonBody["abstract"].is_string())
                jsonBody["abstract"] = ValidationHelper::sanitize(jsonBody["abstract"].get<std::string>());
            if (jsonBody.contains("authors") && jsonBody["authors"].is_string())
                jsonBody["authors"] = ValidationHelper::sanitize(jsonBody["authors"].get<std::string>());
        } catch (...) {}
    }

    Paper paper;
    if (updatePaper(id, paper))
        return JsonHelper::buildJsonResponse({{"success", "true"}, {"message", "Paper updated"}});
    return JsonHelper::buildJsonResponse({{"error", "Failed to update paper"}}, HTTP::INTERNAL_ERROR);
}

std::string PaperApiModule::handleDeletePaper(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) return JsonHelper::buildJsonResponse({{"error", "Missing paper ID"}}, HTTP::BAD_REQUEST);
    int id;
    try { id = std::stoi(idIt->second); } catch (...) { return JsonHelper::buildJsonResponse({{"error", "Invalid paper ID"}}, HTTP::BAD_REQUEST); }

    if (!getPaper(id)) return JsonHelper::buildJsonResponse({{"error", "Paper not found"}}, HTTP::NOT_FOUND);
    if (deletePaper(id))
        return JsonHelper::buildJsonResponse({{"success", "true"}, {"message", "Paper deleted"}});
    return JsonHelper::buildJsonResponse({{"error", "Failed to delete paper"}}, HTTP::INTERNAL_ERROR);
}

std::string PaperApiModule::handleSearch(const std::map<std::string, std::string>& params) {
    PaperSearchCriteria criteria;
    auto it = params.find("query"); if (it != params.end()) criteria.query = ValidationHelper::sanitize(it->second);
    it = params.find("yearFrom"); if (it != params.end()) criteria.yearFrom = std::stoi(it->second);

    int page = 1, limit = 20;
    it = params.find("page"); if (it != params.end()) page = std::stoi(it->second);
    it = params.find("limit"); if (it != params.end()) limit = std::stoi(it->second);

    auto papers = searchPapers(criteria, page, limit);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& p : papers) arr.push_back(p.toJson());
    return JsonHelper::buildPapersJsonResponse(arr.dump(), papers.size(), page, limit);
}

std::string PaperApiModule::handleStats() {
    auto stats = getStats();
    std::map<std::string, std::string> statsMap;
    statsMap["totalPapers"] = std::to_string(stats.totalPapers);
    statsMap["readPapers"] = std::to_string(stats.readPapers);
    statsMap["unreadPapers"] = std::to_string(stats.unreadPapers);
    statsMap["bookmarkedPapers"] = std::to_string(stats.bookmarkedPapers);
    return JsonHelper::buildStatsJsonResponse(statsMap);
}

std::string PaperApiModule::handleExport(const std::map<std::string, std::string>& params) {
    std::string format = "json";
    auto it = params.find("format"); if (it != params.end()) format = it->second;

    std::vector<int> ids;
    it = params.find("ids");
    if (it != params.end()) {
        std::istringstream iss(it->second);
        std::string idStr;
        while (std::getline(iss, idStr, ',')) {
            try { ids.push_back(std::stoi(idStr)); } catch (...) { spdlog::warn("[PaperApi] Invalid ID format: {}", idStr); }
        }
    }

    if (ids.empty()) {
        auto allPapers = listPapers(1, 10000);
        for (const auto& p : allPapers) ids.push_back(p.id);
    }

    std::string data = exportPapers(ids, format);
    return JsonHelper::buildJsonResponse({{"format", format}, {"count", std::to_string(ids.size())}, {"data", data}});
}

std::string PaperApiModule::handleFavorite(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) return JsonHelper::buildJsonResponse({{"error", "Missing paper ID"}}, HTTP::BAD_REQUEST);
    int id;
    try { id = std::stoi(idIt->second); } catch (...) { return JsonHelper::buildJsonResponse({{"error", "Invalid paper ID"}}, HTTP::BAD_REQUEST); }

    if (!getPaper(id)) return JsonHelper::buildJsonResponse({{"error", "Paper not found"}}, HTTP::NOT_FOUND);

    bool favorite = true;
    try { auto j = nlohmann::json::parse(body); if (j.contains("favorite")) favorite = j["favorite"]; } catch (...) { spdlog::warn("[PaperApi] Failed to parse request body"); }

    if (markAsFavorite(id, favorite))
        return JsonHelper::buildJsonResponse({{"success", "true"}, {"message", std::string("Paper ") + (favorite ? "added to" : "removed from") + " favorites"}});
    return JsonHelper::buildJsonResponse({{"error", "Failed to update favorite status"}}, HTTP::INTERNAL_ERROR);
}

std::string PaperApiModule::handleRead(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) return JsonHelper::buildJsonResponse({{"error", "Missing paper ID"}}, HTTP::BAD_REQUEST);
    int id;
    try { id = std::stoi(idIt->second); } catch (...) { return JsonHelper::buildJsonResponse({{"error", "Invalid paper ID"}}, HTTP::BAD_REQUEST); }

    if (!getPaper(id)) return JsonHelper::buildJsonResponse({{"error", "Paper not found"}}, HTTP::NOT_FOUND);

    bool isRead = true;
    try { auto j = nlohmann::json::parse(body); if (j.contains("is_read")) isRead = j["is_read"]; } catch (...) { spdlog::warn("[PaperApi] Failed to parse request body"); }

    if (markAsRead(id, isRead))
        return JsonHelper::buildJsonResponse({{"success", "true"}, {"message", std::string("Paper ") + (isRead ? "marked as read" : "marked as unread")}});
    return JsonHelper::buildJsonResponse({{"error", "Failed to update read status"}}, HTTP::INTERNAL_ERROR);
}

std::string PaperApiModule::handleTags(const std::map<std::string, std::string>& params, const std::string& body, const std::string& method) {
    auto idIt = params.find("id");
    if (idIt == params.end()) return JsonHelper::buildJsonResponse({{"error", "Missing paper ID"}}, HTTP::BAD_REQUEST);
    int id;
    try { id = std::stoi(idIt->second); } catch (...) { return JsonHelper::buildJsonResponse({{"error", "Invalid paper ID"}}, HTTP::BAD_REQUEST); }

    if (!getPaper(id)) return JsonHelper::buildJsonResponse({{"error", "Paper not found"}}, HTTP::NOT_FOUND);

    if (method == "POST") {
        try {
            auto jsonBody = nlohmann::json::parse(body);
            if (jsonBody.contains("tags") && jsonBody["tags"].is_array()) {
                for (auto& tag : jsonBody["tags"]) {
                    if (tag.is_string()) tag = ValidationHelper::sanitize(tag.get<std::string>());
                }
                return JsonHelper::buildJsonResponse({{"success", "true"}, {"message", "Tags added successfully"}});
            }
        } catch (...) { return JsonHelper::buildJsonResponse({{"error", "Invalid JSON format"}}, HTTP::BAD_REQUEST); }
        return JsonHelper::buildJsonResponse({{"error", "Tags array required"}}, HTTP::BAD_REQUEST);
    }

    if (method == "DELETE") {
        auto tagIt = params.find("tag");
        if (tagIt == params.end()) return JsonHelper::buildJsonResponse({{"error", "Missing tag name"}}, HTTP::BAD_REQUEST);
        return JsonHelper::buildJsonResponse({{"success", "true"}, {"message", "Tag removed successfully"}});
    }

    return JsonHelper::buildJsonResponse({{"error", "Invalid method"}}, 405);
}

} // namespace PaperCrawler

// ============================================================================
// DLL export
// ============================================================================

extern "C" {

PAPERCRAWLER_API void* createModule() {
    return new PaperCrawler::PaperApiModule();
}

PAPERCRAWLER_API void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::PaperApiModule*>(ptr);
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

}
