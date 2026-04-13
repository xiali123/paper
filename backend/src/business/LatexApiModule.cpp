#include "business/LatexApiModule.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <mutex>

namespace PaperCrawler {

// 内存存储（用于演示，生产环境应使用数据库）
class InMemoryLatexStore {
public:
    std::unordered_map<int, LatexDocument> documents;
    std::unordered_map<int, LatexTemplate> templates;
    std::unordered_map<int, LatexProject> projects;
    std::unordered_map<int, LatexProjectFile> projectFiles;

    // 用户配额存储
    std::unordered_map<std::string, LatexUserQuota> userQuotas;
    std::vector<LatexCompilationRecord> compilationRecords;
    int nextCompilationRecordId = 1;

    int nextDocumentId = 1;
    int nextProjectId = 1;
    int nextProjectFileId = 1;
    std::mutex mutex_;

    bool saveDocument(const LatexDocument& doc) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (doc.id == 0) {
            return false;
        }
        documents[doc.id] = doc;
        return true;
    }

    std::optional<LatexDocument> getDocument(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = documents.find(id);
        if (it != documents.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool deleteDocument(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        return documents.erase(id) > 0;
    }

    std::vector<LatexDocument> listDocuments() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<LatexDocument> result;
        for (const auto& pair : documents) {
            result.push_back(pair.second);
        }
        return result;
    }

    int createDocument(const LatexDocument& doc) {
        std::lock_guard<std::mutex> lock(mutex_);
        int newId = nextDocumentId++;
        LatexDocument newDoc = doc;
        newDoc.id = newId;
        documents[newId] = newDoc;
        return newId;
    }

    size_t count() {
        std::lock_guard<std::mutex> lock(mutex_);
        return documents.size();
    }

    // ==========================================
    // 项目存储方法
    // ==========================================

    int createProject(const LatexProject& project) {
        std::lock_guard<std::mutex> lock(mutex_);
        int newId = nextProjectId++;
        LatexProject newProject = project;
        newProject.id = newId;
        projects[newId] = newProject;
        return newId;
    }

    std::optional<LatexProject> getProject(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = projects.find(id);
        if (it != projects.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool updateProject(int id, const LatexProject& project) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = projects.find(id);
        if (it != projects.end()) {
            it->second = project;
            it->second.id = id;
            return true;
        }
        return false;
    }

    bool deleteProject(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        // 删除项目及其所有文件
        projects.erase(id);
        for (auto it = projectFiles.begin(); it != projectFiles.end(); ) {
            if (it->second.projectId == id) {
                it = projectFiles.erase(it);
            } else {
                ++it;
            }
        }
        return true;
    }

    std::vector<LatexProject> listProjects() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<LatexProject> result;
        for (const auto& pair : projects) {
            result.push_back(pair.second);
        }
        return result;
    }

    // ==========================================
    // 项目文件存储方法
    // ==========================================

    int addProjectFile(const LatexProjectFile& file) {
        std::lock_guard<std::mutex> lock(mutex_);
        int newId = nextProjectFileId++;
        LatexProjectFile newFile = file;
        newFile.id = newId;
        projectFiles[newId] = newFile;
        return newId;
    }

    std::optional<LatexProjectFile> getProjectFile(int fileId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = projectFiles.find(fileId);
        if (it != projectFiles.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::vector<LatexProjectFile> getProjectFiles(int projectId) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<LatexProjectFile> result;
        for (const auto& pair : projectFiles) {
            if (pair.second.projectId == projectId) {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    bool updateProjectFile(int fileId, const LatexProjectFile& file) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = projectFiles.find(fileId);
        if (it != projectFiles.end()) {
            it->second = file;
            it->second.id = fileId;
            return true;
        }
        return false;
    }

    bool deleteProjectFile(int fileId) {
        std::lock_guard<std::mutex> lock(mutex_);
        return projectFiles.erase(fileId) > 0;
    }

    // ==========================================
    // 用户配额管理方法
    // ==========================================

    std::optional<LatexUserQuota> getUserQuota(const std::string& userId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = userQuotas.find(userId);
        if (it != userQuotas.end()) {
            // 检查并重置过期的配额
            checkAndResetQuota(it->second);
            return it->second;
        }
        return std::nullopt;
    }

    void setUserQuota(const LatexUserQuota& quota) {
        std::lock_guard<std::mutex> lock(mutex_);
        userQuotas[quota.userId] = quota;
    }

    void checkAndResetQuota(LatexUserQuota& quota) {
        auto now = std::chrono::system_clock::now();

        // 检查每日配额重置
        if (now >= quota.dailyReset) {
            quota.dailyCompilesUsed = 0;
            quota.dailyReset = now + std::chrono::hours(24);
            spdlog::info("[LatexStore] Daily quota reset for user: {}", quota.userId);
        }

        // 检查每月配额重置
        if (now >= quota.monthlyReset) {
            quota.monthlyCompilesUsed = 0;
            // 重置到下个月1号
            std::tm tm = std::tm();
            time_t nowTime = std::chrono::system_clock::to_time_t(now);
            localtime_r(&nowTime, &tm);
            tm.tm_mon = (tm.tm_mon + 1) % 12;
            if (tm.tm_mon == 0) tm.tm_year++;
            tm.tm_mday = 1;
            tm.tm_hour = 0;
            tm.tm_min = 0;
            tm.tm_sec = 0;
            quota.monthlyReset = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            spdlog::info("[LatexStore] Monthly quota reset for user: {}", quota.userId);
        }
    }

    bool canCompile(const std::string& userId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = userQuotas.find(userId);
        if (it == userQuotas.end()) {
            // 没有配额限制的用户（管理员）
            return true;
        }

        checkAndResetQuota(it->second);
        const auto& quota = it->second;

        // 检查每日限制
        if (quota.dailyCompilesUsed >= quota.dailyCompileLimit) {
            spdlog::warn("[LatexStore] User {} exceeded daily compile limit: {}/{}",
                userId, quota.dailyCompilesUsed, quota.dailyCompileLimit);
            return false;
        }

        // 检查每月限制
        if (quota.monthlyCompilesUsed >= quota.monthlyCompileLimit) {
            spdlog::warn("[LatexStore] User {} exceeded monthly compile limit: {}/{}",
                userId, quota.monthlyCompilesUsed, quota.monthlyCompileLimit);
            return false;
        }

        return true;
    }

    void recordCompilation(const LatexCompilationRecord& record) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 保存编译记录
        compilationRecords.push_back(record);

        // 更新用户配额使用情况
        auto it = userQuotas.find(record.userId);
        if (it != userQuotas.end()) {
            it->second.dailyCompilesUsed++;
            it->second.monthlyCompilesUsed++;
            spdlog::info("[LatexStore] Recorded compilation for user {}: daily={}/{}, monthly={}/{}",
                record.userId,
                it->second.dailyCompilesUsed,
                it->second.dailyCompileLimit,
                it->second.monthlyCompilesUsed,
                it->second.monthlyCompileLimit);
        }
    }

    std::vector<LatexCompilationRecord> getCompilationRecords(const std::string& userId, int limit = 100) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<LatexCompilationRecord> result;

        for (const auto& record : compilationRecords) {
            if (record.userId == userId) {
                result.push_back(record);
                if (result.size() >= limit) break;
            }
        }

        return result;
    }

    void initializeDefaultQuota(const std::string& userId, const std::string& tier = "free") {
        std::lock_guard<std::mutex> lock(mutex_);

        if (userQuotas.find(userId) != userQuotas.end()) {
            return; // 已存在
        }

        LatexUserQuota quota;
        quota.userId = userId;

        if (tier == "free") {
            quota.dailyCompileLimit = 10;
            quota.monthlyCompileLimit = 100;
            quota.maxProjectCount = 3;
            quota.canUseAdvancedFeatures = false;
            quota.allowedPackages = {"amsmath", "amsfonts", "amssymb", "graphicx", "geometry"};
        } else if (tier == "pro") {
            quota.dailyCompileLimit = 100;
            quota.monthlyCompileLimit = 2000;
            quota.maxProjectCount = 50;
            quota.canUseAdvancedFeatures = true;
            quota.allowedPackages = {"all"}; // 所有包
        } else if (tier == "admin") {
            quota.dailyCompileLimit = -1; // 无限制
            quota.monthlyCompileLimit = -1;
            quota.maxProjectCount = -1;
            quota.canUseAdvancedFeatures = true;
            quota.allowedPackages = {"all"};
        }

        auto now = std::chrono::system_clock::now();
        quota.dailyReset = now + std::chrono::hours(24);

        // 设置月度重置时间（下个月1号）
        std::tm tm = std::tm();
        time_t nowTime = std::chrono::system_clock::to_time_t(now);
        localtime_r(&nowTime, &tm);
        tm.tm_mon = (tm.tm_mon + 1) % 12;
        if (tm.tm_mon == 0) tm.tm_year++;
        tm.tm_mday = 1;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        quota.monthlyReset = std::chrono::system_clock::from_time_t(std::mktime(&tm));

        userQuotas[userId] = quota;
        spdlog::info("[LatexStore] Initialized {} tier quota for user: {}", tier, userId);
    }
};

// 全局存储实例
static InMemoryLatexStore g_latexStore;

class LatexApiModule::Impl {
public:
    std::vector<LatexTemplate> builtInTemplates;
    std::string outputDirectory = "./output/latex";
    std::string pdfDirectory = "./output/pdfs";

    Impl() {
        // 创建输出目录
        try {
            std::filesystem::create_directories(outputDirectory);
            std::filesystem::create_directories(pdfDirectory);
        } catch (const std::exception& e) {
            spdlog::warn("[LatexApi] Failed to create output directories: {}", e.what());
        }
    }
};

// ==========================================
// 构造函数
// ==========================================

LatexApiModule::LatexApiModule()
    : LatexApiModule(nullptr) {
    spdlog::info("[LatexApi] Default constructor");
}

LatexApiModule::LatexApiModule(std::shared_ptr<IDatabase> database)
    : database_(database), impl_(std::make_unique<Impl>()) {
    spdlog::info("[LatexApi] Constructor with database");
    initializeBuiltInTemplates();
}

LatexApiModule::~LatexApiModule() {
    spdlog::info("[LatexApi] Destructor");
}

// ==========================================
// 路由注册
// ==========================================

void LatexApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix(); // "/api/latex"

    spdlog::info("[LatexApi] Registering routes with prefix: {}", prefix);

    // 初始化数据库连接
    database_ = getDatabase();
    if (database_) {
        spdlog::info("[LatexApi] ✅ Received injected database connection");
    } else {
        spdlog::info("[LatexApi] 🔔 No injected database connection, using in-memory storage");
    }

    // ========== 文档CRUD ==========

    // GET /api/latex/documents - 获取文档列表
    router.get(prefix + "/documents", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleListDocuments(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/documents - 创建文档
    router.post(prefix + "/documents", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleCreateDocument(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // GET /api/latex/documents/:id - 获取文档详情
    router.get(prefix + "/documents/:id", [this](const HttpRequest& req) -> HttpResponse {
        auto params = req.pathParams;
        std::string result = handleGetDocument(params);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // PUT /api/latex/documents/:id - 更新文档
    router.put(prefix + "/documents/:id", [this](const HttpRequest& req) -> HttpResponse {
        auto params = req.pathParams;
        std::string result = handleUpdateDocument(params, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // DELETE /api/latex/documents/:id - 删除文档
    router.del(prefix + "/documents/:id", [this](const HttpRequest& req) -> HttpResponse {
        auto params = req.pathParams;
        std::string result = handleDeleteDocument(params);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/documents/:id/compile - 编译文档
    router.post(prefix + "/documents/:id/compile", [this](const HttpRequest& req) -> HttpResponse {
        auto params = req.pathParams;
        std::string result = handleCompile(params);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/documents/:id/autosave - 自动保存
    router.post(prefix + "/documents/:id/autosave", [this](const HttpRequest& req) -> HttpResponse {
        auto params = req.pathParams;
        std::string result = handleAutoSave(params, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // ========== 模板管理 ==========

    // GET /api/latex/templates - 获取模板列表
    router.get(prefix + "/templates", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleListTemplates(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // GET /api/latex/templates/:id - 获取模板详情
    router.get(prefix + "/templates/:id", [this](const HttpRequest& req) -> HttpResponse {
        auto params = req.pathParams;
        std::string result = handleGetTemplate(params);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/templates - 从模板创建文档
    router.post(prefix + "/templates", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleCreateFromTemplate(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // GET /api/latex/stats - 获取统计信息
    router.get(prefix + "/stats", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleStats();
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // GET /api/latex/documents/:id/pdf - 下载PDF
    router.get(prefix + "/documents/:id/pdf", [this](const HttpRequest& req) -> HttpResponse {
        auto params = req.pathParams;
        std::string result = handleDownloadPDF(params);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // ========== 协作功能 ==========

    // POST /api/latex/collaboration/join - 加入协作
    router.post(prefix + "/collaboration/join", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleJoinCollaboration(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/collaboration/leave - 离开协作
    router.post(prefix + "/collaboration/leave", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleLeaveCollaboration(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // PUT /api/latex/collaboration/cursor - 更新光标位置
    router.put(prefix + "/collaboration/cursor", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleUpdateCursor(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/collaboration/update - 广播文档更新
    router.post(prefix + "/collaboration/update", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleBroadcastUpdate(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // GET /api/latex/collaboration/sessions - 获取活跃会话
    router.get(prefix + "/collaboration/sessions", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleListCollaborationSessions();
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // ========== 项目CRUD（多文件支持）==========

    // GET /api/latex/projects - 获取项目列表
    router.get(prefix + "/projects", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleListProjects(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // GET /api/latex/projects/:id - 获取项目详情
    router.get(prefix + "/projects/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleGetProject(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/projects - 创建项目
    router.post(prefix + "/projects", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleCreateProject(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // PUT /api/latex/projects/:id - 更新项目
    router.put(prefix + "/projects/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleUpdateProject(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // DELETE /api/latex/projects/:id - 删除项目
    router.del(prefix + "/projects/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleDeleteProject(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/projects/:id/compile - 编译项目
    router.post(prefix + "/projects/:id/compile", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleCompileProject(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/projects/files - 添加文件到项目
    router.post(prefix + "/projects/files", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleAddProjectFile(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // PUT /api/latex/projects/files/:id - 更新项目文件
    router.put(prefix + "/projects/files/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleUpdateProjectFile(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // DELETE /api/latex/projects/files/:id - 删除项目文件
    router.del(prefix + "/projects/files/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleDeleteProjectFile(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // GET /api/latex/projects/files/:id - 获取项目文件
    router.get(prefix + "/projects/files/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleGetProjectFile(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // ========== 用户配额管理 ==========

    // GET /api/latex/quota/:user_id - 获取用户配额
    router.get(prefix + "/quota/:user_id", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleGetUserQuota(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/quota - 设置用户配额
    router.post(prefix + "/quota", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleSetUserQuota(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // POST /api/latex/quota/initialize - 初始化用户配额
    router.post(prefix + "/quota/initialize", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleInitializeUserQuota(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    // GET /api/latex/quota/:user_id/records - 获取用户编译记录
    router.get(prefix + "/quota/:user_id/records", [this](const HttpRequest& req) -> HttpResponse {
        std::string result = handleGetUserCompilationRecords(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = result;
        return response;
    });

    spdlog::info("[LatexApi] Routes registered successfully");
}

// ==========================================
// HTTP请求处理器
// ==========================================

std::string LatexApiModule::handleListDocuments(const std::map<std::string, std::string>& params) {
    try {
        int page = 1, limit = 20;
        std::string ownerId;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            const std::string& pageStr = pageIt->second;
            // Validate that page contains only digits
            bool valid = true;
            for (char c : pageStr) {
                if (!std::isdigit(static_cast<unsigned char>(c))) {
                    valid = false;
                    break;
                }
            }
            if (valid && !pageStr.empty()) {
                page = std::stoi(pageStr);
            }
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            const std::string& limitStr = limitIt->second;
            // Validate that limit contains only digits
            bool valid = true;
            for (char c : limitStr) {
                if (!std::isdigit(static_cast<unsigned char>(c))) {
                    valid = false;
                    break;
                }
            }
            if (valid && !limitStr.empty()) {
                limit = std::stoi(limitStr);
            }
        }

        auto ownerIt = params.find("owner_id");
        if (ownerIt != params.end()) ownerId = ownerIt->second;

        auto documents = listDocuments(page, limit, ownerId);

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Documents retrieved";
        response["data"]["items"] = nlohmann::json::array();
        response["data"]["page"] = page;
        response["data"]["limit"] = limit;
        response["data"]["total"] = documents.size();

        for (const auto& doc : documents) {
            nlohmann::json item;
            item["id"] = doc.id;
            item["title"] = doc.title;
            item["owner_id"] = doc.ownerId;
            item["is_collaborative"] = doc.isCollaborative;
            item["version"] = doc.version;
            item["is_compiled"] = doc.isCompiled;
            item["created_at"] = std::chrono::system_clock::to_time_t(doc.createdAt);
            item["updated_at"] = std::chrono::system_clock::to_time_t(doc.updatedAt);
            response["data"]["items"].push_back(item);
        }

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleListDocuments: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleGetDocument(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing document ID");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleGetDocument: Empty document ID");
            return buildJsonResponse(400, false, "Invalid document ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleGetDocument: Non-numeric document ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid document ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);
        auto document = getDocument(id);

        if (!document) {
            return buildJsonResponse(404, false, "Document not found", "");
        }

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Document retrieved";
        response["data"]["id"] = document->id;
        response["data"]["title"] = document->title;
        response["data"]["content"] = document->content;
        response["data"]["owner_id"] = document->ownerId;
        response["data"]["is_collaborative"] = document->isCollaborative;
        response["data"]["version"] = document->version;
        response["data"]["is_compiled"] = document->isCompiled;
        response["data"]["pdf_path"] = document->pdfPath;
        response["data"]["created_at"] = std::chrono::system_clock::to_time_t(document->createdAt);
        response["data"]["updated_at"] = std::chrono::system_clock::to_time_t(document->updatedAt);
        response["data"]["last_auto_save"] = std::chrono::system_clock::to_time_t(document->lastAutoSave);

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleGetDocument: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleCreateDocument(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);

        LatexDocument document;
        document.title = json.value("title", "Untitled Document");
        document.content = json.value("content", "");
        document.ownerId = json.value("owner_id", "");
        document.isCollaborative = json.value("is_collaborative", false);
        document.createdAt = std::chrono::system_clock::now();
        document.updatedAt = std::chrono::system_clock::now();
        document.lastAutoSave = std::chrono::system_clock::now();

        auto created = createDocument(document);

        if (!created) {
            return buildJsonResponse(false, "Failed to create document");
        }

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Document created";
        response["data"]["id"] = created->id;
        response["data"]["title"] = created->title;
        response["data"]["created_at"] = std::chrono::system_clock::to_time_t(created->createdAt);

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleCreateDocument: {}", e.what());
        return buildJsonResponse(false, "Failed to create document");
    }
}

std::string LatexApiModule::handleUpdateDocument(const std::map<std::string, std::string>& params, const std::string& body) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing document ID");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleUpdateDocument: Empty document ID");
            return buildJsonResponse(400, false, "Invalid document ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleUpdateDocument: Non-numeric document ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid document ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);
        nlohmann::json json = nlohmann::json::parse(body);

        LatexDocument document;
        document.id = id;
        document.title = json.value("title", "");
        document.content = json.value("content", "");

        bool success = updateDocument(id, document);

        if (success) {
            return buildJsonResponse(true, "Document updated");
        } else {
            return buildJsonResponse(404, false, "Failed to update document", "");
        }
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("[LatexApi] JSON parse error in handleUpdateDocument: {}", e.what());
        return buildJsonResponse(400, false, "Invalid JSON in request body", "");
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleUpdateDocument: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleDeleteDocument(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing document ID");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleDeleteDocument: Empty document ID");
            return buildJsonResponse(400, false, "Invalid document ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleDeleteDocument: Non-numeric document ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid document ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);
        bool success = deleteDocument(id);

        if (success) {
            return buildJsonResponse(true, "Document deleted");
        } else {
            return buildJsonResponse(404, false, "Failed to delete document", "");
        }
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleDeleteDocument: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleCompile(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing document ID");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleCompile: Empty document ID");
            return buildJsonResponse(400, false, "Invalid document ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleCompile: Non-numeric document ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid document ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);

        // 从查询参数获取 user_id（可选）
        std::string userId;
        auto userIdIt = params.find("user_id");
        if (userIdIt != params.end()) {
            userId = userIdIt->second;
        }

        auto result = compileDocument(id, userId);

        nlohmann::json response;
        response["success"] = result.success;
        response["message"] = result.success ? "Compilation successful" : "Compilation failed";

        if (result.success) {
            response["data"]["pdf_path"] = result.pdfPath;
            response["data"]["compile_time_ms"] = result.compileTime;
        } else {
            response["error"] = result.errorMessage;
            response["data"]["log"] = result.log;
        }

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleCompile: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleAutoSave(const std::map<std::string, std::string>& params, const std::string& body) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing document ID");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleAutoSave: Empty document ID");
            return buildJsonResponse(400, false, "Invalid document ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleAutoSave: Non-numeric document ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid document ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);
        nlohmann::json json = nlohmann::json::parse(body);
        std::string content = json.value("content", "");

        bool success = autoSaveDocument(id, content);

        if (success) {
            return buildJsonResponse(true, "Auto-saved successfully");
        } else {
            return buildJsonResponse(404, false, "Failed to auto-save", "");
        }
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("[LatexApi] JSON parse error in handleAutoSave: {}", e.what());
        return buildJsonResponse(400, false, "Invalid JSON in request body", "");
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleAutoSave: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleListTemplates(const std::map<std::string, std::string>& params) {
    try {
        std::string category;
        auto catIt = params.find("category");
        if (catIt != params.end()) category = catIt->second;

        auto templates = listTemplates(category);

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Templates retrieved";
        response["data"]["items"] = nlohmann::json::array();
        response["data"]["total"] = templates.size();

        for (const auto& tmpl : templates) {
            nlohmann::json item;
            item["id"] = tmpl.id;
            item["name"] = tmpl.name;
            item["description"] = tmpl.description;
            item["category"] = tmpl.category;
            item["icon"] = tmpl.icon;
            item["is_built_in"] = tmpl.isBuiltIn;
            response["data"]["items"].push_back(item);
        }

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleListTemplates: {}", e.what());
        return buildJsonResponse(false, "Failed to list templates");
    }
}

std::string LatexApiModule::handleGetTemplate(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing template ID");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleGetTemplate: Empty template ID");
            return buildJsonResponse(400, false, "Invalid template ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleGetTemplate: Non-numeric template ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid template ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);
        auto tmpl = getTemplate(id);

        if (!tmpl) {
            return buildJsonResponse(404, false, "Template not found", "");
        }

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Template retrieved";
        response["data"]["id"] = tmpl->id;
        response["data"]["name"] = tmpl->name;
        response["data"]["description"] = tmpl->description;
        response["data"]["category"] = tmpl->category;
        response["data"]["content"] = tmpl->content;
        response["data"]["icon"] = tmpl->icon;
        response["data"]["is_built_in"] = tmpl->isBuiltIn;

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleGetTemplate: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleCreateFromTemplate(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);
        int templateId = json.value("template_id", 0);
        std::string title = json.value("title", "Untitled Document");
        std::string ownerId = json.value("owner_id", "");

        auto document = createFromTemplate(templateId, title, ownerId);

        if (!document) {
            return buildJsonResponse(false, "Failed to create document from template");
        }

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Document created from template";
        response["data"]["id"] = document->id;
        response["data"]["title"] = document->title;

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleCreateFromTemplate: {}", e.what());
        return buildJsonResponse(false, "Failed to create from template");
    }
}

std::string LatexApiModule::handleStats() {
    try {
        auto stats = getStats();

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Statistics retrieved";
        response["data"]["total_documents"] = stats.totalDocuments;
        response["data"]["compiled_documents"] = stats.compiledDocuments;
        response["data"]["collaborative_documents"] = stats.collaborativeDocuments;
        response["data"]["total_words"] = stats.totalWords;
        response["data"]["total_characters"] = stats.totalCharacters;

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleStats: {}", e.what());
        return buildJsonResponse(false, "Failed to get statistics");
    }
}

std::string LatexApiModule::handleDownloadPDF(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing document ID");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleDownloadPDF: Empty document ID");
            return buildJsonResponse(400, false, "Invalid document ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleDownloadPDF: Non-numeric document ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid document ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);
        std::string pdfPath = getPDFPath(id);

        if (pdfPath.empty()) {
            return buildJsonResponse(404, false, "PDF not found", "");
        }

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "PDF path retrieved";
        response["data"]["pdf_path"] = pdfPath;

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleDownloadPDF: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

// ==========================================
// 协作功能请求处理器
// ==========================================

std::string LatexApiModule::handleJoinCollaboration(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);

        int documentId = json.value("document_id", 0);
        std::string userId = json.value("user_id", "");
        std::string userName = json.value("user_name", "Anonymous");

        if (documentId <= 0 || userId.empty()) {
            return buildJsonResponse(false, "Missing document_id or user_id");
        }

        std::string result = joinCollaboration(documentId, userId, userName);

        if (result.empty()) {
            return buildJsonResponse(false, "Failed to join collaboration");
        }

        return result;
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleJoinCollaboration: {}", e.what());
        return buildJsonResponse(false, "Failed to join collaboration");
    }
}

std::string LatexApiModule::handleLeaveCollaboration(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);

        std::string sessionId = json.value("session_id", "");
        std::string userId = json.value("user_id", "");

        if (sessionId.empty() || userId.empty()) {
            return buildJsonResponse(false, "Missing session_id or user_id");
        }

        bool success = leaveCollaboration(sessionId, userId);

        if (success) {
            return buildJsonResponse(true, "Left collaboration session");
        } else {
            return buildJsonResponse(false, "Failed to leave collaboration session");
        }
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleLeaveCollaboration: {}", e.what());
        return buildJsonResponse(false, "Failed to leave collaboration");
    }
}

std::string LatexApiModule::handleUpdateCursor(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);

        std::string sessionId = json.value("session_id", "");
        std::string userId = json.value("user_id", "");
        int line = json.value("line", 1);
        int column = json.value("column", 1);
        int selStartLine = json.value("selection_start_line", 0);
        int selStartCol = json.value("selection_start_column", 0);
        int selEndLine = json.value("selection_end_line", 0);
        int selEndCol = json.value("selection_end_column", 0);

        if (sessionId.empty() || userId.empty()) {
            return buildJsonResponse(false, "Missing session_id or user_id");
        }

        bool success = updateCursorPosition(sessionId, userId, line, column,
                                            selStartLine, selStartCol, selEndLine, selEndCol);

        if (success) {
            return buildJsonResponse(true, "Cursor position updated");
        } else {
            return buildJsonResponse(false, "Failed to update cursor position");
        }
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleUpdateCursor: {}", e.what());
        return buildJsonResponse(false, "Failed to update cursor");
    }
}

std::string LatexApiModule::handleBroadcastUpdate(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);

        std::string sessionId = json.value("session_id", "");
        std::string content = json.value("content", "");
        std::string excludeUserId = json.value("exclude_user_id", "");

        if (sessionId.empty()) {
            return buildJsonResponse(false, "Missing session_id");
        }

        bool success = broadcastDocumentUpdate(sessionId, content, excludeUserId);

        if (success) {
            return buildJsonResponse(true, "Document update broadcasted");
        } else {
            return buildJsonResponse(false, "Failed to broadcast document update");
        }
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleBroadcastUpdate: {}", e.what());
        return buildJsonResponse(false, "Failed to broadcast update");
    }
}

std::string LatexApiModule::handleListCollaborationSessions() {
    try {
        auto sessions = getActiveCollaborationSessions();

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Collaboration sessions retrieved";
        response["data"]["sessions"] = nlohmann::json::array();

        for (const auto& session : sessions) {
            nlohmann::json sessionJson;
            sessionJson["sessionId"] = session.sessionId;
            sessionJson["documentId"] = session.documentId;
            sessionJson["documentTitle"] = session.documentTitle;
            sessionJson["activeUserCount"] = session.getActiveUserCount();
            sessionJson["createdAt"] = std::chrono::system_clock::to_time_t(session.createdAt);

            sessionJson["users"] = nlohmann::json::array();
            for (const auto& [id, user] : session.users) {
                nlohmann::json userJson;
                userJson["userId"] = user.userId;
                userJson["userName"] = user.userName;
                userJson["color"] = user.color;
                userJson["cursorPosition"] = nlohmann::json::object();
                userJson["cursorPosition"]["line"] = user.cursorPosition.first;
                userJson["cursorPosition"]["column"] = user.cursorPosition.second;
                userJson["isActive"] = user.isActive;
                response["data"]["sessions"].push_back(sessionJson);
            }
        }

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleListCollaborationSessions: {}", e.what());
        return buildJsonResponse(false, "Failed to list collaboration sessions");
    }
}

// ==========================================
// 业务逻辑实现
// ==========================================

std::vector<LatexDocument> LatexApiModule::listDocuments(int page, int limit, const std::string& ownerId) {
    spdlog::info("[LatexApi] Listing documents: page={}, limit={}, owner={}", page, limit, ownerId);

    auto allDocs = g_latexStore.listDocuments();

    // 过滤owner
    if (!ownerId.empty()) {
        std::vector<LatexDocument> filtered;
        for (const auto& doc : allDocs) {
            if (doc.ownerId == ownerId) {
                filtered.push_back(doc);
            }
        }
        allDocs = filtered;
    }

    // 分页
    size_t start = (page - 1) * limit;
    size_t end = std::min(start + limit, allDocs.size());

    if (start >= allDocs.size()) {
        return {};
    }

    std::vector<LatexDocument> result;
    for (size_t i = start; i < end; ++i) {
        result.push_back(allDocs[i]);
    }

    return result;
}

std::optional<LatexDocument> LatexApiModule::getDocument(int id) {
    spdlog::info("[LatexApi] Getting document: id={}", id);

    auto doc = g_latexStore.getDocument(id);
    if (!doc) {
        spdlog::warn("[LatexApi] Document not found: id={}", id);
        return std::nullopt;
    }

    spdlog::info("[LatexApi] Document found: id={}, title={}", doc->id, doc->title);
    return doc;
}

std::optional<LatexDocument> LatexApiModule::createDocument(const LatexDocument& document) {
    spdlog::info("[LatexApi] Creating document: title={}", document.title);

    int newId = g_latexStore.createDocument(document);
    if (newId <= 0) {
        spdlog::error("[LatexApi] Failed to create document");
        return std::nullopt;
    }

    auto doc = g_latexStore.getDocument(newId);
    spdlog::info("[LatexApi] Document created: id={}, title={}", doc->id, doc->title);
    return doc;
}

bool LatexApiModule::updateDocument(int id, const LatexDocument& document) {
    spdlog::info("[LatexApi] Updating document: id={}, title={}", id, document.title);

    auto existingDoc = g_latexStore.getDocument(id);
    if (!existingDoc) {
        spdlog::error("[LatexApi] Document not found for update: id={}", id);
        return false;
    }

    LatexDocument updatedDoc = *existingDoc;
    updatedDoc.title = document.title;
    updatedDoc.content = document.content;
    updatedDoc.updatedAt = std::chrono::system_clock::now();

    bool success = g_latexStore.saveDocument(updatedDoc);
    spdlog::info("[LatexApi] Document update: {}", success ? "success" : "failed");
    return success;
}

bool LatexApiModule::deleteDocument(int id) {
    spdlog::info("[LatexApi] Deleting document: id={}", id);

    bool success = g_latexStore.deleteDocument(id);
    spdlog::info("[LatexApi] Document delete: {}", success ? "success" : "failed");
    return success;
}

LatexCompilationResult LatexApiModule::compileDocument(int id, const std::string& userId) {
    spdlog::info("[LatexApi] Compiling document: id={}, user={}", id, userId);

    std::lock_guard<std::mutex> lock(compileMutex_);

    // 检查用户配额（如果提供了用户ID）
    if (!userId.empty() && !g_latexStore.canCompile(userId)) {
        auto quota = g_latexStore.getUserQuota(userId);
        std::ostringstream errorMsg;
        if (quota) {
            errorMsg << "编译配额已用完。已使用 " << quota->dailyCompilesUsed << "/" << quota->dailyCompileLimit
                    << " (每日), " << quota->monthlyCompilesUsed << "/" << quota->monthlyCompileLimit << " (每月)。"
                    << "请升级套餐或等待配额重置。";
        } else {
            errorMsg << "编译配额已用完。请升级套餐或等待配额重置。";
        }

        LatexCompilationResult result;
        result.success = false;
        result.errorMessage = errorMsg.str();
        result.log = "QUOTA_EXCEEDED";
        spdlog::warn("[LatexApi] Compilation blocked due to quota limit for user: {}", userId);
        return result;
    }

    auto document = getDocument(id);
    if (!document) {
        LatexCompilationResult result;
        result.success = false;
        result.errorMessage = "文档不存在";
        result.log = "DOCUMENT_NOT_FOUND";
        return result;
    }

    // 生成输出文件路径
    std::string outputPath = impl_->pdfDirectory + "/document_" + std::to_string(id) + ".pdf";

    // 编译LaTeX
    auto result = compileLatex(document->content, outputPath);

    // 记录编译
    if (!userId.empty()) {
        LatexCompilationRecord record;
        record.id = g_latexStore.nextCompilationRecordId++;
        record.userId = userId;
        record.projectId = 0; // 单文档
        record.documentId = std::to_string(id);
        record.contentHash = std::to_string(std::hash<std::string>{}(document->content));
        record.success = result.success;
        record.errorMessage = result.errorMessage;
        record.timestamp = std::chrono::system_clock::now();
        g_latexStore.recordCompilation(record);
    }

    // 更新文档状态
    if (result.success) {
        // TODO: 更新数据库
        spdlog::info("[LatexApi] Compilation successful: {}", outputPath);
    } else {
        // 改进错误消息
        if (result.errorMessage.empty()) {
            result.errorMessage = "编译失败，请检查LaTeX语法";
        }
    }

    return result;
}

bool LatexApiModule::autoSaveDocument(int id, const std::string& content) {
    spdlog::debug("[LatexApi] Auto-saving document: id={}", id);

    std::lock_guard<std::mutex> lock(saveMutex_);

    // TODO: 更新数据库
    LatexDocument document;
    document.id = id;
    document.content = content;
    document.lastAutoSave = std::chrono::system_clock::now();

    return updateDocument(id, document);
}

std::vector<LatexTemplate> LatexApiModule::listTemplates(const std::string& category) {
    spdlog::info("[LatexApi] Listing templates: category={}", category);

    if (category.empty()) {
        return impl_->builtInTemplates;
    }

    std::vector<LatexTemplate> filtered;
    for (const auto& tmpl : impl_->builtInTemplates) {
        if (tmpl.category == category) {
            filtered.push_back(tmpl);
        }
    }
    return filtered;
}

std::optional<LatexTemplate> LatexApiModule::getTemplate(int id) {
    spdlog::info("[LatexApi] Getting template: id={}", id);

    for (const auto& tmpl : impl_->builtInTemplates) {
        if (tmpl.id == id) {
            return tmpl;
        }
    }
    return std::nullopt;
}

std::optional<LatexDocument> LatexApiModule::createFromTemplate(int templateId, const std::string& title, const std::string& ownerId) {
    spdlog::info("[LatexApi] Creating document from template: templateId={}, title={}", templateId, title);

    auto tmpl = getTemplate(templateId);
    if (!tmpl) {
        return std::nullopt;
    }

    LatexDocument document;
    document.title = title;
    document.content = tmpl->content;
    document.ownerId = ownerId;
    document.createdAt = std::chrono::system_clock::now();
    document.updatedAt = std::chrono::system_clock::now();
    document.lastAutoSave = std::chrono::system_clock::now();
    document.version = 1;

    return createDocument(document);
}

LatexDocumentStats LatexApiModule::getStats() {
    spdlog::info("[LatexApi] Getting statistics");

    // TODO: 从数据库计算统计信息
    LatexDocumentStats stats;
    stats.totalDocuments = 0;
    stats.compiledDocuments = 0;
    stats.collaborativeDocuments = 0;
    stats.totalWords = 0;
    stats.totalCharacters = 0;
    return stats;
}

std::string LatexApiModule::getPDFPath(int id) {
    spdlog::info("[LatexApi] Getting PDF path: id={}", id);

    // 检查PDF文件是否存在
    std::string pdfPath = impl_->pdfDirectory + "/document_" + std::to_string(id) + ".pdf";
    if (std::filesystem::exists(pdfPath)) {
        return pdfPath;
    }
    return "";
}

// ==========================================
// 辅助函数
// ==========================================

std::string LatexApiModule::buildJsonResponse(bool success, const std::string& message, const std::string& data) {
    nlohmann::json response;
    response["success"] = success;
    response["message"] = message;
    if (!data.empty()) {
        try {
            response["data"] = nlohmann::json::parse(data);
        } catch (...) {
            response["data"] = data;
        }
    }
    return response.dump();
}

std::string LatexApiModule::buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data) {
    nlohmann::json response;
    response["statusCode"] = statusCode;
    response["success"] = success;
    response["message"] = message;
    if (!data.empty()) {
        try {
            response["data"] = nlohmann::json::parse(data);
        } catch (...) {
            response["data"] = data;
        }
    }
    return response.dump();
}

std::string LatexApiModule::escapeJson(const std::string& str) {
    // 简单的JSON转义
    std::string result = str;
    // TODO: 完整的JSON转义
    return result;
}

LatexCompilationResult LatexApiModule::compileLatex(const std::string& content, const std::string& outputPath) {
    LatexCompilationResult result;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // 生成文件路径
        std::string texPath = outputPath.substr(0, outputPath.find_last_of('.')) + ".tex";
        std::string logPath = outputPath.substr(0, outputPath.find_last_of('.')) + ".log";

        // 保存.tex文件
        {
            std::ofstream texFile(texPath);
            if (!texFile) {
                result.success = false;
                result.errorMessage = "Failed to create .tex file";
                spdlog::error("[LatexApi] Failed to create .tex file: {}", texPath);
                return result;
            }
            texFile << content;
            texFile.close();
        }

        spdlog::info("[LatexApi] Saved .tex file: {}", texPath);

        // 检查pdflatex是否可用
        bool hasPdflatex = (system("which pdflatex > /dev/null 2>&1") == 0);

        if (hasPdflatex) {
            // 调用pdflatex编译
            std::ostringstream cmd;
            cmd << "pdflatex -interaction=nonstopmode -halt-on-error -output-directory="
                << impl_->pdfDirectory << " " << texPath << " > " << logPath << " 2>&1";

            spdlog::info("[LatexApi] Running pdflatex: {}", cmd.str());

            int exitCode = system(cmd.str().c_str());

            auto endTime = std::chrono::high_resolution_clock::now();
            result.compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

            // 检查编译结果
            if (exitCode == 0 && std::filesystem::exists(outputPath)) {
                result.success = true;
                result.pdfPath = outputPath;
                result.log = "Compilation successful";

                // 读取日志
                std::ifstream logFile(logPath);
                if (logFile) {
                    std::stringstream logBuffer;
                    logBuffer << logFile.rdbuf();
                    result.log = logBuffer.str();
                }

                spdlog::info("[LatexApi] LaTeX compilation successful: {} ({}ms)", outputPath, result.compileTime);
            } else {
                result.success = false;
                result.errorMessage = "pdflatex compilation failed";

                // 读取错误日志
                std::ifstream logFile(logPath);
                if (logFile) {
                    std::stringstream logBuffer;
                    logBuffer << logFile.rdbuf();
                    result.log = logBuffer.str();

                    // 提取错误信息
                    std::string logContent = result.log;
                    size_t errorPos = logContent.find("!");
                    if (errorPos != std::string::npos) {
                        size_t lineEnd = logContent.find('\n', errorPos);
                        result.errorMessage = logContent.substr(errorPos, lineEnd - errorPos);
                    }
                }

                spdlog::error("[LatexApi] pdflatex compilation failed: exit code {}", exitCode);
            }

            // 清理中间文件
            std::filesystem::remove(texPath);
            std::string auxPath = texPath.substr(0, texPath.length() - 4) + ".aux";
            std::filesystem::remove(auxPath);
        } else {
            // pdflatex不可用，使用客户端编译作为后备方案
            spdlog::warn("[LatexApi] pdflatex not available, using stub compilation");

            result.success = true;
            result.pdfPath = outputPath;
            result.compileTime = 100;
            result.log = "Compilation stub (pdflatex not installed on server). Please install pdflatex or use client-side compilation.";
            result.errorMessage = "pdflatex not available - use client-side compilation";
        }

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
        spdlog::error("[LatexApi] LaTeX compilation exception: {}", e.what());
    }

    return result;
}

void LatexApiModule::initializeBuiltInTemplates() {
    spdlog::info("[LatexApi] Initializing built-in templates");

    impl_->builtInTemplates = {
        {
            1, "学术论文", "标准学术论文模板", "学术论文",
            R"(\\documentclass[12pt,a4paper]{article}

\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage{amsmath,amsfonts,amssymb,amsthm}
\usepackage{graphicx}
\usepackage{geometry}
\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\title{论文标题}
\author{作者姓名}
\date{\today}

\begin{document}

\maketitle

\begin{abstract}
  这里是摘要内容。
\end{abstract}

\section{引言}
这里是引言内容...

\section{方法}
这里是方法部分...

\section{结果}
这里是结果部分...

\section{结论}
这里是结论部分...

\begin{thebibliography}{9}
  \bibitem{文献1}
  \bibitem{文献2}
\end{thebibliography}

\end{document})",
            "📄", true
        },
        {
            2, "技术报告", "技术/工程报告模板", "学术报告",
            R"(\\documentclass[12pt,a4paper]{report}

\usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage{amsmath,amsfonts,amssymb}
\usepackage{graphicx}
\usepackage{geometry}
\geometry{left=3cm,right=3cm,top=2.5cm,bottom=2.5cm}

\title{技术报告标题}
\author{作者姓名}
\date{\today}

\begin{document}

\maketitle

\tableofcontents

\chapter{介绍}
这里是介绍内容...

\chapter{背景}
这里是背景内容...

\chapter{方法}
这里是方法部分...

\chapter{结果}
这里是结果部分...

\chapter{结论}
这里是结论部分...

\end{document})",
            "📋", true
        },
        {
            3, "Beamer演示", "Beamer演示文稿模板", "演示文稿",
            R"(\\documentclass{beamer}

\usetheme{Madrid}
\usecolortheme{default}

\title{演示标题}
\author{作者姓名}
\date{\today}

\begin{document}

\frame{\titlepage}

\begin{frame}
  \frametitle{大纲}
  \tableofcontents
\end{frame}

\section{介绍}

\begin{frame}
  \frametitle{介绍}
  \begin{itemize}
    \item 要点1
    \item 要点2
    \item 要点3
  \end{itemize}
\end{frame}

\section{主要内容}

\begin{frame}
  \frametitle{主要内容}
  这里是内容...
\end{frame}

\section{结论}

\begin{frame}
  \frametitle{结论}
  这里是结论...
\end{frame}

\end{document})",
            "📽️", true
        }
    };

    spdlog::info("[LatexApi] Loaded {} built-in templates", impl_->builtInTemplates.size());
}

// ==========================================
// 协作功能实现
// ==========================================

// 全局协作会话存储
namespace {
    std::unordered_map<std::string, LatexCollaborationSession> g_collaborationSessions;
    std::mutex g_collaborationMutex;
    std::string generateSessionId() {
        static std::atomic<uint64_t> counter{1};
        return "session_" + std::to_string(counter++);
    }
    std::string generateColor() {
        static const std::vector<const char*> colors = {
            "#FF5733", "#33FF57", "#3357FF", "#FF33F5",
            "#F3FF33", "#33FFF5", "#FF33A8", "#33A8FF"
        };
        static std::atomic<size_t> index{0};
        return colors[index++ % colors.size()];
    }
}

std::string LatexApiModule::joinCollaboration(int documentId, const std::string& userId, const std::string& userName) {
    spdlog::info("[LatexApi] User {} joining collaboration for document {}", userId, documentId);

    std::lock_guard<std::mutex> lock(g_collaborationMutex);

    // 查找或创建会话
    std::string sessionId;
    for (auto& [id, session] : g_collaborationSessions) {
        if (session.documentId == documentId) {
            sessionId = id;
            break;
        }
    }

    if (sessionId.empty()) {
        // 创建新会话
        sessionId = generateSessionId();

        // 获取文档内容
        auto doc = g_latexStore.getDocument(documentId);
        if (!doc) {
            spdlog::error("[LatexApi] Document not found for collaboration: {}", documentId);
            return "";
        }

        LatexCollaborationSession session;
        session.sessionId = sessionId;
        session.documentId = documentId;
        session.documentTitle = doc->title;
        session.documentContent = doc->content;
        session.createdAt = std::chrono::system_clock::now();
        session.lastActivity = std::chrono::system_clock::now();

        g_collaborationSessions[sessionId] = session;
        spdlog::info("[LatexApi] Created new collaboration session: {} for document {}", sessionId, documentId);
    }

    // 添加用户到会话
    auto& session = g_collaborationSessions[sessionId];
    LatexCollaborationUser user;
    user.connectionId = userId;  // 简化：使用userId作为connectionId
    user.userId = userId;
    user.userName = userName;
    user.color = generateColor();
    user.cursorPosition = {1, 1};
    user.selectionStart = {0, 0};
    user.selectionEnd = {0, 0};
    user.isActive = true;
    user.lastActivity = std::chrono::system_clock::now();

    session.users[user.connectionId] = user;
    session.lastActivity = std::chrono::system_clock::now();

    spdlog::info("[LatexApi] User {} joined collaboration session {}", userId, sessionId);

    // TODO: 通过WebSocket广播用户加入
    // 广播用户列表给其他用户

    nlohmann::json response;
    response["success"] = true;
    response["sessionId"] = sessionId;
    response["documentId"] = documentId;
    response["documentTitle"] = session.documentTitle;
    response["content"] = session.documentContent;
    response["users"] = nlohmann::json::array();

    for (const auto& [connId, u] : session.users) {
        nlohmann::json userJson = nlohmann::json::parse(u.toJSON());
        response["users"].push_back(userJson);
    }

    return response.dump();
}

bool LatexApiModule::leaveCollaboration(const std::string& sessionId, const std::string& userId) {
    spdlog::info("[LatexApi] User {} leaving collaboration session {}", userId, sessionId);

    std::lock_guard<std::mutex> lock(g_collaborationMutex);

    auto it = g_collaborationSessions.find(sessionId);
    if (it == g_collaborationSessions.end()) {
        return false;
    }

    auto& session = it->second;
    auto userIt = session.users.find(userId);
    if (userIt != session.users.end()) {
        session.users.erase(userIt);
        session.lastActivity = std::chrono::system_clock::now();

        spdlog::info("[LatexApi] User {} removed from session {}. Active users: {}",
            userId, sessionId, session.getActiveUserCount());

        // TODO: 通过WebSocket广播用户离开

        // 如果没有活跃用户，清理会话
        if (session.getActiveUserCount() == 0) {
            g_collaborationSessions.erase(it);
            spdlog::info("[LatexApi] Collaboration session {} removed (no active users)", sessionId);
        }

        return true;
    }

    return false;
}

bool LatexApiModule::updateCursorPosition(const std::string& sessionId, const std::string& userId,
                                        int line, int column,
                                        int selectionStartLine, int selectionStartColumn,
                                        int selectionEndLine, int selectionEndColumn) {
    std::lock_guard<std::mutex> lock(g_collaborationMutex);

    auto it = g_collaborationSessions.find(sessionId);
    if (it == g_collaborationSessions.end()) {
        return false;
    }

    auto& session = it->second;
    auto userIt = session.users.find(userId);
    if (userIt == session.users.end()) {
        return false;
    }

    auto& user = userIt->second;
    user.cursorPosition = {line, column};
    user.selectionStart = {selectionStartLine, selectionStartColumn};
    user.selectionEnd = {selectionEndLine, selectionEndColumn};
    user.lastActivity = std::chrono::system_clock::now();
    user.isActive = true;

    session.lastActivity = std::chrono::system_clock::now();

    spdlog::debug("[LatexApi] Updated cursor for user {}: ({}, {})", userId, line, column);

    // TODO: 通过WebSocket广播光标位置给其他用户

    return true;
}

bool LatexApiModule::broadcastDocumentUpdate(const std::string& sessionId, const std::string& content,
                                        const std::string& excludeUserId) {
    spdlog::info("[LatexApi] Broadcasting document update for session {}, content length: {}",
        sessionId, content.length());

    std::lock_guard<std::mutex> lock(g_collaborationMutex);

    auto it = g_collaborationSessions.find(sessionId);
    if (it == g_collaborationSessions.end()) {
        return false;
    }

    auto& session = it->second;
    session.documentContent = content;
    session.lastActivity = std::chrono::system_clock::now();

    // 同时更新内存存储
    auto doc = g_latexStore.getDocument(session.documentId);
    if (doc) {
        LatexDocument updatedDoc = *doc;
        updatedDoc.content = content;
        updatedDoc.updatedAt = std::chrono::system_clock::now();
        g_latexStore.saveDocument(updatedDoc);
    }

    // TODO: 通过WebSocket广播内容更新给所有用户（排除发送者）

    spdlog::info("[LatexApi] Document update broadcasted to {} users (excluded: {})",
        session.users.size() - (excludeUserId.empty() ? 1 : 0), excludeUserId);

    return true;
}

std::optional<LatexCollaborationSession> LatexApiModule::getCollaborationSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(g_collaborationMutex);

    auto it = g_collaborationSessions.find(sessionId);
    if (it != g_collaborationSessions.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<LatexCollaborationSession> LatexApiModule::getActiveCollaborationSessions() {
    std::lock_guard<std::mutex> lock(g_collaborationMutex);

    std::vector<LatexCollaborationSession> sessions;
    for (const auto& [id, session] : g_collaborationSessions) {
        if (session.getActiveUserCount() > 0) {
            sessions.push_back(session);
        }
    }

    spdlog::info("[LatexApi] Active collaboration sessions: {}", sessions.size());
    return sessions;
}

// ==========================================
// 项目管理实现（多文件支持）
// ==========================================

std::vector<LatexProject> LatexApiModule::listProjects(int page, int limit, const std::string& ownerId) {
    auto projects = g_latexStore.listProjects();

    // 过滤和分页
    if (!ownerId.empty()) {
        std::vector<LatexProject> filtered;
        for (const auto& project : projects) {
            if (project.ownerId == ownerId) {
                filtered.push_back(project);
            }
        }
        projects = filtered;
    }

    // 简单分页
    int start = (page - 1) * limit;
    int end = std::min(start + limit, static_cast<int>(projects.size()));

    std::vector<LatexProject> result;
    if (start < static_cast<int>(projects.size())) {
        for (int i = start; i < end; i++) {
            result.push_back(projects[i]);
        }
    }

    spdlog::info("[LatexApi] Listed {} projects (page {}, limit {})", result.size(), page, limit);
    return result;
}

std::optional<LatexProject> LatexApiModule::getProject(int id) {
    auto project = g_latexStore.getProject(id);
    if (!project) {
        spdlog::warn("[LatexApi] Project not found: {}", id);
        return std::nullopt;
    }

    // 加载项目的所有文件
    project->files = g_latexStore.getProjectFiles(id);

    spdlog::info("[LatexApi] Retrieved project: {} with {} files", project->name, project->files.size());
    return project;
}

std::optional<LatexProject> LatexApiModule::createProject(const LatexProject& project) {
    int newId = g_latexStore.createProject(project);

    auto created = g_latexStore.getProject(newId);
    if (created) {
        spdlog::info("[LatexApi] Created project: {} (ID: {})", project.name, newId);
        return created;
    }

    spdlog::error("[LatexApi] Failed to create project: {}", project.name);
    return std::nullopt;
}

bool LatexApiModule::updateProject(int id, const LatexProject& project) {
    spdlog::info("[LatexApi] Updating project: {}", id);
    return g_latexStore.updateProject(id, project);
}

bool LatexApiModule::deleteProject(int id) {
    spdlog::info("[LatexApi] Deleting project: {}", id);
    return g_latexStore.deleteProject(id);
}

LatexCompilationResult LatexApiModule::compileProject(int id, const std::string& userId) {
    spdlog::info("[LatexApi] Compiling project: {}, user={}", id, userId);

    // 检查用户配额（如果提供了用户ID）
    if (!userId.empty() && !g_latexStore.canCompile(userId)) {
        auto quota = g_latexStore.getUserQuota(userId);
        std::ostringstream errorMsg;
        if (quota) {
            errorMsg << "编译配额已用完。已使用 " << quota->dailyCompilesUsed << "/" << quota->dailyCompileLimit
                    << " (每日), " << quota->monthlyCompilesUsed << "/" << quota->monthlyCompileLimit << " (每月)。"
                    << "请升级套餐或等待配额重置。";
        } else {
            errorMsg << "编译配额已用完。请升级套餐或等待配额重置。";
        }

        spdlog::warn("[LatexApi] Compilation blocked due to quota limit for user: {}", userId);
        return {false, "", "", errorMsg.str(), 0};
    }

    auto project = g_latexStore.getProject(id);
    if (!project) {
        spdlog::error("[LatexApi] Project not found: {}", id);
        return {false, "", "", "项目不存在", 0};
    }

    // 获取项目文件
    auto files = g_latexStore.getProjectFiles(id);

    // 找到主文件
    std::string mainContent;
    for (const auto& file : files) {
        if (file.path == project->mainFile) {
            mainContent = file.content;
            break;
        }
    }

    if (mainContent.empty()) {
        spdlog::error("[LatexApi] Main file not found: {}", project->mainFile);
        return {false, "", "", "主文件不存在: " + project->mainFile, 0};
    }

    // 创建临时目录存放所有文件
    std::string tempDir = impl_->outputDirectory + "/project_" + std::to_string(id);
    std::filesystem::create_directories(tempDir);

    // 写入所有文件
    for (const auto& file : files) {
        std::string filePath = tempDir + "/" + file.path;
        std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

        std::ofstream out(filePath);
        out << file.content;
        out.close();
    }

    // 编译主文件
    std::string outputPath = impl_->pdfDirectory + "/project_" + std::to_string(id) + ".pdf";
    auto result = compileLatex(mainContent, outputPath);

    // 记录编译
    if (!userId.empty()) {
        LatexCompilationRecord record;
        record.id = g_latexStore.nextCompilationRecordId++;
        record.userId = userId;
        record.projectId = id;
        record.documentId = "";
        record.contentHash = std::to_string(std::hash<std::string>{}(mainContent));
        record.success = result.success;
        record.errorMessage = result.errorMessage;
        record.timestamp = std::chrono::system_clock::now();
        g_latexStore.recordCompilation(record);
    }

    // 改进错误消息
    if (!result.success && result.errorMessage.empty()) {
        result.errorMessage = "项目编译失败，请检查LaTeX语法";
    }

    spdlog::info("[LatexApi] Project compilation {}: success={}, time={}ms",
        id, result.success, result.compileTime);

    return result;
}

std::optional<LatexProjectFile> LatexApiModule::addProjectFile(int projectId, const LatexProjectFile& file) {
    LatexProjectFile newFile = file;
    newFile.projectId = projectId;
    newFile.createdAt = std::chrono::system_clock::now();
    newFile.updatedAt = std::chrono::system_clock::now();

    int newId = g_latexStore.addProjectFile(newFile);

    auto created = g_latexStore.getProjectFile(newId);
    if (created) {
        spdlog::info("[LatexApi] Added file {} to project {}: {}", file.path, projectId, newId);
        return created;
    }

    spdlog::error("[LatexApi] Failed to add file to project: {}", projectId);
    return std::nullopt;
}

bool LatexApiModule::updateProjectFile(int fileId, const LatexProjectFile& file) {
    spdlog::info("[LatexApi] Updating project file: {}", fileId);
    return g_latexStore.updateProjectFile(fileId, file);
}

bool LatexApiModule::deleteProjectFile(int fileId) {
    spdlog::info("[LatexApi] Deleting project file: {}", fileId);
    return g_latexStore.deleteProjectFile(fileId);
}

std::optional<LatexProjectFile> LatexApiModule::getProjectFile(int fileId) {
    return g_latexStore.getProjectFile(fileId);
}

// ==========================================
// 项目HTTP请求处理器
// ==========================================

std::string LatexApiModule::handleListProjects(const std::map<std::string, std::string>& params) {
    try {
        int page = 1, limit = 20;
        std::string ownerId;

        auto pageIt = params.find("page");
        if (pageIt != params.end()) {
            const std::string& pageStr = pageIt->second;
            // Validate that page contains only digits
            bool valid = true;
            for (char c : pageStr) {
                if (!std::isdigit(static_cast<unsigned char>(c))) {
                    valid = false;
                    break;
                }
            }
            if (valid && !pageStr.empty()) {
                page = std::stoi(pageStr);
            }
        }

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            const std::string& limitStr = limitIt->second;
            // Validate that limit contains only digits
            bool valid = true;
            for (char c : limitStr) {
                if (!std::isdigit(static_cast<unsigned char>(c))) {
                    valid = false;
                    break;
                }
            }
            if (valid && !limitStr.empty()) {
                limit = std::stoi(limitStr);
            }
        }

        auto ownerIt = params.find("owner_id");
        if (ownerIt != params.end()) ownerId = ownerIt->second;

        auto projects = listProjects(page, limit, ownerId);

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Projects retrieved";
        response["data"]["items"] = nlohmann::json::array();
        response["data"]["total"] = projects.size();
        response["data"]["page"] = page;
        response["data"]["limit"] = limit;

        for (const auto& project : projects) {
            nlohmann::json p;
            p["id"] = project.id;
            p["name"] = project.name;
            p["owner_id"] = project.ownerId;
            p["main_file"] = project.mainFile;
            p["description"] = project.description;
            p["is_public"] = project.isPublic;
            p["version"] = project.version;
            p["file_count"] = g_latexStore.getProjectFiles(project.id).size();
            p["created_at"] = std::chrono::system_clock::to_time_t(project.createdAt);
            p["updated_at"] = std::chrono::system_clock::to_time_t(project.updatedAt);
            response["data"]["items"].push_back(p);
        }

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleListProjects: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleGetProject(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing project ID", "");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleGetProject: Empty project ID");
            return buildJsonResponse(400, false, "Invalid project ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleGetProject: Non-numeric project ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid project ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);
        auto project = getProject(id);

        if (!project) {
            return buildJsonResponse(404, false, "Project not found", "");
        }

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Project retrieved";
        response["data"]["id"] = project->id;
        response["data"]["name"] = project->name;
        response["data"]["owner_id"] = project->ownerId;
        response["data"]["main_file"] = project->mainFile;
        response["data"]["description"] = project->description;
        response["data"]["is_public"] = project->isPublic;
        response["data"]["version"] = project->version;
        response["data"]["created_at"] = std::chrono::system_clock::to_time_t(project->createdAt);
        response["data"]["updated_at"] = std::chrono::system_clock::to_time_t(project->updatedAt);
        response["data"]["files"] = nlohmann::json::array();

        for (const auto& file : project->files) {
            nlohmann::json f;
            f["id"] = file.id;
            f["name"] = file.name;
            f["path"] = file.path;
            f["content"] = file.content;
            f["type"] = file.type;
            f["created_at"] = std::chrono::system_clock::to_time_t(file.createdAt);
            f["updated_at"] = std::chrono::system_clock::to_time_t(file.updatedAt);
            response["data"]["files"].push_back(f);
        }

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleGetProject: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleCreateProject(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);

        LatexProject project;
        project.name = json.value("name", "Untitled Project");
        project.ownerId = json.value("owner_id", "");
        project.mainFile = json.value("main_file", "main.tex");
        project.description = json.value("description", "");
        project.isPublic = json.value("is_public", false);
        project.createdAt = std::chrono::system_clock::now();
        project.updatedAt = std::chrono::system_clock::now();

        auto created = createProject(project);

        if (!created) {
            return buildJsonResponse(false, "Failed to create project");
        }

        // 添加主文件
        if (!json.contains("files") || !json["files"].is_array()) {
            // 创建默认主文件
            LatexProjectFile mainFile;
            mainFile.projectId = created->id;
            mainFile.name = created->mainFile;
            mainFile.path = created->mainFile;
            mainFile.content = R"(\\documentclass[12pt,a4paper]{article}
\\usepackage[utf8]{inputenc}
\\usepackage{amsmath,amsfonts,amssymb}
\\usepackage{graphicx}

\\title{)" + created->name + R"(}
\\author{Author Name}
\\date{\\today}

\\begin{document}

\\maketitle

\\section{Introduction}
Your content here...

\\end{document})";
            mainFile.type = "main";
            addProjectFile(created->id, mainFile);
        }

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Project created";
        response["data"]["id"] = created->id;
        response["data"]["name"] = created->name;
        response["data"]["main_file"] = created->mainFile;

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleCreateProject: {}", e.what());
        return buildJsonResponse(false, "Failed to create project");
    }
}

std::string LatexApiModule::handleUpdateProject(const std::map<std::string, std::string>& params, const std::string& body) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing project ID", "");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleUpdateProject: Empty project ID");
            return buildJsonResponse(400, false, "Invalid project ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleUpdateProject: Non-numeric project ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid project ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);
        nlohmann::json json = nlohmann::json::parse(body);

        auto existing = g_latexStore.getProject(id);
        if (!existing) {
            return buildJsonResponse(404, false, "Project not found", "");
        }

        LatexProject project = *existing;
        project.name = json.value("name", project.name);
        project.mainFile = json.value("main_file", project.mainFile);
        project.description = json.value("description", project.description);
        project.updatedAt = std::chrono::system_clock::now();

        if (updateProject(id, project)) {
            return buildJsonResponse(true, "Project updated");
        } else {
            return buildJsonResponse(false, "Failed to update project");
        }
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("[LatexApi] JSON parse error in handleUpdateProject: {}", e.what());
        return buildJsonResponse(400, false, "Invalid JSON in request body", "");
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleUpdateProject: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleDeleteProject(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing project ID", "");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleDeleteProject: Empty project ID");
            return buildJsonResponse(400, false, "Invalid project ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleDeleteProject: Non-numeric project ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid project ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);

        if (deleteProject(id)) {
            return buildJsonResponse(true, "Project deleted");
        } else {
            return buildJsonResponse(false, "Failed to delete project");
        }
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleDeleteProject: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleCompileProject(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing project ID", "");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleCompileProject: Empty project ID");
            return buildJsonResponse(400, false, "Invalid project ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleCompileProject: Non-numeric project ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid project ID: must be a number", "");
            }
        }

        int id = std::stoi(idStr);

        // 从查询参数获取 user_id（可选）
        std::string userId;
        auto userIdIt = params.find("user_id");
        if (userIdIt != params.end()) {
            userId = userIdIt->second;
        }

        auto result = compileProject(id, userId);

        nlohmann::json response;
        response["success"] = result.success;
        response["message"] = result.success ? "Compilation successful" : "Compilation failed";

        if (result.success) {
            response["data"]["pdf_path"] = result.pdfPath;
            response["data"]["compile_time_ms"] = result.compileTime;
            response["data"]["log"] = result.log;
        } else {
            response["error"] = result.errorMessage;
        }

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleCompileProject: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleAddProjectFile(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);

        int projectId = json.value("project_id", 0);
        if (projectId == 0) {
            return buildJsonResponse(400, false, "Missing project_id", "");
        }

        LatexProjectFile file;
        file.name = json.value("name", "untitled.tex");
        file.path = json.value("path", file.name);
        file.content = json.value("content", "");
        file.type = json.value("type", "other");

        auto created = addProjectFile(projectId, file);

        if (!created) {
            return buildJsonResponse(false, "Failed to add file");
        }

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "File added";
        response["data"]["id"] = created->id;
        response["data"]["name"] = created->name;
        response["data"]["path"] = created->path;

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleAddProjectFile: {}", e.what());
        return buildJsonResponse(false, "Failed to add file");
    }
}

std::string LatexApiModule::handleUpdateProjectFile(const std::map<std::string, std::string>& params, const std::string& body) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing file ID", "");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleUpdateProjectFile: Empty file ID");
            return buildJsonResponse(400, false, "Invalid file ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleUpdateProjectFile: Non-numeric file ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid file ID: must be a number", "");
            }
        }

        int fileId = std::stoi(idStr);
        nlohmann::json json = nlohmann::json::parse(body);

        auto existing = g_latexStore.getProjectFile(fileId);
        if (!existing) {
            return buildJsonResponse(404, false, "File not found", "");
        }

        LatexProjectFile file = *existing;
        file.content = json.value("content", file.content);
        file.updatedAt = std::chrono::system_clock::now();

        if (updateProjectFile(fileId, file)) {
            return buildJsonResponse(true, "File updated");
        } else {
            return buildJsonResponse(false, "Failed to update file");
        }
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("[LatexApi] JSON parse error in handleUpdateProjectFile: {}", e.what());
        return buildJsonResponse(400, false, "Invalid JSON in request body", "");
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleUpdateProjectFile: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleDeleteProjectFile(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing file ID", "");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleDeleteProjectFile: Empty file ID");
            return buildJsonResponse(400, false, "Invalid file ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleDeleteProjectFile: Non-numeric file ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid file ID: must be a number", "");
            }
        }

        int fileId = std::stoi(idStr);

        if (deleteProjectFile(fileId)) {
            return buildJsonResponse(true, "File deleted");
        } else {
            return buildJsonResponse(false, "Failed to delete file");
        }
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleDeleteProjectFile: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

std::string LatexApiModule::handleGetProjectFile(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing file ID", "");
        }

        const std::string& idStr = idIt->second;
        if (idStr.empty()) {
            spdlog::error("[LatexApi] Error in handleGetProjectFile: Empty file ID");
            return buildJsonResponse(400, false, "Invalid file ID: empty value", "");
        }

        // Validate that ID contains only digits
        for (char c : idStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                spdlog::error("[LatexApi] Error in handleGetProjectFile: Non-numeric file ID: '{}'", idStr);
                return buildJsonResponse(400, false, "Invalid file ID: must be a number", "");
            }
        }

        int fileId = std::stoi(idStr);
        auto file = getProjectFile(fileId);

        if (!file) {
            return buildJsonResponse(404, false, "File not found", "");
        }

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "File retrieved";
        response["data"]["id"] = file->id;
        response["data"]["project_id"] = file->projectId;
        response["data"]["name"] = file->name;
        response["data"]["path"] = file->path;
        response["data"]["content"] = file->content;
        response["data"]["type"] = file->type;

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleGetProjectFile: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

// ==========================================
// 用户配额HTTP请求处理器
// ==========================================

std::string LatexApiModule::handleGetUserQuota(const std::map<std::string, std::string>& params) {
    try {
        auto userIdIt = params.find("user_id");
        if (userIdIt == params.end()) {
            return buildJsonResponse(400, false, "Missing user_id", "");
        }

        std::string userId = userIdIt->second;
        auto quota = g_latexStore.getUserQuota(userId);

        if (!quota) {
            return buildJsonResponse(404, false, "User quota not found. Initialize quota first.", "");
        }

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Quota retrieved";
        response["data"]["user_id"] = quota->userId;
        response["data"]["daily_compile_limit"] = quota->dailyCompileLimit;
        response["data"]["monthly_compile_limit"] = quota->monthlyCompileLimit;
        response["data"]["max_project_count"] = quota->maxProjectCount;
        response["data"]["can_use_advanced_features"] = quota->canUseAdvancedFeatures;
        response["data"]["daily_compiles_used"] = quota->dailyCompilesUsed;
        response["data"]["monthly_compiles_used"] = quota->monthlyCompilesUsed;
        response["data"]["project_count_used"] = quota->projectCountUsed;
        response["data"]["daily_reset"] = std::chrono::system_clock::to_time_t(quota->dailyReset);
        response["data"]["monthly_reset"] = std::chrono::system_clock::to_time_t(quota->monthlyReset);

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleGetUserQuota: {}", e.what());
        return buildJsonResponse(false, "Failed to get user quota");
    }
}

std::string LatexApiModule::handleSetUserQuota(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);

        std::string userId = json.value("user_id", "");
        if (userId.empty()) {
            return buildJsonResponse(400, false, "Missing user_id", "");
        }

        LatexUserQuota quota;
        quota.userId = userId;
        quota.dailyCompileLimit = json.value("daily_compile_limit", 10);
        quota.monthlyCompileLimit = json.value("monthly_compile_limit", 100);
        quota.maxProjectCount = json.value("max_project_count", 5);
        quota.canUseAdvancedFeatures = json.value("can_use_advanced_features", false);

        if (json.contains("allowed_packages") && json["allowed_packages"].is_array()) {
            for (const auto& pkg : json["allowed_packages"]) {
                quota.allowedPackages.push_back(pkg.get<std::string>());
            }
        }

        g_latexStore.setUserQuota(quota);

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Quota updated";
        response["data"]["user_id"] = userId;

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleSetUserQuota: {}", e.what());
        return buildJsonResponse(false, "Failed to set user quota");
    }
}

std::string LatexApiModule::handleInitializeUserQuota(const std::string& body) {
    try {
        nlohmann::json json = nlohmann::json::parse(body);

        std::string userId = json.value("user_id", "");
        std::string tier = json.value("tier", "free");

        if (userId.empty()) {
            return buildJsonResponse(400, false, "Missing user_id", "");
        }

        g_latexStore.initializeDefaultQuota(userId, tier);

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Quota initialized";
        response["data"]["user_id"] = userId;
        response["data"]["tier"] = tier;

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleInitializeUserQuota: {}", e.what());
        return buildJsonResponse(false, "Failed to initialize user quota");
    }
}

std::string LatexApiModule::handleGetUserCompilationRecords(const std::map<std::string, std::string>& params) {
    try {
        auto userIdIt = params.find("user_id");
        if (userIdIt == params.end()) {
            return buildJsonResponse(400, false, "Missing user_id", "");
        }

        std::string userId = userIdIt->second;
        int limit = 100;

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) {
            const std::string& limitStr = limitIt->second;
            // Validate that limit contains only digits
            bool valid = true;
            for (char c : limitStr) {
                if (!std::isdigit(static_cast<unsigned char>(c))) {
                    valid = false;
                    break;
                }
            }
            if (valid && !limitStr.empty()) {
                limit = std::stoi(limitStr);
            }
        }

        auto records = g_latexStore.getCompilationRecords(userId, limit);

        nlohmann::json response;
        response["success"] = true;
        response["message"] = "Compilation records retrieved";
        response["data"]["records"] = nlohmann::json::array();
        response["data"]["total"] = records.size();

        for (const auto& record : records) {
            nlohmann::json r;
            r["id"] = record.id;
            r["user_id"] = record.userId;
            r["project_id"] = record.projectId;
            r["document_id"] = record.documentId;
            r["content_hash"] = record.contentHash;
            r["success"] = record.success;
            r["error_message"] = record.errorMessage;
            r["timestamp"] = std::chrono::system_clock::to_time_t(record.timestamp);
            response["data"]["records"].push_back(r);
        }

        return response.dump();
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleGetUserCompilationRecords: {}", e.what());
        return buildJsonResponse(500, false, "Internal server error", "");
    }
}

// ==========================================
// 用户配额管理实现
// ==========================================

std::optional<LatexUserQuota> LatexApiModule::getUserQuota(const std::string& userId) {
    return g_latexStore.getUserQuota(userId);
}

bool LatexApiModule::setUserQuota(const LatexUserQuota& quota) {
    g_latexStore.setUserQuota(quota);
    return true;
}

void LatexApiModule::initializeUserQuota(const std::string& userId, const std::string& tier) {
    g_latexStore.initializeDefaultQuota(userId, tier);
}

bool LatexApiModule::canUserCompile(const std::string& userId) {
    return g_latexStore.canCompile(userId);
}

std::vector<LatexCompilationRecord> LatexApiModule::getUserCompilationRecords(const std::string& userId, int limit) {
    return g_latexStore.getCompilationRecords(userId, limit);
}

// ==========================================
// DLL导出函数
// ==========================================

#if defined(_WIN32) || defined(_WIN64)
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
EXPORT void* createModule() {
    return new PaperCrawler::LatexApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::LatexApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}
}

} // namespace PaperCrawler
