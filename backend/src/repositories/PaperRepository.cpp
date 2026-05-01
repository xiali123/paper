#include "repositories/PaperRepository.hpp"
#include "data/PreparedStatement.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>

namespace PaperCrawler {

PaperRepository::PaperRepository(std::shared_ptr<IDatabase> database)
    : database_(database) {}

Paper PaperRepository::paperFromDbRow(const std::map<std::string, std::string>& row) {
    Paper paper;
    paper.id = std::stoi(row.at("id"));
    paper.title = row.at("title");
    paper.authors = row.at("authors");

    if (row.count("user_id") && !row.at("user_id").empty()) {
        try { paper.userId = std::stoi(row.at("user_id")); } catch (...) {}
    }

    if (row.count("publication_date") && !row.at("publication_date").empty()) {
        try {
            paper.year = row.at("publication_date").substr(0, 4);
        } catch (...) {
            paper.year = row.count("year") ? row.at("year") : "2023";
        }
    } else {
        paper.year = row.count("year") ? row.at("year") : "2023";
    }

    paper.abstract = row.count("abstract") ? row.at("abstract") : "";
    paper.publication = row.count("journal") ? row.at("journal") : "";
    paper.volume = row.count("volume") ? row.at("volume") : "";
    paper.issue = row.count("issue") ? row.at("issue") : "";
    paper.pages = row.count("pages") ? row.at("pages") : "";
    paper.doi = row.count("doi") ? row.at("doi") : "";
    paper.url = row.count("url") ? row.at("url") : "";
    paper.pdfPath = row.count("pdf_url") ? row.at("pdf_url") :
                    row.count("pdf_path") ? row.at("pdf_path") : "";
    paper.citationCount = row.count("citation_count") ? std::stoi(row.at("citation_count")) : 0;
    paper.isRead = row.count("is_read") ? (row.at("is_read") == "1") : false;
    paper.isBookmarked = row.count("is_favorite") ? (row.at("is_favorite") == "1") :
                         row.count("is_bookmarked") ? (row.at("is_bookmarked") == "1") : false;
    paper.notes = row.count("notes") ? row.at("notes") : "";
    paper.createdAt = row.count("created_at") ? row.at("created_at") : "";
    paper.updatedAt = row.count("updated_at") ? row.at("updated_at") : "";
    return paper;
}

// ============================================================================
// IPaperRepository interface (Domain::Paper-based)
// ============================================================================

bool PaperRepository::save(const Domain::Paper& paper) {
    try {
        auto sql = "INSERT INTO papers (title, authors, abstract, keywords, doi, publication, "
                  "year, volume, issue, pages, url, pdf_path, source, category, tags, "
                  "citation_count, is_read, is_bookmarked, reading_progress, notes, user_id, "
                  "created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                  "?, ?, ?, ?, ?, ?, ?, NOW(), NOW())";
        PreparedStatement stmt(database_, sql);
        stmt.bind(0, paper.title).bind(1, paper.authors).bind(2, paper.abstract)
            .bind(3, paper.keywords).bind(4, paper.doi).bind(5, paper.publication)
            .bind(6, paper.year).bind(7, paper.volume).bind(8, paper.issue)
            .bind(9, paper.pages).bind(10, paper.url).bind(11, paper.pdfPath)
            .bind(12, paper.source).bind(13, paper.category).bind(14, paper.tags)
            .bind(15, paper.citationCount).bind(16, paper.isRead ? 1 : 0)
            .bind(17, paper.isBookmarked ? 1 : 0).bind(18, paper.readingProgress)
            .bind(19, paper.notes).bind(20, paper.userId);
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] save failed: {}", e.what());
        return false;
    }
}

size_t PaperRepository::batchSave(const std::vector<Domain::Paper>& papers) {
    size_t saved = 0;
    for (const auto& p : papers) {
        if (save(p)) ++saved;
    }
    return saved;
}

std::optional<Domain::Paper> PaperRepository::findById(const std::string& id) {
    try {
        PreparedStatement stmt(database_, "SELECT * FROM papers WHERE id = ?");
        stmt.bind(0, std::stoi(id));
        auto results = stmt.query();
        if (!results.empty()) {
            return paperFromDbRow(results[0]);
        }
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] findById failed: {}", e.what());
    }
    return std::nullopt;
}

