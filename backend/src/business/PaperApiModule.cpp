#include <iostream>
#include "business/PaperApiModule.hpp"
#include "business/JsonHelper.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <algorithm>

namespace PaperCrawler {

// ============================================================================
// Mock 数据（从 simple_api_server.cpp 提取）
// ============================================================================

/**
 * @brief 获取 Mock 论文列表数据
 */
std::string getMockPapers() {
    return "[\n"
           "  {\n"
           "    \"id\": 1,\n"
           "    \"title\": \"Attention Is All You Need\",\n"
           "    \"authors\": \"Ashish Vaswani et al.\",\n"
           "    \"year\": 2023,\n"
           "    \"citation_count\": 150\n"
           "  },\n"
           "  {\n"
           "    \"id\": 2,\n"
           "    \"title\": \"BERT: Pre-training of Deep Bidirectional Transformers\",\n"
           "    \"authors\": \"Jacob Devlin et al.\",\n"
           "    \"year\": 2019,\n"
           "    \"citation_count\": 89000\n"
           "  },\n"
           "  {\n"
           "    \"id\": 3,\n"
           "    \"title\": \"Deep Residual Learning for Image Recognition\",\n"
           "    \"authors\": \"Kaiming He et al.\",\n"
           "    \"year\": 2016,\n"
           "    \"citation_count\": 150000\n"
           "  },\n"
           "  {\n"
           "    \"id\": 4,\n"
           "    \"title\": \"GPT-4 Technical Report\",\n"
           "    \"authors\": \"OpenAI\",\n"
           "    \"year\": 2023,\n"
           "    \"citation_count\": 5000\n"
           "  }\n"
           "]";
}

/**
 * @brief 获取单个论文的 Mock 数据
 */
std::string getMockPaper(int id) {
    std::ostringstream json;
    json << "[\n";
    json << "  {\n";
    json << "    \"id\": " << id << ",\n";
    json << "    \"title\": \"Sample Paper\",\n";
    json << "    \"authors\": \"Test Author\",\n";
    json << "    \"year\": 2023,\n";
    json << "    \"abstract\": \"This is a test abstract...\"\n";
    json << "  }\n";
    json << "]";
    return json.str();
}

// ============================================================================
// PaperApiModule 实现
// ============================================================================

class PaperApiModule::Impl {
public:
    // 依赖注入：数据库接口
    std::shared_ptr<IDatabase> database_;

    // 构造函数：接受数据库依赖
    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {
        // 不再加载Mock数据
    }

    // 从数据库行数据构建Paper对象
    Paper paperFromDbRow(const std::map<std::string, std::string>& row) {
        Paper paper;
        paper.id = std::stoi(row.at("id"));
        paper.title = row.at("title");
        paper.authors = row.at("authors");
        paper.year = std::stoi(row.at("year"));
        paper.abstract = row.count("abstract") ? row.at("abstract") : "";
        paper.journal = row.count("journal") ? row.at("journal") : "";
        paper.volume = row.count("volume") ? row.at("volume") : "";
        paper.issue = row.count("issue") ? row.at("issue") : "";
        paper.pages = row.count("pages") ? row.at("pages") : "";
        paper.doi = row.count("doi") ? row.at("doi") : "";
        paper.url = row.count("url") ? row.at("url") : "";
        paper.pdfPath = row.count("pdf_path") ? row.at("pdf_path") : "";
        paper.citationCount = row.count("citation_count") ? std::stoi(row.at("citation_count")) : 0;
        paper.isRead = row.count("is_read") ? (row.at("is_read") == "1") : false;
        paper.isFavorite = row.count("is_favorite") ? (row.at("is_favorite") == "1") : false;
        paper.notes = row.count("notes") ? row.at("notes") : "";
        return paper;
    }

