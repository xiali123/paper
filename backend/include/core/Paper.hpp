#pragma once

#include "domain/models/Paper.hpp"
#include "data/DatabaseManager.hpp"

namespace PaperCrawler {

struct PaperQuery {
    int userId = 0;
    std::string keyword;
    std::string category;
    std::string tags;
    std::string source;
    bool isRead = false;
    bool isBookmarked = false;
    std::string orderBy = "created_at";
    std::string order = "DESC";
    int page = 1;
    int pageSize = 20;

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

class PaperRepository {
public:
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

    static bool remove(int id, int userId) {
        auto& db = DatabaseManager::getInstance();

        std::string where = "id = " + std::to_string(id) + " AND user_id = " + std::to_string(userId);
        std::string sql = db.buildDelete("papers", where);

        auto result = db.execute(sql);
        return result.success;
    }

    static bool toggleBookmark(int id, int userId) {
        auto& db = DatabaseManager::getInstance();

        std::string sql = "UPDATE papers SET is_bookmarked = NOT is_bookmarked "
                         "WHERE id = " + std::to_string(id) + " AND user_id = " + std::to_string(userId);

        auto result = db.execute(sql);
        return result.success;
    }

    static bool markAsRead(int id, int userId, bool isRead) {
        auto& db = DatabaseManager::getInstance();

        std::string sql = "UPDATE papers SET is_read = " + std::string(isRead ? "1" : "0") +
                         " WHERE id = " + std::to_string(id) + " AND user_id = " + std::to_string(userId);

        auto result = db.execute(sql);
        return result.success;
    }

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

    static PaperStats getStats(int userId) {
        auto& db = DatabaseManager::getInstance();
        PaperStats stats;

        std::string sql = "SELECT COUNT(*) as count FROM papers WHERE user_id = " + std::to_string(userId);
        auto result = db.query(sql);
        if (result.success && !result.rows.empty()) {
            stats.totalPapers = std::stoi(result.rows[0].at("count"));
        }

        sql = "SELECT COUNT(*) as count FROM papers WHERE user_id = " + std::to_string(userId) + " AND is_read = 1";
        result = db.query(sql);
        if (result.success && !result.rows.empty()) {
            stats.readPapers = std::stoi(result.rows[0].at("count"));
        }

        stats.unreadPapers = stats.totalPapers - stats.readPapers;

        sql = "SELECT COUNT(*) as count FROM papers WHERE user_id = " + std::to_string(userId) + " AND is_bookmarked = 1";
        result = db.query(sql);
        if (result.success && !result.rows.empty()) {
            stats.bookmarkedPapers = std::stoi(result.rows[0].at("count"));
        }

        return stats;
    }
};

} // namespace PaperCrawler
