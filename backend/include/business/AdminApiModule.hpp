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
    std::string passwordHash;  // 密码哈希（不返回给前端）
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastLoginAt;
    std::string lastLoginIp;
    int loginCount{0};

    std::string getActivityStatus() const {
        if (lastLoginAt == std::chrono::system_clock::from_time_t(0)) {
            return "inactive";
        }
        auto daysSince = std::chrono::duration_cast<std::chrono::hours>(
            std::chrono::system_clock::now() - lastLoginAt).count() / 24;
        if (daysSince <= 30) return "active";
        if (daysSince <= 90) return "idle";
        return "inactive";
    }

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
        json << "\"login_count\":" << loginCount << ",";
        json << "\"activity_status\":\"" << getActivityStatus() << "\",";
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
    int recentlyActiveUsers{0};
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
        json << "\"recently_active_users\":" << recentlyActiveUsers << ",";
        json << "\"enabled_modules\":" << enabledModules << ",";
        json << "\"total_modules\":" << totalModules;
        json << "}";
        return json.str();
    }
};

/**
 * @brief 登录历史
 */
struct LoginHistory {
    int id;
    int userId;
    std::string loginTime;
    std::string ipAddress;
    std::string userAgent;
    bool success;

    std::string toJSON() const {
        std::ostringstream json;
        json << "{";
        json << "\"id\":" << id << ",";
        json << "\"user_id\":" << userId << ",";
        json << "\"login_time\":\"" << loginTime << "\",";
        json << "\"ip_address\":\"" << ipAddress << "\",";
        json << "\"user_agent\":\"" << userAgent << "\",";
        json << "\"success\":" << (success ? "true" : "false");
        json << "}";
        return json.str();
    }
};

/**
 * @brief 公告
 */
struct Announcement {
    int id;
    std::string title;
    std::string content;
    std::string type;  // info, warning, maintenance
    std::string targetRole;  // all, user, premium, admin, superadmin
    int createdBy;
    bool isActive;
    std::string createdAt;
    std::string expiresAt;

    std::string toJSON() const {
        std::ostringstream json;
        json << "{";
        json << "\"id\":" << id << ",";
        json << "\"title\":\"" << title << "\",";
        json << "\"content\":\"" << content << "\",";
        json << "\"type\":\"" << type << "\",";
        json << "\"target_role\":\"" << targetRole << "\",";
        json << "\"created_by\":" << createdBy << ",";
        json << "\"is_active\":" << (isActive ? "true" : "false") << ",";
        json << "\"created_at\":\"" << createdAt << "\",";
        json << "\"expires_at\":\"" << expiresAt << "\"";
        json << "}";
        return json.str();
    }
};

/**
 * @brief 系统日志条目
 */
struct SystemLog {
    int64_t id;
    std::string level;  // debug, info, warning, error, critical
    std::string module;
    std::string message;
    std::string context;  // JSON string
    std::string file;
    int line;
    std::string threadId;
    std::string createdAt;

};

/**
 * @brief 系统资源指标
 */
struct SystemMetrics {
    double cpuPercent;
    double memoryUsedMB;
    double memoryTotalMB;
    double memoryPercent;
    double diskUsedGB;
    double diskTotalGB;
    double diskPercent;
    double networkRxMBps;
    double networkTxMBps;
    int64_t uptimeSeconds;
    int activeConnections;

};

/**
 * @brief 服务健康状态
 */
struct ServiceHealth {
    std::string name;
    std::string status;  // healthy, degraded, down
    int64_t responseTimeMs;
    std::string lastCheck;

};

/**
 * @brief 性能指标
 */
struct PerformanceMetric {
    std::string endpoint;
    std::string method;
    int requestCount;
    int successCount;
    int errorCount;
    int avgResponseTimeMs;
    int maxResponseTimeMs;
    int minResponseTimeMs;
    int p95ResponseTimeMs;
    int p99ResponseTimeMs;
    std::string lastRequestAt;

};

/**
 * @brief 慢查询记录
 */
struct SlowQuery {
    int64_t id;
    std::string queryText;
    int executionTimeMs;
    int rowsExamined;
    int rowsReturned;
    std::string module;
    std::string endpoint;
    std::string createdAt;

};

/**
 * @brief 日志统计
 */
struct LogStats {
    std::string level;
    int count;
    std::string date;
};

