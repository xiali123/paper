#pragma once

#include <string>
#include <memory>

namespace PaperCrawler {

/**
 * @brief Journal/Conference model
 */
class Journal {
public:
    Journal() = default;

    // Getters
    int getId() const { return id_; }
    std::string getName() const { return name_; }
    std::string getFullName() const { return fullName_; }
    std::string getLevel() const { return level_; }
    std::string getFLevel() const { return fLevel_; }
    std::string getInfo() const { return info_; }
    std::string getUrl() const { return url_; }

    // Setters
    void setId(int id) { id_ = id; }
    void setName(const std::string& name) { name_ = name; }
    void setFullName(const std::string& fullName) { fullName_ = fullName; }
    void setLevel(const std::string& level) { level_ = level; }
    void setFLevel(const std::string& fLevel) { fLevel_ = fLevel; }
    void setInfo(const std::string& info) { info_ = info; }
    void setUrl(const std::string& url) { url_ = url; }

    /**
     * @brief Check if this is a top-tier journal/conference
     */
    bool isTopTier() const {
        return level_ == "a" || level_ == "A";
    }

    /**
     * @brief Convert to string representation
     */
    std::string toString() const {
        return "Journal{id=" + std::to_string(id_) +
               ", name=" + name_ +
               ", fullName=" + fullName_ +
               ", level=" + level_ + "}";
    }

private:
    int id_{0};
    std::string name_;
    std::string fullName_;
    std::string level_;
    std::string fLevel_;
    std::string info_;
    std::string url_;
};

} // namespace PaperCrawler
