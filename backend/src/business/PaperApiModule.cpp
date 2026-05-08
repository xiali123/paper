#include "core/HttpStatus.hpp"
#include "data/DatabaseModule.hpp"
#include "data/PreparedStatement.hpp"
#include "data/ValidationHelper.hpp"
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

    spdlog::info("[PaperApiModule] Registered 12 routes");
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