/**
 * @brief 登录尝试记录
 */
struct LoginAttempt {
    int64_t id;
    std::string username;
    std::string ipAddress;
    std::string userAgent;
    bool success;
    std::string failureReason;
    std::string createdAt;
};

/**
 * @brief IP黑名单条目
 */
struct IpBlacklistEntry {
    int id;
    std::string ipAddress;
    std::string reason;
    std::string threatLevel;  // low, medium, high, critical
    int attemptCount;
    int createdBy;
    std::string createdAt;
    std::string expiresAt;
    bool isActive;
};

/**
 * @brief 可疑登录活动
 */
struct SuspiciousLogin {
    int64_t id;
    std::string username;
    std::string ipAddress;
    std::string suspicionReason;  // multiple_failures, unknown_location, impossible_travel, bot_pattern, blacklisted_ip
    int riskScore;  // 0-100
    std::string status;  // pending, reviewed, whitelisted, confirmed_threat
    int reviewedBy;
    std::string reviewedAt;
    std::string createdAt;
};

/**
 * @brief 账户锁定记录
 */
struct AccountLockout {
    int id;
    int userId;
    std::string username;  // Joined from users table
    std::string lockedUntil;
    std::string lockoutReason;
    int failedAttempts;
    std::string ipAddress;
    std::string createdAt;
};

/**
 * @brief 登录统计
 */
struct LoginStats {
    std::string date;
    int successfulLogins;
    int failedLogins;
    int uniqueUsers;
    int uniqueIps;
};

/**
 * @brief 系统配置项
 */
struct SystemConfig {
    int id;
    std::string key;
    std::string value;
    std::string valueType;  // string, number, boolean, json
    std::string category;    // auth, storage, limits, email, maintenance, system
    std::string description;
    std::string defaultValue;
    bool isPublic;
    bool isEncrypted;
    int updatedBy;
    std::string updatedAt;
    std::string createdAt;
};

/**
 * @brief 配置变更历史
 */
struct ConfigHistoryEntry {
    int64_t id;
    int configId;
    std::string configKey;
    std::string oldValue;
    std::string newValue;
    int changedBy;
    std::string changedByUsername;  // Joined from users table
    std::string changeReason;
    std::string changeType;  // create, update, delete
    std::string createdAt;
};

/**
 * @brief 备份任务
 */
struct BackupJob {
    int id;
    std::string name;
    std::string jobType;  // full, incremental, database_only, files_only
    std::string description;
    std::string scheduleCron;
    std::string backupPath;
    int retentionDays;
    bool isEnabled;
    std::string lastRunAt;
    std::string lastRunStatus;  // success, failed, running
    std::string lastRunMessage;
    int createdBy;
    std::string createdAt;
};

/**
 * @brief 备份记录
 */
struct BackupRecord {
    int64_t id;
    int jobId;
    std::string filename;
    std::string filePath;
    int64_t fileSize;
    std::string backupType;
    std::string status;  // pending, in_progress, success, failed, deleted
    std::string startedAt;
    std::string completedAt;
    int durationSeconds;
    std::string errorMessage;
    int tablesBackedUp;
    int64_t rowsBackedUp;
    int createdBy;
    std::string jobName;  // Joined from backup_jobs
};

/**
 * @brief 配置分类统计
 */
struct ConfigCategorySummary {
    std::string category;
    int configCount;
    int recentlyUpdated;
};

// ============================================================================
// RBAC 权限相关数据结构
// ============================================================================

/**
 * @brief 角色
 */
struct Role {
    int id;
    std::string name;  // user, premium, admin, superadmin
    std::string displayName;
    std::string description;
    int level;  // 10, 20, 50, 100
    bool isSystem;
    bool isDefault;
    std::string createdAt;
    std::string updatedAt;
};

/**
 * @brief 权限
 */
struct Permission {
    int id;
    std::string resource;  // user, paper, module, audit, etc.
    std::string action;    // read, create, update, delete, etc.
    std::string description;
};

/**
 * @brief 角色权限映射
 */
struct RolePermission {
    int roleId;
    std::string roleName;
    int permissionId;
    std::string resource;
    std::string action;
    std::string grantedAt;
    std::string grantedByUsername;
};

/**
 * @brief 用户角色分配
 */
