#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <mutex>
#include <functional>
#include <memory>

namespace PaperCrawler {

// 前向声明
class Router;

/**
 * @brief 用户角色
 */
enum class UserRole {
    USER = 0,
    PREMIUM = 1,
    ADMIN = 2,
    SUPERADMIN = 3
};

/**
 * @brief 用户信息（管理版）
 */
struct AdminUser {
    int id;
    std::string username;
    std::string email;
    std::string fullName;
    std::string avatar;
    UserRole role;
    bool active;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastLoginAt;
    std::string lastLoginIp;

    std::string getRoleString() const {
        switch (role) {
            case UserRole::USER: return "user";
            case UserRole::PREMIUM: return "premium";
            case UserRole::ADMIN: return "admin";
            case UserRole::SUPERADMIN: return "superadmin";
            default: return "user";
        }
    }

    static UserRole fromString(const std::string& roleStr) {
        if (roleStr == "premium") return UserRole::PREMIUM;
        if (roleStr == "admin") return UserRole::ADMIN;
        if (roleStr == "superadmin") return UserRole::SUPERADMIN;
        return UserRole::USER;
    }

    std::string toJSON() const {
        std::ostringstream json;
        json << "{";
        json << "\"id\":" << id << ",";
        json << "\"username\":\"" << username << "\",";
        json << "\"email\":\"" << email << "\",";
        json << "\"full_name\":\"" << fullName << "\",";
        json << "\"avatar\":\"" << avatar << "\",";
        json << "\"role\":\"" << getRoleString() << "\",";
        json << "\"active\":" << (active ? "true" : "false") << ",";
        json << "\"created_at\":" << std::chrono::system_clock::to_time_t(createdAt) << ",";
        json << "\"last_login_at\":" << std::chrono::system_clock::to_time_t(lastLoginAt) << ",";
        json << "\"last_login_ip\":\"" << lastLoginIp << "\"";
        json << "}";
        return json.str();
    }
};

/**
 * @brief 模块信息
 */
struct ModuleInfo {
    std::string name;
    std::string version;
    std::string description;
    bool enabled;
    std::string type;  // "business", "feature", "infrastructure"
    std::chrono::system_clock::time_point lastLoaded;
    int loadOrder;

    std::string toJSON() const {
        std::ostringstream json;
        json << "{";
        json << "\"name\":\"" << name << "\",";
        json << "\"version\":\"" << version << "\",";
        json << "\"description\":\"" << description << "\",";
        json << "\"enabled\":" << (enabled ? "true" : "false") << ",";
        json << "\"type\":\"" << type << "\",";
        json << "\"last_loaded\":" << std::chrono::system_clock::to_time_t(lastLoaded) << ",";
        json << "\"load_order\":" << loadOrder;
        json << "}";
        return json.str();
    }
};

/**
 * @brief 审计日志条目
 */
struct AuditLog {
    int id;
    std::string action;
    std::string entityType;  // "user", "module", "system"
    int entityId;
    std::string actorUsername;
    int actorId;
    std::string details;
    std::string ipAddress;
    std::chrono::system_clock::time_point createdAt;

    std::string toJSON() const {
        std::ostringstream json;
        json << "{";
        json << "\"id\":" << id << ",";
        json << "\"action\":\"" << action << "\",";
        json << "\"entity_type\":\"" << entityType << "\",";
        json << "\"entity_id\":" << entityId << ",";
        json << "\"actor_username\":\"" << actorUsername << "\",";
        json << "\"actor_id\":" << actorId << ",";
        json << "\"details\":\"" << details << "\",";
        json << "\"ip_address\":\"" << ipAddress << "\",";
        json << "\"created_at\":" << std::chrono::system_clock::to_time_t(createdAt);
        json << "}";
        return json.str();
    }
};

/**
 * @brief 管理统计数据
 */
struct AdminStats {
    int totalUsers{0};
    int activeUsers{0};
    int premiumUsers{0};
    int adminUsers{0};
    int totalPapers{0};
    int totalSearches{0};
    int enabledModules{0};
    int totalModules{0};

    std::string toJSON() const {
        std::ostringstream json;
        json << "{";
        json << "\"total_users\":" << totalUsers << ",";
        json << "\"active_users\":" << activeUsers << ",";
        json << "\"premium_users\":" << premiumUsers << ",";
        json << "\"admin_users\":" << adminUsers << ",";
        json << "\"total_papers\":" << totalPapers << ",";
        json << "\"total_searches\":" << totalSearches << ",";
        json << "\"enabled_modules\":" << enabledModules << ",";
        json << "\"total_modules\":" << totalModules;
        json << "}";
        return json.str();
    }
};

/**
 * @brief 分页响应
 */
template<typename T>
struct PaginatedResponse {
    std::vector<T> items;
    int total{0};
    int page{1};
    int limit{20};
    int totalPages{1};

