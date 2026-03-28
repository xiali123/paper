/**
 * @file paper_handlers.cpp
 * @brief 论文管理HTTP请求处理程序
 * @details 处理论文的增删改查、搜索、统计等API请求
 */

#include "../include/models/Paper.hpp"
#include "../include/database/DatabaseManager.hpp"
#include "../include/jwt/JWT.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;
namespace http = httplib;

namespace PaperCrawler {

/**
 * @brief 通用响应构建器
 */
struct ApiResponse {
    bool success = true;
    std::string message;
    json data;
    std::string error;

    json toJson() const {
        json j;
        j["success"] = success;
        if (!message.empty()) j["message"] = message;
        if (!data.is_null()) j["data"] = data;
        if (!error.empty()) j["error"] = error;
        return j;
    }

    std::string dump() const {
        return toJson().dump();
    }
};

/**
 * @brief 从请求中提取用户ID
 */
int extractUserId(const http::Request& req) {
    auto& jwt = JWTManager::getInstance();

    // 从Authorization header获取token
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        return 0;
    }

    std::string token = authHeader.substr(7);
    auto payload = jwt.verifyToken(token);
    if (!payload) {
        return 0;
    }

    try {
        return payload->value("userId", 0);
    } catch (...) {
        return 0;
    }
}

/**
 * @brief 注册论文管理相关路由
 */
