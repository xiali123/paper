#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
#include "domain/models/Paper.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <mutex>
#include <functional>
#include <sstream>
#include <memory>

namespace PaperCrawler {

struct PaperSearchCriteria {
    std::string query;
    std::string author;
    int yearFrom{0};
    int yearTo{0};
    std::string journal;
    std::vector<std::string> tags;
    bool isRead{false};
    bool isFavorite{false};
};

class PaperApiModule : public BusinessModuleBase {
public:
    PaperApiModule();
    explicit PaperApiModule(std::shared_ptr<IDatabase> database);
    ~PaperApiModule() override;

    std::string getName() const override { return "PaperApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Paper management API";
    }

    std::vector<Paper> listPapers(int page = 1, int limit = 20, const std::string& sortBy = "created_at", bool ascending = false);
    std::optional<Paper> getPaper(int id);
    std::optional<Paper> createPaper(const Paper& paper);
    bool updatePaper(int id, const Paper& paper);
    bool deletePaper(int id);
    std::vector<Paper> searchPapers(const PaperSearchCriteria& criteria, int page = 1, int limit = 20);
    PaperStats getStats();
    size_t importPapers(const std::vector<Paper>& papers);
    std::string exportPapers(const std::vector<int>& ids, const std::string& format = "json");
    bool markAsRead(int id, bool read = true);
    bool markAsFavorite(int id, bool favorite = true);
    bool addTag(int id, const std::string& tag);
    bool removeTag(int id, const std::string& tag);
    bool uploadPDF(int id, const std::string& filePath);
    std::string getPDFPath(int id);
    std::map<std::string, std::vector<Paper>> groupByAuthor(const std::vector<Paper>& papers);
    std::map<std::string, std::vector<Paper>> groupByYear(const std::vector<Paper>& papers);
    std::map<std::string, std::vector<Paper>> groupByTag(const std::vector<Paper>& papers);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    std::shared_ptr<IDatabase> database_;

    void registerRoutes() override;
    std::string handleListPapers(const std::map<std::string, std::string>& params);
    std::string handleGetPaper(const std::map<std::string, std::string>& params);
    std::string handleCreatePaper(const std::string& body);
    std::string handleUpdatePaper(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeletePaper(const std::map<std::string, std::string>& params);
    std::string handleSearch(const std::map<std::string, std::string>& params);
    std::string handleStats();
    std::string handleExport(const std::map<std::string, std::string>& params);
    std::string handleFavorite(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleRead(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleTags(const std::map<std::string, std::string>& params, const std::string& body, const std::string& method);
};

} // namespace PaperCrawler