    // 从数据库查询单个论文
    std::optional<Paper> getPaperById(int id) {
        try {
            auto sql = "SELECT * FROM papers WHERE id = " + std::to_string(id);
            auto results = database_->query(sql);

            if (!results.empty()) {
                return paperFromDbRow(results[0]);
            }
            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[PaperAPI] Failed to get paper: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    // 从数据库查询论文列表（带分页和排序）
    std::vector<Paper> listPapersFromDb(int page, int limit, const std::string& sortBy, bool ascending) {
        std::vector<Paper> papers;

        try {
            int offset = (page - 1) * limit;
            std::string orderDirection = ascending ? "ASC" : "DESC";

            // 防止SQL注入：只允许特定字段
            std::string allowedSortBy = sortBy;
            if (sortBy != "title" && sortBy != "year" && sortBy != "citation_count" &&
                sortBy != "created_at" && sortBy != "updated_at") {
                allowedSortBy = "created_at";  // 默认排序
            }

            auto sql = "SELECT * FROM papers ORDER BY " + allowedSortBy + " " +
                      orderDirection + " LIMIT " + std::to_string(limit) +
                      " OFFSET " + std::to_string(offset);

            auto results = database_->query(sql);

            for (const auto& row : results) {
                papers.push_back(paperFromDbRow(row));
            }
        } catch (const std::exception& e) {
            std::cerr << "[PaperAPI] Failed to list papers: " << e.what() << std::endl;
        }

        return papers;
    }

    // 在数据库中创建论文
    std::optional<Paper> createPaperInDb(const Paper& paper) {
        try {
            // 转义字符串（简化版，生产环境应使用prepared statements）
            auto escape = [](const std::string& s) {
                std::string result;
                for (char c : s) {
                    if (c == '\'') result += "''";
                    else if (c == '\\') result += "\\\\";
                    else result += c;
                }
                return result;
            };

            auto sql = "INSERT INTO papers (title, authors, year, abstract, journal, volume, issue, "
                      "pages, doi, url, pdf_path, citation_count, is_read, is_favorite, notes, "
                      "created_at, updated_at) VALUES ('" +
                      escape(paper.title) + "', '" +
                      escape(paper.authors) + "', " +
                      std::to_string(paper.year) + ", '" +
                      escape(paper.abstract) + "', '" +
                      escape(paper.journal) + "', '" +
                      escape(paper.volume) + "', '" +
                      escape(paper.issue) + "', '" +
                      escape(paper.pages) + "', '" +
                      escape(paper.doi) + "', '" +
                      escape(paper.url) + "', '" +
                      escape(paper.pdfPath) + "', " +
                      std::to_string(paper.citationCount) + ", " +
                      (paper.isRead ? "1" : "0") + ", " +
                      (paper.isFavorite ? "1" : "0") + ", '" +
                      escape(paper.notes) + "', NOW(), NOW())";

            if (database_->execute(sql)) {
                // 查询新插入的论文（通过title和year）
                auto querySql = "SELECT * FROM papers WHERE title = '" + escape(paper.title) +
                               "' AND year = " + std::to_string(paper.year) +
                               " ORDER BY id DESC LIMIT 1";
                auto results = database_->query(querySql);

                if (!results.empty()) {
                    return paperFromDbRow(results[0]);
                }
            }

            return std::nullopt;
        } catch (const std::exception& e) {
            std::cerr << "[PaperAPI] Failed to create paper: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    // 在数据库中更新论文
    bool updatePaperInDb(int id, const Paper& paper) {
        try {
            auto escape = [](const std::string& s) {
                std::string result;
                for (char c : s) {
                    if (c == '\'') result += "''";
                    else if (c == '\\') result += "\\\\";
                    else result += c;
                }
                return result;
            };

            auto sql = "UPDATE papers SET "
                      "title = '" + escape(paper.title) + "', "
                      "authors = '" + escape(paper.authors) + "', "
                      "year = " + std::to_string(paper.year) + ", "
                      "abstract = '" + escape(paper.abstract) + "', "
                      "journal = '" + escape(paper.journal) + "', "
                      "volume = '" + escape(paper.volume) + "', "
                      "issue = '" + escape(paper.issue) + "', "
                      "pages = '" + escape(paper.pages) + "', "
                      "doi = '" + escape(paper.doi) + "', "
                      "url = '" + escape(paper.url) + "', "
                      "pdf_path = '" + escape(paper.pdfPath) + "', "
                      "citation_count = " + std::to_string(paper.citationCount) + ", "
                      "is_read = " + (paper.isRead ? "1" : "0") + ", "
                      "is_favorite = " + (paper.isFavorite ? "1" : "0") + ", "
                      "notes = '" + escape(paper.notes) + "', "
                      "updated_at = NOW() "
                      "WHERE id = " + std::to_string(id);

            return database_->execute(sql);
        } catch (const std::exception& e) {
            std::cerr << "[PaperAPI] Failed to update paper: " << e.what() << std::endl;
            return false;
        }
    }

    // 从数据库删除论文
    bool deletePaperFromDb(int id) {
        try {
            auto sql = "DELETE FROM papers WHERE id = " + std::to_string(id);
            return database_->execute(sql);
        } catch (const std::exception& e) {
            std::cerr << "[PaperAPI] Failed to delete paper: " << e.what() << std::endl;
            return false;
        }
    }

    // 搜索论文（使用数据库）
    std::vector<Paper> searchPapersFromDb(const PaperSearchCriteria& criteria, int page, int limit) {
        std::vector<Paper> papers;
        try {
            std::string sql = "SELECT * FROM papers WHERE 1=1";
            int paramCount = 0;

            // 构建WHERE条件
            if (!criteria.query.empty()) {
                sql += " AND (title LIKE '%" + criteria.query + "%' OR "
                       "authors LIKE '%" + criteria.query + "%' OR "
                       "abstract LIKE '%" + criteria.query + "%')";
            }

            if (criteria.yearFrom > 0) {
                sql += " AND year >= " + std::to_string(criteria.yearFrom);
            }

            if (criteria.yearTo > 0) {
                sql += " AND year <= " + std::to_string(criteria.yearTo);
            }

            if (criteria.isRead) {
                sql += " AND is_read = 1";
            }

            if (criteria.isFavorite) {
                sql += " AND is_favorite = 1";
            }

            // 分页
            int offset = (page - 1) * limit;
            sql += " LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

            auto results = database_->query(sql);
            for (const auto& row : results) {
                papers.push_back(paperFromDbRow(row));
            }
        } catch (const std::exception& e) {
            std::cerr << "[PaperAPI] Failed to search papers: " << e.what() << std::endl;
        }
        return papers;
    }

    // 获取统计信息（使用数据库）
    PaperStats getStatsFromDb() {
        PaperStats stats;
        try {
            // 总论文数
            auto totalSql = "SELECT COUNT(*) as count FROM papers";
            auto totalResults = database_->query(totalSql);
            if (!totalResults.empty()) {
                stats.totalPapers = std::stoi(totalResults[0]["count"]);
            }

            // 已读/未读统计
            auto readSql = "SELECT is_read, COUNT(*) as count FROM papers GROUP BY is_read";
            auto readResults = database_->query(readSql);
            for (const auto& row : readResults) {
                bool isRead = (row.at("is_read") == "1");
                int count = std::stoi(row.at("count"));
                if (isRead) {
                    stats.readPapers = count;
                } else {
                    stats.unreadPapers = count;
                }
            }

            // 收藏统计
            auto favSql = "SELECT COUNT(*) as count FROM papers WHERE is_favorite = 1";
            auto favResults = database_->query(favSql);
            if (!favResults.empty()) {
                stats.favoritePapers = std::stoi(favResults[0]["count"]);
            }

            // 按年份统计
            auto yearSql = "SELECT year, COUNT(*) as count FROM papers GROUP BY year ORDER BY year";
            auto yearResults = database_->query(yearSql);
            for (const auto& row : yearResults) {
                int year = std::stoi(row.at("year"));
                int count = std::stoi(row.at("count"));
                stats.papersByYear[year] = count;
            }

            // 按期刊统计
            auto journalSql = "SELECT journal, COUNT(*) as count FROM papers WHERE journal IS NOT NULL AND journal != '' GROUP BY journal";
            auto journalResults = database_->query(journalSql);
            for (const auto& row : journalResults) {
                std::string journal = row.at("journal");
                int count = std::stoi(row.at("count"));
                stats.papersByJournal[journal] = count;
            }

            // 按作者统计（简化版，可能需要更复杂的处理）
            auto authorSql = "SELECT authors, COUNT(*) as count FROM papers WHERE authors IS NOT NULL AND authors != '' GROUP BY authors";
            auto authorResults = database_->query(authorSql);
            for (const auto& row : authorResults) {
                std::string authors = row.at("authors");
                int count = std::stoi(row.at("count"));
                stats.papersByAuthor[authors] = count;
            }
        } catch (const std::exception& e) {
            std::cerr << "[PaperAPI] Failed to get stats: " << e.what() << std::endl;
        }
        return stats;
    }

    // 标记论文为已读/未读
    bool markAsReadInDb(int id, bool read) {
        try {
            auto sql = "UPDATE papers SET is_read = " + std::string(read ? "1" : "0") +
                      ", updated_at = NOW() WHERE id = " + std::to_string(id);
            return database_->execute(sql);
        } catch (const std::exception& e) {
            std::cerr << "[PaperAPI] Failed to mark paper: " << e.what() << std::endl;
            return false;
        }
    }

    // 标记论文为收藏/取消收藏
    bool markAsFavoriteInDb(int id, bool favorite) {
        try {
            auto sql = "UPDATE papers SET is_favorite = " + std::string(favorite ? "1" : "0") +
                      ", updated_at = NOW() WHERE id = " + std::to_string(id);
            return database_->execute(sql);
        } catch (const std::exception& e) {
            std::cerr << "[PaperAPI] Failed to mark favorite: " << e.what() << std::endl;
            return false;
        }
    }
};

// ============================================================================

PaperApiModule::PaperApiModule()
    : PaperApiModule(nullptr) {
    std::cout << "[PaperApi] PaperApiModule default constructor (database=nullptr)" << std::endl;
}

PaperApiModule::PaperApiModule(std::shared_ptr<IDatabase> database)
    : database_(database),
      impl_(nullptr) {  // ⭐ 延迟创建Impl，使用懒加载
}

PaperApiModule::~PaperApiModule() = default;

std::vector<Paper> PaperApiModule::listPapers(int page, int limit, const std::string& sortBy, bool ascending) {
    // ⭐ 懒加载：首次调用时创建Impl
    if (!impl_) {
        std::cout << "[PaperApi] Lazy loading database implementation..." << std::endl;
        impl_ = std::make_unique<Impl>(database_);
    }

    // ✅ 优雅降级：没有数据库时返回mock数据
    if (!database_) {
        std::cout << "[PaperApi] No database connection, returning mock data" << std::endl;
        // 返回mock数据
        std::vector<Paper> papers;

        Paper p1;
        p1.id = 1;
        p1.title = "Attention Is All You Need";
        p1.authors = "Ashish Vaswani et al.";
        p1.year = 2023;
        p1.abstract = "";
        p1.journal = "";
        p1.volume = "";
        p1.issue = "";
        p1.pages = "";
        p1.doi = "";
        p1.url = "";
        p1.pdfPath = "";
        p1.citationCount = 0;

        Paper p2;
        p2.id = 2;
        p2.title = "BERT: Pre-training of Deep Bidirectional Transformers";
        p2.authors = "Jacob Devlin et al.";
        p2.year = 2019;
        p2.abstract = "";
        p2.journal = "";
        p2.volume = "";
        p2.issue = "";
        p2.pages = "";
        p2.doi = "";
        p2.url = "";
        p2.pdfPath = "";
        p2.citationCount = 89000;

        papers.push_back(p1);
        papers.push_back(p2);
        return papers;
    }

    try {
        return impl_->listPapersFromDb(page, limit, sortBy, ascending);
    } catch (const std::exception& e) {
        std::cerr << "[PaperApi] Exception in listPapers: " << e.what() << std::endl;
        // 返回空列表而不是崩溃
        return {};
    }
}

std::optional<Paper> PaperApiModule::getPaper(int id) {
    // ⭐ 懒加载
    if (!impl_) {
        impl_ = std::make_unique<Impl>(database_);
    }

    // ✅ 优雅降级
    if (!database_) {
        std::cout << "[PaperApi] No database connection" << std::endl;
        return std::nullopt;
    }

    try {
        return impl_->getPaperById(id);
    } catch (const std::exception& e) {
        std::cerr << "[PaperApi] Exception in getPaper: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<Paper> PaperApiModule::createPaper(const Paper& paper) {
    if (!impl_) {
        impl_ = std::make_unique<Impl>(database_);
    }

    if (!database_) {
        std::cout << "[PaperApi] No database connection, returning stub" << std::endl;
        return std::nullopt;
    }

    try {
        return impl_->createPaperInDb(paper);
    } catch (const std::exception& e) {
        std::cerr << "[PaperApi] Exception in createPaper: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool PaperApiModule::updatePaper(int id, const Paper& paper) {
    if (!impl_) {
        impl_ = std::make_unique<Impl>(database_);
    }

    if (!database_) {
        std::cout << "[PaperApi] No database connection, returning stub" << std::endl;
        return false;
    }

    try {
        return impl_->updatePaperInDb(id, paper);
    } catch (const std::exception& e) {
        std::cerr << "[PaperApi] Exception in updatePaper: " << e.what() << std::endl;
        return false;
    }
}

bool PaperApiModule::deletePaper(int id) {
    if (!impl_) {
        impl_ = std::make_unique<Impl>(database_);
    }

    if (!database_) {
        std::cout << "[PaperApi] No database connection, returning stub" << std::endl;
        return false;
    }

    try {
        return impl_->deletePaperFromDb(id);
    } catch (const std::exception& e) {
        std::cerr << "[PaperApi] Exception in deletePaper: " << e.what() << std::endl;
        return false;
    }
}

std::vector<Paper> PaperApiModule::searchPapers(const PaperSearchCriteria& criteria, int page, int limit) {
    if (!impl_) {
        impl_ = std::make_unique<Impl>(database_);
    }

    if (!database_) {
        std::cout << "[PaperApi] No database connection, returning stub" << std::endl;
        return {};
    }

    try {
        return impl_->searchPapersFromDb(criteria, page, limit);
    } catch (const std::exception& e) {
        std::cerr << "[PaperApi] Exception in searchPapers: " << e.what() << std::endl;
        return {};
    }
}

PaperStats PaperApiModule::getStats() {
    if (!impl_) {
        impl_ = std::make_unique<Impl>(database_);
    }

    if (!database_) {
        std::cout << "[PaperApi] No database connection, returning stub stats" << std::endl;
        PaperStats stats;
        stats.totalPapers = 0;
        // 其他字段默认初始化为0
        return stats;
    }

    try {
        return impl_->getStatsFromDb();
    } catch (const std::exception& e) {
        std::cerr << "[PaperApi] Exception in getStats: " << e.what() << std::endl;
        PaperStats stats;
        stats.totalPapers = 0;
        // 其他字段默认初始化为0
        return stats;
    }
}

size_t PaperApiModule::importPapers(const std::vector<Paper>& papers) {
    if (!impl_) {
        impl_ = std::make_unique<Impl>(database_);
    }

    if (!database_) {
        std::cout << "[PaperApi] No database connection, returning stub" << std::endl;
        return 0;
    }

    try {
        size_t imported = 0;
        for (const auto& paper : papers) {
            if (impl_->createPaperInDb(paper).has_value()) {
                imported++;
            }
        }
        return imported;
    } catch (const std::exception& e) {
        std::cerr << "[PaperApi] Exception in importPapers: " << e.what() << std::endl;
        return 0;
    }
}

std::string PaperApiModule::exportPapers(const std::vector<int>& ids, const std::string& format) {
    // TODO: 实现导出逻辑
    if (format == "json") {
        // 构建 JSON
    } else if (format == "bibtex") {
        // 构建 BibTeX
    }

    return "{}";
}

bool PaperApiModule::markAsRead(int id, bool read) {
    if (!impl_) {
        impl_ = std::make_unique<Impl>(database_);
    }

    if (!database_) {
        std::cout << "[PaperApi] No database connection, returning stub" << std::endl;
        return false;
    }

    try {
        return impl_->markAsReadInDb(id, read);
    } catch (const std::exception& e) {
        std::cerr << "[PaperApi] Exception in markAsRead: " << e.what() << std::endl;
        return false;
    }
}

bool PaperApiModule::markAsFavorite(int id, bool favorite) {
    if (!impl_) {
        impl_ = std::make_unique<Impl>(database_);
    }

    if (!database_) {
        std::cout << "[PaperApi] No database connection, returning stub" << std::endl;
        return false;
    }

    try {
        return impl_->markAsFavoriteInDb(id, favorite);
    } catch (const std::exception& e) {
        std::cerr << "[PaperApi] Exception in markAsFavorite: " << e.what() << std::endl;
        return false;
    }
}

bool PaperApiModule::addTag(int id, const std::string& tag) {
    // TODO: 实现标签添加的数据库操作
    std::cout << "[PaperAPI] addTag not yet implemented for database" << std::endl;
    return false;
}

bool PaperApiModule::removeTag(int id, const std::string& tag) {
    // TODO: 实现标签移除的数据库操作
    std::cout << "[PaperAPI] removeTag not yet implemented for database" << std::endl;
    return false;
}

bool PaperApiModule::uploadPDF(int id, const std::string& filePath) {
    try {
        // 使用数据库更新PDF路径
        auto sql = "UPDATE papers SET pdf_path = '" + filePath + "' WHERE id = " + std::to_string(id);
        impl_->database_->execute(sql);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[PaperAPI] Failed to upload PDF: " << e.what() << std::endl;
        return false;
    }
}

std::string PaperApiModule::getPDFPath(int id) {
    auto paper = impl_->getPaperById(id);
    if (paper.has_value()) {
        return paper->pdfPath;
    }
    return "";
}

std::map<std::string, std::vector<Paper>> PaperApiModule::groupByAuthor(const std::vector<Paper>& papers) {
    std::map<std::string, std::vector<Paper>> grouped;
    for (const auto& paper : papers) {
        grouped[paper.authors].push_back(paper);
    }
    return grouped;
}

std::map<int, std::vector<Paper>> PaperApiModule::groupByYear(const std::vector<Paper>& papers) {
    std::map<int, std::vector<Paper>> grouped;
    for (const auto& paper : papers) {
        grouped[paper.year].push_back(paper);
    }
    return grouped;
}

std::map<std::string, std::vector<Paper>> PaperApiModule::groupByTag(const std::vector<Paper>& papers) {
    std::map<std::string, std::vector<Paper>> grouped;
    for (const auto& paper : papers) {
        for (const auto& tag : paper.tags) {
            grouped[tag].push_back(paper);
        }
    }
    return grouped;
}

// ============================================================================
// 路由注册和处理
// ============================================================================

void PaperApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix(); // "/api/papers"

    spdlog::info("[PaperApiModule] Registering routes with prefix: {}", prefix);

    // GET /api/papers - 论文列表
    router.get(prefix, [this](const HttpRequest& req) {
        // 将查询参数转换为map
        std::map<std::string, std::string> params;
        for (const auto& pair : req.queryParams) {
            params[pair.first] = pair.second;
        }

        std::string jsonResult = handleListPapers(params);

        HttpResponse response;
        response.statusCode = 200;
        response.setJson(jsonResult);
        return response;
    });

    // GET /api/papers/:id - 论文详情
    router.get(prefix + "/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");

        std::string jsonResult = handleGetPaper(params);

        HttpResponse response;
        // handleGetPaper返回的JSON可能是error或success
        if (jsonResult.find("\"error\"") != std::string::npos) {
            response.statusCode = 404;
        } else {
            response.statusCode = 200;
        }
        response.setJson(jsonResult);
        return response;
    });

    // POST /api/papers - 创建论文
    router.post(prefix, [this](const HttpRequest& req) {
        std::string jsonResult = handleCreatePaper(req.body);

        HttpResponse response;
        response.statusCode = 201;
        response.setJson(jsonResult);
        return response;
    });

    // PUT /api/papers/:id - 更新论文
    router.put(prefix + "/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");

        std::string jsonResult = handleUpdatePaper(params, req.body);

        HttpResponse response;
        response.statusCode = 200;
        response.setJson(jsonResult);
        return response;
    });

    // DELETE /api/papers/:id - 删除论文
    router.del(prefix + "/:id", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        params["id"] = req.getPathParam("id", "0");

        std::string jsonResult = handleDeletePaper(params);

        HttpResponse response;
        response.statusCode = 200;
        response.setJson(jsonResult);
        return response;
    });

