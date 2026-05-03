#include "business/AdminAuditModule.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include "features/security/SecurityModule.hpp"
#include "data/PreparedStatement.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <algorithm>
#include <regex>
#include <chrono>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include "data/ValidationHelper.hpp"
#include "data/StringUtil.hpp"
#include "core/HttpStatus.hpp"

namespace PaperCrawler {

class AdminAuditModule::Impl {
public:
    std::shared_ptr<IDatabase> database_;
    std::vector<AuditLog> auditLogs_;
    int nextAuditId_{1};

    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {}
};

// ============================================================================
// AdminAuditModule - Constructor and destructor
// ============================================================================

AdminAuditModule::AdminAuditModule()
    : impl_(std::make_unique<Impl>(nullptr)), database_(nullptr) {
    spdlog::info("[AdminAudit] Default constructor called");
}

AdminAuditModule::~AdminAuditModule() {
    spdlog::info("[AdminAudit] Destructor called");
}

void AdminAuditModule::setDatabase(std::shared_ptr<IDatabase> database) {
    spdlog::info("[AdminAudit] Received injected database connection");
    BusinessModuleBase::setDatabase(database);
    database_ = database;
    if (impl_) {
        impl_->database_ = database;
    }
}

void AdminAuditModule::addAuditLog(const std::string& action, const std::string& entityType, int entityId,
                                  const std::string& actorUsername, int actorId,
                                  const std::string& details, const std::string& ipAddress) {
    if (!database_) {
        spdlog::debug("[AdminAudit] Cannot add audit log: no database");
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
        spdlog::debug("[AdminAudit] Audit log: {} {} by {}", action, entityType, actorUsername);
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to add audit log: {}", e.what());
    }
}

void AdminAuditModule::registerRoutes() {
    auto& router = Router::getInstance();
    const std::string prefix = "/api/admin";

    database_ = getDatabase();
    if (database_) {
        spdlog::info("[AdminAudit] Received injected database connection");
        if (impl_) impl_->database_ = database_;
    } else {
        spdlog::warn("[AdminAudit] No injected database connection available");
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
        resp.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        resp.body = R"({"success":false,"error":"Unauthorized. Admin authentication required."})";
        return resp;
    };

    // Audit logs
    router.get(prefix + "/audit-logs", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetAuditLogs(req.queryParams);
        return response;
    });

    // RBAC - Roles
    router.get(prefix + "/roles", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetRoles(req.queryParams);
        return response;
    });

    router.post(prefix + "/roles", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleCreateRole(req.queryParams, req.body);
        return response;
    });

    router.put(prefix + "/roles/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleUpdateRole(req.pathParams, req.body);
        return response;
    });

    router.del(prefix + "/roles/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleDeleteRole(req.pathParams);
        return response;
    });

    // RBAC - Permissions
    router.get(prefix + "/permissions", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetPermissions(req.queryParams);
        return response;
    });

    router.get(prefix + "/permission-matrix", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetPermissionMatrix(req.queryParams);
        return response;
    });

    router.get(prefix + "/roles/:id/permissions", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetRolePermissions(req.pathParams);
        return response;
    });

    router.put(prefix + "/roles/:id/permissions", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleUpdateRolePermissions(req.pathParams, req.body);
        return response;
    });

    router.get(prefix + "/users/:id/roles", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetUserRoles(req.pathParams);
        return response;
    });

    router.post(prefix + "/users/:id/roles", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleAssignUserRole(req.pathParams, req.body);
        return response;
    });

    router.del(prefix + "/users/:id/roles/:roleid", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleRemoveUserRole(req.pathParams);
        return response;
    });

    router.post(prefix + "/permissions/check", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleCheckPermission(req.queryParams, req.body);
        return response;
    });

    // Content moderation
    router.get(prefix + "/content/pending", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetPendingPapers(req.queryParams);
        return response;
    });

    router.get(prefix + "/content/pending/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetPaperModeration(req.pathParams);
        return response;
    });

    router.post(prefix + "/content/pending/:id/approve", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleApprovePaper(req.pathParams, req.body);
        return response;
    });

    router.post(prefix + "/content/pending/:id/reject", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleRejectPaper(req.pathParams, req.body);
        return response;
    });

    router.get(prefix + "/content/reports", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetUserReports(req.queryParams);
        return response;
    });

    router.post(prefix + "/content/reports/:id/resolve", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleResolveReport(req.pathParams, req.body);
        return response;
    });

    router.get(prefix + "/content/sensitive-words", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetSensitiveWords(req.queryParams);
        return response;
    });

    router.post(prefix + "/content/sensitive-words", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleCreateSensitiveWord(req.queryParams, req.body);
        return response;
    });

    router.del(prefix + "/content/sensitive-words/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleDeleteSensitiveWord(req.pathParams);
        return response;
    });

    router.post(prefix + "/content/sensitive-words/check", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleCheckSensitiveWords(req.queryParams, req.body);
        return response;
    });

    router.get(prefix + "/content/sensitive-words/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetSensitiveWordStats(req.queryParams);
        return response;
    });

    // API key management
    router.get(prefix + "/api-keys", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetApiKeys(req.queryParams);
        return response;
    });

    router.post(prefix + "/api-keys", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleCreateApiKey(req.queryParams, req.body);
        return response;
    });

    router.del(prefix + "/api-keys/:id", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleDeleteApiKey(req.pathParams);
        return response;
    });

    router.post(prefix + "/api-keys/:id/regenerate", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleRegenerateApiKey(req.pathParams, req.body);
        return response;
    });

    router.get(prefix + "/api-keys/usage", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetApiKeyUsage(req.queryParams);
        return response;
    });

    router.get(prefix + "/api-keys/stats", [this, requireAdminAuth, unauthorizedResp](const HttpRequest& req) -> HttpResponse {
        if (!requireAdminAuth(req)) return unauthorizedResp();
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.setHeader("Content-Type", HTTP::CONTENT_TYPE_JSON);
        response.body = handleGetApiKeyStats(req.queryParams);
        return response;
    });

    spdlog::info("[AdminAudit] Routes registered successfully");
}


// ============================================================================
// AdminAuditModule - Business logic methods
// ============================================================================

