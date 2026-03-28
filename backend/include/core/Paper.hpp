/**
 * @file Paper.hpp
 * @brief 论文数据模型
 * @details 定义论文的数据结构和访问接口
 */

#ifndef PAPERCRAWLER_PAPER_HPP
#define PAPERCRAWLER_PAPER_HPP

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "data/DatabaseManager.hpp"

namespace PaperCrawler {

/**
 * @brief 论文数据模型
 */
struct Paper {
    int id = 0;
    int userId = 0;
    std::string title;
    std::string authors;
    std::string abstract;
    std::string keywords;
    std::string doi;
    std::string publication;
    std::string year;
    std::string volume;
    std::string issue;
    std::string pages;
    std::string url;
    std::string pdfPath;
    std::string source; // 爬虫来源：cnki, ieee, arxiv, pubmed, manual
    std::string category;
    std::string tags;
    int citationCount = 0;
    bool isRead = false;
    bool isBookmarked = false;
    int readingProgress = 0; // 阅读进度 0-100
    std::string notes;
    std::string createdAt;
    std::string updatedAt;

    /**
     * @brief 从JSON构造
     */
    static Paper fromJson(const nlohmann::json& json) {
        Paper paper;
        paper.id = json.value("id", 0);
        paper.userId = json.value("userId", 0);
        paper.title = json.value("title", "");
        paper.authors = json.value("authors", "");
        paper.abstract = json.value("abstract", "");
        paper.keywords = json.value("keywords", "");
        paper.doi = json.value("doi", "");
        paper.publication = json.value("publication", "");
        paper.year = json.value("year", "");
        paper.volume = json.value("volume", "");
        paper.issue = json.value("issue", "");
        paper.pages = json.value("pages", "");
        paper.url = json.value("url", "");
        paper.pdfPath = json.value("pdfPath", "");
        paper.source = json.value("source", "manual");
        paper.category = json.value("category", "");
        paper.tags = json.value("tags", "");
        paper.citationCount = json.value("citationCount", 0);
        paper.isRead = json.value("isRead", false);
        paper.isBookmarked = json.value("isBookmarked", false);
        paper.readingProgress = json.value("readingProgress", 0);
        paper.notes = json.value("notes", "");
        paper.createdAt = json.value("createdAt", "");
        paper.updatedAt = json.value("updatedAt", "");
        return paper;
    }

    /**
     * @brief 转换为JSON
     */
    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"userId", userId},
            {"title", title},
            {"authors", authors},
            {"abstract", abstract},
            {"keywords", keywords},
            {"doi", doi},
            {"publication", publication},
            {"year", year},
            {"volume", volume},
            {"issue", issue},
            {"pages", pages},
            {"url", url},
            {"pdfPath", pdfPath},
            {"source", source},
            {"category", category},
            {"tags", tags},
            {"citationCount", citationCount},
            {"isRead", isRead},
            {"isBookmarked", isBookmarked},
            {"readingProgress", readingProgress},
            {"notes", notes},
            {"createdAt", createdAt},
            {"updatedAt", updatedAt}
        };
    }

    /**
     * @brief 从数据库结果构造
     */
    static Paper fromDbRow(const std::map<std::string, std::string>& row) {
        Paper paper;
        paper.id = std::stoi(row.at("id"));
        paper.userId = std::stoi(row.at("user_id"));
        paper.title = row.at("title");
        paper.authors = row.count("authors") ? row.at("authors") : "";
        paper.abstract = row.count("abstract") ? row.at("abstract") : "";
        paper.keywords = row.count("keywords") ? row.at("keywords") : "";
        paper.doi = row.count("doi") ? row.at("doi") : "";
        paper.publication = row.count("publication") ? row.at("publication") : "";
        paper.year = row.count("year") ? row.at("year") : "";
        paper.volume = row.count("volume") ? row.at("volume") : "";
        paper.issue = row.count("issue") ? row.at("issue") : "";
        paper.pages = row.count("pages") ? row.at("pages") : "";
        paper.url = row.count("url") ? row.at("url") : "";
        paper.pdfPath = row.count("pdf_path") ? row.at("pdf_path") : "";
        paper.source = row.count("source") ? row.at("source") : "manual";
        paper.category = row.count("category") ? row.at("category") : "";
        paper.tags = row.count("tags") ? row.at("tags") : "";
        paper.citationCount = row.count("citation_count") ? std::stoi(row.at("citation_count")) : 0;
        paper.isRead = row.count("is_read") ? (row.at("is_read") == "1") : false;
        paper.isBookmarked = row.count("is_bookmarked") ? (row.at("is_bookmarked") == "1") : false;
        paper.readingProgress = row.count("reading_progress") ? std::stoi(row.at("reading_progress")) : 0;
        paper.notes = row.count("notes") ? row.at("notes") : "";
        paper.createdAt = row.count("created_at") ? row.at("created_at") : "";
        paper.updatedAt = row.count("updated_at") ? row.at("updated_at") : "";
        return paper;
    }
};

/**
 * @brief 论文查询参数
 */