    std::string toJSON(std::function<std::string(const T&)> itemToJson) const {
        std::ostringstream json;
        json << "{";
        json << "\"items\":[";
        for (size_t i = 0; i < items.size(); i++) {
            if (i > 0) json << ",";
            json << itemToJson(items[i]);
        }
        json << "],";
        json << "\"total\":" << total << ",";
        json << "\"page\":" << page << ",";
        json << "\"limit\":" << limit << ",";
        json << "\"total_pages\":" << totalPages;
        json << "}";
        return json.str();
    }
};

/**
 * @brief 管理API模块
 *
 * 功能：
 * - 用户管理（列表、查看、更新、删除、激活/停用）
 * - 角色管理
 * - 模块管理（启用/禁用、列表）
 * - 审计日志（仅超级管理员）
 * - 管理统计
 *
 * 端点：
 * - GET    /api/admin/stats           - 管理统计
 * - GET    /api/admin/users          - 用户列表
 * - GET    /api/admin/users/:id      - 用户详情
 * - PUT    /api/admin/users/:id      - 更新用户
 * - DELETE /api/admin/users/:id      - 删除用户
 * - POST   /api/admin/users/:id/activate   - 激活用户
 * - POST   /api/admin/users/:id/deactivate - 停用用户
 * - GET    /api/admin/modules        - 模块列表
 * - POST   /api/admin/modules/:name/enable   - 启用模块
 * - POST   /api/admin/modules/:name/disable  - 禁用模块
 * - GET    /api/admin/audit-logs     - 审计日志
 */
class AdminApiModule : public BusinessModuleBase {
public:
    // 默认构造函数（用于DLL导出）
    AdminApiModule();

    // 构造函数：注入IDatabase依赖
    explicit AdminApiModule(std::shared_ptr<IDatabase> database);
    ~AdminApiModule() override;

    std::string getName() const override { return "AdminApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Admin and superadmin operations for user and module management";
    }

    // ========================================================================
    // 用户管理方法
    // ========================================================================

    /**
     * @brief 获取用户列表（分页）
     */
    PaginatedResponse<AdminUser> listUsers(int page = 1, int limit = 20, const std::string& search = "", UserRole roleFilter = UserRole::USER);

    /**
     * @brief 获取用户详情
     */
    std::optional<AdminUser> getUser(int id);

    /**
     * @brief 更新用户
     */
    std::optional<AdminUser> updateUser(int id, const AdminUser& user);

    /**
     * @brief 删除用户
     */
    bool deleteUser(int id);

    /**
     * @brief 激活用户
     */
    std::optional<AdminUser> activateUser(int id);

    /**
     * @brief 停用用户
     */
    std::optional<AdminUser> deactivateUser(int id);

    // ========================================================================
    // 模块管理方法
    // ========================================================================

    /**
     * @brief 获取所有模块列表
     */
    std::vector<ModuleInfo> listModules();

    /**
     * @brief 启用模块
     */
    bool enableModule(const std::string& moduleName);

    /**
     * @brief 禁用模块
     */
    bool disableModule(const std::string& moduleName);

    // ========================================================================
    // 审计日志方法
    // ========================================================================

    /**
     * @brief 获取审计日志（分页）
     */
    PaginatedResponse<AuditLog> getAuditLogs(int page = 1, int limit = 20, const std::string& action = "", int userId = 0);

    /**
     * @brief 添加审计日志
     */
    void addAuditLog(const std::string& action, const std::string& entityType, int entityId,
                     const std::string& actorUsername, int actorId,
                     const std::string& details = "", const std::string& ipAddress = "");

    // ========================================================================
    // 统计方法
    // ========================================================================

    /**
     * @brief 获取管理统计
     */
    AdminStats getStats();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // 依赖注入：数据库接口（允许Mock测试）
    std::shared_ptr<IDatabase> database_;

    // 互斥锁
    std::mutex usersMutex_;
    std::mutex modulesMutex_;
    std::mutex auditMutex_;

    void registerRoutes() override;  // BusinessModuleBase要求实现

    // HTTP请求处理器 - 用户管理
    std::string handleGetStats(const std::map<std::string, std::string>& params);
    std::string handleListUsers(const std::map<std::string, std::string>& params);
    std::string handleGetUser(const std::map<std::string, std::string>& params);
    std::string handleUpdateUser(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteUser(const std::map<std::string, std::string>& params);
    std::string handleActivateUser(const std::map<std::string, std::string>& params);
    std::string handleDeactivateUser(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 模块管理
    std::string handleListModules(const std::map<std::string, std::string>& params);
    std::string handleEnableModule(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDisableModule(const std::map<std::string, std::string>& params, const std::string& body);

    // HTTP请求处理器 - 审计日志
    std::string handleGetAuditLogs(const std::map<std::string, std::string>& params);

    // 辅助函数
    std::string buildJsonResponse(bool success, const std::string& message, const std::string& data = "");
    std::string buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data = "");
    std::string escapeJson(const std::string& str);

    // 初始化测试数据
    void initializeTestData();
    void initializeModuleInfo();
};

} // namespace PaperCrawler