PaginatedResponse<AuditLog> AdminAuditModule::getAuditLogs(int page, int limit, const std::string& action, int userId) {
    PaginatedResponse<AuditLog> response;
    response.page = page;
    response.limit = limit;

    if (!database_) return response;

    try {
        std::string sql = "SELECT * FROM audit_logs WHERE 1=1";
        if (!action.empty()) sql += " AND action = ?";
        if (userId > 0) sql += " AND actor_id = ?";
        sql += " ORDER BY created_at DESC";

        auto allResults = database_->query(sql);

        // Filter in-memory if prepared statement binding not available for dynamic query
        std::vector<std::map<std::string, std::string>> filtered;
        for (const auto& row : allResults) {
            if (!action.empty() && StringUtil::cleanDbString(row.count("action") ? row.at("action") : "") != action) continue;
            if (userId > 0 && std::stoi(StringUtil::cleanDbString(row.count("actor_id") ? row.at("actor_id") : "0")) != userId) continue;
            filtered.push_back(row);
        }

        response.total = filtered.size();
        response.totalPages = (response.total + limit - 1) / limit;

        int start = (page - 1) * limit;
        int end = std::min(start + limit, (int)filtered.size());
        for (int i = start; i < end; i++) {
            AuditLog log;
            log.id = std::stoi(StringUtil::cleanDbString(filtered[i].count("id") ? filtered[i].at("id") : "0"));
            log.action = StringUtil::cleanDbString(filtered[i].count("action") ? filtered[i].at("action") : "");
            log.entityType = StringUtil::cleanDbString(filtered[i].count("entity_type") ? filtered[i].at("entity_type") : "");
            log.entityId = std::stoi(StringUtil::cleanDbString(filtered[i].count("entity_id") ? filtered[i].at("entity_id") : "0"));
            log.actorUsername = StringUtil::cleanDbString(filtered[i].count("actor_username") ? filtered[i].at("actor_username") : "");
            log.actorId = std::stoi(StringUtil::cleanDbString(filtered[i].count("actor_id") ? filtered[i].at("actor_id") : "0"));
            log.details = StringUtil::cleanDbString(filtered[i].count("details") ? filtered[i].at("details") : "");
            log.ipAddress = StringUtil::cleanDbString(filtered[i].count("ip_address") ? filtered[i].at("ip_address") : "");
            response.items.push_back(log);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get audit logs: {}", e.what());
    }

    return response;
}

std::vector<Role> AdminAuditModule::getRoles() {
    std::vector<Role> roles;
    if (!database_) return roles;
    try {
        auto results = database_->query("SELECT * FROM roles ORDER BY level DESC");
        for (const auto& row : results) {
            Role role;
            role.id = std::stoi(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            role.name = StringUtil::cleanDbString(row.count("name") ? row.at("name") : "");
            role.displayName = StringUtil::cleanDbString(row.count("display_name") ? row.at("display_name") : "");
            role.description = StringUtil::cleanDbString(row.count("description") ? row.at("description") : "");
            role.level = std::stoi(StringUtil::cleanDbString(row.count("level") ? row.at("level") : "0"));
            role.isSystem = StringUtil::cleanDbString(row.count("is_system") ? row.at("is_system") : "0") == "1";
            role.isDefault = StringUtil::cleanDbString(row.count("is_default") ? row.at("is_default") : "0") == "1";
            role.createdAt = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            role.updatedAt = StringUtil::cleanDbString(row.count("updated_at") ? row.at("updated_at") : "");
            roles.push_back(role);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get roles: {}", e.what());
    }
    return roles;
}

int AdminAuditModule::createRole(const std::string& name, const std::string& displayName, const std::string& description, int level, int createdBy) {
    if (!database_) return 0;
    try {
        PreparedStatement stmt(database_, "INSERT INTO roles (name, display_name, description, level, is_system, is_default, created_by) VALUES (?, ?, ?, ?, 0, 0, ?)");
        stmt.bind(0, name);
        stmt.bind(1, displayName);
        stmt.bind(2, description);
        stmt.bind(3, level);
        stmt.bind(4, createdBy);
        stmt.execute();
        auto lastId = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastId.empty() && lastId[0].count("id")) return std::stoi(StringUtil::cleanDbString(lastId[0].at("id")));
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to create role: {}", e.what());
    }
    return 0;
}

bool AdminAuditModule::updateRole(int roleId, const std::string& displayName, const std::string& description, int level) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "UPDATE roles SET display_name = ?, description = ?, level = ? WHERE id = ? AND is_system = 0");
        stmt.bind(0, displayName);
        stmt.bind(1, description);
        stmt.bind(2, level);
        stmt.bind(3, roleId);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to update role: {}", e.what());
        return false;
    }
}

bool AdminAuditModule::deleteRole(int roleId) {
    if (!database_) return false;
    try {
        PreparedStatement checkStmt(database_, "SELECT is_system FROM roles WHERE id = ?");
        checkStmt.bind(0, roleId);
        auto results = checkStmt.query();
        if (!results.empty()) {
            bool isSystem = StringUtil::cleanDbString(results[0].count("is_system") ? results[0].at("is_system") : "0") == "1";
            if (isSystem) return false;
        }
        PreparedStatement delStmt(database_, "DELETE FROM roles WHERE id = ?");
        delStmt.bind(0, roleId);
        delStmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to delete role: {}", e.what());
        return false;
    }
}

std::vector<Permission> AdminAuditModule::getPermissions() {
    std::vector<Permission> permissions;
    if (!database_) return permissions;
    try {
        auto results = database_->query("SELECT * FROM permissions ORDER BY resource, action");
        for (const auto& row : results) {
            Permission perm;
            perm.id = std::stoi(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            perm.resource = StringUtil::cleanDbString(row.count("resource") ? row.at("resource") : "");
            perm.action = StringUtil::cleanDbString(row.count("action") ? row.at("action") : "");
            perm.description = StringUtil::cleanDbString(row.count("description") ? row.at("description") : "");
            permissions.push_back(perm);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get permissions: {}", e.what());
    }
    return permissions;
}

std::vector<PermissionMatrix> AdminAuditModule::getPermissionMatrix() {
    std::vector<PermissionMatrix> matrix;
    if (!database_) return matrix;
    try {
        auto results = database_->query("SELECT * FROM v_permission_matrix ORDER BY role_level DESC, role_name, resource, action");
        std::string currentRole;
        PermissionMatrix currentMatrix;
        for (const auto& row : results) {
            std::string roleName = StringUtil::cleanDbString(row.count("role_name") ? row.at("role_name") : "");
            std::string resource = StringUtil::cleanDbString(row.count("resource") ? row.at("resource") : "");
            if (roleName != currentRole) {
                if (!currentRole.empty()) matrix.push_back(currentMatrix);
                currentMatrix = PermissionMatrix();
                currentMatrix.roleName = roleName;
                currentMatrix.totalPermissions = 0;
                currentRole = roleName;
            }
            currentMatrix.totalPermissions++;
            currentMatrix.permissionsByResource[resource]++;
        }
        if (!currentRole.empty()) matrix.push_back(currentMatrix);
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get permission matrix: {}", e.what());
    }
    return matrix;
}

std::vector<RolePermission> AdminAuditModule::getRolePermissions(int roleId) {
    std::vector<RolePermission> rolePermissions;
    if (!database_) return rolePermissions;
    try {
        PreparedStatement stmt(database_,
            "SELECT rp.*, r.name as role_name, p.resource, p.action, u.username as granted_by_username "
            "FROM role_permissions rp JOIN roles r ON rp.role_id = r.id "
            "JOIN permissions p ON rp.permission_id = p.id "
            "LEFT JOIN users u ON rp.granted_by = u.id WHERE rp.role_id = ?");
        stmt.bind(0, roleId);
        auto results = stmt.query();
        for (const auto& row : results) {
            RolePermission rp;
            rp.roleId = std::stoi(StringUtil::cleanDbString(row.count("role_id") ? row.at("role_id") : "0"));
            rp.roleName = StringUtil::cleanDbString(row.count("role_name") ? row.at("role_name") : "");
            rp.permissionId = std::stoi(StringUtil::cleanDbString(row.count("permission_id") ? row.at("permission_id") : "0"));
            rp.resource = StringUtil::cleanDbString(row.count("resource") ? row.at("resource") : "");
            rp.action = StringUtil::cleanDbString(row.count("action") ? row.at("action") : "");
            rp.grantedAt = StringUtil::cleanDbString(row.count("granted_at") ? row.at("granted_at") : "");
            rp.grantedByUsername = StringUtil::cleanDbString(row.count("granted_by_username") ? row.at("granted_by_username") : "");
            rolePermissions.push_back(rp);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get role permissions: {}", e.what());
    }
    return rolePermissions;
}

bool AdminAuditModule::updateRolePermissions(int roleId, const std::vector<int>& permissionIds, int updatedBy) {
    if (!database_) return false;
    try {
        database_->execute("START TRANSACTION");
        PreparedStatement delStmt(database_, "DELETE FROM role_permissions WHERE role_id = ?");
        delStmt.bind(0, roleId);
        delStmt.execute();
        for (int permId : permissionIds) {
            PreparedStatement insStmt(database_, "INSERT INTO role_permissions (role_id, permission_id, granted_by) VALUES (?, ?, ?)");
            insStmt.bind(0, roleId);
            insStmt.bind(1, permId);
            insStmt.bind(2, updatedBy);
            insStmt.execute();
        }
        database_->execute("COMMIT");
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to update role permissions: {}", e.what());
        database_->execute("ROLLBACK");
        return false;
    }
}

std::vector<UserRoleAssignment> AdminAuditModule::getUserRoles(int userId) {
    std::vector<UserRoleAssignment> userRoles;
    if (!database_) return userRoles;
    try {
        PreparedStatement stmt(database_,
            "SELECT ur.*, u.username, r.name as role_name, r.level as role_level "
            "FROM user_roles ur JOIN users u ON ur.user_id = u.id "
            "JOIN roles r ON ur.role_id = r.id WHERE ur.user_id = ?");
        stmt.bind(0, userId);
        auto results = stmt.query();
        for (const auto& row : results) {
            UserRoleAssignment userRole;
            userRole.id = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            userRole.userId = std::stoi(StringUtil::cleanDbString(row.count("user_id") ? row.at("user_id") : "0"));
            userRole.username = StringUtil::cleanDbString(row.count("username") ? row.at("username") : "");
            userRole.roleId = std::stoi(StringUtil::cleanDbString(row.count("role_id") ? row.at("role_id") : "0"));
            userRole.roleName = StringUtil::cleanDbString(row.count("role_name") ? row.at("role_name") : "");
            userRole.roleLevel = std::stoi(StringUtil::cleanDbString(row.count("role_level") ? row.at("role_level") : "0"));
            userRole.assignedAt = StringUtil::cleanDbString(row.count("assigned_at") ? row.at("assigned_at") : "");
            userRole.expiresAt = StringUtil::cleanDbString(row.count("expires_at") ? row.at("expires_at") : "");
            userRole.reason = StringUtil::cleanDbString(row.count("reason") ? row.at("reason") : "");
            userRoles.push_back(userRole);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get user roles: {}", e.what());
    }
    return userRoles;
}

bool AdminAuditModule::assignUserRole(int userId, int roleId, const std::string& reason, int assignedBy, const std::string& expiresAt) {
    if (!database_) return false;
    try {
        if (!expiresAt.empty()) {
            PreparedStatement stmt(database_, "INSERT INTO user_roles (user_id, role_id, reason, assigned_by, expires_at) VALUES (?, ?, ?, ?, ?)");
            stmt.bind(0, userId); stmt.bind(1, roleId); stmt.bind(2, reason); stmt.bind(3, assignedBy); stmt.bind(4, expiresAt);
            stmt.execute();
        } else {
            PreparedStatement stmt(database_, "INSERT INTO user_roles (user_id, role_id, reason, assigned_by) VALUES (?, ?, ?, ?)");
            stmt.bind(0, userId); stmt.bind(1, roleId); stmt.bind(2, reason); stmt.bind(3, assignedBy);
            stmt.execute();
        }
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to assign user role: {}", e.what());
        return false;
    }
}

bool AdminAuditModule::removeUserRole(int userId, int roleId) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "DELETE FROM user_roles WHERE user_id = ? AND role_id = ?");
        stmt.bind(0, userId);
        stmt.bind(1, roleId);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to remove user role: {}", e.what());
        return false;
    }
}

bool AdminAuditModule::checkUserPermission(int userId, const std::string& resource, const std::string& action) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "SELECT COUNT(*) as count FROM v_user_permissions WHERE user_id = ?");
        stmt.bind(0, userId);
        auto results = stmt.query();
        if (!results.empty()) {
            int count = std::stoi(StringUtil::cleanDbString(results[0].count("count") ? results[0].at("count") : "0"));
            return count > 0;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to check permission: {}", e.what());
    }
    return false;
}

