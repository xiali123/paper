#include "business/AdminUserManagementModule.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include "core/ModuleLoader.hpp"
#include "core/ModuleMetadata.hpp"
#include "features/security/SecurityModule.hpp"
#include "data/PreparedStatement.hpp"
#include "../../core/external/nlohmann/json.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <algorithm>
#include <regex>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <random>
#include <filesystem>
#include <chrono>
#include "data/ValidationHelper.hpp"
#include "data/StringUtil.hpp"
#include "core/HttpStatus.hpp"

namespace PaperCrawler {

// ============================================================================
// MySQL datetime helpers
// ============================================================================

static std::chrono::system_clock::time_point parseMysqlDateTime(const std::string& datetime) {
    if (datetime.empty() || datetime == "0000-00-00 00:00:00" || datetime == "NULL") {
        return std::chrono::system_clock::from_time_t(0);
    }

    struct tm tm = {};
    int y, m, d, h, min, s;
    if (sscanf(datetime.c_str(), "%d-%d-%d %d-%d-%d", &y, &m, &d, &h, &min, &s) == 6) {
        tm.tm_year = y - 1900;
        tm.tm_mon = m - 1;
        tm.tm_mday = d;
        tm.tm_hour = h;
        tm.tm_min = min;
        tm.tm_sec = s;
        tm.tm_isdst = -1;
        time_t t = mktime(&tm);
        return std::chrono::system_clock::from_time_t(t);
    }

    return std::chrono::system_clock::from_time_t(0);
}

// ============================================================================
// AdminUserManagementModule::Impl - Internal implementation
// ============================================================================

class AdminUserManagementModule::Impl {
public:
    std::shared_ptr<IDatabase> database_;

    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {
    }

    std::string hashPassword(const std::string& password) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), hash);

        std::ostringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    std::string generateRandomPassword(int length = 12) {
        const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, chars.length() - 1);

        std::string password;
        for (int i = 0; i < length; i++) {
            password += chars[dis(gen)];
        }
        return password;
    }

    int extractAdminUserIdFromHeaders(const std::map<std::string, std::string>& headers) {
        auto authIt = headers.find("Authorization");
        if (authIt == headers.end()) {
            authIt = headers.find("authorization");
        }
        if (authIt == headers.end()) return 0;

        std::string token = authIt->second;
        if (token.find("Bearer ") == 0) token = token.substr(7);
        if (token.empty()) return 0;

        SecurityModule sec;
        auto result = sec.verifyJWT(token);
        if (!result.valid) return 0;

        auto subIt = result.claims.find("sub");
        if (subIt == result.claims.end()) return 0;

        try { return std::stoi(subIt->second); }
        catch (...) { return 0; }
    }
};

// ============================================================================
// AdminUserManagementModule - Constructor and destructor
// ============================================================================

AdminUserManagementModule::AdminUserManagementModule()
    : impl_(std::make_unique<Impl>(nullptr)), database_(nullptr) {
    spdlog::info("[AdminUserManagement] Default constructor called");
}

AdminUserManagementModule::~AdminUserManagementModule() {
    spdlog::info("[AdminUserManagement] Destructor called");
}

void AdminUserManagementModule::setDatabase(std::shared_ptr<IDatabase> database) {
    spdlog::info("[AdminUserManagement] Received injected database connection");
    BusinessModuleBase::setDatabase(database);
    database_ = database;
    if (impl_) {
        impl_->database_ = database;
    }
}

// ============================================================================
// AdminUserManagementModule - Business logic methods
// ============================================================================

AdminStats AdminUserManagementModule::getStats() {
    std::lock_guard<std::mutex> lock(usersMutex_);

    AdminStats stats;

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return stats;
        }

        // 查询总用户数
        auto totalResults = database_->query("SELECT COUNT(*) as total FROM users");
        if (!totalResults.empty()) {
            stats.totalUsers = std::stoi(totalResults[0]["total"]);
        }

        // 查询活跃用户数
        auto activeResults = database_->query("SELECT COUNT(*) as total FROM users WHERE is_active = 1");
        if (!activeResults.empty()) {
            stats.activeUsers = std::stoi(activeResults[0]["total"]);
        }

        // 查询premium用户数
        auto premiumResults = database_->query("SELECT COUNT(*) as total FROM users WHERE role = 'premium'");
        if (!premiumResults.empty()) {
            stats.premiumUsers = std::stoi(premiumResults[0]["total"]);
        }

        // 查询admin和superadmin用户数
        auto adminResults = database_->query("SELECT COUNT(*) as total FROM users WHERE role IN ('admin', 'superadmin')");
        if (!adminResults.empty()) {
            stats.adminUsers = std::stoi(adminResults[0]["total"]);
        }

        // 查询近30天活跃用户数
        auto recentActiveResults = database_->query(
            "SELECT COUNT(*) as total FROM users WHERE is_active = 1 AND last_login_at >= DATE_SUB(NOW(), INTERVAL 30 DAY)");
        if (!recentActiveResults.empty()) {
            stats.recentlyActiveUsers = std::stoi(recentActiveResults[0]["total"]);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminUserManagement] Failed to get stats: {}", e.what());
    }

    return stats;
}