std::vector<Domain::Paper> PaperRepository::findByIds(const std::vector<std::string>& ids) {
    std::vector<Domain::Paper> papers;
    for (const auto& id : ids) {
        auto p = findById(id);
        if (p) papers.push_back(*p);
    }
    return papers;
}

std::vector<Domain::Paper> PaperRepository::findByAuthor(const std::string& author, int limit) {
    try {
        PreparedStatement stmt(database_,
            "SELECT * FROM papers WHERE authors LIKE ? ORDER BY citation_count DESC LIMIT ?");
        stmt.bind(0, "%" + author + "%").bind(1, limit);
        auto results = stmt.query();
        std::vector<Domain::Paper> papers;
        for (const auto& row : results) {
            papers.push_back(paperFromDbRow(row));
        }
        return papers;
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] findByAuthor failed: {}", e.what());
        return {};
    }
}

std::vector<Domain::Paper> PaperRepository::search(const Domain::PaperQuery& query) {
    try {
        std::string sql = "SELECT * FROM papers WHERE 1=1";
        int paramIdx = 0;
        std::vector<ParameterValue> params;

        if (!query.searchQuery.empty()) {
            sql += " AND (title LIKE ? OR authors LIKE ? OR abstract LIKE ?)";
            std::string pattern = "%" + query.searchQuery + "%";
            params.push_back(pattern); params.push_back(pattern); params.push_back(pattern);
            paramIdx += 3;
        }
        if (!query.yearFrom.empty()) {
            sql += " AND year >= ?";
            params.push_back(query.yearFrom); paramIdx++;
        }
        if (!query.yearTo.empty()) {
            sql += " AND year <= ?";
            params.push_back(query.yearTo); paramIdx++;
        }
        sql += " LIMIT ? OFFSET ?";
        params.push_back(query.limit); params.push_back(query.offset);
        paramIdx += 2;

        PreparedStatement stmt(database_, sql);
        for (int i = 0; i < paramIdx; i++) {
            stmt.bind(i, params[i]);
        }
        auto results = stmt.query();
        std::vector<Domain::Paper> papers;
        for (const auto& row : results) {
            papers.push_back(paperFromDbRow(row));
        }
        return papers;
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] search failed: {}", e.what());
        return {};
    }
}

std::vector<Domain::Paper> PaperRepository::fullTextSearch(const std::string& searchText, int limit) {
    try {
        PreparedStatement stmt(database_,
            "SELECT * FROM papers WHERE MATCH(title, abstract, authors) AGAINST(? IN BOOLEAN MODE) LIMIT ?");
        stmt.bind(0, searchText).bind(1, limit);
        auto results = stmt.query();
        std::vector<Domain::Paper> papers;
        for (const auto& row : results) {
            papers.push_back(paperFromDbRow(row));
        }
        return papers;
    } catch (const std::exception& e) {
        spdlog::warn("[PaperRepository] fullTextSearch failed, falling back to LIKE: {}", e.what());
        return search(Domain::PaperQuery{{}, {}, {}, {}, searchText, {}, limit, 0});
    }
}

bool PaperRepository::update(const Domain::Paper& paper) {
    try {
        auto sql = "UPDATE papers SET title=?, authors=?, abstract=?, keywords=?, doi=?, "
                  "publication=?, year=?, volume=?, issue=?, pages=?, url=?, pdf_path=?, "
                  "source=?, category=?, tags=?, citation_count=?, is_read=?, is_bookmarked=?, "
                  "reading_progress=?, notes=?, updated_at=NOW() WHERE id=?";
        PreparedStatement stmt(database_, sql);
        stmt.bind(0, paper.title).bind(1, paper.authors).bind(2, paper.abstract)
            .bind(3, paper.keywords).bind(4, paper.doi).bind(5, paper.publication)
            .bind(6, paper.year).bind(7, paper.volume).bind(8, paper.issue)
            .bind(9, paper.pages).bind(10, paper.url).bind(11, paper.pdfPath)
            .bind(12, paper.source).bind(13, paper.category).bind(14, paper.tags)
            .bind(15, paper.citationCount).bind(16, paper.isRead ? 1 : 0)
            .bind(17, paper.isBookmarked ? 1 : 0).bind(18, paper.readingProgress)
            .bind(19, paper.notes).bind(20, paper.id);
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] update failed: {}", e.what());
        return false;
    }
}