PaginatedResponse<PaperModeration> AdminAuditModule::getPendingPapers(int page, int limit) {
    PaginatedResponse<PaperModeration> response;
    response.page = page; response.limit = limit;
    if (!database_) return response;
    try {
        auto countResults = database_->query("SELECT COUNT(*) as total FROM paper_moderations WHERE status = 'pending'");
        response.total = countResults.empty() ? 0 : std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));
        int offset = (page - 1) * limit;
        PreparedStatement stmt(database_,
            "SELECT pm.*, u.username as moderator_username FROM paper_moderations pm "
            "LEFT JOIN users u ON pm.moderator_id = u.id WHERE pm.status = 'pending' "
            "ORDER BY pm.created_at DESC LIMIT ? OFFSET ?");
        stmt.bind(0, limit);
        stmt.bind(1, offset);
        auto results = stmt.query();
        for (const auto& row : results) {
            PaperModeration m;
            m.id = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            m.paperId = std::stoi(StringUtil::cleanDbString(row.count("paper_id") ? row.at("paper_id") : "0"));
            m.status = StringUtil::cleanDbString(row.count("status") ? row.at("status") : "");
            m.moderatorId = row.count("moderator_id") && row.at("moderator_id") != "NULL" ? std::stoi(StringUtil::cleanDbString(row.at("moderator_id"))) : 0;
            m.moderatorUsername = StringUtil::cleanDbString(row.count("moderator_username") ? row.at("moderator_username") : "");
            m.reason = StringUtil::cleanDbString(row.count("reason") ? row.at("reason") : "");
            m.reviewedAt = StringUtil::cleanDbString(row.count("reviewed_at") ? row.at("reviewed_at") : "");
            m.flags = StringUtil::cleanDbString(row.count("flags") ? row.at("flags") : "{}");
            m.createdAt = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(m);
        }
        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get pending papers: {}", e.what());
    }
    return response;
}

std::optional<PaperModeration> AdminAuditModule::getPaperModeration(int64_t id) {
    if (!database_) return std::nullopt;
    try {
        PreparedStatement stmt(database_,
            "SELECT pm.*, u.username as moderator_username FROM paper_moderations pm "
            "LEFT JOIN users u ON pm.moderator_id = u.id WHERE pm.id = ?");
        stmt.bind(0, static_cast<int>(id));
        auto results = stmt.query();
        if (!results.empty()) {
            PaperModeration m;
            m.id = std::stoll(StringUtil::cleanDbString(results[0].count("id") ? results[0].at("id") : "0"));
            m.paperId = std::stoi(StringUtil::cleanDbString(results[0].count("paper_id") ? results[0].at("paper_id") : "0"));
            m.status = StringUtil::cleanDbString(results[0].count("status") ? results[0].at("status") : "");
            m.moderatorId = results[0].count("moderator_id") && results[0].at("moderator_id") != "NULL" ? std::stoi(StringUtil::cleanDbString(results[0].at("moderator_id"))) : 0;
            m.moderatorUsername = StringUtil::cleanDbString(results[0].count("moderator_username") ? results[0].at("moderator_username") : "");
            m.reason = StringUtil::cleanDbString(results[0].count("reason") ? results[0].at("reason") : "");
            m.reviewedAt = StringUtil::cleanDbString(results[0].count("reviewed_at") ? results[0].at("reviewed_at") : "");
            m.flags = StringUtil::cleanDbString(results[0].count("flags") ? results[0].at("flags") : "{}");
            m.createdAt = StringUtil::cleanDbString(results[0].count("created_at") ? results[0].at("created_at") : "");
            return m;
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get paper moderation: {}", e.what());
    }
    return std::nullopt;
}

bool AdminAuditModule::approvePaper(int paperId, int moderatorId) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_,
            "UPDATE paper_moderations SET status = 'approved', moderator_id = ?, reviewed_at = NOW() "
            "WHERE paper_id = ? AND status = 'pending'");
        stmt.bind(0, moderatorId);
        stmt.bind(1, paperId);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to approve paper: {}", e.what());
        return false;
    }
}

bool AdminAuditModule::rejectPaper(int paperId, int moderatorId, const std::string& reason) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "UPDATE paper_moderations SET status = 'rejected', moderator_id = ?, reason = ?, reviewed_at = NOW() WHERE paper_id = ? AND status = 'pending'");
        stmt.bind(0, moderatorId); stmt.bind(1, reason); stmt.bind(2, paperId);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to reject paper: {}", e.what());
        return false;
    }
}

