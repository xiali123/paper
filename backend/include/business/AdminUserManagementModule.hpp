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
 * @brief Admin User Management Module
 *
 * Handles user CRUD, password management, user details,
 * user export, and login security (login history, stats,
 * suspicious logins, IP blacklist, account lockouts).
 */
class AdminUserManagementModule : public BusinessModuleBase {
public:
    AdminUserManagementModule();
    ~AdminUserManagementModule() override;

    void setDatabase(std::shared_ptr<IDatabase> database);

    std::string getName() const override { return "AdminUserManagement"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Admin user management, login security and user export";
    }

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<IDatabase> database_;
    std::mutex usersMutex_;

    void registerRoutes() override;

    // Business logic - User management
    AdminStats getStats();
    PaginatedResponse<AdminUser> listUsers(int page = 1, int limit = 20, const std::string& search = "", UserRole roleFilter = UserRole::USER);
    std::optional<AdminUser> getUser(int id);
    std::optional<AdminUser> getUserByUsername(const std::string& username);
    std::optional<AdminUser> createUser(const AdminUser& user);
    std::optional<AdminUser> updateUser(int id, const AdminUser& user);
    bool deleteUser(int id);
    std::optional<AdminUser> activateUser(int id);
    std::optional<AdminUser> deactivateUser(int id);
    bool verifyUserPassword(int userId, const std::string& password);
    bool changeUserPassword(int userId, const std::string& oldPassword, const std::string& newPassword);
    bool resetUserPassword(int userId, const std::string& newPassword);

    // HTTP request handlers - User CRUD
    std::string handleListUsers(const std::map<std::string, std::string>& params);
    std::string handleGetUser(const std::map<std::string, std::string>& params);
    std::string handleCreateUser(const std::string& body);
    std::string handleUpdateUser(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteUser(const std::map<std::string, std::string>& params);
    std::string handleActivateUser(const std::map<std::string, std::string>& params);
    std::string handleDeactivateUser(const std::map<std::string, std::string>& params);

    // Password management
    std::string handleChangePassword(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleResetPassword(const std::map<std::string, std::string>& params, const std::string& body);

    // Stats
    std::string handleGetStats(const std::map<std::string, std::string>& params);

    // User details
    std::string handleGetUserHistory(const std::map<std::string, std::string>& params);
    std::string handleGetUserSessions(const std::map<std::string, std::string>& params);
    std::string handleKickUserSession(const std::map<std::string, std::string>& params);

    // Data export
    std::string handleExportUsers(const std::map<std::string, std::string>& params);

    // Login security
    std::string handleGetLoginHistory(const std::map<std::string, std::string>& params);
    std::string handleGetLoginStats(const std::map<std::string, std::string>& params);
    std::string handleGetSuspiciousLogins(const std::map<std::string, std::string>& params);
    std::string handleGetIpBlacklist(const std::map<std::string, std::string>& params);
    std::string handleAddIpBlacklist(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleRemoveIpBlacklist(const std::map<std::string, std::string>& params);
    std::string handleGetAccountLockouts(const std::map<std::string, std::string>& params);
    std::string handleLockUserAccount(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUnlockUserAccount(const std::map<std::string, std::string>& params);
    std::string handleHandleSuspiciousLogin(const std::map<std::string, std::string>& params, const std::string& body,
                                            const std::map<std::string, std::string>& headers);

    // Helper methods
    std::string buildJsonResponse(bool success, const std::string& message, const std::string& data = "");
    std::string buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data = "");
    std::string escapeJson(const std::string& str);
    std::string escapeSql(const std::string& str);
    void addAuditLog(const std::string& action, const std::string& entityType, int entityId,
                     const std::string& actorUsername, int actorId,
                     const std::string& details = "", const std::string& ipAddress = "");
};

} // namespace PaperCrawler