struct UserRoleAssignment {
    int64_t id;
    int userId;
    std::string username;
    int roleId;
    std::string roleName;
    int roleLevel;
    std::string assignedAt;
    std::string expiresAt;
    std::string reason;
};

/**
 * @brief 权限矩阵汇总
 */
struct PermissionMatrix {
    std::string roleName;
    int totalPermissions;
    std::map<std::string, int> permissionsByResource;  // resource -> count
};

// ============================================================================
// 通知管理相关数据结构
// ============================================================================

/**
 * @brief 通知模板
 */
struct NotificationTemplate {
    int id;
    std::string name;
    std::string titleTemplate;
    std::string contentTemplate;
    std::string channel;  // email, inapp, sms, push
    std::string description;
    std::string language;
    bool isActive;
    std::string createdAt;
};

/**
 * @brief 系统通知
 */
struct SystemNotification {
    int64_t id;
    int templateId;
    std::string title;
    std::string content;
    std::string channel;
    std::string targetRole;
    int totalRecipients;
    int sentCount;
    int failedCount;
    std::string status;  // pending, sending, sent, failed
    std::string scheduledAt;
    std::string sentAt;
    int createdBy;
    std::string createdAt;
};

/**
 * @brief 通知投递记录
 */
struct NotificationDelivery {
    int64_t id;
    int64_t notificationId;
    int userId;
    std::string username;
    std::string status;  // pending, sent, failed, read
    std::string sentAt;
    std::string readAt;
    std::string errorMessage;
    std::string createdAt;
};

// ============================================================================
// 数据清理相关数据结构
// ============================================================================

/**
 * @brief 清理任务
 */
struct CleanupTask {
    int id;
    std::string name;
    std::string displayName;
    std::string taskType;  // logs, sessions, temp_files, cache, expired_data, custom_sql
    std::string description;
    std::string cleanupConfig;  // JSON string
    std::string scheduleCron;
    bool isEnabled;
    bool isSystem;
    std::string lastRunAt;
    std::string lastRunStatus;  // success, failed, running
    std::string lastRunMessage;
    int createdBy;
    std::string createdAt;
};

/**
 * @brief 清理执行历史
 */
struct CleanupExecution {
    int64_t id;
    int taskId;
    std::string taskName;
    std::string status;  // running, success, failed, cancelled
    std::string startedAt;
    std::string completedAt;
    int durationSeconds;
    int itemsProcessed;
    double spaceFreedMb;
    std::string outputMessage;
    std::string errorMessage;
    int triggeredBy;
    std::string createdAt;
};

/**
 * @brief 存储统计
 */
struct StorageStat {
    int64_t id;
    std::string tableName;
    int64_t rowCount;
    double dataLengthMb;
    double indexLengthMb;
    double totalLengthMb;
    double fragmentRatio;
    std::string recordedAt;
};

// ============================================================================
// 内容审核相关数据结构
// ============================================================================

/**
 * @brief 论文审核记录
 */
struct PaperModeration {
    int64_t id;
    int paperId;
    std::string status;  // pending, approved, rejected, flagged
    int moderatorId;
    std::string moderatorUsername;
    std::string reason;
    std::string reviewedAt;
    std::string flags;  // JSON string
    std::string createdAt;
};

/**
 * @brief 用户举报
 */
struct UserReport {
    int64_t id;
    int reporterId;
    std::string reporterUsername;
    std::string targetType;  // paper, user, comment
    int targetId;
    std::string reason;  // spam, inappropriate, abuse, copyright, other
    std::string description;
    std::string status;  // pending, reviewed, resolved, dismissed
    std::string priority;  // low, medium, high, urgent
    int reviewerId;
    std::string reviewerUsername;
    std::string resolution;
    std::string createdAt;
};

/**
 * @brief 敏感词
 */
struct SensitiveWord {
    int id;
    std::string word;
    std::string category;  // politics, violence, adult, spam, other
    std::string severity;  // low, medium, high
    bool isRegex;
    std::string replacement;
    bool isActive;
    int matchCount;
    int createdBy;
    std::string createdAt;
};

/**
 * @brief 敏感词匹配结果
 */
struct SensitiveWordMatch {
    std::string word;
    std::string category;
    int startPosition;
    int endPosition;
    std::string matchedText;
};

// ============================================================================
// API密钥相关数据结构
// ============================================================================