struct PaperQuery {
    int userId = 0;
    std::string keyword;
    std::string category;
    std::string tags;
    std::string source;
    bool isRead = false; // -1:全部, 0:未读, 1:已读
    bool isBookmarked = false; // -1:全部, 0:未收藏, 1:已收藏
    std::string orderBy = "created_at";
    std::string order = "DESC";
    int page = 1;
    int pageSize = 20;

    /**
     * @brief 构建WHERE条件
     */
    std::string buildWhereClause(DatabaseManager& db) const {
        std::vector<std::string> conditions;

        conditions.push_back("user_id = " + std::to_string(userId));

        if (!keyword.empty()) {
            std::string escaped = db.escape(keyword);
            conditions.push_back("(title LIKE '%" + escaped + "%' OR "
                               "authors LIKE '%" + escaped + "%' OR "
                               "abstract LIKE '%" + escaped + "%')");
        }

        if (!category.empty()) {
            conditions.push_back("category = '" + db.escape(category) + "'");
        }

        if (!tags.empty()) {
            conditions.push_back("tags LIKE '%" + db.escape(tags) + "%'");
        }

        if (!source.empty()) {
            conditions.push_back("source = '" + db.escape(source) + "'");
        }

        if (isRead) {
            conditions.push_back("is_read = 1");
        }

        if (isBookmarked) {
            conditions.push_back("is_bookmarked = 1");
        }

        if (conditions.empty()) {
            return "";
        }

        std::string result = conditions[0];
        for (size_t i = 1; i < conditions.size(); i++) {
            result += " AND " + conditions[i];
        }
        return result;
    }
};

/**
 * @brief 论文统计信息
 */
struct PaperStats {
    int totalPapers = 0;
    int readPapers = 0;
    int unreadPapers = 0;
    int bookmarkedPapers = 0;
    int papersBySource[4] = {0}; // manual, cnki, ieee, arxiv
    int papersByCategory[10] = {0}; // 根据实际分类数量调整

    nlohmann::json toJson() const {
        return {
            {"totalPapers", totalPapers},
            {"readPapers", readPapers},
            {"unreadPapers", unreadPapers},
            {"bookmarkedPapers", bookmarkedPapers},
            {"papersBySource", papersBySource},
            {"papersByCategory", papersByCategory}
        };
    }
};

/**
 * @brief 论文访问层（Repository）
 */
class PaperRepository {
public:
    /**
     * @brief 创建论文
     */
    static bool create(const Paper& paper, int& outId) {
        auto& db = DatabaseManager::getInstance();

        std::map<std::string, std::string> data;
        data["user_id"] = std::to_string(paper.userId);
        data["title"] = paper.title;
        data["authors"] = paper.authors;
        data["abstract"] = paper.abstract;
        data["keywords"] = paper.keywords;
        data["doi"] = paper.doi;
        data["publication"] = paper.publication;
        data["year"] = paper.year;
        data["volume"] = paper.volume;
        data["issue"] = paper.issue;
        data["pages"] = paper.pages;
        data["url"] = paper.url;
        data["pdf_path"] = paper.pdfPath;
        data["source"] = paper.source;
        data["category"] = paper.category;
        data["tags"] = paper.tags;
        data["citation_count"] = std::to_string(paper.citationCount);
        data["is_read"] = paper.isRead ? "1" : "0";
        data["is_bookmarked"] = paper.isBookmarked ? "1" : "0";
        data["reading_progress"] = std::to_string(paper.readingProgress);
        data["notes"] = paper.notes;

        std::string sql = db.buildInsert("papers", data);
        auto result = db.execute(sql);

        if (result.success) {
            outId = result.insertId;
            return true;
        }
        return false;
    }

    /**
     * @brief 根据ID获取论文
     */
    static std::unique_ptr<Paper> getById(int id, int userId) {
        auto& db = DatabaseManager::getInstance();

        std::string where = "id = " + std::to_string(id) + " AND user_id = " + std::to_string(userId);
        std::string sql = db.buildSelect("papers", "*", where);

        auto result = db.query(sql);
        if (result.success && !result.rows.empty()) {
            return std::make_unique<Paper>(Paper::fromDbRow(result.rows[0]));
        }
        return nullptr;
    }

    /**
     * @brief 查询论文列表
     */
    static std::vector<Paper> query(const PaperQuery& query) {
        auto& db = DatabaseManager::getInstance();

        std::string where = query.buildWhereClause(db);
        std::string order = query.orderBy + " " + query.order;
        int offset = (query.page - 1) * query.pageSize;

        std::string sql = db.buildSelect("papers", "*", where, order, query.pageSize, offset);

        auto result = db.query(sql);
        std::vector<Paper> papers;

        if (result.success) {
            for (const auto& row : result.rows) {
                papers.push_back(Paper::fromDbRow(row));
            }
        }
        return papers;
    }

    /**
     * @brief 统计论文数量
     */
    static int count(const PaperQuery& query) {
        auto& db = DatabaseManager::getInstance();

        std::string where = query.buildWhereClause(db);
        std::string sql = db.buildSelect("papers", "COUNT(*) as count", where);

        auto result = db.query(sql);
        if (result.success && !result.rows.empty()) {
            return std::stoi(result.rows[0].at("count"));
        }
        return 0;
    }