    // GET /api/papers/search - 搜索论文
    router.get(prefix + "/search", [this](const HttpRequest& req) {
        std::map<std::string, std::string> params;
        for (const auto& pair : req.queryParams) {
            params[pair.first] = pair.second;
        }

        std::string jsonResult = handleSearch(params);

        HttpResponse response;
        response.statusCode = 200;
        response.setJson(jsonResult);
        return response;
    });

    // GET /api/papers/stats - 统计信息
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        std::string jsonResult = handleStats();

        HttpResponse response;
        response.statusCode = 200;
        response.setJson(jsonResult);
        return response;
    });

    spdlog::info("[PaperApiModule] Registered 7 routes");
}

std::string PaperApiModule::handleListPapers(const std::map<std::string, std::string>& params) {
    int page = 1;
    int limit = 20;

    auto pageIt = params.find("page");
    if (pageIt != params.end()) {
        page = std::stoi(pageIt->second);
    }

    auto limitIt = params.find("limit");
    if (limitIt != params.end()) {
        limit = std::stoi(limitIt->second);
    }

    auto papers = listPapers(page, limit);

    // 构建JSON
    std::ostringstream json;
    json << "[";
    bool first = true;
    for (const auto& paper : papers) {
        if (!first) json << ",";
        first = false;
        json << paper.toJSON();
    }
    json << "]";

    return JsonHelper::buildPapersJsonResponse(json.str(), papers.size(), page, limit);
}

