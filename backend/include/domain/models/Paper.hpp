#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace PaperCrawler {

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
    std::string source;
    std::string category;
    std::string tags;
    int citationCount = 0;
    bool isRead = false;
    bool isBookmarked = false;
    int readingProgress = 0;
    std::string notes;
    std::string createdAt;
    std::string updatedAt;

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

    static Paper fromDbRow(const std::map<std::string, std::string>& row) {
        Paper paper;
        paper.id = std::stoi(row.at("id"));
        paper.userId = row.count("user_id") ? std::stoi(row.at("user_id")) : 0;
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

struct PaperStats {
    int totalPapers = 0;
    int readPapers = 0;
    int unreadPapers = 0;
    int bookmarkedPapers = 0;

    nlohmann::json toJson() const {
        return {
            {"totalPapers", totalPapers},
            {"readPapers", readPapers},
            {"unreadPapers", unreadPapers},
            {"bookmarkedPapers", bookmarkedPapers}
        };
    }
};

} // namespace PaperCrawler
