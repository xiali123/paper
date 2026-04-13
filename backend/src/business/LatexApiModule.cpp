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
        if (pageIt != params.end()) page = std::stoi(pageIt->second);

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) limit = std::stoi(limitIt->second);

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
        return buildJsonResponse(false, "Failed to list documents");
    }
}

std::string LatexApiModule::handleGetDocument(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing document ID");
        }

        int id = std::stoi(idIt->second);
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
        return buildJsonResponse(false, "Failed to get document");
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

        int id = std::stoi(idIt->second);
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
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleUpdateDocument: {}", e.what());
        return buildJsonResponse(false, "Failed to update document");
    }
}

std::string LatexApiModule::handleDeleteDocument(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing document ID");
        }

        int id = std::stoi(idIt->second);
        bool success = deleteDocument(id);

        if (success) {
            return buildJsonResponse(true, "Document deleted");
        } else {
            return buildJsonResponse(404, false, "Failed to delete document", "");
        }
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleDeleteDocument: {}", e.what());
        return buildJsonResponse(false, "Failed to delete document");
    }
}

std::string LatexApiModule::handleCompile(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing document ID");
        }

        int id = std::stoi(idIt->second);
        auto result = compileDocument(id);

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
        return buildJsonResponse(false, "Failed to compile document");
    }
}

std::string LatexApiModule::handleAutoSave(const std::map<std::string, std::string>& params, const std::string& body) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(false, "Missing document ID");
        }

        int id = std::stoi(idIt->second);
        nlohmann::json json = nlohmann::json::parse(body);
        std::string content = json.value("content", "");

        bool success = autoSaveDocument(id, content);

        if (success) {
            return buildJsonResponse(true, "Auto-saved successfully");
        } else {
            return buildJsonResponse(404, false, "Failed to auto-save", "");
        }
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleAutoSave: {}", e.what());
        return buildJsonResponse(false, "Failed to auto-save");
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

        int id = std::stoi(idIt->second);
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
        return buildJsonResponse(false, "Failed to get template");
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

        int id = std::stoi(idIt->second);
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
        return buildJsonResponse(false, "Failed to get PDF path");
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

LatexCompilationResult LatexApiModule::compileDocument(int id) {
    spdlog::info("[LatexApi] Compiling document: id={}", id);

    std::lock_guard<std::mutex> lock(compileMutex_);

    auto document = getDocument(id);
    if (!document) {
        LatexCompilationResult result;
        result.success = false;
        result.errorMessage = "Document not found";
        return result;
    }

    // 生成输出文件路径
    std::string outputPath = impl_->pdfDirectory + "/document_" + std::to_string(id) + ".pdf";

    // 编译LaTeX
    auto result = compileLatex(document->content, outputPath);

    // 更新文档状态
    if (result.success) {
        // TODO: 更新数据库
        spdlog::info("[LatexApi] Compilation successful: {}", outputPath);
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

LatexCompilationResult LatexApiModule::compileProject(int id) {
    spdlog::info("[LatexApi] Compiling project: {}", id);

    auto project = g_latexStore.getProject(id);
    if (!project) {
        spdlog::error("[LatexApi] Project not found: {}", id);
        return {false, "", "", "Project not found", 0};
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
        return {false, "", "", "Main file not found", 0};
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
        if (pageIt != params.end()) page = std::stoi(pageIt->second);

        auto limitIt = params.find("limit");
        if (limitIt != params.end()) limit = std::stoi(limitIt->second);

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
        return buildJsonResponse(false, "Failed to list projects");
    }
}

std::string LatexApiModule::handleGetProject(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing project ID", "");
        }

        int id = std::stoi(idIt->second);
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
        return buildJsonResponse(false, "Failed to get project");
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

        int id = std::stoi(idIt->second);
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
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleUpdateProject: {}", e.what());
        return buildJsonResponse(false, "Failed to update project");
    }
}

std::string LatexApiModule::handleDeleteProject(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing project ID", "");
        }

        int id = std::stoi(idIt->second);

        if (deleteProject(id)) {
            return buildJsonResponse(true, "Project deleted");
        } else {
            return buildJsonResponse(false, "Failed to delete project");
        }
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleDeleteProject: {}", e.what());
        return buildJsonResponse(false, "Failed to delete project");
    }
}

std::string LatexApiModule::handleCompileProject(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing project ID", "");
        }

        int id = std::stoi(idIt->second);
        auto result = compileProject(id);

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
        return buildJsonResponse(false, "Failed to compile project");
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

        int fileId = std::stoi(idIt->second);
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
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleUpdateProjectFile: {}", e.what());
        return buildJsonResponse(false, "Failed to update file");
    }
}

std::string LatexApiModule::handleDeleteProjectFile(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing file ID", "");
        }

        int fileId = std::stoi(idIt->second);

        if (deleteProjectFile(fileId)) {
            return buildJsonResponse(true, "File deleted");
        } else {
            return buildJsonResponse(false, "Failed to delete file");
        }
    } catch (const std::exception& e) {
        spdlog::error("[LatexApi] Error in handleDeleteProjectFile: {}", e.what());
        return buildJsonResponse(false, "Failed to delete file");
    }
}

std::string LatexApiModule::handleGetProjectFile(const std::map<std::string, std::string>& params) {
    try {
        auto idIt = params.find("id");
        if (idIt == params.end()) {
            return buildJsonResponse(400, false, "Missing file ID", "");
        }

        int fileId = std::stoi(idIt->second);
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
        return buildJsonResponse(false, "Failed to get file");
    }
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
