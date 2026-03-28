#pragma once

#include <string>
#include <memory>

namespace PaperCrawler {

/**
 * @brief Paper model representing academic paper information
 */
class Paper {
public:
    Paper() = default;

    // Getters
    int getId() const { return id_; }
    int getKid() const { return kid_; }
    std::string getType() const { return type_; }
    std::string getTitle() const { return title_; }
    std::string getJournalFull() const { return journalFull_; }
    std::string getJournalShort() const { return journalShort_; }
    std::string getYear() const { return year_; }
    std::string getAuthor() const { return author_; }
    std::string getJournalUrl() const { return journalUrl_; }
    std::string getDoiUrl() const { return doiUrl_; }
    std::string getInfo() const { return info_; }
    int getQkid() const { return qkid_; }
    std::string getLevel() const { return level_; }

    // Setters
    void setId(int id) { id_ = id; }
    void setKid(int kid) { kid_ = kid; }
    void setType(const std::string& type) { type_ = type; }
    void setTitle(const std::string& title) { title_ = title; }
    void setJournalFull(const std::string& journal) { journalFull_ = journal; }
    void setJournalShort(const std::string& journal) { journalShort_ = journal; }
    void setYear(const std::string& year) { year_ = year; }
    void setAuthor(const std::string& author) { author_ = author; }
    void setJournalUrl(const std::string& url) { journalUrl_ = url; }
    void setDoiUrl(const std::string& url) { doiUrl_ = url; }
    void setInfo(const std::string& info) { info_ = info; }
    void setQkid(int qkid) { qkid_ = qkid; }
    void setLevel(const std::string& level) { level_ = level; }

    /**
     * @brief Convert paper to string representation
     */
    std::string toString() const {
        return "Paper{id=" + std::to_string(id_) +
               ", title=" + title_ +
               ", journal=" + journalShort_ +
               ", year=" + year_ + "}";
    }

private:
    int id_{0};
    int kid_{0};
    std::string type_;
    std::string title_;
    std::string journalFull_;
    std::string journalShort_;
    std::string year_;
    std::string author_;
    std::string journalUrl_;
    std::string doiUrl_;
    std::string info_;
    int qkid_{0};
    std::string level_;
};

} // namespace PaperCrawler