bool PaperRepository::deleteById(const std::string& id) {
    try {
        PreparedStatement stmt(database_, "DELETE FROM papers WHERE id = ?");
        stmt.bind(0, std::stoi(id));
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] deleteById failed: {}", e.what());
        return false;
    }
}

size_t PaperRepository::batchDelete(const std::vector<std::string>& ids) {
    size_t deleted = 0;
    for (const auto& id : ids) {
        if (deleteById(id)) ++deleted;
    }
    return deleted;
}

size_t PaperRepository::count() const {
    try {
        auto results = database_->query("SELECT COUNT(*) as count FROM papers");
        if (!results.empty()) {
            return std::stoul(results[0].at("count"));
        }
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] count failed: {}", e.what());
    }
    return 0;
}

bool PaperRepository::exists(const std::string& id) const {
    try {
        PreparedStatement stmt(database_, "SELECT 1 FROM papers WHERE id = ? LIMIT 1");
        stmt.bind(0, std::stoi(id));
        return !stmt.query().empty();
    } catch (const std::exception& e) {
        return false;
    }
}

std::vector<Domain::Paper> PaperRepository::getRecent(int limit) {
    try {
        PreparedStatement stmt(database_, "SELECT * FROM papers ORDER BY created_at DESC LIMIT ?");
        stmt.bind(0, limit);
        auto results = stmt.query();
        std::vector<Domain::Paper> papers;
        for (const auto& row : results) {
            papers.push_back(paperFromDbRow(row));
        }
        return papers;
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] getRecent failed: {}", e.what());
        return {};
    }
}

std::vector<Domain::Paper> PaperRepository::getTopCited(int limit) {
    try {
        PreparedStatement stmt(database_, "SELECT * FROM papers ORDER BY citation_count DESC LIMIT ?");
        stmt.bind(0, limit);
        auto results = stmt.query();
        std::vector<Domain::Paper> papers;
        for (const auto& row : results) {
            papers.push_back(paperFromDbRow(row));
        }
        return papers;
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] getTopCited failed: {}", e.what());
        return {};
    }
}

Domain::IPaperRepository::RepositoryStats PaperRepository::getStats() const {
    RepositoryStats stats;
    try {
        auto results = database_->query("SELECT COUNT(*) as count FROM papers");
        if (!results.empty()) stats.totalPapers = std::stoul(results[0].at("count"));

        auto authorRes = database_->query("SELECT COUNT(DISTINCT authors) as count FROM papers");
        if (!authorRes.empty()) stats.totalAuthors = std::stoul(authorRes[0].at("count"));

        auto citeRes = database_->query("SELECT COALESCE(SUM(citation_count), 0) as total FROM papers");
        if (!citeRes.empty()) stats.totalCitations = std::stoull(citeRes[0].at("total"));
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] getStats failed: {}", e.what());
    }
    return stats;
}

void PaperRepository::beginTransaction() { /* TODO: transaction support */ }
void PaperRepository::commitTransaction() { /* TODO: transaction support */ }
void PaperRepository::rollbackTransaction() { /* TODO: transaction support */ }

// ============================================================================
// PaperApiModule-compatible methods (Paper-based)
// ============================================================================

std::optional<Paper> PaperRepository::getPaperById(int id) {
    try {
        PreparedStatement stmt(database_, "SELECT * FROM papers WHERE id = ?");
        stmt.bind(0, id);
        auto results = stmt.query();
        if (!results.empty()) return paperFromDbRow(results[0]);
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] getPaperById({}) failed: {}", id, e.what());
    }
    return std::nullopt;
}

std::vector<Paper> PaperRepository::listPapers(int page, int limit, const std::string& sortBy, bool ascending) {
    std::vector<Paper> papers;
    try {
        int offset = (page - 1) * limit;
        std::string orderDirection = ascending ? "ASC" : "DESC";
        std::string allowedSortBy = sortBy;
        if (sortBy != "title" && sortBy != "citation_count" &&
            sortBy != "created_at" && sortBy != "updated_at" && sortBy != "publication_date") {
            allowedSortBy = "created_at";
        }
        auto sql = "SELECT * FROM papers ORDER BY " + allowedSortBy + " " + orderDirection + " LIMIT ? OFFSET ?";
        PreparedStatement stmt(database_, sql);
        stmt.bind(0, limit).bind(1, offset);
        auto results = stmt.query();
        for (const auto& row : results) {
            papers.push_back(paperFromDbRow(row));
        }
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] listPapers failed: {}", e.what());
    }
    return papers;
}

