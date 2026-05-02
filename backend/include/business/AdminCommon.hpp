#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <chrono>
#include <sstream>
#include <functional>
#include <memory>

namespace PaperCrawler {

// ============================================================================
// 用户角色
// ============================================================================

enum class UserRole {
    USER = 0,
    PREMIUM = 1,
    ADMIN = 2,
    SUPERADMIN = 3
};

// ============================================================================
// 用户信息（管理版）
// ============================================================================

struct AdminUser {
    int id;
    std::string username;
    std::string email;
    std::string fullName;
    std::string avatar;
    UserRole role;
    bool active;
    std::string passwordHash;
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

// ============================================================================
// 模块信息
// ============================================================================

struct ModuleInfo {
    std::string name;
    std::string version;
    std::string description;
    bool enabled;
    std::string type;
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

// ============================================================================
// 审计日志条目
// ============================================================================

struct AuditLog {
    int id;
    std::string action;
    std::string entityType;
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

// ============================================================================
// 管理统计数据
// ============================================================================

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

// ============================================================================
// 登录历史
// ============================================================================

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

// ============================================================================
// 公告
// ============================================================================

struct Announcement {
    int id;
    std::string title;
    std::string content;
    std::string type;
    std::string targetRole;
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

// ============================================================================
// 系统日志条目
// ============================================================================

struct SystemLog {
    int64_t id;
    std::string level;
    std::string module;
    std::string message;
    std::string context;
    std::string file;
    int line;
    std::string threadId;
    std::string createdAt;
};

// ============================================================================
// 系统资源指标
// ============================================================================

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

// ============================================================================
// 服务健康状态
// ============================================================================

struct ServiceHealth {
    std::string name;
    std::string status;
    int64_t responseTimeMs;
    std::string lastCheck;
};

// ============================================================================
// 性能指标
// ============================================================================

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

// ============================================================================
// 慢查询记录
// ============================================================================

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

// ============================================================================
// 日志统计
// ============================================================================

struct LogStats {
    std::string level;
    int count;
    std::string date;
};

// ============================================================================
// 登录尝试记录
// ============================================================================

struct LoginAttempt {
    int64_t id;
    std::string username;
    std::string ipAddress;
    std::string userAgent;
    bool success;
    std::string failureReason;
    std::string createdAt;
};

// ============================================================================
// IP黑名单条目
// ============================================================================

struct IpBlacklistEntry {
    int id;
    std::string ipAddress;
    std::string reason;
    std::string threatLevel;
    int attemptCount;
    int createdBy;
    std::string createdAt;
    std::string expiresAt;
    bool isActive;
};

// ============================================================================
// 可疑登录活动
// ============================================================================

struct SuspiciousLogin {
    int64_t id;
    std::string username;
    std::string ipAddress;
    std::string suspicionReason;
    int riskScore;
    std::string status;
    int reviewedBy;
    std::string reviewedAt;
    std::string createdAt;
};

// ============================================================================
// 账户锁定记录
// ============================================================================

struct AccountLockout {
    int id;
    int userId;
    std::string username;
    std::string lockedUntil;
    std::string lockoutReason;
    int failedAttempts;
    std::string ipAddress;
    std::string createdAt;
};

// ============================================================================
// 登录统计
// ============================================================================

struct LoginStats {
    std::string date;
    int successfulLogins;
    int failedLogins;
    int uniqueUsers;
    int uniqueIps;
};

// ============================================================================
// 系统配置项
// ============================================================================

struct SystemConfig {
    int id;
    std::string key;
    std::string value;
    std::string valueType;
    std::string category;
    std::string description;
    std::string defaultValue;
    bool isPublic;
    bool isEncrypted;
    int updatedBy;
    std::string updatedAt;
    std::string createdAt;
};

// ============================================================================
// 配置变更历史
// ============================================================================

struct ConfigHistoryEntry {
    int64_t id;
    int configId;
    std::string configKey;
    std::string oldValue;
    std::string newValue;
    int changedBy;
    std::string changedByUsername;
    std::string changeReason;
    std::string changeType;
    std::string createdAt;
};

// ============================================================================
// 备份任务
// ============================================================================

struct BackupJob {
    int id;
    std::string name;
    std::string jobType;
    std::string description;
    std::string scheduleCron;
    std::string backupPath;
    int retentionDays;
    bool isEnabled;
    std::string lastRunAt;
    std::string lastRunStatus;
    std::string lastRunMessage;
    int createdBy;
    std::string createdAt;
};

// ============================================================================
// 备份记录
// ============================================================================

struct BackupRecord {
    int64_t id;
    int jobId;
    std::string filename;
    std::string filePath;
    int64_t fileSize;
    std::string backupType;
    std::string status;
    std::string startedAt;
    std::string completedAt;
    int durationSeconds;
    std::string errorMessage;
    int tablesBackedUp;
    int64_t rowsBackedUp;
    int createdBy;
    std::string jobName;
};

// ============================================================================
// 配置分类统计
// ============================================================================

struct ConfigCategorySummary {
    std::string category;
    int configCount;
    int recentlyUpdated;
};

// ============================================================================
// RBAC 角色权限相关
// ============================================================================

struct Role {
    int id;
    std::string name;
    std::string displayName;
    std::string description;
    int level;
    bool isSystem;
    bool isDefault;
    std::string createdAt;
    std::string updatedAt;
};

struct Permission {
    int id;
    std::string resource;
    std::string action;
    std::string description;
};

struct RolePermission {
    int roleId;
    std::string roleName;
    int permissionId;
    std::string resource;
    std::string action;
    std::string grantedAt;
    std::string grantedByUsername;
};

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

struct PermissionMatrix {
    std::string roleName;
    int totalPermissions;
    std::map<std::string, int> permissionsByResource;
};

// ============================================================================
// 通知管理相关
// ============================================================================

struct NotificationTemplate {
    int id;
    std::string name;
    std::string titleTemplate;
    std::string contentTemplate;
    std::string channel;
    std::string description;
    std::string language;
    bool isActive;
    std::string createdAt;
};

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
    std::string status;
    std::string scheduledAt;
    std::string sentAt;
    int createdBy;
    std::string createdAt;
};

struct NotificationDelivery {
    int64_t id;
    int64_t notificationId;
    int userId;
    std::string username;
    std::string status;
    std::string sentAt;
    std::string readAt;
    std::string errorMessage;
    std::string createdAt;
};

// ============================================================================
// 数据清理相关
// ============================================================================

struct CleanupTask {
    int id;
    std::string name;
    std::string displayName;
    std::string taskType;
    std::string description;
    std::string cleanupConfig;
    std::string scheduleCron;
    bool isEnabled;
    bool isSystem;
    std::string lastRunAt;
    std::string lastRunStatus;
    std::string lastRunMessage;
    int createdBy;
    std::string createdAt;
};

struct CleanupExecution {
    int64_t id;
    int taskId;
    std::string taskName;
    std::string status;
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
// 内容审核相关
// ============================================================================

struct PaperModeration {
    int64_t id;
    int paperId;
    std::string status;
    int moderatorId;
    std::string moderatorUsername;
    std::string reason;
    std::string reviewedAt;
    std::string flags;
    std::string createdAt;
};

struct UserReport {
    int64_t id;
    int reporterId;
    std::string reporterUsername;
    std::string targetType;
    int targetId;
    std::string reason;
    std::string description;
    std::string status;
    std::string priority;
    int reviewerId;
    std::string reviewerUsername;
    std::string resolution;
    std::string createdAt;
};

struct SensitiveWord {
    int id;
    std::string word;
    std::string category;
    std::string severity;
    bool isRegex;
    std::string replacement;
    bool isActive;
    int matchCount;
    int createdBy;
    std::string createdAt;
};

struct SensitiveWordMatch {
    std::string word;
    std::string category;
    int startPosition;
    int endPosition;
    std::string matchedText;
};

// ============================================================================
// API密钥相关
// ============================================================================

struct ApiKey {
    int id;
    int userId;
    std::string username;
    std::string name;
    std::string keyPrefix;
    std::string scopes;
    int rateLimitPerHour;
    std::string expiresAt;
    std::string lastUsedAt;
    int64_t requestCount;
    bool isActive;
    int createdBy;
    std::string createdAt;
};

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

struct ApiUsageStats {
    int64_t totalRequests;
    int64_t successfulRequests;
    int64_t failedRequests;
    double avgResponseTime;
    std::map<std::string, int64_t> requestsByEndpoint;
    std::map<std::string, int64_t> requestsByDay;
};

// ============================================================================
// 分页响应模板
// ============================================================================

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

} // namespace PaperCrawler