PaginatedResponse<AdminUser> AdminUserManagementModule::listUsers(int page, int limit, const std::string& search, UserRole roleFilter) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    PaginatedResponse<AdminUser> response;
    response.page = page;
    response.limit = limit;

    try {
        // 从数据库查询用户
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return response;
        }

        // 构建SQL查询
        std::string sql = "SELECT * FROM users";
        // Build WHERE clause using PreparedStatement to prevent SQL injection
        std::string whereClause;
        if (!search.empty() && roleFilter != UserRole::USER) {
            whereClause = " WHERE (username LIKE ? OR email LIKE ?) AND role = ?";
        } else if (!search.empty()) {
            whereClause = " WHERE (username LIKE ? OR email LIKE ?)";
        } else if (roleFilter != UserRole::USER) {
            whereClause = " WHERE role = ?";
        }

        sql += whereClause + " LIMIT ? OFFSET ?";

        spdlog::info("[AdminApiModule] Executing prepared SQL for user list");
        PreparedStatement stmt(database_, sql);
        int bindIdx = 0;
        if (!search.empty()) {
            stmt.bind(bindIdx++, std::string("%" + search + "%"));
            stmt.bind(bindIdx++, std::string("%" + search + "%"));
        }
        if (roleFilter != UserRole::USER) {
            std::string roleStr;
            switch (roleFilter) {
                case UserRole::PREMIUM: roleStr = "premium"; break;
                case UserRole::ADMIN: roleStr = "admin"; break;
                case UserRole::SUPERADMIN: roleStr = "superadmin"; break;
                default: roleStr = "user"; break;
            }
            stmt.bind(bindIdx++, roleStr);
        }
        stmt.bind(bindIdx++, limit);
        stmt.bind(bindIdx, (page - 1) * limit);
        auto results = stmt.query();

        // 查询总数
        std::string countSql = "SELECT COUNT(*) as total FROM users" + whereClause;
        PreparedStatement countStmt(database_, countSql);
        bindIdx = 0;
        if (!search.empty()) {
            countStmt.bind(bindIdx++, std::string("%" + search + "%"));
            countStmt.bind(bindIdx++, std::string("%" + search + "%"));
        }
        if (roleFilter != UserRole::USER) {
            std::string roleStr;
            switch (roleFilter) {
                case UserRole::PREMIUM: roleStr = "premium"; break;
                case UserRole::ADMIN: roleStr = "admin"; break;
                case UserRole::SUPERADMIN: roleStr = "superadmin"; break;
                default: roleStr = "user"; break;
            }
            countStmt.bind(bindIdx++, roleStr);
        }
        auto countResults = countStmt.query();
        if (!countResults.empty()) {
            response.total = std::stoi(countResults[0]["total"]);
        }

        response.totalPages = (response.total + limit - 1) / limit;

        // 解析用户数据
        for (const auto& row : results) {
            AdminUser user;
            user.id = std::stoi(row.at("id"));
            user.username = row.at("username");
            user.email = row.at("email");
            user.fullName = row.at("full_name");
            user.avatar = row.count("avatar") > 0 ? row.at("avatar") : "";
            user.role = AdminUser::fromString(row.at("role"));
            user.active = (row.at("is_active") == "1" || row.at("is_active") == "TRUE");

            // 时间戳转换：解析MySQL datetime字符串
            if (row.count("created_at") > 0 && !row.at("created_at").empty()) {
                user.createdAt = parseMysqlDateTime(row.at("created_at"));
            } else {
                user.createdAt = std::chrono::system_clock::from_time_t(0);
            }
            if (row.count("last_login_at") > 0 && !row.at("last_login_at").empty()) {
                user.lastLoginAt = parseMysqlDateTime(row.at("last_login_at"));
            } else {
                user.lastLoginAt = std::chrono::system_clock::from_time_t(0);
            }
            user.lastLoginIp = StringUtil::cleanDbString(row.count("last_login_ip") > 0 ? row.at("last_login_ip") : "");
            user.loginCount = row.count("login_count") > 0 && row.at("login_count") != "NULL" ? std::stoi(row.at("login_count")) : 0;

            response.items.push_back(user);

            response.items.push_back(user);
        }

    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to list users: {}", e.what());
    }

    return response;
}

std::optional<AdminUser> AdminUserManagementModule::getUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        PreparedStatement stmt(database_, "SELECT * FROM users WHERE id = ?");
        stmt.bind(0, id);
        auto results = stmt.query();

        if (!results.empty()) {
            AdminUser user;
            user.id = std::stoi(results[0].at("id"));
            user.username = results[0].at("username");
            user.email = results[0].at("email");
            user.fullName = results[0].at("full_name");
            user.avatar = results[0].count("avatar") > 0 ? results[0].at("avatar") : "";
            user.role = AdminUser::fromString(results[0].at("role"));
            user.active = (results[0].at("is_active") == "1" || results[0].at("is_active") == "TRUE");

            // 时间戳转换：解析MySQL datetime字符串
            if (results[0].count("created_at") > 0 && !results[0].at("created_at").empty()) {
                user.createdAt = parseMysqlDateTime(results[0].at("created_at"));
            } else {
                user.createdAt = std::chrono::system_clock::from_time_t(0);
            }
            if (results[0].count("last_login_at") > 0 && !results[0].at("last_login_at").empty()) {
                user.lastLoginAt = parseMysqlDateTime(results[0].at("last_login_at"));
            } else {
                user.lastLoginAt = std::chrono::system_clock::from_time_t(0);
            }
            user.lastLoginIp = StringUtil::cleanDbString(results[0].count("last_login_ip") > 0 ? results[0].at("last_login_ip") : "");
            user.loginCount = results[0].count("login_count") > 0 && results[0].at("login_count") != "NULL" ? std::stoi(results[0].at("login_count")) : 0;

            return user;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get user: {}", e.what());
    }

    return std::nullopt;
}

std::optional<AdminUser> AdminUserManagementModule::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        PreparedStatement stmt(database_, "SELECT * FROM users WHERE username = ?");
        stmt.bind(0, username);
        auto results = stmt.query();

        if (!results.empty()) {
            AdminUser user;
            user.id = std::stoi(results[0].at("id"));
            user.username = results[0].at("username");
            user.email = results[0].at("email");
            user.fullName = results[0].at("full_name");
            user.avatar = results[0].count("avatar") > 0 ? results[0].at("avatar") : "";
            user.role = AdminUser::fromString(results[0].at("role"));
            user.active = (results[0].at("is_active") == "1" || results[0].at("is_active") == "TRUE");

            // 时间戳转换：解析MySQL datetime字符串
            if (results[0].count("created_at") > 0 && !results[0].at("created_at").empty()) {
                user.createdAt = parseMysqlDateTime(results[0].at("created_at"));
            } else {
                user.createdAt = std::chrono::system_clock::from_time_t(0);
            }
            if (results[0].count("last_login_at") > 0 && !results[0].at("last_login_at").empty()) {
                user.lastLoginAt = parseMysqlDateTime(results[0].at("last_login_at"));
            } else {
                user.lastLoginAt = std::chrono::system_clock::from_time_t(0);
            }
            user.lastLoginIp = StringUtil::cleanDbString(results[0].count("last_login_ip") > 0 ? results[0].at("last_login_ip") : "");
            user.loginCount = results[0].count("login_count") > 0 && results[0].at("login_count") != "NULL" ? std::stoi(results[0].at("login_count")) : 0;

            return user;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get user by username: {}", e.what());
    }

    return std::nullopt;
}

bool AdminUserManagementModule::verifyUserPassword(int userId, const std::string& password) {
    try {
        if (!database_) return false;
        PreparedStatement stmt(database_, "SELECT password_hash FROM users WHERE id = ?");
        stmt.bind(0, userId);
        auto results = stmt.query();
        if (results.empty()) return false;
        std::string inputHash = impl_->hashPassword(password);
        return inputHash == results[0]["password_hash"];
    } catch (const std::exception& e) {
        spdlog::error("[AdminUserManagement] verifyUserPassword failed: {}", e.what());
        return false;
    }
}

bool AdminUserManagementModule::changeUserPassword(int userId, const std::string& oldPassword, const std::string& newPassword) {
    try {
        if (!database_) return false;
        PreparedStatement fetchStmt(database_, "SELECT username, password_hash FROM users WHERE id = ?");
        fetchStmt.bind(0, userId);
        auto results = fetchStmt.query();
        if (results.empty()) return false;

        std::string username = results[0]["username"];
        std::string storedHash = results[0]["password_hash"];

        // 验证旧密码
        std::string oldHash = impl_->hashPassword(oldPassword);
        if (oldHash != storedHash) {
            spdlog::warn("[AdminUserManagement] Password change failed for user {}: old password mismatch", username);
            return false;
        }

        // 更新为新密码
        std::string newHash = impl_->hashPassword(newPassword);
        PreparedStatement updateStmt(database_, "UPDATE users SET password_hash = ? WHERE id = ?");
        updateStmt.bind(0, newHash);
        updateStmt.bind(1, userId);

        if (updateStmt.execute()) {
            addAuditLog("password_changed", "user", userId, username, userId,
                        "User changed password", "127.0.0.1");
            spdlog::info("[AdminUserManagement] Password changed for user: {}", username);
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminUserManagement] changeUserPassword failed: {}", e.what());
    }
    return false;
}