std::optional<Paper> PaperRepository::createPaper(const Paper& paper) {
    try {
        auto sql = "INSERT INTO papers (title, authors, year, abstract, journal, volume, issue, "
                  "pages, doi, url, pdf_path, citation_count, is_read, is_favorite, notes, "
                  "created_at, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NOW(), NOW())";
        PreparedStatement stmt(database_, sql);
        stmt.bind(0, paper.title).bind(1, paper.authors)
            .bind(2, paper.year).bind(3, paper.abstract)
            .bind(4, paper.publication).bind(5, paper.volume)
            .bind(6, paper.issue).bind(7, paper.pages)
            .bind(8, paper.doi).bind(9, paper.url)
            .bind(10, paper.pdfPath).bind(11, paper.citationCount)
            .bind(12, paper.isRead ? 1 : 0).bind(13, paper.isBookmarked ? 1 : 0)
            .bind(14, paper.notes);
        if (stmt.execute()) {
            PreparedStatement q(database_, "SELECT * FROM papers WHERE title = ? AND year = ? ORDER BY id DESC LIMIT 1");
            q.bind(0, paper.title).bind(1, paper.year);
            auto results = q.query();
            if (!results.empty()) return paperFromDbRow(results[0]);
        }
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] createPaper failed: {}", e.what());
    }
    return std::nullopt;
}

bool PaperRepository::updatePaper(int id, const Paper& paper) {
    try {
        auto sql = "UPDATE papers SET title=?, authors=?, year=?, abstract=?, journal=?, "
                  "volume=?, issue=?, pages=?, doi=?, url=?, pdf_path=?, citation_count=?, "
                  "is_read=?, is_favorite=?, notes=?, updated_at=NOW() WHERE id=?";
        PreparedStatement stmt(database_, sql);
        stmt.bind(0, paper.title).bind(1, paper.authors)
            .bind(2, paper.year).bind(3, paper.abstract)
            .bind(4, paper.publication).bind(5, paper.volume)
            .bind(6, paper.issue).bind(7, paper.pages)
            .bind(8, paper.doi).bind(9, paper.url)
            .bind(10, paper.pdfPath).bind(11, paper.citationCount)
            .bind(12, paper.isRead ? 1 : 0).bind(13, paper.isBookmarked ? 1 : 0)
            .bind(14, paper.notes).bind(15, id);
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] updatePaper failed: {}", e.what());
        return false;
    }
}

bool PaperRepository::deletePaper(int id) {
    try {
        PreparedStatement stmt(database_, "DELETE FROM papers WHERE id = ?");
        stmt.bind(0, id);
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] deletePaper failed: {}", e.what());
        return false;
    }
}

bool PaperRepository::markAsRead(int id, bool read) {
    try {
        PreparedStatement stmt(database_, "UPDATE papers SET is_read = ?, updated_at = NOW() WHERE id = ?");
        stmt.bind(0, read ? 1 : 0).bind(1, id);
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] markAsRead failed: {}", e.what());
        return false;
    }
}

bool PaperRepository::markAsFavorite(int id, bool favorite) {
    try {
        PreparedStatement stmt(database_, "UPDATE papers SET is_favorite = ?, updated_at = NOW() WHERE id = ?");
        stmt.bind(0, favorite ? 1 : 0).bind(1, id);
        return stmt.execute();
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] markAsFavorite failed: {}", e.what());
        return false;
    }
}

PaperStats PaperRepository::getPaperStats() {
    PaperStats stats;
    try {
        auto totalRes = database_->query("SELECT COUNT(*) as count FROM papers");
        if (!totalRes.empty()) stats.totalPapers = std::stoi(totalRes[0]["count"]);

        auto readRes = database_->query("SELECT is_read, COUNT(*) as count FROM papers GROUP BY is_read");
        for (const auto& row : readRes) {
            int count = std::stoi(row.at("count"));
            if (row.at("is_read") == "1") stats.readPapers = count;
            else stats.unreadPapers = count;
        }

        auto favRes = database_->query("SELECT COUNT(*) as count FROM papers WHERE is_favorite = 1");
        if (!favRes.empty()) stats.bookmarkedPapers = std::stoi(favRes[0]["count"]);
    } catch (const std::exception& e) {
        spdlog::error("[PaperRepository] getPaperStats failed: {}", e.what());
    }
    return stats;
}

} // namespace PaperCrawler
