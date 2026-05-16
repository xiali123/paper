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
 * @brief Admin Config Module
 *
 * Handles global configuration, data backup, notification management,
 * and data cleanup tasks.
 */
class AdminConfigModule : public BusinessModuleBase {
public:
    AdminConfigModule();
    ~AdminConfigModule() override;

    void setDatabase(std::shared_ptr<IDatabase> database);

    std::string getName() const override { return "AdminConfig"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Global config, data backup, notifications and data cleanup";
    }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<IDatabase> database_;

    void registerRoutes() override;

    // Global config
    std::string handleGetConfigCategories(const std::map<std::string, std::string>& params);
    std::string handleGetConfigs(const std::map<std::string, std::string>& params);
    std::string handleUpdateConfig(const std::map<std::string, std::string>& params, const std::string& body,
                                   const std::map<std::string, std::string>& headers);
    std::string handleGetConfigHistory(const std::map<std::string, std::string>& params);
    std::string handleGetConfigSummary(const std::map<std::string, std::string>& params);
    std::string handleReloadConfigs(const std::map<std::string, std::string>& params);

    // Data backup
    std::string handleGetBackupJobs(const std::map<std::string, std::string>& params);
    std::string handleCreateBackupJob(const std::map<std::string, std::string>& params, const std::string& body,
                                      const std::map<std::string, std::string>& headers);
    std::string handleUpdateBackupJob(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteBackupJob(const std::map<std::string, std::string>& params);
    std::string handleTriggerBackup(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetBackupRecords(const std::map<std::string, std::string>& params);
    std::string handleDeleteBackupFile(const std::map<std::string, std::string>& params);
    std::string handleGetBackupStats(const std::map<std::string, std::string>& params);

    // Notifications
    std::string handleGetNotificationTemplates(const std::map<std::string, std::string>& params);
    std::string handleCreateNotificationTemplate(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUpdateNotificationTemplate(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteNotificationTemplate(const std::map<std::string, std::string>& params);
    std::string handleGetSystemNotifications(const std::map<std::string, std::string>& params);
    std::string handleSendNotification(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetNotificationHistory(const std::map<std::string, std::string>& params);
    std::string handleGetNotificationStats(const std::map<std::string, std::string>& params);

    // Data cleanup
    std::string handleGetCleanupTasks(const std::map<std::string, std::string>& params);
    std::string handleCreateCleanupTask(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUpdateCleanupTask(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteCleanupTask(const std::map<std::string, std::string>& params);
    std::string handleTriggerCleanup(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetCleanupHistory(const std::map<std::string, std::string>& params);
    std::string handleGetStorageStats(const std::map<std::string, std::string>& params);

    // Helper methods
    void addAuditLog(const std::string& action, const std::string& entityType, int entityId,
                     const std::string& actorUsername, int actorId,
                     const std::string& details = "", const std::string& ipAddress = "");

    // Business logic methods
    std::vector<NotificationTemplate> getNotificationTemplates();
    int createNotificationTemplate(const std::string& name, const std::string& titleTemplate, const std::string& contentTemplate,
                                    const std::string& channel, const std::string& description, const std::string& language, int createdBy);
    bool updateNotificationTemplate(int id, const std::string& titleTemplate, const std::string& contentTemplate, const std::string& description);
    bool deleteNotificationTemplate(int id);
    PaginatedResponse<SystemNotification> getSystemNotifications(int page, int limit, const std::string& status);
    int64_t sendNotification(int templateId, const std::string& title, const std::string& content, const std::string& channel,
                             const std::string& targetRole, const std::string& targetUsers, const std::string& scheduledAt, int createdBy);
    PaginatedResponse<NotificationDelivery> getNotificationDeliveries(int page, int limit, int64_t notificationId);
    std::map<std::string, std::string> getNotificationStats();
    std::vector<CleanupTask> getCleanupTasks();
    int createCleanupTask(const std::string& name, const std::string& displayName, const std::string& taskType,
                           const std::string& description, const std::string& cleanupConfig, const std::string& scheduleCron,
                           bool isSystem, int createdBy);
    bool updateCleanupTask(int id, const std::string& displayName, const std::string& description,
                            const std::string& cleanupConfig, const std::string& scheduleCron, bool isEnabled);
    bool deleteCleanupTask(int id);
    int64_t triggerCleanup(int taskId, int triggeredBy);
    PaginatedResponse<CleanupExecution> getCleanupHistory(int page, int limit, int taskId);
    std::vector<StorageStat> getStorageStats();
};

} // namespace PaperCrawler