bool AdminUserManagementModule::resetUserPassword(int userId, const std::string& newPassword) {
    try {
        if (!database_) return false;
        PreparedStatement fetchStmt(database_, "SELECT username FROM users WHERE id = ?");
        fetchStmt.bind(0, userId);
        auto results = fetchStmt.query();
        if (results.empty()) return false;

        std::string username = results[0]["username"];
        std::string newHash = impl_->hashPassword(newPassword);
        PreparedStatement updateStmt(database_, "UPDATE users SET password_hash = ? WHERE id = ?");
        updateStmt.bind(0, newHash);
        updateStmt.bind(1, userId);

        if (updateStmt.execute()) {
            addAuditLog("password_reset", "user", userId, "admin", 0,
                        "Password reset by admin for user: " + username, "127.0.0.1");
            spdlog::info("[AdminUserManagement] Password reset for user: {}", username);
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminUserManagement] resetUserPassword failed: {}", e.what());
    }
    return false;
}

std::optional<AdminUser> AdminUserManagementModule::updateUser(int id, const AdminUser& user) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        // 构建角色字符串
        std::string roleStr;
        switch (user.role) {
            case UserRole::PREMIUM: roleStr = "premium"; break;
            case UserRole::ADMIN: roleStr = "admin"; break;
            case UserRole::SUPERADMIN: roleStr = "superadmin"; break;
            default: roleStr = "user"; break;
        }

        PreparedStatement stmt(database_, "UPDATE users SET "
                         "email = ?, "
                         "full_name = ?, "
                         "avatar = ?, "
                         "role = ? "
                         "WHERE id = ?");
        stmt.bind(0, user.email);
        stmt.bind(1, user.fullName);
        stmt.bind(2, user.avatar);
        stmt.bind(3, roleStr);
        stmt.bind(4, id);

        if (stmt.execute()) {
            // 记录审计日志
            addAuditLog("user_updated", "user", id, "system", 0,
                        "Updated user: " + user.username, "127.0.0.1");

            // 返回更新后的用户
            return getUser(id);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to update user: {}", e.what());
    }

    return std::nullopt;
}

bool AdminUserManagementModule::deleteUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return false;
        }

        // 获取用户名用于审计日志
        auto user = getUser(id);
        std::string username = user ? user->username : "unknown";

        PreparedStatement stmt(database_, "DELETE FROM users WHERE id = ?");
        stmt.bind(0, id);

        if (stmt.execute()) {
            // 记录审计日志
            addAuditLog("user_deleted", "user", id, "system", 0,
                        "Deleted user: " + username, "127.0.0.1");

            spdlog::info("[AdminApiModule] Deleted user: {}", username);
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to delete user: {}", e.what());
    }

    return false;
}

std::optional<AdminUser> AdminUserManagementModule::activateUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        PreparedStatement stmt(database_, "UPDATE users SET is_active = 1 WHERE id = ?");
        stmt.bind(0, id);

        if (stmt.execute()) {
            // 获取用户名用于审计日志
            auto user = getUser(id);
            if (user) {
                addAuditLog("user_activated", "user", id, "system", 0,
                            "Activated user: " + user->username, "127.0.0.1");
                spdlog::info("[AdminApiModule] Activated user: {}", user->username);
            }
            return getUser(id);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to activate user: {}", e.what());
    }

    return std::nullopt;
}

std::optional<AdminUser> AdminUserManagementModule::deactivateUser(int id) {
    std::lock_guard<std::mutex> lock(usersMutex_);

    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        PreparedStatement stmt(database_, "UPDATE users SET is_active = 0 WHERE id = ?");
        stmt.bind(0, id);

        if (stmt.execute()) {
            // 获取用户名用于审计日志
            auto user = getUser(id);
            if (user) {
                addAuditLog("user_deactivated", "user", id, "system", 0,
                            "Deactivated user: " + user->username, "127.0.0.1");
                spdlog::info("[AdminApiModule] Deactivated user: {}", user->username);
            }
            return getUser(id);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to deactivate user: {}", e.what());
    }

    return std::nullopt;
}