PaginatedResponse<UserReport> AdminAuditModule::getUserReports(int page, int limit, const std::string& status) {
    PaginatedResponse<UserReport> response;
    response.page = page; response.limit = limit;
    if (!database_) return response;
    try {
        std::string countSql = "SELECT COUNT(*) as total FROM user_reports";
        int countBindIdx = -1;
        if (!status.empty()) countSql += " WHERE status = ?";
        PreparedStatement countStmt(database_, countSql);
        if (!status.empty()) { countBindIdx = 0; countStmt.bind(countBindIdx, status); }
        auto countResults = countStmt.query();
        response.total = countResults.empty() ? 0 : std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));
        int offset = (page - 1) * limit;
        std::string sql = "SELECT ur.*, reporter.username as reporter_username, reviewer.username as reviewer_username "
                         "FROM user_reports ur LEFT JOIN users reporter ON ur.reporter_id = reporter.id "
                         "LEFT JOIN users reviewer ON ur.reviewer_id = reviewer.id";
        int bindIdx = 0;
        if (!status.empty()) { sql += " WHERE ur.status = ?"; }
        sql += " ORDER BY ur.created_at DESC LIMIT ? OFFSET ?";
        PreparedStatement stmt(database_, sql);
        if (!status.empty()) { stmt.bind(bindIdx, status); bindIdx++; }
        stmt.bind(bindIdx, limit); bindIdx++;
        stmt.bind(bindIdx, offset);
        auto results = stmt.query();
        for (const auto& row : results) {
            UserReport report;
            report.id = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            report.reporterId = std::stoi(StringUtil::cleanDbString(row.count("reporter_id") ? row.at("reporter_id") : "0"));
            report.reporterUsername = StringUtil::cleanDbString(row.count("reporter_username") ? row.at("reporter_username") : "");
            report.targetType = StringUtil::cleanDbString(row.count("target_type") ? row.at("target_type") : "");
            report.targetId = std::stoi(StringUtil::cleanDbString(row.count("target_id") ? row.at("target_id") : "0"));
            report.reason = StringUtil::cleanDbString(row.count("reason") ? row.at("reason") : "");
            report.description = StringUtil::cleanDbString(row.count("description") ? row.at("description") : "");
            report.status = StringUtil::cleanDbString(row.count("status") ? row.at("status") : "");
            report.priority = StringUtil::cleanDbString(row.count("priority") ? row.at("priority") : "medium");
            report.reviewerId = row.count("reviewer_id") && row.at("reviewer_id") != "NULL" ? std::stoi(StringUtil::cleanDbString(row.at("reviewer_id"))) : 0;
            report.reviewerUsername = StringUtil::cleanDbString(row.count("reviewer_username") ? row.at("reviewer_username") : "");
            report.resolution = StringUtil::cleanDbString(row.count("resolution") ? row.at("resolution") : "");
            report.createdAt = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(report);
        }
        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get user reports: {}", e.what());
    }
    return response;
}

bool AdminAuditModule::resolveReport(int64_t reportId, int reviewerId, const std::string& resolution, const std::string& status) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "UPDATE user_reports SET status = ?, reviewer_id = ?, resolution = ? WHERE id = ?");
        stmt.bind(0, status); stmt.bind(1, reviewerId); stmt.bind(2, resolution); stmt.bind(3, static_cast<int>(reportId));
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to resolve report: {}", e.what());
        return false;
    }
}

std::vector<SensitiveWord> AdminAuditModule::getSensitiveWords() {
    std::vector<SensitiveWord> words;
    if (!database_) return words;
    try {
        auto results = database_->query("SELECT * FROM sensitive_words WHERE is_active = 1 ORDER BY category, severity DESC");
        for (const auto& row : results) {
            SensitiveWord word;
            word.id = std::stoi(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            word.word = StringUtil::cleanDbString(row.count("word") ? row.at("word") : "");
            word.category = StringUtil::cleanDbString(row.count("category") ? row.at("category") : "");
            word.severity = StringUtil::cleanDbString(row.count("severity") ? row.at("severity") : "medium");
            word.isRegex = StringUtil::cleanDbString(row.count("is_regex") ? row.at("is_regex") : "0") == "1";
            word.replacement = StringUtil::cleanDbString(row.count("replacement") ? row.at("replacement") : "");
            word.isActive = StringUtil::cleanDbString(row.count("is_active") ? row.at("is_active") : "1") == "1";
            word.matchCount = std::stoi(StringUtil::cleanDbString(row.count("match_count") ? row.at("match_count") : "0"));
            word.createdBy = std::stoi(StringUtil::cleanDbString(row.count("created_by") ? row.at("created_by") : "0"));
            word.createdAt = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            words.push_back(word);
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get sensitive words: {}", e.what());
    }
    return words;
}

int AdminAuditModule::createSensitiveWord(const std::string& word, const std::string& category, const std::string& severity, bool isRegex, const std::string& replacement, int createdBy) {
    if (!database_) return 0;
    try {
        PreparedStatement stmt(database_, "INSERT INTO sensitive_words (word, category, severity, is_regex, replacement, created_by) VALUES (?, ?, ?, ?, ?, ?)");
        stmt.bind(0, word); stmt.bind(1, category); stmt.bind(2, severity); stmt.bind(3, isRegex ? 1 : 0); stmt.bind(4, replacement); stmt.bind(5, createdBy);
        stmt.execute();
        auto lastId = database_->query("SELECT LAST_INSERT_ID() as id");
        if (!lastId.empty() && lastId[0].count("id")) return std::stoi(StringUtil::cleanDbString(lastId[0].at("id")));
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to create sensitive word: {}", e.what());
    }
    return 0;
}

bool AdminAuditModule::deleteSensitiveWord(int id) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "DELETE FROM sensitive_words WHERE id = ?");
        stmt.bind(0, id);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to delete sensitive word: {}", e.what());
        return false;
    }
}