/**
 * @brief API密钥
 */
struct ApiKey {
    int id;
    int userId;
    std::string username;
    std::string name;
    std::string keyPrefix;  // First 10 chars
    std::string scopes;  // JSON string
    int rateLimitPerHour;
    std::string expiresAt;
    std::string lastUsedAt;
    int64_t requestCount;
    bool isActive;
    int createdBy;
    std::string createdAt;
};

/**
 * @brief API使用记录
 */
struct ApiUsage {
    int64_t id;
    int keyId;
    std::string keyName;
    std::string endpoint;
    std::string method;
    int statusCode;
    int responseTimeMs;
    std::string ipAddress;
    std::string userAgent;
    std::string createdAt;
};

/**
 * @brief API使用统计
 */
struct ApiUsageStats {
    int64_t totalRequests;
    int64_t successfulRequests;
    int64_t failedRequests;
    double avgResponseTime;
    std::map<std::string, int64_t> requestsByEndpoint;
    std::map<std::string, int64_t> requestsByDay;
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

    /**
     * @brief 接收数据库连接注入
     */
    void setDatabase(std::shared_ptr<IDatabase> database);

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
     * @brief 根据用户名获取用户
     */
    std::optional<AdminUser> getUserByUsername(const std::string& username);

    /**
     * @brief 验证用户密码
     */
    bool verifyUserPassword(int userId, const std::string& password);

    /**
     * @brief 修改用户密码
     */
    bool changeUserPassword(int userId, const std::string& oldPassword, const std::string& newPassword);

    /**
     * @brief 重置用户密码（管理员）
     */
    bool resetUserPassword(int userId, const std::string& newPassword);

    /**
     * @brief 创建用户
     */
    std::optional<AdminUser> createUser(const AdminUser& user);

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

    /**
     * @brief 上传模块DLL文件
     * @param fileData 文件二进制数据
     * @param filename 文件名
     * @return 保存的文件路径
     */
    std::string uploadModule(const std::string& fileData, const std::string& filename);

    /**
     * @brief 安装/注册模块
     * @param moduleName 模块名称
     * @param modulePath 模块DLL路径
     * @return 是否成功
     */
    bool installModule(const std::string& moduleName, const std::string& modulePath);

    /**
     * @brief 卸载模块
     * @param moduleName 模块名称
     * @return 是否成功
     */
    bool uninstallModule(const std::string& moduleName);

    /**
     * @brief 重载模块
     * @param moduleName 模块名称
     * @return 是否成功
     */
    bool reloadModule(const std::string& moduleName);

    /**
     * @brief 扫描目录中的模块
     * @param directory 目录路径
     * @return 发现的模块列表
     */
    std::vector<ModuleInfo> scanModules(const std::string& directory = "modules");

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

    // ========================================================================
    // 系统监控方法
    // ========================================================================

    /**
     * @brief 获取实时系统指标
     */
    SystemMetrics getSystemMetrics();

    /**
     * @brief 获取服务健康状态列表
     */
    std::vector<ServiceHealth> getServiceHealth();

    /**
     * @brief 更新服务健康状态
     */
    void updateServiceHealth(const std::string& serviceName, const std::string& status, int responseTimeMs = 0);

    /**
     * @brief 获取错误日志列表（分页）
     */
    PaginatedResponse<SystemLog> getSystemLogs(int page = 1, int limit = 20, const std::string& level = "", const std::string& module = "");

    /**
     * @brief 获取日志统计
     */
    std::vector<LogStats> getLogStats(const std::string& groupBy = "level");

    /**
     * @brief 添加系统日志
     */
    void addSystemLog(const std::string& level, const std::string& module, const std::string& message,
                     const std::string& context = "", const std::string& file = "", int line = 0);

    /**
     * @brief 清理历史日志
     */
    int cleanOldLogs(const std::string& beforeDate);

    /**
     * @brief 获取性能指标列表
     */
    std::vector<PerformanceMetric> getPerformanceMetrics();

    /**
     * @brief 获取慢查询列表（分页）
     */
    PaginatedResponse<SlowQuery> getSlowQueries(int page = 1, int limit = 20, int minExecutionTimeMs = 1000);

    /**
     * @brief 记录慢查询
     */
    void recordSlowQuery(const std::string& queryText, int executionTimeMs, int rowsExamined, int rowsReturned,
                        const std::string& module, const std::string& endpoint = "");