std::optional<AdminUser> AdminUserManagementModule::createUser(const AdminUser& user) {
    try {
        if (!database_) {
            spdlog::error("[AdminApiModule] No database connection available");
            return std::nullopt;
        }

        // 检查用户名是否已存在（不持有mutex，使用直接查询）
        PreparedStatement checkUserStmt(database_, "SELECT id FROM users WHERE username = ?");
        checkUserStmt.bind(0, user.username);
        auto usernameResults = checkUserStmt.query();
        if (!usernameResults.empty()) {
            spdlog::warn("[AdminApiModule] Username already exists: {}", user.username);
            return std::nullopt;
        }

        // 检查邮箱是否已存在
        PreparedStatement checkEmailStmt(database_, "SELECT id FROM users WHERE email = ?");
        checkEmailStmt.bind(0, user.email);
        auto emailResults = checkEmailStmt.query();
        if (!emailResults.empty()) {
            spdlog::warn("[AdminApiModule] Email already exists: {}", user.email);
            return std::nullopt;
        }

        std::lock_guard<std::mutex> lock(usersMutex_);

        // 构建角色字符串
        std::string roleStr;
        switch (user.role) {
            case UserRole::PREMIUM: roleStr = "premium"; break;
            case UserRole::ADMIN: roleStr = "admin"; break;
            case UserRole::SUPERADMIN: roleStr = "superadmin"; break;
            default: roleStr = "user"; break;
        }

        // 生成密码哈希
        std::string passwordHash = impl_->hashPassword(user.passwordHash);

        // 获取当前时间并格式化为MySQL datetime格式
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        std::tm* tm = std::localtime(&timestamp);
        char datetimeBuffer[64];
        std::strftime(datetimeBuffer, sizeof(datetimeBuffer), "%Y-%m-%d %H:%M:%S", tm);
        std::string datetimeStr(datetimeBuffer);

        // 插入新用户
        PreparedStatement insertStmt(database_, "INSERT INTO users (username, email, password_hash, full_name, role, is_active, created_at, is_verified) VALUES (?, ?, ?, ?, ?, 1, ?, 1)");
        insertStmt.bind(0, user.username);
        insertStmt.bind(1, user.email);
        insertStmt.bind(2, passwordHash);
        insertStmt.bind(3, user.fullName);
        insertStmt.bind(4, roleStr);
        insertStmt.bind(5, datetimeStr);

        if (insertStmt.execute()) {
            // 获取新创建的用户完整信息
            PreparedStatement fetchNewStmt(database_, "SELECT * FROM users WHERE username = ?");
            fetchNewStmt.bind(0, user.username);
            auto newResults = fetchNewStmt.query();

            if (!newResults.empty()) {
                AdminUser newUser;
                newUser.id = std::stoi(newResults[0]["id"]);
                newUser.username = newResults[0]["username"];
                newUser.email = newResults[0]["email"];
                newUser.fullName = newResults[0].count("full_name") > 0 ? newResults[0]["full_name"] : "";
                newUser.avatar = newResults[0].count("avatar") > 0 ? newResults[0]["avatar"] : "";
                newUser.role = AdminUser::fromString(newResults[0]["role"]);
                newUser.active = (newResults[0]["is_active"] == "1" || newResults[0]["is_active"] == "TRUE");
                // 解析实际DB时间戳
                if (newResults[0].count("created_at") > 0 && !newResults[0]["created_at"].empty()) {
                    newUser.createdAt = parseMysqlDateTime(newResults[0]["created_at"]);
                } else {
                    newUser.createdAt = std::chrono::system_clock::from_time_t(0);
                }
                newUser.lastLoginAt = std::chrono::system_clock::from_time_t(0);
                newUser.lastLoginIp = "";

                // 记录审计日志
                addAuditLog("user_created", "user", newUser.id, "system", 0,
                            "Created user: " + user.username, "127.0.0.1");

                spdlog::info("[AdminApiModule] Created user: {}", user.username);
                return newUser;
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to create user: {}", e.what());
    }

    return std::nullopt;
}

void AdminUserManagementModule::registerRoutes() {
    auto& router = Router::getInstance();
    const std::string prefix = "/api/admin";

    database_ = getDatabase();
    if (database_) {
        spdlog::info("[AdminUserManagement] Received injected database connection");
        if (impl_) {
            impl_->database_ = database_;
        }
    } else {
        spdlog::warn("[AdminUserManagement] No injected database connection available");
    }

    auto requireAdminAuth = [](const HttpRequest& req) -> bool {
        auto authIt = req.headers.find("Authorization");
        if (authIt == req.headers.end()) return false;

        const std::string& authHeader = authIt->second;
        if (authHeader.substr(0, 7) != "Bearer ") return false;

        std::string token = authHeader.substr(7);
        if (token.empty()) return false;

        SecurityModule sec;
        auto result = sec.verifyJWT(token);
        if (!result.valid) return false;

        auto roleIt = result.claims.find("role");
        if (roleIt == result.claims.end()) return false;
        if (roleIt->second != "admin" && roleIt->second != "superadmin") return false;

        return true;
    };

    auto unauthorizedResp = []() -> HttpResponse {
        HttpResponse resp;
        resp.statusCode = HTTP::UNAUTHORIZED;
        resp.setHeader("Content-Type", "application/json");
        resp.body = R"({"success":false,"error":"Unauthorized. Admin authentication required."})";
        return resp;
    };

    // Stats
    router.get(prefix + "/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetStats(req.queryParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // User CRUD
    router.get(prefix + "/users", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleListUsers(req.queryParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/users/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetUser(req.pathParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleCreateUser(req.body);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/users/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUpdateUser(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/users/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeleteUser(req.pathParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users/:id/activate", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleActivateUser(req.pathParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users/:id/deactivate", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleDeactivateUser(req.pathParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // Password management
    router.post(prefix + "/users/:id/change-password", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleChangePassword(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/users/:id/reset-password", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleResetPassword(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // User details
    router.get(prefix + "/users/:id/history", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetUserHistory(req.pathParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/users/:id/sessions", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetUserSessions(req.pathParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/users/:id/sessions/:sid", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleKickUserSession(req.pathParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // Data export
    router.post(prefix + "/export/users", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string csv = handleExportUsers(req.queryParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "text/csv; charset=utf-8");
        response.setHeader("Content-Disposition", "attachment; filename=\"users_export.csv\"");
        response.body = csv;
        return response;
    });

    // Login security
    router.get(prefix + "/security/login-history", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetLoginHistory(req.queryParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/security/login-stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetLoginStats(req.queryParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/security/suspicious", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetSuspiciousLogins(req.queryParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/security/ip-blacklist", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetIpBlacklist(req.queryParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/security/ip-blacklist", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleAddIpBlacklist(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/security/ip-blacklist/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleRemoveIpBlacklist(req.pathParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/security/account-lockouts", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleGetAccountLockouts(req.queryParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/security/lock-user", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleLockUserAccount(req.queryParams, req.body);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/security/unlock-user", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleUnlockUserAccount(req.queryParams);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/security/suspicious/:id/handle", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        std::string body = handleHandleSuspiciousLogin(req.pathParams, req.body, req.headers);
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    spdlog::info("[AdminUserManagement] Routes registered successfully");
}

// ============================================================================
// AdminUserManagementModule - Handler implementations
// ============================================================================

std::string AdminUserManagementModule::handleGetStats(const std::map<std::string, std::string>& params) {
    auto stats = getStats();

    // 直接构建符合前端期望的响应格式
    std::ostringstream result;
    result << "{";
    result << "\"success\":true,";
    result << "\"data\":" << stats.toJSON();
    result << "}";
    return result.str();
}

std::string AdminUserManagementModule::handleListUsers(const std::map<std::string, std::string>& params) {
    int page = 1, limit = 20;
    std::string search;
    UserRole roleFilter = UserRole::USER;

    auto pageIt = params.find("page");
    if (pageIt != params.end()) page = std::stoi(pageIt->second);

    auto limitIt = params.find("limit");
    if (limitIt != params.end()) limit = std::stoi(limitIt->second);

    auto searchIt = params.find("search");
    if (searchIt != params.end()) search = searchIt->second;

    auto roleIt = params.find("role");
    if (roleIt != params.end()) {
        roleFilter = AdminUser::fromString(roleIt->second);
    }

    auto response = listUsers(page, limit, search, roleFilter);

    // 构建用户数组JSON
    std::ostringstream usersJson;
    usersJson << "[";
    for (size_t i = 0; i < response.items.size(); i++) {
        if (i > 0) usersJson << ",";
        usersJson << response.items[i].toJSON();
    }
    usersJson << "]";

    // 直接构建符合前端期望的响应格式
    std::ostringstream result;
    result << "{";
    result << "\"success\":true,";
    result << "\"data\":{";
    result << "\"users\":" << usersJson.str() << ",";
    result << "\"pagination\":{";
    result << "\"page\":" << response.page << ",";
    result << "\"limit\":" << response.limit << ",";
    result << "\"total\":" << response.total << ",";
    result << "\"totalPages\":" << response.totalPages;
    result << "}}}";
    return result.str();
}

std::string AdminUserManagementModule::handleGetUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto user = getUser(id);

        if (!user) {
            return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "User not found");
        }

        return StringUtil::buildJsonResponse(HTTP::OK, true, "User retrieved", user->toJSON());
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleCreateUser(const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);

        // 验证必填字段
        if (!jsonBody.contains("username") || !jsonBody.contains("email")) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing required fields: username and email are required");
        }

        AdminUser newUser;
        newUser.username = ValidationHelper::sanitize(jsonBody["username"].get<std::string>());
        newUser.email = ValidationHelper::sanitize(jsonBody["email"].get<std::string>());
        newUser.fullName = ValidationHelper::sanitize(jsonBody.value("full_name", ""));
        newUser.avatar = jsonBody.value("avatar", "");
        newUser.role = AdminUser::fromString(jsonBody.value("role", "user"));

        // 密码必须由管理员在请求中提供，无默认值
        std::string password = jsonBody.value("password", "");
        if (password.empty()) {
            spdlog::warn("[Admin] createUser: password is required");
            return "{\"success\":false,\"error\":\"Password is required\"}";
        }
        // Hash password using SHA256 (in production should use SecurityModule)
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);
        std::ostringstream hashHex;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            hashHex << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        newUser.passwordHash = hashHex.str();

        newUser.active = true;
        newUser.createdAt = std::chrono::system_clock::now();
        newUser.lastLoginAt = std::chrono::system_clock::from_time_t(0);
        newUser.lastLoginIp = "";

        auto createdUser = createUser(newUser);

        if (!createdUser) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Failed to create user - username or email may already exist");
        }

        return StringUtil::buildJsonResponse(HTTP::OK, true, "User created successfully", createdUser->toJSON());
    } catch (const nlohmann::json::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleUpdateUser(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto jsonBody = nlohmann::json::parse(body);

        AdminUser user;
        user.id = id;
        user.email = ValidationHelper::sanitize(jsonBody.value("email", ""));
        user.fullName = ValidationHelper::sanitize(jsonBody.value("full_name", ""));
        user.avatar = jsonBody.value("avatar", "");
        user.role = AdminUser::fromString(jsonBody.value("role", "user"));

        auto updatedUser = updateUser(id, user);

        if (!updatedUser) {
            return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "User not found");
        }

        return StringUtil::buildJsonResponse(HTTP::OK, true, "User updated", updatedUser->toJSON());
    } catch (const nlohmann::json::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleDeleteUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);

        if (deleteUser(id)) {
            return StringUtil::buildJsonResponse(true, "User deleted");
        }

        return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "User not found");
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleActivateUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto user = activateUser(id);

        if (!user) {
            return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "User not found");
        }

        return StringUtil::buildJsonResponse(HTTP::OK, true, "User activated", user->toJSON());
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleDeactivateUser(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto user = deactivateUser(id);

        if (!user) {
            return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "User not found");
        }

        return StringUtil::buildJsonResponse(HTTP::OK, true, "User deactivated", user->toJSON());
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleChangePassword(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto jsonBody = nlohmann::json::parse(body);

        std::string oldPassword = jsonBody.value("old_password", "");
        std::string newPassword = jsonBody.value("new_password", "");

        if (oldPassword.empty() || newPassword.empty()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing old_password or new_password");
        }

        if (newPassword.length() < 6) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "New password must be at least 6 characters");
        }

        if (changeUserPassword(id, oldPassword, newPassword)) {
            return StringUtil::buildJsonResponse(true, "Password changed successfully");
        }

        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Old password is incorrect");
    } catch (const nlohmann::json::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleResetPassword(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto jsonBody = nlohmann::json::parse(body);

        std::string newPassword = jsonBody.value("new_password", "");

        if (newPassword.empty()) {
            // 如果没有提供密码，自动生成一个
            newPassword = impl_->generateRandomPassword(12);
        }

        if (newPassword.length() < 6) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Password must be at least 6 characters");
        }

        if (resetUserPassword(id, newPassword)) {
            nlohmann::json result;
            result["message"] = "Password reset successfully";
            // 只有自动生成的密码才返回
            if (jsonBody.value("new_password", "").empty()) {
                result["generated_password"] = newPassword;
            }
            return StringUtil::buildJsonResponse(HTTP::OK, true, "Password reset successfully", result.dump());
        }

        return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "User not found");
    } catch (const nlohmann::json::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleGetUserHistory(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user ID");
    }

    try {
        if (!database_) return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database");

        int userId = std::stoi(idIt->second);
        int page = 1, limit = 20;

        // Parse pagination from params (may be empty for path params only)
        auto pageIt = params.find("page");
        if (pageIt != params.end()) page = std::stoi(pageIt->second);
        auto limitIt = params.find("limit");
        if (limitIt != params.end()) limit = std::stoi(limitIt->second);

        int offset = (page - 1) * limit;

        // Query login history
        PreparedStatement stmt(database_, "SELECT * FROM login_history WHERE user_id = ? ORDER BY login_time DESC LIMIT ? OFFSET ?");
        stmt.bind(0, userId);
        stmt.bind(1, limit);
        stmt.bind(2, offset);
        auto results = stmt.query();

        // Count total
        PreparedStatement countStmt(database_, "SELECT COUNT(*) as total FROM login_history WHERE user_id = ?");
        countStmt.bind(0, userId);
        auto countResults = countStmt.query();
        int total = 0;
        if (!countResults.empty()) {
            total = std::stoi(StringUtil::cleanDbString(countResults[0]["total"]).empty() ? "0" : countResults[0]["total"]);
        }

        // Build JSON array
        std::ostringstream itemsJson;
        itemsJson << "[";
        for (size_t i = 0; i < results.size(); i++) {
            if (i > 0) itemsJson << ",";
            const auto& row = results[i];
            itemsJson << "{";
            itemsJson << "\"id\":" << StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0") << ",";
            itemsJson << "\"user_id\":" << userId << ",";
            itemsJson << "\"login_time\":\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("login_time") ? row.at("login_time") : "")) << "\",";
            itemsJson << "\"ip_address\":\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("ip_address") ? row.at("ip_address") : "")) << "\",";
            itemsJson << "\"user_agent\":\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("user_agent") ? row.at("user_agent") : "")) << "\",";
            std::string successVal = StringUtil::cleanDbString(row.count("success") ? row.at("success") : "0");
            itemsJson << "\"success\":" << (successVal == "1" || successVal == "true" ? "true" : "false");
            itemsJson << "}";
        }
        itemsJson << "]";

        int totalPages = (total + limit - 1) / limit;
        if (totalPages < 1) totalPages = 1;

        std::ostringstream data;
        data << "{\"items\":" << itemsJson.str() << ","
             << "\"total\":" << total << ","
             << "\"page\":" << page << ","
             << "\"limit\":" << limit << ","
             << "\"total_pages\":" << totalPages << "}";

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Login history retrieved", data.str());
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleGetUserSessions(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user ID");
    }

    try {
        if (!database_) return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database");

        int userId = std::stoi(idIt->second);

        // Query active sessions (not expired)
        PreparedStatement stmt(database_, "SELECT * FROM user_sessions WHERE user_id = ? AND expires_at > NOW() ORDER BY created_at DESC");
        stmt.bind(0, userId);
        auto results = stmt.query();

        // Build JSON array
        std::ostringstream itemsJson;
        itemsJson << "[";
        for (size_t i = 0; i < results.size(); i++) {
            if (i > 0) itemsJson << ",";
            const auto& row = results[i];
            itemsJson << "{";
            itemsJson << "\"id\":" << StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0") << ",";
            itemsJson << "\"user_id\":" << userId << ",";
            itemsJson << "\"token\":\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("token") ? row.at("token") : "")) << "\",";
            itemsJson << "\"ip_address\":\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("ip_address") ? row.at("ip_address") : "")) << "\",";
            itemsJson << "\"user_agent\":\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("user_agent") ? row.at("user_agent") : "")) << "\",";
            itemsJson << "\"created_at\":\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "")) << "\",";
            itemsJson << "\"expires_at\":\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("expires_at") ? row.at("expires_at") : "")) << "\"";
            itemsJson << "}";
        }
        itemsJson << "]";

        std::ostringstream data;
        data << "{\"sessions\":" << itemsJson.str() << ","
             << "\"total\":" << results.size() << "}";

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Active sessions retrieved", data.str());
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleKickUserSession(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    auto sidIt = params.find("sid");
    if (idIt == params.end() || sidIt == params.end()) {
        return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user ID or session ID");
    }

    try {
        if (!database_) return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database");

        int userId = std::stoi(idIt->second);
        int sessionId = std::stoi(sidIt->second);

        PreparedStatement stmt(database_, "DELETE FROM user_sessions WHERE id = ? AND user_id = ?");
        stmt.bind(0, sessionId);
        stmt.bind(1, userId);

        if (stmt.execute()) {
            addAuditLog("session_kicked", "user", userId, "admin", 0,
                        "Kicked session " + std::to_string(sessionId) + " for user " + std::to_string(userId), "127.0.0.1");
            return StringUtil::buildJsonResponse(true, "Session kicked successfully");
        }

        return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "Session not found");
    } catch (const std::exception& e) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, std::string("Error: ") + e.what());
    }
}

std::string AdminUserManagementModule::handleExportUsers(const std::map<std::string, std::string>& params) {
    try {
        if (!database_) {
            return "id,username,email,full_name,role,is_active,created_at,last_login_at,last_login_ip\n";
        }

        std::string search;
        auto searchIt = params.find("search");
        if (searchIt != params.end()) search = searchIt->second;

        // Build query with optional search filter using PreparedStatement
        std::vector<std::map<std::string, std::string>> results;
        if (!search.empty()) {
            PreparedStatement stmt(database_, "SELECT * FROM users WHERE (username LIKE ? OR email LIKE ?) ORDER BY id ASC");
            stmt.bind(0, std::string("%" + search + "%"));
            stmt.bind(1, std::string("%" + search + "%"));
            results = stmt.query();
        } else {
            PreparedStatement stmt(database_, "SELECT * FROM users ORDER BY id ASC");
            results = stmt.query();
        }

        // Build CSV string
        std::ostringstream csv;
        csv << "id,username,email,full_name,role,is_active,created_at,last_login_at,last_login_ip\n";

        for (const auto& row : results) {
            csv << StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0") << ",";
            csv << "\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("username") ? row.at("username") : "")) << "\",";
            csv << "\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("email") ? row.at("email") : "")) << "\",";
            csv << "\"" << StringUtil::escapeJson(StringUtil::cleanDbString(row.count("full_name") ? row.at("full_name") : "")) << "\",";
            csv << StringUtil::cleanDbString(row.count("role") ? row.at("role") : "user") << ",";
            csv << StringUtil::cleanDbString(row.count("is_active") ? row.at("is_active") : "0") << ",";
            csv << "\"" << StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "") << "\",";
            csv << "\"" << StringUtil::cleanDbString(row.count("last_login_at") ? row.at("last_login_at") : "") << "\",";
            csv << "\"" << StringUtil::cleanDbString(row.count("last_login_ip") ? row.at("last_login_ip") : "") << "\"";
            csv << "\n";
        }

        addAuditLog("users_exported", "user", 0, "admin", 0,
                    "Exported " + std::to_string(results.size()) + " users", "127.0.0.1");

        return csv.str();
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to export users: {}", e.what());
        return "id,username,email,full_name,role,is_active,created_at,last_login_at,last_login_ip\n";
    }
}

std::string AdminUserManagementModule::handleGetLoginHistory(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        int page = 1;
        int limit = 20;
        std::string usernameFilter;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        auto usernameIt = params.find("username");
        if (usernameIt != params.end()) {
            usernameFilter = usernameIt->second;
        }

        int offset = (page - 1) * limit;

        nlohmann::json attempts = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 构建WHERE条件 using PreparedStatement
            std::string whereClause;
            if (!usernameFilter.empty()) {
                whereClause = " WHERE username = ?";
            }

            // 获取总数
            PreparedStatement countStmt(database_, std::string("SELECT COUNT(*) as total FROM login_attempts") + whereClause);
            if (!usernameFilter.empty()) countStmt.bind(0, usernameFilter);
            auto countResults = countStmt.query();
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));
            }

            // 获取登录历史
            PreparedStatement stmt(database_, std::string("SELECT * FROM login_attempts") + whereClause +
                             " ORDER BY created_at DESC LIMIT ? OFFSET ?");
            int bindIdx = 0;
            if (!usernameFilter.empty()) stmt.bind(bindIdx++, usernameFilter);
            stmt.bind(bindIdx++, limit);
            stmt.bind(bindIdx, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json attempt;
                attempt["id"] = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
                attempt["username"] = StringUtil::cleanDbString(row.count("username") ? row.at("username") : "");
                attempt["ip_address"] = StringUtil::cleanDbString(row.count("ip_address") ? row.at("ip_address") : "");
                attempt["user_agent"] = StringUtil::cleanDbString(row.count("user_agent") ? row.at("user_agent") : "");
                attempt["success"] = StringUtil::cleanDbString(row.count("success") ? row.at("success") : "0") == "1";
                attempt["failure_reason"] = StringUtil::cleanDbString(row.count("failure_reason") ? row.at("failure_reason") : "");
                attempt["created_at"] = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                attempts.push_back(attempt);
            }
        }

        nlohmann::json data;
        data["attempts"] = attempts;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Login history retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get login history: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve login history: " + std::string(e.what()));
    }
}

std::string AdminUserManagementModule::handleGetLoginStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        int days = 30;
        auto daysIt = params.find("days");
        if (daysIt != params.end()) {
            days = std::stoi(daysIt->second);
        }

        nlohmann::json stats = nlohmann::json::array();

        if (database_) {
            PreparedStatement stmt(database_, "SELECT * FROM v_login_stats LIMIT ?");
            stmt.bind(0, days);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json stat;
                stat["date"] = StringUtil::cleanDbString(row.count("date") ? row.at("date") : "");
                stat["successful_logins"] = std::stoi(StringUtil::cleanDbString(row.count("successful_logins") ? row.at("successful_logins") : "0"));
                stat["failed_logins"] = std::stoi(StringUtil::cleanDbString(row.count("failed_logins") ? row.at("failed_logins") : "0"));
                stat["unique_users"] = std::stoi(StringUtil::cleanDbString(row.count("unique_users") ? row.at("unique_users") : "0"));
                stat["unique_ips"] = std::stoi(StringUtil::cleanDbString(row.count("unique_ips") ? row.at("unique_ips") : "0"));
                stats.push_back(stat);
            }
        }

        nlohmann::json data;
        data["stats"] = stats;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Login statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get login stats: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve login statistics: " + std::string(e.what()));
    }
}