std::string PaperApiModule::handleGetPaper(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return JsonHelper::buildJsonResponse({{"error", "Missing paper ID"}}, 400);
    }

    int id = std::stoi(idIt->second);
    auto paper = getPaper(id);

    if (!paper.has_value()) {
        return JsonHelper::buildJsonResponse({{"error", "Paper not found"}}, 404);
    }

    return JsonHelper::buildPapersJsonResponse("[" + paper->toJSON() + "]", 1, 1, 1);
}

std::string PaperApiModule::handleCreatePaper(const std::string& body) {
    // TODO: 解析 JSON body
    Paper paper;
    auto newPaper = createPaper(paper);

    if (newPaper.has_value()) {
        return JsonHelper::buildJsonResponse({
            {"success", "true"},
            {"message", "Paper created"},
            {"id", std::to_string(newPaper->id)}
        }, 201);
    }

    return JsonHelper::buildJsonResponse({{"error", "Failed to create paper"}}, 500);
}

std::string PaperApiModule::handleUpdatePaper(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return JsonHelper::buildJsonResponse({{"error", "Missing paper ID"}}, 400);
    }

    int id = std::stoi(idIt->second);

    // TODO: 解析 JSON body
    Paper paper;

    if (updatePaper(id, paper)) {
        return JsonHelper::buildJsonResponse({
            {"success", "true"},
            {"message", "Paper updated"}
        });
    }

    return JsonHelper::buildJsonResponse({{"error", "Failed to update paper"}}, 500);
}

