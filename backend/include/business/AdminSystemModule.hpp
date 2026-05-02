#pragma once

#include "business/AdminCommon.hpp"
#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <mutex>
#include <memory>

namespace PaperCrawler {

class Router;

/**
 * @brief Admin System Module
 *
 * Handles dashboard/stats, module management, system monitoring
 * (metrics, service health, system logs, performance), and announcements.
 */
class AdminSystemModule : public BusinessModuleBase {
public:
    AdminSystemModule();
    ~AdminSystemModule() override;

    void setDatabase(std::shared_ptr<IDatabase> database);

    std::string getName() const override { return "AdminSystem"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Admin dashboard, module management, system monitoring and announcements";
    }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<IDatabase> database_;
    std::mutex modulesMutex_;

    void registerRoutes() override;

    // Business logic - Module management
    std::vector<ModuleInfo> listModules();
    bool enableModule(const std::string& moduleName);
    bool disableModule(const std::string& moduleName);
    std::string uploadModule(const std::string& fileData, const std::string& filename);
    bool installModule(const std::string& moduleName, const std::string& modulePath);
    bool uninstallModule(const std::string& moduleName);
    bool reloadModule(const std::string& moduleName);
    std::vector<ModuleInfo> scanModules(const std::string& directory = "modules");
    void initializeModuleInfo();

    // Dashboard
    std::string handleGetDashboard(const std::map<std::string, std::string>& params);

    // Module management
    std::string handleListModules(const std::map<std::string, std::string>& params);
    std::string handleEnableModule(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDisableModule(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUploadModule(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleInstallModule(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUninstallModule(const std::map<std::string, std::string>& params);
    std::string handleReloadModule(const std::map<std::string, std::string>& params);
    std::string handleScanModules(const std::map<std::string, std::string>& params);

    // System monitoring
    std::string handleGetSystemMetrics(const std::map<std::string, std::string>& params);
    std::string handleGetServiceHealth(const std::map<std::string, std::string>& params);
    std::string handleGetSystemLogs(const std::map<std::string, std::string>& params);
    std::string handleGetLogStats(const std::map<std::string, std::string>& params);
    std::string handleCleanLogs(const std::map<std::string, std::string>& params);
    std::string handleGetPerformanceMetrics(const std::map<std::string, std::string>& params);
    std::string handleGetSlowQueries(const std::map<std::string, std::string>& params);
    std::string handleGetPerformanceBottlenecks(const std::map<std::string, std::string>& params);

    // Announcements
    std::string handleListAnnouncements(const std::map<std::string, std::string>& params);
    std::string handleCreateAnnouncement(const std::string& body);
    std::string handleUpdateAnnouncement(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteAnnouncement(const std::map<std::string, std::string>& params);
    std::string handleToggleAnnouncement(const std::map<std::string, std::string>& params);

    // Helper methods
    std::string buildJsonResponse(bool success, const std::string& message, const std::string& data = "");
    std::string buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data = "");
    std::string escapeJson(const std::string& str);
    std::string escapeSql(const std::string& str);
    void addAuditLog(const std::string& action, const std::string& entityType, int entityId,
                     const std::string& actorUsername, int actorId,
                     const std::string& details = "", const std::string& ipAddress = "");

    // Internal data access
    AdminStats getStats();
};

} // namespace PaperCrawler