std::string AdminUserManagementModule::handleGetSuspiciousLogins(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        int page = 1;
        int limit = 20;
        std::string statusFilter;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        auto statusIt = params.find("status");
        if (statusIt != params.end()) {
            statusFilter = statusIt->second;
        }

        int offset = (page - 1) * limit;

        nlohmann::json suspicious = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 构建WHERE条件 using PreparedStatement
            std::string whereClause;
            if (!statusFilter.empty()) {
                whereClause = " WHERE s.status = ?";
            }

            // 获取总数
            PreparedStatement countStmt(database_, std::string("SELECT COUNT(*) as total FROM suspicious_logins s") + whereClause);
            if (!statusFilter.empty()) countStmt.bind(0, statusFilter);
            auto countResults = countStmt.query();
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));
            }

            // 获取可疑登录列表
            PreparedStatement stmt(database_, std::string("SELECT s.*, u.username as reviewed_by_username FROM suspicious_logins s "
                             "LEFT JOIN users u ON s.reviewed_by = u.id") +
                             whereClause +
                             " ORDER BY s.created_at DESC LIMIT ? OFFSET ?");
            int bindIdx = 0;
            if (!statusFilter.empty()) stmt.bind(bindIdx++, statusFilter);
            stmt.bind(bindIdx++, limit);
            stmt.bind(bindIdx, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
                item["username"] = StringUtil::cleanDbString(row.count("username") ? row.at("username") : "");
                item["ip_address"] = StringUtil::cleanDbString(row.count("ip_address") ? row.at("ip_address") : "");
                item["suspicion_reason"] = StringUtil::cleanDbString(row.count("suspicion_reason") ? row.at("suspicion_reason") : "");
                item["risk_score"] = std::stoi(StringUtil::cleanDbString(row.count("risk_score") ? row.at("risk_score") : "0"));
                item["status"] = StringUtil::cleanDbString(row.count("status") ? row.at("status") : "pending");
                item["reviewed_by"] = StringUtil::cleanDbString(row.count("reviewed_by_username") ? row.at("reviewed_by_username") : "");
                item["reviewed_at"] = StringUtil::cleanDbString(row.count("reviewed_at") ? row.at("reviewed_at") : "");
                item["created_at"] = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                suspicious.push_back(item);
            }
        }

        nlohmann::json data;
        data["suspicious"] = suspicious;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Suspicious logins retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get suspicious logins: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve suspicious logins: " + std::string(e.what()));
    }
}