    /**
     * @brief 更新端点性能指标
     */
    void updatePerformanceMetric(const std::string& endpoint, const std::string& method, bool success, int responseTimeMs);

    /**
     * @brief 获取性能瓶颈分析
     */
    std::map<std::string, std::string> analyzePerformanceBottlenecks();

    // ========================================================================
    // 登录安全方法
    // ========================================================================

    /**
     * @brief 获取登录历史记录（分页）
     */
    PaginatedResponse<LoginAttempt> getLoginHistory(int page = 1, int limit = 20, const std::string& username = "");

    /**
     * @brief 获取登录统计数据
     */
    std::vector<LoginStats> getLoginStats(int days = 30);

    /**
     * @brief 获取可疑登录活动列表（分页）
     */
    PaginatedResponse<SuspiciousLogin> getSuspiciousLogins(int page = 1, int limit = 20, const std::string& status = "");

    /**
     * @brief 获取IP黑名单列表（分页）
     */
    PaginatedResponse<IpBlacklistEntry> getIpBlacklist(int page = 1, int limit = 20);

    /**
     * @brief 添加IP到黑名单
     */
    bool addIpToBlacklist(const std::string& ipAddress, const std::string& reason, const std::string& threatLevel,
                          int createdBy, const std::string& expiresAt = "");

    /**
     * @brief 从黑名单移除IP
     */
    bool removeIpFromBlacklist(int id);

    /**
     * @brief 检查IP是否在黑名单中
     */
    bool isIpBlacklisted(const std::string& ipAddress);

    /**
     * @brief 获取账户锁定列表（分页）
     */
    PaginatedResponse<AccountLockout> getAccountLockouts(int page = 1, int limit = 20);

    /**
     * @brief 锁定用户账户
     */
    bool lockUserAccount(int userId, int lockMinutes, const std::string& reason, const std::string& ipAddress = "");

    /**
     * @brief 解锁用户账户
     */
    bool unlockUserAccount(int userId);

    /**
     * @brief 处理可疑登录（标记为已审核/白名单/确认威胁）
     */
    bool handleSuspiciousLogin(int id, const std::string& action, int reviewedBy);

    // ========================================================================
    // RBAC权限管理方法
    // ========================================================================

    /**
     * @brief 获取所有角色
     */
    std::vector<Role> getRoles();

    /**
     * @brief 创建角色
     */
    int createRole(const std::string& name, const std::string& displayName, const std::string& description,
                   int level, int createdBy);

    /**
     * @brief 更新角色
     */
    bool updateRole(int roleId, const std::string& displayName, const std::string& description, int level);

    /**
     * @brief 删除角色
     */
    bool deleteRole(int roleId);

    /**
     * @brief 获取所有权限
     */
    std::vector<Permission> getPermissions();

    /**
     * @brief 获取权限矩阵（角色×权限）
     */
    std::vector<PermissionMatrix> getPermissionMatrix();

    /**
     * @brief 获取角色权限
     */
    std::vector<RolePermission> getRolePermissions(int roleId);

    /**
     * @brief 更新角色权限
     */
    bool updateRolePermissions(int roleId, const std::vector<int>& permissionIds, int updatedBy);

    /**
     * @brief 获取用户角色
     */
    std::vector<UserRoleAssignment> getUserRoles(int userId);

    /**
     * @brief 分配用户角色
     */
    bool assignUserRole(int userId, int roleId, const std::string& reason, int assignedBy, const std::string& expiresAt = "");

    /**
     * @brief 移除用户角色
     */
    bool removeUserRole(int userId, int roleId);

    /**
     * @brief 检查用户是否有某权限
     */
    bool checkUserPermission(int userId, const std::string& resource, const std::string& action);

    // ========================================================================
    // 全局配置方法
    // ========================================================================

    /**
     * @brief 获取所有配置分类
     */
    std::vector<std::string> getConfigCategories();

    /**
     * @brief 按分类获取配置列表
     */
    std::vector<SystemConfig> getConfigsByCategory(const std::string& category);

    /**
     * @brief 获取单个配置
     */
    std::optional<SystemConfig> getConfig(const std::string& key);

    /**
     * @brief 更新配置值
     */
    bool updateConfig(const std::string& key, const std::string& value, int updatedBy, const std::string& reason = "");

