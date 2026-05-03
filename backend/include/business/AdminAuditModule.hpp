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
#include <utility>

namespace PaperCrawler {

class Router;

/**
 * @brief Admin Audit Module
 *
 * Handles audit logs, RBAC (roles/permissions), content moderation
 * (papers, reports, sensitive words), and API key management.
 */
class AdminAuditModule : public BusinessModuleBase {
public:
    AdminAuditModule();
    ~AdminAuditModule() override;

    void setDatabase(std::shared_ptr<IDatabase> database);

    std::string getName() const override { return "AdminAudit"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Audit logs, RBAC, content moderation and API key management";
    }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<IDatabase> database_;
    std::mutex auditMutex_;

    void registerRoutes() override;

    // Audit logs
    std::string handleGetAuditLogs(const std::map<std::string, std::string>& params);

    // RBAC - Roles
    std::string handleGetRoles(const std::map<std::string, std::string>& params);
    std::string handleCreateRole(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUpdateRole(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteRole(const std::map<std::string, std::string>& params);

    // RBAC - Permissions
    std::string handleGetPermissions(const std::map<std::string, std::string>& params);
    std::string handleGetPermissionMatrix(const std::map<std::string, std::string>& params);
    std::string handleGetRolePermissions(const std::map<std::string, std::string>& params);
    std::string handleUpdateRolePermissions(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetUserRoles(const std::map<std::string, std::string>& params);
    std::string handleAssignUserRole(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleRemoveUserRole(const std::map<std::string, std::string>& params);
    std::string handleCheckPermission(const std::map<std::string, std::string>& params, const std::string& body);

    // Content moderation
    std::string handleGetPendingPapers(const std::map<std::string, std::string>& params);
    std::string handleGetPaperModeration(const std::map<std::string, std::string>& params);
    std::string handleApprovePaper(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleRejectPaper(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetUserReports(const std::map<std::string, std::string>& params);
    std::string handleResolveReport(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetSensitiveWords(const std::map<std::string, std::string>& params);
    std::string handleCreateSensitiveWord(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteSensitiveWord(const std::map<std::string, std::string>& params);
    std::string handleCheckSensitiveWords(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetSensitiveWordStats(const std::map<std::string, std::string>& params);

    // API key management
    std::string handleGetApiKeys(const std::map<std::string, std::string>& params);
    std::string handleCreateApiKey(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteApiKey(const std::map<std::string, std::string>& params);
    std::string handleRegenerateApiKey(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetApiKeyUsage(const std::map<std::string, std::string>& params);
    std::string handleGetApiKeyStats(const std::map<std::string, std::string>& params);

    // Helper methods
    void addAuditLog(const std::string& action, const std::string& entityType, int entityId,
                     const std::string& actorUsername, int actorId,
                     const std::string& details = "", const std::string& ipAddress = "");

    // Business logic methods
    PaginatedResponse<AuditLog> getAuditLogs(int page, int limit, const std::string& action, int userId);
    std::vector<Role> getRoles();
    int createRole(const std::string& name, const std::string& displayName, const std::string& description, int level, int createdBy);
    bool updateRole(int roleId, const std::string& displayName, const std::string& description, int level);
    bool deleteRole(int roleId);
    std::vector<Permission> getPermissions();
    std::vector<PermissionMatrix> getPermissionMatrix();
    std::vector<RolePermission> getRolePermissions(int roleId);
    bool updateRolePermissions(int roleId, const std::vector<int>& permissionIds, int updatedBy);
    std::vector<UserRoleAssignment> getUserRoles(int userId);
    bool assignUserRole(int userId, int roleId, const std::string& reason, int assignedBy, const std::string& expiresAt);
    bool removeUserRole(int userId, int roleId);
    bool checkUserPermission(int userId, const std::string& resource, const std::string& action);
    PaginatedResponse<PaperModeration> getPendingPapers(int page, int limit);
    std::optional<PaperModeration> getPaperModeration(int64_t id);
    bool approvePaper(int paperId, int moderatorId);
    bool rejectPaper(int paperId, int moderatorId, const std::string& reason);
    PaginatedResponse<UserReport> getUserReports(int page, int limit, const std::string& status);
    bool resolveReport(int64_t reportId, int reviewerId, const std::string& resolution, const std::string& status);
    std::vector<SensitiveWord> getSensitiveWords();
    int createSensitiveWord(const std::string& word, const std::string& category, const std::string& severity, bool isRegex, const std::string& replacement, int createdBy);
    bool deleteSensitiveWord(int id);
    std::vector<SensitiveWordMatch> checkSensitiveWords(const std::string& text);
    std::map<std::string, int> getSensitiveWordStats();
    PaginatedResponse<ApiKey> getApiKeys(int page, int limit, int userId);
    std::pair<int, std::string> createApiKey(int userId, const std::string& name, const std::string& scopes, int rateLimitPerHour, const std::string& expiresAt, int createdBy);
    bool deleteApiKey(int id);
    std::string regenerateApiKey(int id);
    PaginatedResponse<ApiUsage> getApiKeyUsage(int page, int limit, int keyId);
    ApiUsageStats getApiKeyStats(int keyId);
};

} // namespace PaperCrawler