std::vector<SensitiveWordMatch> AdminAuditModule::checkSensitiveWords(const std::string& text) {
    std::vector<SensitiveWordMatch> matches;
    if (!database_) return matches;
    try {
        auto words = getSensitiveWords();
        for (const auto& sw : words) {
            std::vector<std::pair<size_t, size_t>> positions;
            if (sw.isRegex) {
                try {
                    std::regex pattern(sw.word, std::regex_constants::icase);
                    std::sregex_iterator it(text.begin(), text.end(), pattern);
                    std::sregex_iterator regex_end;
                    for (; it != regex_end; ++it) positions.push_back(std::make_pair(it->position(), it->position() + it->length()));
                } catch (const std::regex_error&) { continue; }
            } else {
                std::string lowerText = text, lowerWord = sw.word;
                std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
                std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);
                size_t pos = 0;
                while ((pos = lowerText.find(lowerWord, pos)) != std::string::npos) {
                    positions.push_back(std::make_pair(pos, pos + sw.word.length()));
                    pos += sw.word.length();
                }
            }
            for (const auto& pair : positions) {
                SensitiveWordMatch match;
                match.word = sw.word; match.category = sw.category;
                match.startPosition = pair.first; match.endPosition = pair.second;
                match.matchedText = text.substr(pair.first, pair.second - pair.first);
                matches.push_back(match);
                PreparedStatement updateStmt(database_, "UPDATE sensitive_words SET match_count = match_count + 1 WHERE id = ?");
                updateStmt.bind(0, sw.id); updateStmt.execute();
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to check sensitive words: {}", e.what());
    }
    return matches;
}

std::map<std::string, int> AdminAuditModule::getSensitiveWordStats() {
    std::map<std::string, int> stats;
    if (!database_) return stats;
    try {
        auto totalResults = database_->query("SELECT COUNT(*) as total FROM sensitive_words WHERE is_active = 1");
        stats["total_active"] = totalResults.empty() ? 0 : std::stoi(StringUtil::cleanDbString(totalResults[0].at("total")));
        auto catResults = database_->query("SELECT category, COUNT(*) as count FROM sensitive_words WHERE is_active = 1 GROUP BY category");
        for (const auto& row : catResults) stats["category_" + StringUtil::cleanDbString(row.count("category") ? row.at("category") : "")] = std::stoi(StringUtil::cleanDbString(row.count("count") ? row.at("count") : "0"));
        auto matchResults = database_->query("SELECT SUM(match_count) as total FROM sensitive_words WHERE is_active = 1");
        stats["total_matches"] = matchResults.empty() ? 0 : std::stoi(StringUtil::cleanDbString(matchResults[0].at("total")));
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get sensitive word stats: {}", e.what());
    }
    return stats;
}

PaginatedResponse<ApiKey> AdminAuditModule::getApiKeys(int page, int limit, int userId) {
    PaginatedResponse<ApiKey> response;
    response.page = page; response.limit = limit;
    if (!database_) return response;
    try {
        std::string countSql = "SELECT COUNT(*) as total FROM api_keys";
        if (userId > 0) countSql += " WHERE user_id = ?";
        PreparedStatement countStmt(database_, countSql);
        if (userId > 0) countStmt.bind(0, userId);
        auto countResults = countStmt.query();
        response.total = countResults.empty() ? 0 : std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));
        int offset = (page - 1) * limit;
        std::string sql = "SELECT ak.*, u.username FROM api_keys ak LEFT JOIN users u ON ak.user_id = u.id";
        int bindIdx = 0;
        if (userId > 0) { sql += " WHERE ak.user_id = ?"; }
        sql += " ORDER BY ak.created_at DESC LIMIT ? OFFSET ?";
        PreparedStatement stmt(database_, sql);
        if (userId > 0) { stmt.bind(bindIdx, userId); bindIdx++; }
        stmt.bind(bindIdx, limit); bindIdx++;
        stmt.bind(bindIdx, offset);
        auto results = stmt.query();
        for (const auto& row : results) {
            ApiKey key;
            key.id = std::stoi(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            key.userId = std::stoi(StringUtil::cleanDbString(row.count("user_id") ? row.at("user_id") : "0"));
            key.username = StringUtil::cleanDbString(row.count("username") ? row.at("username") : "");
            key.name = StringUtil::cleanDbString(row.count("name") ? row.at("name") : "");
            key.keyPrefix = StringUtil::cleanDbString(row.count("key_prefix") ? row.at("key_prefix") : "");
            key.scopes = StringUtil::cleanDbString(row.count("scopes") ? row.at("scopes") : "[]");
            key.rateLimitPerHour = std::stoi(StringUtil::cleanDbString(row.count("rate_limit_per_hour") ? row.at("rate_limit_per_hour") : "1000"));
            key.expiresAt = StringUtil::cleanDbString(row.count("expires_at") ? row.at("expires_at") : "");
            key.lastUsedAt = StringUtil::cleanDbString(row.count("last_used_at") ? row.at("last_used_at") : "");
            key.requestCount = std::stoll(StringUtil::cleanDbString(row.count("request_count") ? row.at("request_count") : "0"));
            key.isActive = StringUtil::cleanDbString(row.count("is_active") ? row.at("is_active") : "1") == "1";
            key.createdBy = std::stoi(StringUtil::cleanDbString(row.count("created_by") ? row.at("created_by") : "0"));
            key.createdAt = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(key);
        }
        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get API keys: {}", e.what());
    }
    return response;
}

std::pair<int, std::string> AdminAuditModule::createApiKey(int userId, const std::string& name, const std::string& scopes, int rateLimitPerHour, const std::string& expiresAt, int createdBy) {
    if (!database_) return {0, ""};
    try {
        unsigned char keyBytes[32];
        RAND_bytes(keyBytes, sizeof(keyBytes));
        std::ostringstream ss;
        for (int i = 0; i < 32; i++) ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(keyBytes[i]);
        std::string fullKey = ss.str();
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(fullKey.c_str()), fullKey.length(), hash);
        std::ostringstream hashSS;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) hashSS << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        std::string keyHash = hashSS.str();
        std::string keyPrefix = fullKey.substr(0, 10);
        std::string sqlStr = "INSERT INTO api_keys (user_id, name, key_hash, key_prefix, scopes, rate_limit_per_hour, expires_at, created_by) VALUES (?, ?, ?, ?, ?, ?, " + std::string(expiresAt.empty() ? "NULL" : "?") + ", ?)";
        PreparedStatement stmt(database_, sqlStr);
        int bindIdx = 0;
        stmt.bind(bindIdx++, userId); stmt.bind(bindIdx++, name); stmt.bind(bindIdx++, keyHash);
        stmt.bind(bindIdx++, keyPrefix); stmt.bind(bindIdx++, scopes); stmt.bind(bindIdx++, rateLimitPerHour);
        if (!expiresAt.empty()) stmt.bind(bindIdx++, expiresAt);
        stmt.bind(bindIdx, createdBy);
        stmt.execute();
        auto lastId = database_->query("SELECT LAST_INSERT_ID() as id");
        int keyId = 0;
        if (!lastId.empty() && lastId[0].count("id")) keyId = std::stoi(StringUtil::cleanDbString(lastId[0].at("id")));
        return {keyId, fullKey};
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to create API key: {}", e.what());
        return {0, ""};
    }
}

bool AdminAuditModule::deleteApiKey(int id) {
    if (!database_) return false;
    try {
        PreparedStatement stmt(database_, "DELETE FROM api_keys WHERE id = ?");
        stmt.bind(0, id);
        stmt.execute();
        return true;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to delete API key: {}", e.what());
        return false;
    }
}

std::string AdminAuditModule::regenerateApiKey(int id) {
    if (!database_) return "";
    try {
        unsigned char keyBytes[32];
        RAND_bytes(keyBytes, sizeof(keyBytes));
        std::ostringstream ss;
        for (int i = 0; i < 32; i++) ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(keyBytes[i]);
        std::string fullKey = ss.str();
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(fullKey.c_str()), fullKey.length(), hash);
        std::ostringstream hashSS;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) hashSS << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        std::string keyHash = hashSS.str();
        std::string keyPrefix = fullKey.substr(0, 10);
        PreparedStatement stmt(database_,
            "UPDATE api_keys SET key_hash = ?, key_prefix = ?, request_count = 0, last_used_at = NULL WHERE id = ?");
        stmt.bind(0, keyHash);
        stmt.bind(1, keyPrefix);
        stmt.bind(2, id);
        stmt.execute();
        return fullKey;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to regenerate API key: {}", e.what());
        return "";
    }
}