void registerPaperRoutes(http::Server& server) {

    // ========================================================================
    // 论文CRUD接口
    // ========================================================================

    /**
     * GET /api/papers
     * 获取论文列表（支持分页、搜索、筛选）
     */
    server.Get("/api/papers", [](const http::Request& req, http::Response& res) {
        int userId = extractUserId(req);
        if (userId == 0) {
            res.set_content(ApiResponse{false, "", {}, "Unauthorized"}.dump(), "application/json");
            res.status = 401;
            return;
        }

        try {
            // 解析查询参数
            PaperQuery query;
            query.userId = userId;

            if (req.has_param("keyword")) {
                query.keyword = req.get_param_value("keyword");
            }
            if (req.has_param("category")) {
                query.category = req.get_param_value("category");
            }
            if (req.has_param("tags")) {
                query.tags = req.get_param_value("tags");
            }
            if (req.has_param("source")) {
                query.source = req.get_param_value("source");
            }
            if (req.has_param("is_read")) {
                query.isRead = req.get_param_value("is_read") == "1";
            }
            if (req.has_param("is_bookmarked")) {
                query.isBookmarked = req.get_param_value("is_bookmarked") == "1";
            }
            if (req.has_param("order_by")) {
                query.orderBy = req.get_param_value("order_by");
            }
            if (req.has_param("order")) {
                query.order = req.get_param_value("order");
            }
            if (req.has_param("page")) {
                query.page = std::stoi(req.get_param_value("page"));
            }
            if (req.has_param("page_size")) {
                query.pageSize = std::stoi(req.get_param_value("page_size"));
            }

            // 查询论文
            auto papers = PaperRepository::query(query);
            int total = PaperRepository::count(query);

            // 构建响应
            json::array_t papersJson;
            for (const auto& paper : papers) {
                papersJson.push_back(paper.toJson());
            }

            ApiResponse response;
            response.success = true;
            response.message = "Papers retrieved successfully";
            response.data = {
                {"papers", papersJson},
                {"total", total},
                {"page", query.page},
                {"pageSize", query.pageSize},
                {"totalPages", (total + query.pageSize - 1) / query.pageSize}
            };

            res.set_content(response.dump(), "application/json");

        } catch (const std::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Failed to retrieve papers: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 500;
        }
    });

    /**
     * GET /api/papers/:id
     * 获取单个论文详情
     */
    server.Get("/api/papers/(\\d+)", [](const http::Request& req, http::Response& res) {
        int userId = extractUserId(req);
        if (userId == 0) {
            res.set_content(ApiResponse{false, "", {}, "Unauthorized"}.dump(), "application/json");
            res.status = 401;
            return;
        }

        try {
            int paperId = std::stoi(req.matches[1].str());
            auto paper = PaperRepository::getById(paperId, userId);

            if (!paper) {
                ApiResponse response;
                response.success = false;
                response.error = "Paper not found";
                res.set_content(response.dump(), "application/json");
                res.status = 404;
                return;
            }

            ApiResponse response;
            response.success = true;
            response.message = "Paper retrieved successfully";
            response.data = paper->toJson();

            res.set_content(response.dump(), "application/json");

        } catch (const std::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Failed to retrieve paper: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 500;
        }
    });

    /**
     * POST /api/papers
     * 创建新论文
     */
    server.Post("/api/papers", [](const http::Request& req, http::Response& res) {
        int userId = extractUserId(req);
        if (userId == 0) {
            res.set_content(ApiResponse{false, "", {}, "Unauthorized"}.dump(), "application/json");
            res.status = 401;
            return;
        }

        try {
            // 解析请求体
            json body = json::parse(req.body);
            Paper paper = Paper::fromJson(body);
            paper.userId = userId;

            // 验证必填字段
            if (paper.title.empty()) {
                ApiResponse response;
                response.success = false;
                response.error = "Title is required";
                res.set_content(response.dump(), "application/json");
                res.status = 400;
                return;
            }

            // 创建论文
            int paperId;
            if (!PaperRepository::create(paper, paperId)) {
                ApiResponse response;
                response.success = false;
                response.error = "Failed to create paper";
                res.set_content(response.dump(), "application/json");
                res.status = 500;
                return;
            }

            paper.id = paperId;

            ApiResponse response;
            response.success = true;
            response.message = "Paper created successfully";
            response.data = paper.toJson();

            res.set_content(response.dump(), "application/json");
            res.status = 201;

        } catch (const json::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Invalid JSON: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 400;
        } catch (const std::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Failed to create paper: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 500;
        }
    });

    /**
     * PUT /api/papers/:id
     * 更新论文
     */
    server.Put("/api/papers/(\\d+)", [](const http::Request& req, http::Response& res) {
        int userId = extractUserId(req);
        if (userId == 0) {
            res.set_content(ApiResponse{false, "", {}, "Unauthorized"}.dump(), "application/json");
            res.status = 401;
            return;
        }

        try {
            int paperId = std::stoi(req.matches[1].str());

            // 检查论文是否存在
            auto existingPaper = PaperRepository::getById(paperId, userId);
            if (!existingPaper) {
                ApiResponse response;
                response.success = false;
                response.error = "Paper not found";
                res.set_content(response.dump(), "application/json");
                res.status = 404;
                return;
            }

            // 解析请求体
            json body = json::parse(req.body);
            Paper paper = Paper::fromJson(body);
            paper.id = paperId;
            paper.userId = userId;

            // 更新论文
            if (!PaperRepository::update(paper)) {
                ApiResponse response;
                response.success = false;
                response.error = "Failed to update paper";
                res.set_content(response.dump(), "application/json");
                res.status = 500;
                return;
            }

            ApiResponse response;
            response.success = true;
            response.message = "Paper updated successfully";
            response.data = paper.toJson();

            res.set_content(response.dump(), "application/json");

        } catch (const json::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Invalid JSON: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 400;
        } catch (const std::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Failed to update paper: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 500;
        }
    });

    /**
     * DELETE /api/papers/:id
     * 删除论文
     */
    server.Delete("/api/papers/(\\d+)", [](const http::Request& req, http::Response& res) {
        int userId = extractUserId(req);
        if (userId == 0) {
            res.set_content(ApiResponse{false, "", {}, "Unauthorized"}.dump(), "application/json");
            res.status = 401;
            return;
        }

        try {
            int paperId = std::stoi(req.matches[1].str());

            // 删除论文
            if (!PaperRepository::remove(paperId, userId)) {
                ApiResponse response;
                response.success = false;
                response.error = "Failed to delete paper";
                res.set_content(response.dump(), "application/json");
                res.status = 500;
                return;
            }

            ApiResponse response;
            response.success = true;
            response.message = "Paper deleted successfully";

            res.set_content(response.dump(), "application/json");

        } catch (const std::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Failed to delete paper: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 500;
        }
    });

    // ========================================================================
    // 论文操作接口
    // ========================================================================

    /**
     * POST /api/papers/:id/bookmark
     * 切换收藏状态
     */
    server.Post("/api/papers/(\\d+)/bookmark", [](const http::Request& req, http::Response& res) {
        int userId = extractUserId(req);
        if (userId == 0) {
            res.set_content(ApiResponse{false, "", {}, "Unauthorized"}.dump(), "application/json");
            res.status = 401;
            return;
        }

        try {
            int paperId = std::stoi(req.matches[1].str());

            if (!PaperRepository::toggleBookmark(paperId, userId)) {
                ApiResponse response;
                response.success = false;
                response.error = "Failed to toggle bookmark";
                res.set_content(response.dump(), "application/json");
                res.status = 500;
                return;
            }

            // 获取更新后的论文
            auto paper = PaperRepository::getById(paperId, userId);
            if (paper) {
                ApiResponse response;
                response.success = true;
                response.message = "Bookmark toggled successfully";
                response.data = {
                    {"isBookmarked", paper->isBookmarked}
                };
                res.set_content(response.dump(), "application/json");
            }

        } catch (const std::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Failed to toggle bookmark: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 500;
        }
    });

    /**
     * POST /api/papers/:id/read
     * 标记已读/未读
     */
    server.Post("/api/papers/(\\d+)/read", [](const http::Request& req, http::Response& res) {
        int userId = extractUserId(req);
        if (userId == 0) {
            res.set_content(ApiResponse{false, "", {}, "Unauthorized"}.dump(), "application/json");
            res.status = 401;
            return;
        }

        try {
            int paperId = std::stoi(req.matches[1].str());

            // 解析请求体
            json body = json::parse(req.body);
            bool isRead = body.value("isRead", false);

            if (!PaperRepository::markAsRead(paperId, userId, isRead)) {
                ApiResponse response;
                response.success = false;
                response.error = "Failed to mark paper";
                res.set_content(response.dump(), "application/json");
                res.status = 500;
                return;
            }

            ApiResponse response;
            response.success = true;
            response.message = "Paper marked successfully";
            response.data = {
                {"isRead", isRead}
            };

            res.set_content(response.dump(), "application/json");

        } catch (const json::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Invalid JSON: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 400;
        } catch (const std::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Failed to mark paper: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 500;
        }
    });

    /**
     * POST /api/papers/:id/progress
     * 更新阅读进度
     */
    server.Post("/api/papers/(\\d+)/progress", [](const http::Request& req, http::Response& res) {
        int userId = extractUserId(req);
        if (userId == 0) {
            res.set_content(ApiResponse{false, "", {}, "Unauthorized"}.dump(), "application/json");
            res.status = 401;
            return;
        }

        try {
            int paperId = std::stoi(req.matches[1].str());

            // 解析请求体
            json body = json::parse(req.body);
            int progress = body.value("progress", 0);

            if (progress < 0) progress = 0;
            if (progress > 100) progress = 100;

            if (!PaperRepository::updateReadingProgress(paperId, userId, progress)) {
                ApiResponse response;
                response.success = false;
                response.error = "Failed to update progress";
                res.set_content(response.dump(), "application/json");
                res.status = 500;
                return;
            }

            ApiResponse response;
            response.success = true;
            response.message = "Progress updated successfully";
            response.data = {
                {"readingProgress", progress}
            };

            res.set_content(response.dump(), "application/json");

        } catch (const json::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Invalid JSON: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 400;
        } catch (const std::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Failed to update progress: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 500;
        }
    });

    // ========================================================================
    // 统计接口
    // ========================================================================

    /**
     * GET /api/papers/stats
     * 获取论文统计信息
     */
    server.Get("/api/papers/stats", [](const http::Request& req, http::Response& res) {
        int userId = extractUserId(req);
        if (userId == 0) {
            res.set_content(ApiResponse{false, "", {}, "Unauthorized"}.dump(), "application/json");
            res.status = 401;
            return;
        }

        try {
            PaperStats stats = PaperRepository::getStats(userId);

            ApiResponse response;
            response.success = true;
            response.message = "Stats retrieved successfully";
            response.data = stats.toJson();

            res.set_content(response.dump(), "application/json");

        } catch (const std::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Failed to retrieve stats: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 500;
        }
    });

    /**
     * GET /api/papers/search
     * 搜索论文
     */
    server.Get("/api/papers/search", [](const http::Request& req, http::Response& res) {
        int userId = extractUserId(req);
        if (userId == 0) {
            res.set_content(ApiResponse{false, "", {}, "Unauthorized"}.dump(), "application/json");
            res.status = 401;
            return;
        }

        try {
            if (!req.has_param("q")) {
                ApiResponse response;
                response.success = false;
                response.error = "Search query is required";
                res.set_content(response.dump(), "application/json");
                res.status = 400;
                return;
            }

            PaperQuery query;
            query.userId = userId;
            query.keyword = req.get_param_value("q");

            if (req.has_param("page")) {
                query.page = std::stoi(req.get_param_value("page"));
            }
            if (req.has_param("page_size")) {
                query.pageSize = std::stoi(req.get_param_value("page_size"));
            }

            auto papers = PaperRepository::query(query);
            int total = PaperRepository::count(query);

            json::array_t papersJson;
            for (const auto& paper : papers) {
                papersJson.push_back(paper.toJson());
            }

            ApiResponse response;
            response.success = true;
            response.message = "Search completed successfully";
            response.data = {
                {"papers", papersJson},
                {"total", total},
                {"query", query.keyword}
            };

            res.set_content(response.dump(), "application/json");

        } catch (const std::exception& e) {
            ApiResponse response;
            response.success = false;
            response.error = std::string("Search failed: ") + e.what();
            res.set_content(response.dump(), "application/json");
            res.status = 500;
        }
    });
}

} // namespace PaperCrawler