std::string AdminUserManagementModule::handleGetIpBlacklist(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        int page = 1;
        int limit = 20;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        int offset = (page - 1) * limit;

        nlohmann::json blacklist = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 获取总数
            std::string countSql = "SELECT COUNT(*) as total FROM ip_blacklist WHERE is_active = 1";
            auto countResults = database_->query(countSql);
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));
            }

            // 获取黑名单列表
            PreparedStatement stmt(database_, "SELECT b.*, u.username as created_by_username FROM ip_blacklist b "
                             "LEFT JOIN users u ON b.created_by = u.id "
                             "WHERE b.is_active = 1 "
                             "ORDER BY b.created_at DESC LIMIT ? OFFSET ?");
            stmt.bind(0, limit);
            stmt.bind(1, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json entry;
                entry["id"] = std::stoi(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
                entry["ip_address"] = StringUtil::cleanDbString(row.count("ip_address") ? row.at("ip_address") : "");
                entry["reason"] = StringUtil::cleanDbString(row.count("reason") ? row.at("reason") : "");
                entry["threat_level"] = StringUtil::cleanDbString(row.count("threat_level") ? row.at("threat_level") : "medium");
                entry["attempt_count"] = std::stoi(StringUtil::cleanDbString(row.count("attempt_count") ? row.at("attempt_count") : "0"));
                entry["created_by"] = StringUtil::cleanDbString(row.count("created_by_username") ? row.at("created_by_username") : "");
                entry["created_at"] = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                entry["expires_at"] = StringUtil::cleanDbString(row.count("expires_at") ? row.at("expires_at") : "");
                blacklist.push_back(entry);
            }
        }

        nlohmann::json data;
        data["blacklist"] = blacklist;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "IP blacklist retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get IP blacklist: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve IP blacklist: " + std::string(e.what()));
    }
}