PaginatedResponse<ApiUsage> AdminAuditModule::getApiKeyUsage(int page, int limit, int keyId) {
    PaginatedResponse<ApiUsage> response;
    response.page = page; response.limit = limit;
    if (!database_) return response;
    try {
        std::string countSql = "SELECT COUNT(*) as total FROM api_key_usage";
        if (keyId > 0) countSql += " WHERE key_id = ?";
        PreparedStatement countStmt(database_, countSql);
        if (keyId > 0) countStmt.bind(0, keyId);
        auto countResults = countStmt.query();
        response.total = countResults.empty() ? 0 : std::stoi(StringUtil::cleanDbString(countResults[0].at("total")));
        int offset = (page - 1) * limit;
        std::string sql = "SELECT aku.*, ak.name as key_name FROM api_key_usage aku LEFT JOIN api_keys ak ON aku.key_id = ak.id";
        int bindIdx = 0;
        if (keyId > 0) { sql += " WHERE aku.key_id = ?"; }
        sql += " ORDER BY aku.created_at DESC LIMIT ? OFFSET ?";
        PreparedStatement stmt(database_, sql);
        if (keyId > 0) { stmt.bind(bindIdx, keyId); bindIdx++; }
        stmt.bind(bindIdx, limit); bindIdx++;
        stmt.bind(bindIdx, offset);
        auto results = stmt.query();
        for (const auto& row : results) {
            ApiUsage usage;
            usage.id = std::stoll(StringUtil::cleanDbString(row.count("id") ? row.at("id") : "0"));
            usage.keyId = std::stoi(StringUtil::cleanDbString(row.count("key_id") ? row.at("key_id") : "0"));
            usage.keyName = StringUtil::cleanDbString(row.count("key_name") ? row.at("key_name") : "");
            usage.endpoint = StringUtil::cleanDbString(row.count("endpoint") ? row.at("endpoint") : "");
            usage.method = StringUtil::cleanDbString(row.count("method") ? row.at("method") : "");
            usage.statusCode = row.count("status_code") && row.at("status_code") != "NULL" ? std::stoi(StringUtil::cleanDbString(row.at("status_code"))) : 0;
            usage.responseTimeMs = row.count("response_time_ms") && row.at("response_time_ms") != "NULL" ? std::stoi(StringUtil::cleanDbString(row.at("response_time_ms"))) : 0;
            usage.ipAddress = StringUtil::cleanDbString(row.count("ip_address") ? row.at("ip_address") : "");
            usage.userAgent = StringUtil::cleanDbString(row.count("user_agent") ? row.at("user_agent") : "");
            usage.createdAt = StringUtil::cleanDbString(row.count("created_at") ? row.at("created_at") : "");
            response.items.push_back(usage);
        }
        response.totalPages = (response.total + limit - 1) / limit;
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get API key usage: {}", e.what());
    }
    return response;
}

ApiUsageStats AdminAuditModule::getApiKeyStats(int keyId) {
    ApiUsageStats stats;
    if (!database_) return stats;
    try {
        std::string totalSql = "SELECT COUNT(*) as total FROM api_key_usage";
        if (keyId > 0) totalSql += " WHERE key_id = ?";
        PreparedStatement totalStmt(database_, totalSql);
        if (keyId > 0) totalStmt.bind(0, keyId);
        auto totalResults = totalStmt.query();
        stats.totalRequests = totalResults.empty() ? 0 : std::stoll(StringUtil::cleanDbString(totalResults[0].at("total")));

        std::string successSql = "SELECT COUNT(*) as total FROM api_key_usage WHERE status_code >= 200 AND status_code < 400";
        if (keyId > 0) successSql += " AND key_id = ?";
        PreparedStatement successStmt(database_, successSql);
        if (keyId > 0) successStmt.bind(0, keyId);
        auto successResults = successStmt.query();
        stats.successfulRequests = successResults.empty() ? 0 : std::stoll(StringUtil::cleanDbString(successResults[0].at("total")));
        stats.failedRequests = stats.totalRequests - stats.successfulRequests;

        std::string avgSql = "SELECT AVG(response_time_ms) as avg FROM api_key_usage WHERE response_time_ms IS NOT NULL";
        if (keyId > 0) avgSql += " AND key_id = ?";
        PreparedStatement avgStmt(database_, avgSql);
        if (keyId > 0) avgStmt.bind(0, keyId);
        auto avgResults = avgStmt.query();
        stats.avgResponseTime = avgResults.empty() ? 0.0 : std::stod(StringUtil::cleanDbString(avgResults[0].at("avg")));
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get API key stats: {}", e.what());
    }
    return stats;
}

std::string AdminAuditModule::handleGetAuditLogs(const std::map<std::string, std::string>& params) {
    int page = 1, limit = 20;
    std::string action;
    int userId = 0;

    auto pageIt = params.find("page");
    if (pageIt != params.end()) page = std::stoi(pageIt->second);

    auto limitIt = params.find("limit");
    if (limitIt != params.end()) limit = std::stoi(limitIt->second);

    auto actionIt = params.find("action");
    if (actionIt != params.end()) action = actionIt->second;

    auto userIdIt = params.find("user_id");
    if (userIdIt != params.end()) userId = std::stoi(userIdIt->second);

    auto response = getAuditLogs(page, limit, action, userId);

    // 构建日志数组JSON
    std::ostringstream logsJson;
    logsJson << "[";
    for (size_t i = 0; i < response.items.size(); i++) {
        if (i > 0) logsJson << ",";
        logsJson << response.items[i].toJSON();
    }
    logsJson << "]";

    // 构建完整响应
    std::ostringstream result;
    result << "{";
    result << "\"logs\":" << logsJson.str() << ",";
    result << "\"pagination\":{";
    result << "\"page\":" << response.page << ",";
    result << "\"limit\":" << response.limit << ",";
    result << "\"total\":" << response.total << ",";
    result << "\"totalPages\":" << response.totalPages;
    result << "}}";

    return StringUtil::buildJsonResponse(HTTP::OK, true, "Audit logs retrieved", result.str());
}

std::string AdminAuditModule::handleGetRoles(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto roles = getRoles();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& role : roles) {
            nlohmann::json j;
            j["id"] = role.id;
            j["name"] = role.name;
            j["displayName"] = role.displayName;
            j["description"] = role.description;
            j["level"] = role.level;
            j["isSystem"] = role.isSystem;
            j["isDefault"] = role.isDefault;
            j["createdAt"] = role.createdAt;
            j["updatedAt"] = role.updatedAt;
            data.push_back(j);
        }
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Roles retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get roles: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve roles: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleCreateRole(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string name = ValidationHelper::sanitize(jsonBody.value("name", ""));
        std::string displayName = ValidationHelper::sanitize(jsonBody.value("displayName", ""));
        std::string description = ValidationHelper::sanitize(jsonBody.value("description", ""));
        int level = jsonBody.value("level", 10);
        int createdBy = jsonBody.value("createdBy", 1);

        if (name.empty() || displayName.empty()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Name and display name are required");
        }

        int roleId = createRole(name, displayName, description, level, createdBy);
        if (roleId > 0) {
            nlohmann::json data;
            data["roleId"] = roleId;
            return StringUtil::buildJsonResponse(HTTP::OK, true, "Role created successfully", data.dump());
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create role");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to create role: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create role: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleUpdateRole(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Role ID is required");
        }
        int roleId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::string displayName = ValidationHelper::sanitize(jsonBody.value("displayName", ""));
        std::string description = ValidationHelper::sanitize(jsonBody.value("description", ""));
        int level = jsonBody.value("level", 10);

        if (updateRole(roleId, displayName, description, level)) {
            return StringUtil::buildJsonResponse(true, "Role updated successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to update role");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to update role: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to update role: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleDeleteRole(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Role ID is required");
        }
        int roleId = std::stoi(idIt->second);

        if (deleteRole(roleId)) {
            return StringUtil::buildJsonResponse(true, "Role deleted successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete role or role is system role");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to delete role: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete role: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleGetPermissions(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto permissions = getPermissions();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& perm : permissions) {
            nlohmann::json j;
            j["id"] = perm.id;
            j["resource"] = perm.resource;
            j["action"] = perm.action;
            j["description"] = perm.description;
            data.push_back(j);
        }
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Permissions retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get permissions: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve permissions: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleGetPermissionMatrix(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto matrix = getPermissionMatrix();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& item : matrix) {
            nlohmann::json j;
            j["roleName"] = item.roleName;
            j["totalPermissions"] = item.totalPermissions;
            j["permissionsByResource"] = item.permissionsByResource;
            data.push_back(j);
        }
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Permission matrix retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get permission matrix: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve permission matrix: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleGetRolePermissions(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Role ID is required");
        }
        int roleId = std::stoi(idIt->second);

        auto permissions = getRolePermissions(roleId);
        nlohmann::json data = nlohmann::json::array();
        for (const auto& perm : permissions) {
            nlohmann::json j;
            j["permissionId"] = perm.permissionId;
            j["resource"] = perm.resource;
            j["action"] = perm.action;
            j["grantedAt"] = perm.grantedAt;
            j["grantedByUsername"] = perm.grantedByUsername;
            data.push_back(j);
        }
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Role permissions retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get role permissions: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve role permissions: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleUpdateRolePermissions(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Role ID is required");
        }
        int roleId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        std::vector<int> permissionIds = jsonBody.value("permissionIds", std::vector<int>());
        int updatedBy = jsonBody.value("updatedBy", 1);

        if (updateRolePermissions(roleId, permissionIds, updatedBy)) {
            return StringUtil::buildJsonResponse(true, "Role permissions updated successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to update role permissions");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to update role permissions: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to update role permissions: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleGetUserRoles(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "User ID is required");
        }
        int userId = std::stoi(idIt->second);

        auto roles = getUserRoles(userId);
        nlohmann::json data = nlohmann::json::array();
        for (const auto& role : roles) {
            nlohmann::json j;
            j["id"] = role.id;
            j["userId"] = role.userId;
            j["username"] = role.username;
            j["roleId"] = role.roleId;
            j["roleName"] = role.roleName;
            j["roleLevel"] = role.roleLevel;
            j["assignedAt"] = role.assignedAt;
            j["expiresAt"] = role.expiresAt;
            j["reason"] = role.reason;
            data.push_back(j);
        }
        return StringUtil::buildJsonResponse(HTTP::OK, true, "User roles retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get user roles: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve user roles: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleAssignUserRole(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "User ID is required");
        }
        int userId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        int roleId = jsonBody.value("roleId", 0);
        std::string reason = ValidationHelper::sanitize(jsonBody.value("reason", ""));
        int assignedBy = jsonBody.value("assignedBy", 1);
        std::string expiresAt = jsonBody.value("expiresAt", "");

        if (roleId == 0) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Role ID is required");
        }

        if (assignUserRole(userId, roleId, reason, assignedBy, expiresAt)) {
            return StringUtil::buildJsonResponse(true, "User role assigned successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to assign user role");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to assign user role: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to assign user role: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleRemoveUserRole(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "User ID is required");
        }
        int userId = std::stoi(idIt->second);

        auto roleIdIt = params.find("roleid");
        if (roleIdIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Role ID is required");
        }
        int roleId = std::stoi(roleIdIt->second);

        if (removeUserRole(userId, roleId)) {
            return StringUtil::buildJsonResponse(true, "User role removed successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to remove user role");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to remove user role: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to remove user role: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleCheckPermission(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        int userId = jsonBody.value("userId", 0);
        std::string resource = jsonBody.value("resource", "");
        std::string action = jsonBody.value("action", "");

        if (userId == 0 || resource.empty() || action.empty()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "User ID, resource and action are required");
        }

        bool hasPermission = checkUserPermission(userId, resource, action);
        nlohmann::json data;
        data["hasPermission"] = hasPermission;
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Permission checked", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to check permission: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to check permission: " + std::string(e.what()));
    }
}