std::string PaperApiModule::handleDeletePaper(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return JsonHelper::buildJsonResponse({{"error", "Missing paper ID"}}, 400);
    }

    int id = std::stoi(idIt->second);

    if (deletePaper(id)) {
        return JsonHelper::buildJsonResponse({
            {"success", "true"},
            {"message", "Paper deleted"}
        });
    }

    return JsonHelper::buildJsonResponse({{"error", "Failed to delete paper"}}, 500);
}

std::string PaperApiModule::handleSearch(const std::map<std::string, std::string>& params) {
    PaperSearchCriteria criteria;

    auto queryIt = params.find("query");
    if (queryIt != params.end()) {
        criteria.query = queryIt->second;
    }

    auto yearIt = params.find("yearFrom");
    if (yearIt != params.end()) {
        criteria.yearFrom = std::stoi(yearIt->second);
    }

    int page = 1;
    int limit = 20;

    auto pageIt = params.find("page");
    if (pageIt != params.end()) {
        page = std::stoi(pageIt->second);
    }

    auto limitIt = params.find("limit");
    if (limitIt != params.end()) {
        limit = std::stoi(limitIt->second);
    }

    auto papers = searchPapers(criteria, page, limit);

    // 构建JSON
    std::ostringstream json;
    json << "[";
    bool first = true;
    for (const auto& paper : papers) {
        if (!first) json << ",";
        first = false;
        json << paper.toJSON();
    }
    json << "]";

    return JsonHelper::buildPapersJsonResponse(json.str(), papers.size(), page, limit);
}

std::string PaperApiModule::handleStats() {
    auto stats = getStats();

    std::map<std::string, std::string> statsMap;
    statsMap["totalPapers"] = std::to_string(stats.totalPapers);
    statsMap["readPapers"] = std::to_string(stats.readPapers);
    statsMap["unreadPapers"] = std::to_string(stats.unreadPapers);
    statsMap["favoritePapers"] = std::to_string(stats.favoritePapers);

    return JsonHelper::buildStatsJsonResponse(statsMap);
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    return new PaperCrawler::PaperApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::PaperApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}