    /**
     * @brief 更新论文
     */
    static bool update(const Paper& paper) {
        auto& db = DatabaseManager::getInstance();

        std::map<std::string, std::string> data;
        data["title"] = paper.title;
        data["authors"] = paper.authors;
        data["abstract"] = paper.abstract;
        data["keywords"] = paper.keywords;
        data["doi"] = paper.doi;
        data["publication"] = paper.publication;
        data["year"] = paper.year;
        data["volume"] = paper.volume;
        data["issue"] = paper.issue;
        data["pages"] = paper.pages;
        data["url"] = paper.url;
        data["pdf_path"] = paper.pdfPath;
        data["source"] = paper.source;
        data["category"] = paper.category;
        data["tags"] = paper.tags;
        data["citation_count"] = std::to_string(paper.citationCount);
        data["is_read"] = paper.isRead ? "1" : "0";
        data["is_bookmarked"] = paper.isBookmarked ? "1" : "0";
        data["reading_progress"] = std::to_string(paper.readingProgress);
        data["notes"] = paper.notes;
        data["updated_at"] = "NOW()";

        std::string where = "id = " + std::to_string(paper.id) + " AND user_id = " + std::to_string(paper.userId);
        std::string sql = db.buildUpdate("papers", data, where);

        auto result = db.execute(sql);
        return result.success;
    }

    /**
     * @brief 删除论文
     */
    static bool remove(int id, int userId) {
        auto& db = DatabaseManager::getInstance();

        std::string where = "id = " + std::to_string(id) + " AND user_id = " + std::to_string(userId);
        std::string sql = db.buildDelete("papers", where);

        auto result = db.execute(sql);
        return result.success;
    }

    /**
     * @brief 切换收藏状态
     */
    static bool toggleBookmark(int id, int userId) {
        auto& db = DatabaseManager::getInstance();

        std::string sql = "UPDATE papers SET is_bookmarked = NOT is_bookmarked "
                         "WHERE id = " + std::to_string(id) + " AND user_id = " + std::to_string(userId);

        auto result = db.execute(sql);
        return result.success;
    }

    /**
     * @brief 标记为已读/未读
     */
    static bool markAsRead(int id, int userId, bool isRead) {
        auto& db = DatabaseManager::getInstance();

        std::string sql = "UPDATE papers SET is_read = " + std::string(isRead ? "1" : "0") +
                         " WHERE id = " + std::to_string(id) + " AND user_id = " + std::to_string(userId);

        auto result = db.execute(sql);
        return result.success;
    }

    /**
     * @brief 更新阅读进度
     */
    static bool updateReadingProgress(int id, int userId, int progress) {
        auto& db = DatabaseManager::getInstance();

        std::string sql = "UPDATE papers SET reading_progress = " + std::to_string(progress);
        if (progress >= 100) {
            sql += ", is_read = 1";
        }
        sql += " WHERE id = " + std::to_string(id) + " AND user_id = " + std::to_string(userId);

        auto result = db.execute(sql);
        return result.success;
    }

    /**
     * @brief 获取统计信息
     */
    static PaperStats getStats(int userId) {
        auto& db = DatabaseManager::getInstance();
        PaperStats stats;

        // 总数
        std::string sql = "SELECT COUNT(*) as count FROM papers WHERE user_id = " + std::to_string(userId);
        auto result = db.query(sql);
        if (result.success && !result.rows.empty()) {
            stats.totalPapers = std::stoi(result.rows[0].at("count"));
        }

        // 已读
        sql = "SELECT COUNT(*) as count FROM papers WHERE user_id = " + std::to_string(userId) + " AND is_read = 1";
        result = db.query(sql);
        if (result.success && !result.rows.empty()) {
            stats.readPapers = std::stoi(result.rows[0].at("count"));
        }

        stats.unreadPapers = stats.totalPapers - stats.readPapers;

        // 收藏
        sql = "SELECT COUNT(*) as count FROM papers WHERE user_id = " + std::to_string(userId) + " AND is_bookmarked = 1";
        result = db.query(sql);
        if (result.success && !result.rows.empty()) {
            stats.bookmarkedPapers = std::stoi(result.rows[0].at("count"));
        }

        // 按来源统计
        sql = "SELECT source, COUNT(*) as count FROM papers WHERE user_id = " + std::to_string(userId) + " GROUP BY source";
        result = db.query(sql);
        if (result.success) {
            for (const auto& row : result.rows) {
                std::string source = row.at("source");
                int count = std::stoi(row.at("count"));
                if (source == "manual") stats.papersBySource[0] = count;
                else if (source == "cnki") stats.papersBySource[1] = count;
                else if (source == "ieee") stats.papersBySource[2] = count;
                else if (source == "arxiv") stats.papersBySource[3] = count;
            }
        }

        return stats;
    }
};

} // namespace PaperCrawler

#endif // PAPERCRAWLER_PAPER_HPP
