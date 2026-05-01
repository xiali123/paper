#pragma once

#include "core/ModuleBase.hpp"
#include "data/IDatabase.hpp"
#include <string>
#include <memory>
#include <map>
#include <vector>

namespace PaperCrawler {

class DashboardApiModule : public BusinessModuleBase {
public:
    DashboardApiModule();
    explicit DashboardApiModule(std::shared_ptr<IDatabase> database);
    ~DashboardApiModule() override;

    std::string getName() const override { return "DashboardApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Dashboard aggregation API for frontend homepage";
    }

private:
    void registerRoutes() override;
    std::shared_ptr<IDatabase> database_;

    // 端点处理
    std::string handleStats();
    std::string handleActivities(int limit);
    std::string handleRecommendations(int limit);
    std::string handleTrendingSearches(int limit);
    std::string handleTodos();
    std::string handleUpdateTodoStatus(const std::string& id, const std::string& status);
    std::string handleCrawlerTasks();
    std::string handleGrowth(int days);
    std::string handleDistributionJournals();
    std::string handleDistributionCcf();
    std::string handleRefresh();
    std::string handleGetConfig();
    std::string handleUpdateConfig(const std::string& body);

    // 辅助
    std::string getQueryParam(const HttpRequest& req, const std::string& key, const std::string& defaultVal) const;
    HttpResponse makeJsonResponse(int status, const std::string& body) const;
};

} // namespace PaperCrawler