    /**
     * @brief 获取配置变更历史
     */
    PaginatedResponse<ConfigHistoryEntry> getConfigHistory(int page = 1, int limit = 20, const std::string& configKey = "");

    /**
     * @brief 获取配置分类统计
     */
    std::vector<ConfigCategorySummary> getConfigCategorySummary();

    /**
     * @brief 重新加载配置（清除缓存）
     */
    bool reloadConfigs();

    // ========================================================================
    // 数据备份方法
    // ========================================================================

    /**
     * @brief 获取所有备份任务
     */
    std::vector<BackupJob> getBackupJobs();

    /**
     * @brief 创建备份任务
     */
    int createBackupJob(const std::string& name, const std::string& jobType, const std::string& scheduleCron,
                        const std::string& backupPath, int retentionDays, int createdBy);

    /**
     * @brief 更新备份任务
     */
    bool updateBackupJob(int id, const std::string& scheduleCron, int retentionDays, bool isEnabled);

    /**
     * @brief 删除备份任务
     */
    bool deleteBackupJob(int id);

    /**
     * @brief 手动触发备份
     */
    int triggerBackup(int jobId, int createdBy);

    /**
     * @brief 获取备份记录列表
     */
    PaginatedResponse<BackupRecord> getBackupRecords(int page = 1, int limit = 20, int jobId = 0);

    /**
     * @brief 删除备份文件
     */
    bool deleteBackupFile(int recordId);

    /**
     * @brief 获取备份统计
     */
    std::map<std::string, std::string> getBackupStats();

    // ========================================================================
    // 通知管理方法
    // ========================================================================

    /**
     * @brief 获取通知模板列表
     */
    std::vector<NotificationTemplate> getNotificationTemplates();

    /**
     * @brief 创建通知模板
     */
    int createNotificationTemplate(const std::string& name, const std::string& titleTemplate, const std::string& contentTemplate,
                                  const std::string& channel, const std::string& description, const std::string& language, int createdBy);

    /**
     * @brief 更新通知模板
     */
    bool updateNotificationTemplate(int id, const std::string& titleTemplate, const std::string& contentTemplate,
                                   const std::string& description);

    /**
     * @brief 删除通知模板
     */
    bool deleteNotificationTemplate(int id);

    /**
     * @brief 获取系统通知列表（分页）
     */
    PaginatedResponse<SystemNotification> getSystemNotifications(int page = 1, int limit = 20, const std::string& status = "");

    /**
     * @brief 发送系统通知
     */
    int64_t sendNotification(int templateId, const std::string& title, const std::string& content, const std::string& channel,
                            const std::string& targetRole, const std::string& targetUsers, const std::string& scheduledAt, int createdBy);

    /**
     * @brief 获取通知投递历史（分页）
     */
    PaginatedResponse<NotificationDelivery> getNotificationDeliveries(int page = 1, int limit = 20, int64_t notificationId = 0);

    /**
     * @brief 获取通知统计
     */
    std::map<std::string, std::string> getNotificationStats();

    // ========================================================================
    // 数据清理方法
    // ========================================================================

    /**
     * @brief 获取清理任务列表
     */
    std::vector<CleanupTask> getCleanupTasks();

    /**
     * @brief 创建清理任务
     */
    int createCleanupTask(const std::string& name, const std::string& displayName, const std::string& taskType,
                         const std::string& description, const std::string& cleanupConfig, const std::string& scheduleCron,
                         bool isSystem, int createdBy);

    /**
     * @brief 更新清理任务
     */
    bool updateCleanupTask(int id, const std::string& displayName, const std::string& description,
                          const std::string& cleanupConfig, const std::string& scheduleCron, bool isEnabled);

    /**
     * @brief 删除清理任务
     */
    bool deleteCleanupTask(int id);

    /**
     * @brief 触发清理任务
     */
    int64_t triggerCleanup(int taskId, int triggeredBy);

    /**
     * @brief 获取清理执行历史（分页）
     */
    PaginatedResponse<CleanupExecution> getCleanupHistory(int page = 1, int limit = 20, int taskId = 0);

    /**
     * @brief 获取存储统计
     */
    std::vector<StorageStat> getStorageStats();

    // ========================================================================
    // 内容审核方法
    // ========================================================================