std::string AdminUserManagementModule::handleAddIpBlacklist(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string ipAddress = jsonBody.value("ip_address", "");
        std::string reason = ValidationHelper::sanitize(jsonBody.value("reason", ""));
        std::string threatLevel = jsonBody.value("threat_level", "medium");
        std::string expiresAt = jsonBody.value("expires_at", "");

        if (ipAddress.empty() || reason.empty()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "IP address and reason are required");
        }

        if (database_) {
            // 检查是否已存在
            PreparedStatement checkStmt(database_, "SELECT id FROM ip_blacklist WHERE ip_address = ?");
            checkStmt.bind(0, ipAddress);
            auto checkResults = checkStmt.query();
            if (!checkResults.empty()) {
                // 更新现有记录
                std::string updateSqlStr = expiresAt.empty()
                    ? "UPDATE ip_blacklist SET is_active = 1, reason = ?, threat_level = ?, expires_at = NULL WHERE ip_address = ?"
                    : "UPDATE ip_blacklist SET is_active = 1, reason = ?, threat_level = ?, expires_at = ? WHERE ip_address = ?";
                PreparedStatement updateStmt(database_, updateSqlStr);
                updateStmt.bind(0, reason);
                updateStmt.bind(1, threatLevel);
                if (expiresAt.empty()) {
                    updateStmt.bind(2, ipAddress);
                } else {
                    updateStmt.bind(2, expiresAt);
                    updateStmt.bind(3, ipAddress);
                }
                updateStmt.execute();
            } else {
                // 插入新记录
                std::string insertSqlStr = expiresAt.empty()
                    ? "INSERT INTO ip_blacklist (ip_address, reason, threat_level, created_by, expires_at) VALUES (?, ?, ?, 1, NULL)"
                    : "INSERT INTO ip_blacklist (ip_address, reason, threat_level, created_by, expires_at) VALUES (?, ?, ?, 1, ?)";
                PreparedStatement insertStmt(database_, insertSqlStr);
                insertStmt.bind(0, ipAddress);
                insertStmt.bind(1, reason);
                insertStmt.bind(2, threatLevel);
                if (!expiresAt.empty()) {
                    insertStmt.bind(3, expiresAt);
                }
                insertStmt.execute();
            }

            addAuditLog("ip_blacklisted", "ip_blacklist", 0, "superadmin", 0,
                       "Added IP " + ipAddress + " to blacklist: " + reason, "127.0.0.1");

            return StringUtil::buildJsonResponse(true, "IP address added to blacklist");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to add IP to blacklist: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to add IP to blacklist: " + std::string(e.what()));
    }
}

std::string AdminUserManagementModule::handleRemoveIpBlacklist(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing ID parameter");
        }

        int id = std::stoi(idIt->second);

        if (database_) {
            // 软删除：设置为inactive
            PreparedStatement stmt(database_, "UPDATE ip_blacklist SET is_active = 0 WHERE id = ?");
            stmt.bind(0, id);
            stmt.execute();

            addAuditLog("ip_whitelisted", "ip_blacklist", id, "superadmin", 0,
                       "Removed IP from blacklist (ID: " + std::to_string(id) + ")", "127.0.0.1");

            return StringUtil::buildJsonResponse(true, "IP address removed from blacklist");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to remove IP from blacklist: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to remove IP from blacklist: " + std::string(e.what()));
    }
}

