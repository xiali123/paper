#pragma once

#include "core/ModuleBase.hpp"
#include "data/IDatabase.hpp"
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <mutex>

namespace PaperCrawler {

/**
 * @brief 待办事项数据结构（内存存储）
 */
struct DashboardTodoItem {
    std::string id;
    std::string title;
    std::string status;    // "pending" | "completed"
    std::string createdAt;
};

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

    // 内存存储
    std::vector<DashboardTodoItem> todos_;
    std::string configJson_;
    mutable std::mutex storageMutex_;
    int nextTodoId_ = 1;

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
    std::string handleCreateTodo(const std::string& body);
    std::string handleDeleteTodo(const std::string& id);
    std::string handleActivities(const std::map<std::string, std::string>& params);
    std::string handleTrendingPapers(const std::map<std::string, std::string>& params);

    // 辅助
    std::string getQueryParam(const HttpRequest& req, const std::string& key, const std::string& defaultVal) const;
    HttpResponse makeJsonResponse(int status, const std::string& body) const;
    std::string escapeJson(const std::string& input) const;
};

} // namespace PaperCrawler