// ============================================================================
// 通知管理handlers
// ============================================================================


std::string AdminAuditModule::handleGetPendingPapers(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;

        auto papers = getPendingPapers(page, limit);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& paper : papers.items) {
            nlohmann::json j;
            j["id"] = paper.id;
            j["paperId"] = paper.paperId;
            j["status"] = paper.status;
            j["moderatorId"] = paper.moderatorId;
            j["moderatorUsername"] = paper.moderatorUsername;
            j["reason"] = paper.reason;
            j["reviewedAt"] = paper.reviewedAt;
            j["flags"] = paper.flags;
            j["createdAt"] = paper.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = papers.total;
        data["page"] = papers.page;
        data["limit"] = papers.limit;
        data["totalPages"] = papers.totalPages;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "Pending papers retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get pending papers: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve pending papers: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleGetPaperModeration(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Moderation ID is required");
        }
        int64_t id = std::stoll(idIt->second);

        auto moderation = getPaperModeration(id);
        if (moderation.has_value()) {
            nlohmann::json data;
            data["id"] = moderation->id;
            data["paperId"] = moderation->paperId;
            data["status"] = moderation->status;
            data["moderatorId"] = moderation->moderatorId;
            data["moderatorUsername"] = moderation->moderatorUsername;
            data["reason"] = moderation->reason;
            data["reviewedAt"] = moderation->reviewedAt;
            data["flags"] = moderation->flags;
            data["createdAt"] = moderation->createdAt;
            return StringUtil::buildJsonResponse(HTTP::OK, true, "Paper moderation retrieved", data.dump());
        } else {
            return StringUtil::buildJsonResponse(HTTP::NOT_FOUND, false, "Paper moderation not found");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get paper moderation: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve moderation: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleApprovePaper(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Paper ID is required");
        }
        int paperId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        int moderatorId = jsonBody.value("moderatorId", 1);

        if (approvePaper(paperId, moderatorId)) {
            return StringUtil::buildJsonResponse(true, "Paper approved successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to approve paper");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to approve paper: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to approve paper: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleRejectPaper(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Paper ID is required");
        }
        int paperId = std::stoi(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        int moderatorId = jsonBody.value("moderatorId", 1);
        std::string reason = ValidationHelper::sanitize(jsonBody.value("reason", ""));

        if (rejectPaper(paperId, moderatorId, reason)) {
            return StringUtil::buildJsonResponse(true, "Paper rejected successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to reject paper");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to reject paper: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to reject paper: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleGetUserReports(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;
        std::string status = params.count("status") ? params.at("status") : "";

        auto reports = getUserReports(page, limit, status);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& report : reports.items) {
            nlohmann::json j;
            j["id"] = report.id;
            j["reporterId"] = report.reporterId;
            j["reporterUsername"] = report.reporterUsername;
            j["targetType"] = report.targetType;
            j["targetId"] = report.targetId;
            j["reason"] = report.reason;
            j["description"] = report.description;
            j["status"] = report.status;
            j["priority"] = report.priority;
            j["reviewerId"] = report.reviewerId;
            j["reviewerUsername"] = report.reviewerUsername;
            j["resolution"] = report.resolution;
            j["createdAt"] = report.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = reports.total;
        data["page"] = reports.page;
        data["limit"] = reports.limit;
        data["totalPages"] = reports.totalPages;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "User reports retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get user reports: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve reports: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleResolveReport(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Report ID is required");
        }
        int64_t reportId = std::stoll(idIt->second);

        auto jsonBody = nlohmann::json::parse(body);
        int reviewerId = jsonBody.value("reviewerId", 1);
        std::string resolution = ValidationHelper::sanitize(jsonBody.value("resolution", ""));
        std::string status = jsonBody.value("status", "resolved");

        if (resolveReport(reportId, reviewerId, resolution, status)) {
            return StringUtil::buildJsonResponse(true, "Report resolved successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to resolve report");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to resolve report: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to resolve report: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleGetSensitiveWords(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto words = getSensitiveWords();
        nlohmann::json data = nlohmann::json::array();
        for (const auto& word : words) {
            nlohmann::json j;
            j["id"] = word.id;
            j["word"] = word.word;
            j["category"] = word.category;
            j["severity"] = word.severity;
            j["isRegex"] = word.isRegex;
            j["replacement"] = word.replacement;
            j["isActive"] = word.isActive;
            j["matchCount"] = word.matchCount;
            j["createdBy"] = word.createdBy;
            j["createdAt"] = word.createdAt;
            data.push_back(j);
        }
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Sensitive words retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get sensitive words: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve words: " + std::string(e.what()));
    }
}


std::string AdminAuditModule::handleCreateSensitiveWord(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string word = ValidationHelper::sanitize(jsonBody.value("word", ""));
        std::string category = jsonBody.value("category", "other");
        std::string severity = jsonBody.value("severity", "medium");
        bool isRegex = jsonBody.value("isRegex", false);
        std::string replacement = ValidationHelper::sanitize(jsonBody.value("replacement", ""));
        int createdBy = jsonBody.value("createdBy", 1);

        int wordId = createSensitiveWord(word, category, severity, isRegex, replacement, createdBy);
        if (wordId > 0) {
            nlohmann::json data;
            data["wordId"] = wordId;
            return StringUtil::buildJsonResponse(HTTP::OK, true, "Sensitive word created", data.dump());
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create sensitive word");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to create sensitive word: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create word: " + std::string(e.what()));
    }
}

std::string AdminAuditModule::handleDeleteSensitiveWord(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");

        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "Word ID is required");
        }
        int id = std::stoi(idIt->second);

        if (deleteSensitiveWord(id)) {
            return StringUtil::buildJsonResponse(true, "Sensitive word deleted");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete sensitive word");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to delete sensitive word: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete word: " + std::string(e.what()));
    }
}