    /**
     * @brief 获取待审核论文列表（分页）
     */
    PaginatedResponse<PaperModeration> getPendingPapers(int page = 1, int limit = 20);

    /**
     * @brief 获取论文审核详情
     */
    std::optional<PaperModeration> getPaperModeration(int64_t id);

    /**
     * @brief 审核通过论文
     */
    bool approvePaper(int paperId, int moderatorId);

    /**
     * @brief 审核拒绝论文
     */
    bool rejectPaper(int paperId, int moderatorId, const std::string& reason);

    /**
     * @brief 获取用户举报列表（分页）
     */
    PaginatedResponse<UserReport> getUserReports(int page = 1, int limit = 20, const std::string& status = "");

    /**
     * @brief 处理举报
     */
    bool resolveReport(int64_t reportId, int reviewerId, const std::string& resolution, const std::string& status);

    /**
     * @brief 获取敏感词列表
     */
    std::vector<SensitiveWord> getSensitiveWords();

    /**
     * @brief 创建敏感词
     */
    int createSensitiveWord(const std::string& word, const std::string& category, const std::string& severity,
                           bool isRegex, const std::string& replacement, int createdBy);

    /**
     * @brief 删除敏感词
     */
    bool deleteSensitiveWord(int id);

    /**
     * @brief 检查文本是否含敏感词
     */
    std::vector<SensitiveWordMatch> checkSensitiveWords(const std::string& text);

    /**
     * @brief 获取敏感词统计
     */
    std::map<std::string, int> getSensitiveWordStats();

    // ========================================================================
    // API密钥管理方法
    // ========================================================================

    /**
     * @brief 获取API密钥列表（分页）
     */
    PaginatedResponse<ApiKey> getApiKeys(int page = 1, int limit = 20, int userId = 0);

    /**
     * @brief 创建API密钥
     */
    std::pair<int, std::string> createApiKey(int userId, const std::string& name, const std::string& scopes,
                                            int rateLimitPerHour, const std::string& expiresAt, int createdBy);

    /**
     * @brief 删除API密钥
     */
    bool deleteApiKey(int id);

    /**
     * @brief 重新生成API密钥
     */
    std::string regenerateApiKey(int id);

    /**
     * @brief 获取API使用记录（分页）
     */
    PaginatedResponse<ApiUsage> getApiKeyUsage(int page = 1, int limit = 20, int keyId = 0);