std::string AdminUserManagementModule::handleGetAccountLockouts(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        int page = 1;
        int limit = 20;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            page = std::stoi(pageIt->second);
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            limit = std::stoi(limitIt->second);
        }

        int offset = (page - 1) * limit;

        nlohmann::json lockouts = nlohmann::json::array();
        int total = 0;

        if (database_) {
            // 获取总数（仅未过期的）
            std::string countSql = "SELECT COUNT(*) as total FROM account_lockouts WHERE locked_until > NOW()";
            auto countResults = database_->query(countSql);
            if (!countResults.empty() && countResults[0].count("total")) {
                total = std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));
            }

            // 获取锁定列表
            PreparedStatement stmt(database_, "SELECT l.*, u.username FROM account_lockouts l "
                             "JOIN users u ON l.user_id = u.id "
                             "WHERE l.locked_until > NOW() "
                             "ORDER BY l.created_at DESC LIMIT ? OFFSET ?");
            stmt.bind(0, limit);
            stmt.bind(1, offset);
            auto results = stmt.query();

            for (const auto& row : results) {
                nlohmann::json lockout;
                lockout["id"] = std::stoi(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
                lockout["user_id"] = std::stoi(StringUtil::cleanDbString(row.count("user_id") ? row.at("user_id") : "0"));
                lockout["username"] = StringUtil::cleanDbString(row.count("username") ? row.at("username") : "");
                lockout["locked_until"] = StringUtil::cleanDbString(row.count("locked_until") ? row.at("locked_until") : "");
                lockout["lockout_reason"] = StringUtil::cleanDbString(row.count("lockout_reason") ? row.at("lockout_reason") : "");
                lockout["failed_attempts"] = std::stoi(StringUtil::cleanDbString(row.count("failed_attempts") ? row.at("failed_attempts") : "0"));
                lockout["ip_address"] = StringUtil::cleanDbString(row.count("ip_address") ? row.at("ip_address") : "");
                lockout["created_at"] = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
                lockouts.push_back(lockout);
            }
        }

        nlohmann::json data;
        data["lockouts"] = lockouts;
        data["total"] = total;
        data["page"] = page;
        data["limit"] = limit;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Account lockouts retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to get account lockouts: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve account lockouts: " + std::string(e.what()));
    }
}

std::string AdminUserManagementModule::handleLockUserAccount(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        int userId = jsonBody.value("user_id", 0);
        int lockMinutes = jsonBody.value("lock_minutes", 30);
        std::string reason = ValidationHelper::sanitize(jsonBody.value("reason", "Admin action"));
        std::string ipAddress = jsonBody.value("ip_address", "");

        if (userId == 0) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "User ID is required");
        }

        if (database_) {
            // 检查用户是否存在
            PreparedStatement checkStmt(database_, "SELECT username FROM users WHERE id = ?");
            checkStmt.bind(0, userId);
            auto checkResults = checkStmt.query();
            if (checkResults.empty()) {
                return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "User not found");
            }

            // 插入或更新锁定记录
            PreparedStatement stmt(database_, "INSERT INTO account_lockouts (user_id, locked_until, lockout_reason, ip_address) "
                             "VALUES (?, DATE_ADD(NOW(), INTERVAL ? MINUTE), ?, ?) "
                             "ON DUPLICATE KEY UPDATE "
                             "locked_until = DATE_ADD(NOW(), INTERVAL ? MINUTE), "
                             "lockout_reason = ?, "
                             "ip_address = ?");
            stmt.bind(0, userId);
            stmt.bind(1, lockMinutes);
            stmt.bind(2, reason);
            stmt.bind(3, ipAddress);
            stmt.bind(4, lockMinutes);
            stmt.bind(5, reason);
            stmt.bind(6, ipAddress);
            stmt.execute();

            addAuditLog("user_locked", "users", userId, "superadmin", 0,
                       "Locked user account for " + std::to_string(lockMinutes) + " minutes: " + reason, "127.0.0.1");

            return StringUtil::buildJsonResponse(true, "User account locked successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to lock user account: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to lock user account: " + std::string(e.what()));
    }
}

std::string AdminUserManagementModule::handleUnlockUserAccount(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto userIdIt = params.find("user_id");
        if (userIdIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing user_id parameter");
        }

        int userId = std::stoi(userIdIt->second);

        if (database_) {
            // 删除锁定记录
            PreparedStatement stmt(database_, "DELETE FROM account_lockouts WHERE user_id = ?");
            stmt.bind(0, userId);
            stmt.execute();

            addAuditLog("user_unlocked", "users", userId, "superadmin", 0,
                       "Unlocked user account", "127.0.0.1");

            return StringUtil::buildJsonResponse(true, "User account unlocked successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to unlock user account: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to unlock user account: " + std::string(e.what()));
    }
}

std::string AdminUserManagementModule::handleHandleSuspiciousLogin(const std::map<std::string, std::string>& params, const std::string& body,
                                                       const std::map<std::string, std::string>& headers) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Missing ID parameter");
        }

        int id = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::string action = jsonBody.value("action", "");  // reviewed, whitelisted, confirmed_threat
        int reviewedBy = impl_->extractAdminUserIdFromHeaders(headers);

        if (action.empty()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Action is required");
        }

        if (database_) {
            // 更新可疑登录记录状态
            PreparedStatement stmt(database_, "UPDATE suspicious_logins SET status = ?, "
                             "reviewed_by = ?, "
                             "reviewed_at = NOW() "
                             "WHERE id = ?");
            stmt.bind(0, action);
            stmt.bind(1, reviewedBy);
            stmt.bind(2, id);
            stmt.execute();

            addAuditLog("suspicious_login_handled", "suspicious_logins", id, "superadmin", reviewedBy,
                       "Marked suspicious login as: " + action, "127.0.0.1");

            return StringUtil::buildJsonResponse(true, "Suspicious login handled successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "No database connection available");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminApiModule] Failed to handle suspicious login: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to handle suspicious login: " + std::string(e.what()));
    }
}

// ============================================================================
// 全局配置功能实现
// ============================================================================

// ============================================================================
// AdminUserManagementModule - Helper methods
// ============================================================================

void AdminUserManagementModule::addAuditLog(const std::string& action, const std::string& entityType, int entityId,
                                             const std::string& actorUsername, int actorId,
                                             const std::string& details, const std::string& ipAddress) {
    if (!database_) {
        spdlog::debug("[AdminUserManagement] Cannot add audit log: no database");
        return;
    }
    try {
        PreparedStatement stmt(database_,
            "INSERT INTO audit_logs (action, entity_type, entity_id, actor_username, actor_id, details, ip_address, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, NOW())");
        stmt.bind(0, action);
        stmt.bind(1, entityType);
        stmt.bind(2, entityId);
        stmt.bind(3, actorUsername);
        stmt.bind(4, actorId);
        stmt.bind(5, details);
        stmt.bind(6, ipAddress);
        stmt.execute();
        spdlog::debug("[AdminUserManagement] Audit log: {} {} by {}", action, entityType, actorUsername);
    } catch (const std::exception& e) {
        spdlog::error("[AdminUserManagement] Failed to add audit log: {}", e.what());
    }
}

// ============================================================================
// DLL export functions
// ============================================================================

#define EXPORT __attribute__((visibility("default")))

extern "C" {
EXPORT void* createModule() {
    return new PaperCrawler::AdminUserManagementModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::AdminUserManagementModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}
}

} // namespace PaperCrawler