std::string AdminAuditModule::handleCheckSensitiveWords(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string text = ValidationHelper::sanitize(jsonBody.value("text", ""));



        auto matches = checkSensitiveWords(text);
        nlohmann::json data = nlohmann::json::array();
        for (const auto& match : matches) {
            nlohmann::json j;
            j["word"] = match.word;
            j["category"] = match.category;
            j["startPosition"] = match.startPosition;
            j["endPosition"] = match.endPosition;
            j["matchedText"] = match.matchedText;
            data.push_back(j);
        }
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Sensitive words checked", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to check sensitive words: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to check words: " + std::string(e.what()));
    }
}

std::string AdminAuditModule::handleGetSensitiveWordStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto stats = getSensitiveWordStats();
        nlohmann::json data;
        data["stats"] = nlohmann::json::object();
        for (const auto& [key, value] : stats) {
            data["stats"][key] = value;
        }
        return StringUtil::buildJsonResponse(HTTP::OK, true, "Sensitive word statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get sensitive word stats: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve statistics: " + std::string(e.what()));
    }
}

// ============================================================================
// API密钥管理handlers
// ============================================================================

std::string AdminAuditModule::handleGetApiKeys(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;
        int userId = params.count("userId") ? std::stoi(params.at("userId")) : 0;

        auto keys = getApiKeys(page, limit, userId);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& key : keys.items) {
            nlohmann::json j;
            j["id"] = key.id;
            j["userId"] = key.userId;
            j["username"] = key.username;
            j["name"] = key.name;
            j["keyPrefix"] = key.keyPrefix;
            j["scopes"] = key.scopes;
            j["rateLimitPerHour"] = key.rateLimitPerHour;
            j["expiresAt"] = key.expiresAt;
            j["lastUsedAt"] = key.lastUsedAt;

            j["lastUsedAt"] = key.lastUsedAt;
            j["requestCount"] = key.requestCount;
            j["isActive"] = key.isActive;
            j["createdBy"] = key.createdBy;
            j["createdAt"] = key.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = keys.total;
        data["page"] = keys.page;
        data["limit"] = keys.limit;
        data["totalPages"] = keys.totalPages;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "API keys retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get API keys: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve API keys: " + std::string(e.what()));
    }
}

std::string AdminAuditModule::handleCreateApiKey(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto jsonBody = nlohmann::json::parse(body);
        int userId = jsonBody.value("userId", 0);
        std::string name = ValidationHelper::sanitize(jsonBody.value("name", ""));
        std::string scopes = jsonBody.value("scopes", "[]");
        int rateLimitPerHour = jsonBody.value("rateLimitPerHour", 1000);
        std::string expiresAt = jsonBody.value("expiresAt", "");
        int createdBy = jsonBody.value("createdBy", 1);

        auto [keyId, fullKey] = createApiKey(userId, name, scopes, rateLimitPerHour, expiresAt, createdBy);
        if (keyId > 0) {
            nlohmann::json data;
            data["keyId"] = keyId;
            data["apiKey"] = fullKey;  // Only show full key on creation
            return StringUtil::buildJsonResponse(HTTP::OK, true, "API key created successfully", data.dump());
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create API key");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to create API key: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to create API key: " + std::string(e.what()));
    }
}

std::string AdminAuditModule::handleDeleteApiKey(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }



    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "API key ID is required");
        }
        int id = std::stoi(idIt->second);

        if (deleteApiKey(id)) {
            return StringUtil::buildJsonResponse(true, "API key deleted successfully");
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete API key");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to delete API key: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to delete API key: " + std::string(e.what()));
    }
}

std::string AdminAuditModule::handleRegenerateApiKey(const std::map<std::string, std::string>& params, const std::string& body) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return StringUtil::buildJsonResponse(HTTP::BAD_REQUEST, false, "API key ID is required");
        }
        int id = std::stoi(idIt->second);

        std::string newKey = regenerateApiKey(id);
        if (!newKey.empty()) {
            nlohmann::json data;
            data["apiKey"] = newKey;
            return StringUtil::buildJsonResponse(HTTP::OK, true, "API key regenerated successfully", data.dump());
        } else {
            return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to regenerate API key");
        }
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to regenerate API key: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to regenerate API key: " + std::string(e.what()));
    }
}

std::string AdminAuditModule::handleGetApiKeyUsage(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }



    try {
        int page = params.count("page") ? std::stoi(params.at("page")) : 1;
        int limit = params.count("limit") ? std::stoi(params.at("limit")) : 20;
        int keyId = params.count("keyId") ? std::stoi(params.at("keyId")) : 0;

        auto usage = getApiKeyUsage(page, limit, keyId);
        nlohmann::json data;
        data["items"] = nlohmann::json::array();
        for (const auto& record : usage.items) {
            nlohmann::json j;
            j["id"] = record.id;
            j["keyId"] = record.keyId;
            j["keyName"] = record.keyName;
            j["endpoint"] = record.endpoint;
            j["method"] = record.method;
            j["statusCode"] = record.statusCode;
            j["responseTimeMs"] = record.responseTimeMs;
            j["ipAddress"] = record.ipAddress;
            j["userAgent"] = record.userAgent;
            j["createdAt"] = record.createdAt;
            data["items"].push_back(j);
        }
        data["total"] = usage.total;
        data["page"] = usage.page;
        data["limit"] = usage.limit;
        data["totalPages"] = usage.totalPages;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "API key usage retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get API key usage: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve usage: " + std::string(e.what()));
    }
}

std::string AdminAuditModule::handleGetApiKeyStats(const std::map<std::string, std::string>& params) {
    if (!impl_) {
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Implementation not initialized");
    }

    try {
        int keyId = params.count("keyId") ? std::stoi(params.at("keyId")) : 0;

        auto stats = getApiKeyStats(keyId);
        nlohmann::json data;
        data["totalRequests"] = stats.totalRequests;
        data["successfulRequests"] = stats.successfulRequests;
        data["failedRequests"] = stats.failedRequests;
        data["avgResponseTime"] = stats.avgResponseTime;

        nlohmann::json byEndpoint = nlohmann::json::object();
        for (const auto& [endpoint, count] : stats.requestsByEndpoint) {
            byEndpoint[endpoint] = count;
        }
        data["requestsByEndpoint"] = byEndpoint;

        nlohmann::json byDay = nlohmann::json::object();
        for (const auto& [day, count] : stats.requestsByDay) {
            byDay[day] = count;
        }
        data["requestsByDay"] = byDay;

        return StringUtil::buildJsonResponse(HTTP::OK, true, "API key statistics retrieved", data.dump());
    } catch (const std::exception& e) {
        spdlog::error("[AdminAudit] Failed to get API key stats: {}", e.what());
        return StringUtil::buildJsonResponse(HTTP::INTERNAL_ERROR, false, "Failed to retrieve statistics: " + std::string(e.what()));
    }
}

// ============================================================================
// DLL export functions
// ============================================================================

#define EXPORT __attribute__((visibility("default")))

extern "C" {
EXPORT void* createModule() {
    return new PaperCrawler::AdminAuditModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::AdminAuditModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}
}

} // namespace PaperCrawler