    /**
     * @brief 获取API使用统计
     */
    ApiUsageStats getApiKeyStats(int keyId);

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
    std::string handleCreateUser(const std::string& body);
    std::string handleUpdateUser(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteUser(const std::map<std::string, std::string>& params);
    std::string handleActivateUser(const std::map<std::string, std::string>& params);
    std::string handleDeactivateUser(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 模块管理
    std::string handleListModules(const std::map<std::string, std::string>& params);
    std::string handleEnableModule(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDisableModule(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUploadModule(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleInstallModule(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUninstallModule(const std::map<std::string, std::string>& params);
    std::string handleReloadModule(const std::map<std::string, std::string>& params);
    std::string handleScanModules(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 密码管理
    std::string handleChangePassword(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleResetPassword(const std::map<std::string, std::string>& params, const std::string& body);

    // HTTP请求处理器 - 审计日志
    std::string handleGetAuditLogs(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 仪表盘
    std::string handleGetDashboard(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 用户详情
    std::string handleGetUserHistory(const std::map<std::string, std::string>& params);
    std::string handleGetUserSessions(const std::map<std::string, std::string>& params);
    std::string handleKickUserSession(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 公告管理
    std::string handleListAnnouncements(const std::map<std::string, std::string>& params);
    std::string handleCreateAnnouncement(const std::string& body);
    std::string handleUpdateAnnouncement(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteAnnouncement(const std::map<std::string, std::string>& params);
    std::string handleToggleAnnouncement(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 数据导出
    std::string handleExportUsers(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 系统监控
    std::string handleGetSystemMetrics(const std::map<std::string, std::string>& params);
    std::string handleGetServiceHealth(const std::map<std::string, std::string>& params);
    std::string handleGetSystemLogs(const std::map<std::string, std::string>& params);
    std::string handleGetLogStats(const std::map<std::string, std::string>& params);
    std::string handleCleanLogs(const std::map<std::string, std::string>& params);
    std::string handleGetPerformanceMetrics(const std::map<std::string, std::string>& params);
    std::string handleGetSlowQueries(const std::map<std::string, std::string>& params);
    std::string handleGetPerformanceBottlenecks(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 登录安全
    std::string handleGetLoginHistory(const std::map<std::string, std::string>& params);
    std::string handleGetLoginStats(const std::map<std::string, std::string>& params);
    std::string handleGetSuspiciousLogins(const std::map<std::string, std::string>& params);
    std::string handleGetIpBlacklist(const std::map<std::string, std::string>& params);
    std::string handleAddIpBlacklist(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleRemoveIpBlacklist(const std::map<std::string, std::string>& params);
    std::string handleGetAccountLockouts(const std::map<std::string, std::string>& params);
    std::string handleLockUserAccount(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUnlockUserAccount(const std::map<std::string, std::string>& params);
    std::string handleHandleSuspiciousLogin(const std::map<std::string, std::string>& params, const std::string& body);

    // HTTP请求处理器 - 全局配置
    std::string handleGetConfigCategories(const std::map<std::string, std::string>& params);
    std::string handleGetConfigs(const std::map<std::string, std::string>& params);
    std::string handleUpdateConfig(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetConfigHistory(const std::map<std::string, std::string>& params);
    std::string handleGetConfigSummary(const std::map<std::string, std::string>& params);
    std::string handleReloadConfigs(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 数据备份
    std::string handleGetBackupJobs(const std::map<std::string, std::string>& params);
    std::string handleCreateBackupJob(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUpdateBackupJob(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteBackupJob(const std::map<std::string, std::string>& params);
    std::string handleTriggerBackup(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetBackupRecords(const std::map<std::string, std::string>& params);
    std::string handleDeleteBackupFile(const std::map<std::string, std::string>& params);
    std::string handleGetBackupStats(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - RBAC权限管理
    std::string handleGetRoles(const std::map<std::string, std::string>& params);
    std::string handleCreateRole(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUpdateRole(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteRole(const std::map<std::string, std::string>& params);
    std::string handleGetPermissions(const std::map<std::string, std::string>& params);
    std::string handleGetPermissionMatrix(const std::map<std::string, std::string>& params);
    std::string handleGetRolePermissions(const std::map<std::string, std::string>& params);
    std::string handleUpdateRolePermissions(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetUserRoles(const std::map<std::string, std::string>& params);
    std::string handleAssignUserRole(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleRemoveUserRole(const std::map<std::string, std::string>& params);
    std::string handleCheckPermission(const std::map<std::string, std::string>& params, const std::string& body);

    // HTTP请求处理器 - 通知管理
    std::string handleGetNotificationTemplates(const std::map<std::string, std::string>& params);
    std::string handleCreateNotificationTemplate(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUpdateNotificationTemplate(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteNotificationTemplate(const std::map<std::string, std::string>& params);
    std::string handleGetSystemNotifications(const std::map<std::string, std::string>& params);
    std::string handleSendNotification(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetNotificationHistory(const std::map<std::string, std::string>& params);
    std::string handleGetNotificationStats(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 数据清理
    std::string handleGetCleanupTasks(const std::map<std::string, std::string>& params);
    std::string handleCreateCleanupTask(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleUpdateCleanupTask(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteCleanupTask(const std::map<std::string, std::string>& params);
    std::string handleTriggerCleanup(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetCleanupHistory(const std::map<std::string, std::string>& params);
    std::string handleGetStorageStats(const std::map<std::string, std::string>& params);

    // HTTP请求处理器 - 内容审核
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

    // HTTP请求处理器 - API密钥管理
    std::string handleGetApiKeys(const std::map<std::string, std::string>& params);
    std::string handleCreateApiKey(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteApiKey(const std::map<std::string, std::string>& params);
    std::string handleRegenerateApiKey(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleGetApiKeyUsage(const std::map<std::string, std::string>& params);
    std::string handleGetApiKeyStats(const std::map<std::string, std::string>& params);

    // 辅助函数
    std::string buildJsonResponse(bool success, const std::string& message, const std::string& data = "");
    std::string buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data = "");
    std::string escapeJson(const std::string& str);
    std::string escapeSql(const std::string& str);

    // 初始化测试数据
    void initializeTestData();
    void initializeModuleInfo();
};

} // namespace PaperCrawler
