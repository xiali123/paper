#include "business/LatexApiModule.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include "../../core/external/nlohmann/json.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <map>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <random>
#include <openssl/sha.h>
#include <openssl/evp.h>

namespace PaperCrawler {

// ============================================================================
// LatexApiModule::Impl - 内部实现类
// ============================================================================

class LatexApiModule::Impl {
public:
    std::shared_ptr<IDatabase> database_;
    std::map<int, LatexDocument> documents_;
    std::map<int, LatexProject> projects_;
    std::map<int, LatexTemplate> templates_;
    std::map<std::string, LatexCollaborationSession> collaborationSessions_;
    std::map<std::string, LatexUserQuota> userQuotas_;
    std::vector<LatexCompilationRecord> compilationRecords_;
    std::string pdfDirectory_;
    std::string cacheDirectory_;  // PDF缓存目录
    int nextDocumentId_{1};
    int nextProjectId_{1};
    int nextTemplateId_{1};
    int nextCompilationId_{1};

    // 版本控制存储
    std::map<std::string, LatexVersionNode> versions_;  // key: versionId
    std::map<std::string, std::vector<std::string>> fileVersions_;  // key: "userId_projectId_fileId", value: versionIds
    std::map<std::string, std::string> branchTips_;  // key: branchId, value: versionId
    int nextVersionPosition_{1};
    std::map<std::string, int> branchCounters_;  // branch name -> counter

    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {
        pdfDirectory_ = "output/pdfs";
        cacheDirectory_ = "cache/pdf";
        std::filesystem::create_directories(pdfDirectory_);
        std::filesystem::create_directories(cacheDirectory_);
    }

    std::string escapeJson(const std::string& str) {
        std::string result;
        result.reserve(str.length() * 1.2);
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }

    std::string buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data = "") {
        std::ostringstream json;
        json << "{\n";
        json << "  \"statusCode\": " << statusCode << ",\n";
        json << "  \"success\": " << (success ? "true" : "false") << ",\n";
        json << "  \"message\": \"" << escapeJson(message) << "\"";
        if (!data.empty()) {
            json << ",\n  \"data\": " << data;
        }
        json << "\n}";
        return json.str();
    }

    std::string buildJsonResponse(bool success, const std::string& message, const std::string& data = "") {
        return buildJsonResponse(200, success, message, data);
    }

    // 计算内容的SHA256哈希
    std::string computeContentHash(const std::string& content) {
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        const EVP_MD* md = EVP_sha256();
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hashLen;

        if (mdctx == nullptr) {
            return "";
        }

        if (EVP_DigestInit_ex(mdctx, md, nullptr) != 1) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }

        if (EVP_DigestUpdate(mdctx, content.c_str(), content.size()) != 1) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }

        if (EVP_DigestFinal_ex(mdctx, hash, &hashLen) != 1) {
            EVP_MD_CTX_free(mdctx);
            return "";
        }

        EVP_MD_CTX_free(mdctx);

        // 转换为十六进制字符串
        std::ostringstream ss;
        for (unsigned int i = 0; i < hashLen; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }

        return ss.str();
    }

    // 检查缓存是否存在
    std::string checkPdfCache(const std::string& contentHash) {
        std::string cachedPdfPath = cacheDirectory_ + "/" + contentHash + ".pdf";

        if (std::filesystem::exists(cachedPdfPath)) {
            spdlog::info("[LatexApiModule] Cache hit for hash: {}", contentHash);
            return cachedPdfPath;
        }

        return "";
    }

    // 保存PDF到缓存
    void savePdfToCache(const std::string& sourcePdfPath, const std::string& contentHash) {
        std::string cachedPdfPath = cacheDirectory_ + "/" + contentHash + ".pdf";

        try {
            std::filesystem::copy_file(sourcePdfPath, cachedPdfPath, std::filesystem::copy_options::overwrite_existing);
            spdlog::info("[LatexApiModule] Cached PDF: {} -> {}", sourcePdfPath, cachedPdfPath);
        } catch (const std::exception& e) {
            spdlog::error("[LatexApiModule] Failed to cache PDF: {}", e.what());
        }
    }

    // 清理旧缓存（超过30天）
    void cleanOldCache() {
        try {
            auto now = std::chrono::system_clock::now();
            auto maxAge = std::chrono::hours(30 * 24); // 30天

            for (const auto& entry : std::filesystem::directory_iterator(cacheDirectory_)) {
                if (entry.is_regular_file()) {
                    auto lastWrite = std::filesystem::last_write_time(entry.path());
                    // Convert file_clock to system_clock
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        lastWrite - std::filesystem::file_time_type::clock::now() + now
                    );
                    auto age = now - sctp;

                    if (age > maxAge) {
                        std::filesystem::remove(entry.path());
                        spdlog::info("[LatexApiModule] Removed old cache: {}", entry.path().filename().string());
                    }
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("[LatexApiModule] Failed to clean old cache: {}", e.what());
        }
    }

    // 获取缓存目录大小
    size_t getCacheSize() {
        size_t totalSize = 0;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(cacheDirectory_)) {
                if (entry.is_regular_file()) {
                    totalSize += entry.file_size();
                }
            }
        } catch (const std::exception& e) {
            spdlog::error("[LatexApiModule] Failed to calculate cache size: {}", e.what());
        }
        return totalSize;
    }
};

// ============================================================================
// LatexApiModule - 构造函数和析构函数
// ============================================================================

LatexApiModule::LatexApiModule()
    : LatexApiModule(nullptr) {
    spdlog::info("[LatexApiModule] Default constructor called");
}

LatexApiModule::LatexApiModule(std::shared_ptr<IDatabase> database)
    : impl_(std::make_unique<Impl>(database)), database_(database) {
    spdlog::info("[LatexApiModule] Constructor with database");
    initializeBuiltInTemplates();
}

LatexApiModule::~LatexApiModule() {
    spdlog::info("[LatexApiModule] Destructor called");
}

// ============================================================================
// LatexApiModule - 路由注册
// ============================================================================

void LatexApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    const std::string prefix = "/api/latex";

    // Document routes
    router.get(prefix + "/documents", [this](const HttpRequest& req) -> HttpResponse {
        std::map<std::string, std::string> params;
        for (const auto& [key, value] : req.queryParams) {
            params[key] = value;
        }
        std::string body = handleListDocuments(params);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/documents/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleGetDocument(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/documents", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleCreateDocument(req.body);
        HttpResponse response;
        response.statusCode = 201;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/documents/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleUpdateDocument(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/documents/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleDeleteDocument(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // Document compile route
    router.post(prefix + "/documents/:id/compile", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleCompileDocument(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // Project routes
    router.get(prefix + "/projects", [this](const HttpRequest& req) -> HttpResponse {
        std::map<std::string, std::string> params;
        for (const auto& [key, value] : req.queryParams) {
            params[key] = value;
        }
        std::string body = handleListProjects(params);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/projects/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleGetProject(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/projects", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleCreateProject(req.body);
        HttpResponse response;
        response.statusCode = 201;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // Project compile route
    router.post(prefix + "/projects/:id/compile", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleCompileProject(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // Project file routes
    router.post(prefix + "/projects/files", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleAddProjectFile(req.body);
        HttpResponse response;
        response.statusCode = 201;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.put(prefix + "/projects/files/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleUpdateProjectFile(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/projects/files/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleDeleteProjectFile(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/projects/files/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleGetProjectFile(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // Project files list route
    router.get(prefix + "/projects/:id/files", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleListProjectFiles(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // File upload routes
    router.post(prefix + "/projects/:id/upload", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleUploadProjectFile(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 201;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/projects/:id/batch-upload", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleBatchUploadProjectFiles(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 201;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/projects/:id/import", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleImportFilesFromProject(req.pathParams, req.body);
        HttpResponse response;
        response.statusCode = 201;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // PDF download routes (binary)
    router.get(prefix + "/documents/:id/pdf", [this](const HttpRequest& req) -> HttpResponse {
        return handleDownloadPDFBinary(req.pathParams);
    });

    router.get(prefix + "/projects/:id/pdf", [this](const HttpRequest& req) -> HttpResponse {
        return handleDownloadProjectPDFBinary(req.pathParams);
    });

    // PDF debug endpoint (returns info about PDF request)
    router.get(prefix + "/debug/pdf/:type/:id", [this](const HttpRequest& req) -> HttpResponse {
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");

        auto typeIt = req.pathParams.find("type");
        auto idIt = req.pathParams.find("id");

        nlohmann::json debugInfo;
        debugInfo["type"] = typeIt != req.pathParams.end() ? typeIt->second : "unknown";
        debugInfo["id"] = idIt != req.pathParams.end() ? idIt->second : "unknown";
        debugInfo["pdfDirectory"] = impl_->pdfDirectory_;
        debugInfo["cacheDirectory"] = impl_->cacheDirectory_;

        std::string pdfPath;
        if (typeIt != req.pathParams.end() && typeIt->second == "project") {
            pdfPath = impl_->pdfDirectory_ + "/project_" + idIt->second + ".pdf";
        } else {
            pdfPath = impl_->pdfDirectory_ + "/document_" + idIt->second + ".pdf";
        }

        debugInfo["expectedPath"] = pdfPath;
        debugInfo["exists"] = std::filesystem::exists(pdfPath);

        if (std::filesystem::exists(pdfPath)) {
            debugInfo["fileSize"] = std::filesystem::file_size(pdfPath);
        }

        response.body = impl_->buildJsonResponse(200, true, "PDF debug info", debugInfo.dump());
        return response;
    });

    // PDF cache management routes
    router.get(prefix + "/cache/stats", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleGetCacheStats();
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/cache/clear", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleClearCache();
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // Template routes
    router.get(prefix + "/templates", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleListTemplates(req.queryParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/stats", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleStats();
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    // Version control routes
    router.post(prefix + "/versions/save", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleSaveVersion(req.body);
        HttpResponse response;
        response.statusCode = 201;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/versions/history", [this](const HttpRequest& req) -> HttpResponse {
        std::map<std::string, std::string> params;
        for (const auto& [key, value] : req.queryParams) {
            params[key] = value;
        }
        std::string body = handleGetVersionHistory(params);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/versions/tree", [this](const HttpRequest& req) -> HttpResponse {
        std::map<std::string, std::string> params;
        for (const auto& [key, value] : req.queryParams) {
            params[key] = value;
        }
        std::string body = handleGetVersionTree(params);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/versions/restore", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleRestoreVersion(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/versions/branch", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleCreateBranch(req.body);
        HttpResponse response;
        response.statusCode = 201;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.post(prefix + "/versions/merge", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleMergeBranch(req.body);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.del(prefix + "/versions/:id", [this](const HttpRequest& req) -> HttpResponse {
        std::string body = handleDeleteVersion(req.pathParams);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    router.get(prefix + "/versions/compare", [this](const HttpRequest& req) -> HttpResponse {
        std::map<std::string, std::string> params;
        for (const auto& [key, value] : req.queryParams) {
            params[key] = value;
        }
        std::string body = handleCompareVersions(params);
        HttpResponse response;
        response.statusCode = 200;
        response.setHeader("Content-Type", "application/json");
        response.body = body;
        return response;
    });

    spdlog::info("[LatexApiModule] Routes registered successfully");
}

// ============================================================================
// Document operations
// ============================================================================

std::vector<LatexDocument> LatexApiModule::listDocuments(int page, int limit, const std::string& ownerId) {
    std::vector<LatexDocument> result;
    for (const auto& [id, doc] : impl_->documents_) {
        if (ownerId.empty() || doc.ownerId == ownerId) {
            result.push_back(doc);
        }
    }
    return result;
}

std::optional<LatexDocument> LatexApiModule::getDocument(int id) {
    auto it = impl_->documents_.find(id);
    if (it != impl_->documents_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<LatexDocument> LatexApiModule::createDocument(const LatexDocument& document) {
    LatexDocument newDoc = document;
    newDoc.id = impl_->nextDocumentId_++;
    newDoc.createdAt = std::chrono::system_clock::now();
    newDoc.updatedAt = std::chrono::system_clock::now();
    newDoc.lastAutoSave = std::chrono::system_clock::now();
    impl_->documents_[newDoc.id] = newDoc;
    return newDoc;
}

bool LatexApiModule::updateDocument(int id, const LatexDocument& document) {
    auto it = impl_->documents_.find(id);
    if (it == impl_->documents_.end()) {
        return false;
    }
    it->second.title = document.title;
    it->second.content = document.content;
    it->second.updatedAt = std::chrono::system_clock::now();
    return true;
}

bool LatexApiModule::deleteDocument(int id) {
    return impl_->documents_.erase(id) > 0;
}

LatexCompilationResult LatexApiModule::compileDocument(int id, const std::string& userId) {
    LatexCompilationResult result;
    auto docIt = impl_->documents_.find(id);
    if (docIt == impl_->documents_.end()) {
        result.success = false;
        result.errorMessage = "Document not found";
        return result;
    }

    // Create working directory
    std::string workDir = impl_->pdfDirectory_ + "/document_" + std::to_string(id);
    std::filesystem::create_directories(workDir);

    // Compile the document content
    result = compileLatexContent(docIt->second.content, workDir + "/document.pdf", workDir);

    if (result.success) {
        docIt->second.isCompiled = true;
        docIt->second.pdfPath = result.pdfPath;
    }

    return result;
}

bool LatexApiModule::autoSaveDocument(int id, const std::string& content) {
    auto it = impl_->documents_.find(id);
    if (it == impl_->documents_.end()) {
        return false;
    }
    it->second.content = content;
    it->second.lastAutoSave = std::chrono::system_clock::now();
    return true;
}

// ============================================================================
// Project operations
// ============================================================================

std::vector<LatexProject> LatexApiModule::listProjects(int page, int limit, const std::string& ownerId) {
    std::vector<LatexProject> result;
    for (const auto& [id, project] : impl_->projects_) {
        if (ownerId.empty() || project.ownerId == ownerId) {
            result.push_back(project);
        }
    }
    return result;
}

std::optional<LatexProject> LatexApiModule::getProject(int id) {
    auto it = impl_->projects_.find(id);
    if (it != impl_->projects_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<LatexProject> LatexApiModule::createProject(const LatexProject& project) {
    LatexProject newProject = project;
    newProject.id = impl_->nextProjectId_++;
    newProject.createdAt = std::chrono::system_clock::now();
    newProject.updatedAt = std::chrono::system_clock::now();

    // Create main.tex file if not provided
    if (newProject.files.empty()) {
        LatexProjectFile mainFile;
        mainFile.id = 1;
        mainFile.projectId = newProject.id;
        mainFile.name = newProject.mainFile + ".tex";
        mainFile.path = newProject.mainFile + ".tex";
        mainFile.content = getLatexTemplate();
        mainFile.type = "main";
        mainFile.createdAt = std::chrono::system_clock::now();
        mainFile.updatedAt = std::chrono::system_clock::now();
        newProject.files.push_back(mainFile);
    }

    impl_->projects_[newProject.id] = newProject;
    return newProject;
}

bool LatexApiModule::updateProject(int id, const LatexProject& project) {
    auto it = impl_->projects_.find(id);
    if (it == impl_->projects_.end()) {
        return false;
    }
    it->second.name = project.name;
    it->second.description = project.description;
    it->second.updatedAt = std::chrono::system_clock::now();
    return true;
}

bool LatexApiModule::deleteProject(int id) {
    return impl_->projects_.erase(id) > 0;
}

LatexCompilationResult LatexApiModule::compileProject(int id, const std::string& userId) {
    LatexCompilationResult result;
    auto projectIt = impl_->projects_.find(id);
    if (projectIt == impl_->projects_.end()) {
        result.success = false;
        result.errorMessage = "Project not found";
        return result;
    }

    // Create working directory
    std::string workDir = impl_->pdfDirectory_ + "/project_" + std::to_string(id);
    std::filesystem::create_directories(workDir);

    // Write all project files to working directory
    for (const auto& file : projectIt->second.files) {
        std::string filePath = workDir + "/" + file.path;

        // Create subdirectories if needed
        std::filesystem::path dirPath = std::filesystem::path(filePath).parent_path();
        if (!dirPath.empty()) {
            std::filesystem::create_directories(dirPath);
        }

        std::ofstream fileOut(filePath);
        fileOut << file.content;
        fileOut.close();
    }

    // Find main file
    std::string mainFile = workDir + "/" + projectIt->second.mainFile;
    if (!std::filesystem::exists(mainFile)) {
        // Try with .tex extension
        mainFile = workDir + "/" + projectIt->second.mainFile + ".tex";
    }

    if (!std::filesystem::exists(mainFile)) {
        result.success = false;
        result.errorMessage = "Main file not found: " + projectIt->second.mainFile;
        result.pdfPath = createErrorPDF(workDir + "/error.pdf", "Main File Not Found",
            "Could not find main file: " + projectIt->second.mainFile);
        return result;
    }

    // Read main file content
    std::ifstream mainFileIn(mainFile);
    std::string mainContent((std::istreambuf_iterator<char>(mainFileIn)),
                           std::istreambuf_iterator<char>());
    mainFileIn.close();

    // Compile the main file
    std::string outputFile = workDir + "/" + projectIt->second.name + ".pdf";
    result = compileLatexContent(mainContent, outputFile, workDir);

    // Copy compiled PDF to standard location
    // Use result.pdfPath which is the actual path returned by compileLatexContent
    if (result.success && !result.pdfPath.empty()) {
        std::string finalPdfPath = impl_->pdfDirectory_ + "/project_" + std::to_string(id) + ".pdf";
        // Remove existing file if it exists
        if (std::filesystem::exists(finalPdfPath)) {
            std::filesystem::remove(finalPdfPath);
        }
        std::filesystem::copy_file(result.pdfPath, finalPdfPath);
        result.pdfPath = finalPdfPath;
    }

    return result;
}

// ============================================================================
// Template operations
// ============================================================================

std::vector<LatexTemplate> LatexApiModule::listTemplates(const std::string& category) {
    std::vector<LatexTemplate> result;
    for (const auto& [id, tmpl] : impl_->templates_) {
        if (category.empty() || tmpl.category == category) {
            result.push_back(tmpl);
        }
    }
    return result;
}

std::optional<LatexTemplate> LatexApiModule::getTemplate(int id) {
    auto it = impl_->templates_.find(id);
    if (it != impl_->templates_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<LatexDocument> LatexApiModule::createFromTemplate(int templateId, const std::string& title, const std::string& ownerId) {
    auto tmplIt = impl_->templates_.find(templateId);
    if (tmplIt == impl_->templates_.end()) {
        return std::nullopt;
    }

    LatexDocument doc;
    doc.id = 0; // Will be set by createDocument
    doc.title = title;
    doc.content = tmplIt->second.content;
    doc.ownerId = ownerId;
    return createDocument(doc);
}

// ============================================================================
// Statistics
// ============================================================================

LatexDocumentStats LatexApiModule::getStats() {
    LatexDocumentStats stats;
    stats.totalDocuments = impl_->documents_.size();
    stats.compiledDocuments = 0;
    stats.collaborativeDocuments = 0;

    for (const auto& [id, doc] : impl_->documents_) {
        if (doc.isCompiled) stats.compiledDocuments++;
        if (doc.isCollaborative) stats.collaborativeDocuments++;
    }

    return stats;
}

// ============================================================================
// PDF download handlers
// ============================================================================

HttpResponse LatexApiModule::handleDownloadPDFBinary(const std::map<std::string, std::string>& params) {
    HttpResponse response;

    auto idIt = params.find("id");
    if (idIt == params.end()) {
        spdlog::error("[LatexApiModule] Missing document ID in PDF download request");
        response.statusCode = 400;
        response.setHeader("Content-Type", "application/json");
        response.body = impl_->buildJsonResponse(false, "Missing document ID");
        return response;
    }

    try {
        int id = std::stoi(idIt->second);
        std::string pdfPath = impl_->pdfDirectory_ + "/document_" + std::to_string(id) + ".pdf";

        spdlog::info("[LatexApiModule] PDF download request for document: {}, path: {}", id, pdfPath);

        // Check file exists, create minimal valid PDF if not
        if (!std::filesystem::exists(pdfPath)) {
            spdlog::warn("[LatexApiModule] PDF file not found, creating placeholder: {}", pdfPath);
            std::ofstream pdf(pdfPath, std::ios::binary);
            // Minimal valid PDF with one page
            const char* minimalPdf =
                "%PDF-1.4\n"
                "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n"
                "2 0 obj<</Type/Pages/Count 1/Kids[3 0 R]>>endobj\n"
                "3 0 obj<</Type/Page/MediaBox[0 0 612 792]/Parent 2 0 R/Resources<<"
                "/Font<<F1 4 0 R>>>>/Contents 5 0 R>>endobj\n"
                "4 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj\n"
                "5 0 obj<</Length 47>>stream\n"
                "BT\n"
                "/F1 12 Tf\n"
                "50 700 Td\n"
                "(LaTeX Document) Tj\n"
                "ET\n"
                "endstream\n"
                "endobj\n"
                "xref\n"
                "0 6\n"
                "0000000000 65535 f\n"
                "0000000009 00000 n\n"
                "0000000058 00000 n\n"
                "0000000115 00000 n\n"
                "0000000262 00000 n\n"
                "0000000334 00000 n\n"
                "trailer<</Size 6/Root 1 0 R>>\n"
                "startxref\n"
                "432\n"
                "%%EOF\n";
            pdf.write(minimalPdf, strlen(minimalPdf));
            pdf.close();
        }

        // Read PDF file
        std::ifstream file(pdfPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            response.statusCode = 500;
            response.setHeader("Content-Type", "application/json");
            response.body = impl_->buildJsonResponse(false, "Failed to open PDF file");
            return response;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> fileData(fileSize);
        if (!file.read(reinterpret_cast<char*>(fileData.data()), fileSize)) {
            response.statusCode = 500;
            response.setHeader("Content-Type", "application/json");
            response.body = impl_->buildJsonResponse(false, "Failed to read PDF file");
            return response;
        }

        // Set headers
        response.setHeader("Content-Type", "application/pdf");
        response.setHeader("Content-Disposition", "inline; filename=\"document_" + idIt->second + ".pdf\"");
        response.setHeader("Content-Length", std::to_string(fileSize));
        response.setHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response.setHeader("Accept-Ranges", "bytes");

        // Set response body (binary data in string)
        response.body = std::string(fileData.begin(), fileData.end());

        spdlog::info("[LatexApiModule] PDF sent successfully: {} bytes", fileSize);
        return response;
    } catch (const std::invalid_argument& e) {
        spdlog::error("[LatexApiModule] Invalid document ID: {}", idIt->second);
        response.statusCode = 400;
        response.setHeader("Content-Type", "application/json");
        response.body = impl_->buildJsonResponse(false, "Invalid document ID: " + idIt->second);
        return response;
    } catch (const std::exception& e) {
        spdlog::error("[LatexApiModule] PDF download error: {}", e.what());
        response.statusCode = 500;
        response.setHeader("Content-Type", "application/json");
        response.body = impl_->buildJsonResponse(false, std::string("Error: ") + e.what());
        return response;
    }
}

HttpResponse LatexApiModule::handleDownloadProjectPDFBinary(const std::map<std::string, std::string>& params) {
    HttpResponse response;

    auto idIt = params.find("id");
    if (idIt == params.end()) {
        spdlog::error("[LatexApiModule] Missing project ID in PDF download request");
        response.statusCode = 400;
        response.setHeader("Content-Type", "application/json");
        response.body = impl_->buildJsonResponse(false, "Missing project ID");
        return response;
    }

    try {
        int id = std::stoi(idIt->second);
        std::string pdfPath = impl_->pdfDirectory_ + "/project_" + std::to_string(id) + ".pdf";

        spdlog::info("[LatexApiModule] PDF download request for project: {}, path: {}", id, pdfPath);

        // Check file exists, create minimal valid PDF if not
        if (!std::filesystem::exists(pdfPath)) {
            spdlog::warn("[LatexApiModule] PDF file not found, creating placeholder: {}", pdfPath);
            std::ofstream pdf(pdfPath, std::ios::binary);
            // Minimal valid PDF with one page
            const char* minimalPdf =
                "%PDF-1.4\n"
                "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n"
                "2 0 obj<</Type/Pages/Count 1/Kids[3 0 R]>>endobj\n"
                "3 0 obj<</Type/Page/MediaBox[0 0 612 792]/Parent 2 0 R/Resources<<"
                "/Font<<F1 4 0 R>>>>/Contents 5 0 R>>endobj\n"
                "4 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj\n"
                "5 0 obj<</Length 46>>stream\n"
                "BT\n"
                "/F1 12 Tf\n"
                "50 700 Td\n"
                "(LaTeX Project) Tj\n"
                "ET\n"
                "endstream\n"
                "endobj\n"
                "xref\n"
                "0 6\n"
                "0000000000 65535 f\n"
                "0000000009 00000 n\n"
                "0000000058 00000 n\n"
                "0000000115 00000 n\n"
                "0000000262 00000 n\n"
                "0000000333 00000 n\n"
                "trailer<</Size 6/Root 1 0 R>>\n"
                "startxref\n"
                "431\n"
                "%%EOF\n";
            pdf.write(minimalPdf, strlen(minimalPdf));
            pdf.close();
        }

        // Read PDF file
        std::ifstream file(pdfPath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            response.statusCode = 500;
            response.setHeader("Content-Type", "application/json");
            response.body = impl_->buildJsonResponse(false, "Failed to open PDF file");
            return response;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> fileData(fileSize);
        if (!file.read(reinterpret_cast<char*>(fileData.data()), fileSize)) {
            response.statusCode = 500;
            response.setHeader("Content-Type", "application/json");
            response.body = impl_->buildJsonResponse(false, "Failed to read PDF file");
            return response;
        }

        // Set headers
        response.setHeader("Content-Type", "application/pdf");
        response.setHeader("Content-Disposition", "inline; filename=\"project_" + idIt->second + ".pdf\"");
        response.setHeader("Content-Length", std::to_string(fileSize));
        response.setHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response.setHeader("Accept-Ranges", "bytes");

        response.body = std::string(fileData.begin(), fileData.end());

        spdlog::info("[LatexApiModule] Project PDF sent successfully: {} bytes", fileSize);
        return response;
    } catch (const std::invalid_argument& e) {
        spdlog::error("[LatexApiModule] Invalid project ID: {}", idIt->second);
        response.statusCode = 400;
        response.setHeader("Content-Type", "application/json");
        response.body = impl_->buildJsonResponse(false, "Invalid project ID: " + idIt->second);
        return response;
    } catch (const std::exception& e) {
        spdlog::error("[LatexApiModule] Project PDF download error: {}", e.what());
        response.statusCode = 500;
        response.setHeader("Content-Type", "application/json");
        response.body = impl_->buildJsonResponse(false, std::string("Error: ") + e.what());
        return response;
    }
}

// ============================================================================
// HTTP request handlers - Documents
// ============================================================================

std::string LatexApiModule::handleListDocuments(const std::map<std::string, std::string>& params) {
    int page = 1, limit = 20;
    std::string ownerId;

    auto pageIt = params.find("page");
    if (pageIt != params.end()) page = std::stoi(pageIt->second);

    auto limitIt = params.find("limit");
    if (limitIt != params.end()) limit = std::stoi(limitIt->second);

    auto ownerIt = params.find("ownerId");
    if (ownerIt != params.end()) ownerId = ownerIt->second;

    auto docs = listDocuments(page, limit, ownerId);

    nlohmann::json result;
    result["items"] = nlohmann::json::array();
    for (const auto& doc : docs) {
        nlohmann::json item;
        item["id"] = doc.id;
        item["title"] = doc.title;
        item["owner_id"] = doc.ownerId;
        item["is_collaborative"] = doc.isCollaborative;
        item["version"] = doc.version;
        item["is_compiled"] = doc.isCompiled;
        item["created_at"] = std::chrono::system_clock::to_time_t(doc.createdAt);
        item["updated_at"] = std::chrono::system_clock::to_time_t(doc.updatedAt);
        result["items"].push_back(item);
    }
    result["total"] = docs.size();
    result["page"] = page;
    result["limit"] = limit;

    return impl_->buildJsonResponse(200, true, "Documents retrieved", result.dump());
}

std::string LatexApiModule::handleGetDocument(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing document ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto doc = getDocument(id);
        if (!doc) {
            return impl_->buildJsonResponse(404, false, "Document not found");
        }

        nlohmann::json result;
        result["id"] = doc->id;
        result["title"] = doc->title;
        result["content"] = doc->content;
        result["owner_id"] = doc->ownerId;
        result["is_collaborative"] = doc->isCollaborative;
        result["version"] = doc->version;
        result["is_compiled"] = doc->isCompiled;

        return impl_->buildJsonResponse(200, true, "Document retrieved", result.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleCreateDocument(const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);

        LatexDocument doc;
        doc.id = 0;
        doc.title = jsonBody.value("title", "Untitled");
        doc.content = jsonBody.value("content", getLatexTemplate());
        doc.ownerId = jsonBody.value("owner_id", "default");
        doc.isCollaborative = jsonBody.value("is_collaborative", false);

        auto newDoc = createDocument(doc);
        if (newDoc) {
            nlohmann::json result;
            result["id"] = newDoc->id;
            result["title"] = newDoc->title;
            result["owner_id"] = newDoc->ownerId;

            return impl_->buildJsonResponse(201, true, "Document created", result.dump());
        }

        return impl_->buildJsonResponse(500, false, "Failed to create document");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleUpdateDocument(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing document ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto jsonBody = nlohmann::json::parse(body);

        LatexDocument doc;
        doc.id = id;
        doc.title = jsonBody.value("title", "Untitled");
        doc.content = jsonBody.value("content", "");

        if (updateDocument(id, doc)) {
            return impl_->buildJsonResponse(true, "Document updated");
        }

        return impl_->buildJsonResponse(404, false, "Document not found");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleDeleteDocument(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing document ID");
    }

    try {
        int id = std::stoi(idIt->second);
        if (deleteDocument(id)) {
            return impl_->buildJsonResponse(true, "Document deleted");
        }

        return impl_->buildJsonResponse(404, false, "Document not found");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleCompileDocument(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing document ID");
    }

    try {
        int id = std::stoi(idIt->second);

        // Extract userId from request body if provided
        std::string userId;
        if (!body.empty()) {
            try {
                auto jsonBody = nlohmann::json::parse(body);
                userId = jsonBody.value("user_id", "");
            } catch (...) {
                // Ignore JSON parse errors for userId
            }
        }

        auto result = compileDocument(id, userId);

        nlohmann::json responseData;
        responseData["success"] = result.success;
        responseData["pdf_path"] = result.pdfPath;
        responseData["log"] = result.log;
        if (!result.success) {
            responseData["error"] = result.errorMessage;
        }
        responseData["compile_time_ms"] = result.compileTime;

        return impl_->buildJsonResponse(200, result.success,
            result.success ? "Compilation successful" : "Compilation failed",
            responseData.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleCompileProject(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing project ID");
    }

    try {
        int id = std::stoi(idIt->second);

        // Extract userId from request body if provided
        std::string userId;
        if (!body.empty()) {
            try {
                auto jsonBody = nlohmann::json::parse(body);
                userId = jsonBody.value("user_id", "");
            } catch (...) {
                // Ignore JSON parse errors for userId
            }
        }

        auto result = compileProject(id, userId);

        nlohmann::json responseData;
        responseData["success"] = result.success;
        responseData["pdf_path"] = result.pdfPath;
        responseData["log"] = result.log;
        if (!result.success) {
            responseData["error"] = result.errorMessage;
        }
        responseData["compile_time_ms"] = result.compileTime;

        return impl_->buildJsonResponse(200, result.success,
            result.success ? "Compilation successful" : "Compilation failed",
            responseData.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// HTTP request handlers - Projects
// ============================================================================

std::string LatexApiModule::handleListProjects(const std::map<std::string, std::string>& params) {
    int page = 1, limit = 20;
    std::string ownerId;

    auto pageIt = params.find("page");
    if (pageIt != params.end()) page = std::stoi(pageIt->second);

    auto limitIt = params.find("limit");
    if (limitIt != params.end()) limit = std::stoi(limitIt->second);

    auto ownerIt = params.find("ownerId");
    if (ownerIt != params.end()) ownerId = ownerIt->second;

    auto projects = listProjects(page, limit, ownerId);

    nlohmann::json result;
    result["items"] = nlohmann::json::array();
    for (const auto& project : projects) {
        nlohmann::json item;
        item["id"] = project.id;
        item["name"] = project.name;
        item["owner_id"] = project.ownerId;
        item["main_file"] = project.mainFile;
        item["description"] = project.description;
        item["is_public"] = project.isPublic;
        item["version"] = project.version;
        item["created_at"] = std::chrono::system_clock::to_time_t(project.createdAt);
        item["updated_at"] = std::chrono::system_clock::to_time_t(project.updatedAt);

        // Convert files to JSON
        nlohmann::json files = nlohmann::json::array();
        for (const auto& file : project.files) {
            nlohmann::json fileObj;
            fileObj["id"] = file.id;
            fileObj["name"] = file.name;
            fileObj["path"] = file.path;
            fileObj["type"] = file.type;
            files.push_back(fileObj);
        }
        item["files"] = files;

        result["items"].push_back(item);
    }
    result["total"] = projects.size();
    result["page"] = page;
    result["limit"] = limit;

    return impl_->buildJsonResponse(200, true, "Projects retrieved", result.dump());
}

std::string LatexApiModule::handleGetProject(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing project ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto project = getProject(id);
        if (!project) {
            return impl_->buildJsonResponse(404, false, "Project not found");
        }

        nlohmann::json result;
        result["id"] = project->id;
        result["name"] = project->name;
        result["owner_id"] = project->ownerId;
        result["main_file"] = project->mainFile;
        result["description"] = project->description;
        result["is_public"] = project->isPublic;
        result["version"] = project->version;
        result["created_at"] = std::chrono::system_clock::to_time_t(project->createdAt);
        result["updated_at"] = std::chrono::system_clock::to_time_t(project->updatedAt);

        // Add files with content
        nlohmann::json files = nlohmann::json::array();
        for (const auto& file : project->files) {
            nlohmann::json fileObj;
            fileObj["id"] = file.id;
            fileObj["name"] = file.name;
            fileObj["path"] = file.path;
            fileObj["content"] = file.content;
            fileObj["type"] = file.type;
            files.push_back(fileObj);
        }
        result["files"] = files;

        return impl_->buildJsonResponse(200, true, "Project retrieved", result.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleCreateProject(const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);

        LatexProject project;
        project.id = 0;
        project.name = jsonBody.value("name", "Untitled Project");
        project.description = jsonBody.value("description", "");
        project.ownerId = jsonBody.value("owner_id", "default");
        project.mainFile = jsonBody.value("main_file", "main");
        project.isPublic = jsonBody.value("is_public", false);

        auto newProject = createProject(project);
        if (newProject) {
            nlohmann::json result;
            result["id"] = newProject->id;
            result["name"] = newProject->name;
            result["main_file"] = newProject->mainFile;
            result["owner_id"] = newProject->ownerId;

            spdlog::info("[LatexApi] Project created: id={}, name={}, mainFile={}, files={}",
                newProject->id, newProject->name, newProject->mainFile, newProject->files.size());

            return impl_->buildJsonResponse(201, true, "Project created", result.dump());
        }

        return impl_->buildJsonResponse(500, false, "Failed to create project");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// HTTP request handlers - Project Files
// ============================================================================

std::string LatexApiModule::handleAddProjectFile(const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);

        int projectId = jsonBody.value("project_id", 0);
        std::string name = jsonBody.value("name", "");
        std::string path = jsonBody.value("path", "");
        std::string content = jsonBody.value("content", "");
        std::string type = jsonBody.value("type", "other");

        if (projectId == 0 || name.empty()) {
            return impl_->buildJsonResponse(400, false, "Missing required fields: project_id, name");
        }

        LatexProjectFile file;
        file.id = 0; // Will be set when added to project
        file.projectId = projectId;
        file.name = name;
        file.path = path;
        file.content = content;
        file.type = type;
        file.createdAt = std::chrono::system_clock::now();
        file.updatedAt = std::chrono::system_clock::now();

        // Find project and add file
        auto projectIt = impl_->projects_.find(projectId);
        if (projectIt == impl_->projects_.end()) {
            return impl_->buildJsonResponse(404, false, "Project not found");
        }

        // Assign ID (simple increment)
        file.id = projectIt->second.files.size() + 1;
        projectIt->second.files.push_back(file);

        nlohmann::json result;
        result["id"] = file.id;
        result["name"] = file.name;
        result["path"] = file.path;
        result["project_id"] = file.projectId;

        return impl_->buildJsonResponse(201, true, "Project file created", result.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleUpdateProjectFile(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing file ID");
    }

    try {
        int fileId = std::stoi(idIt->second);
        auto jsonBody = nlohmann::json::parse(body);
        std::string content = jsonBody.value("content", "");
        std::string name = jsonBody.value("name", "");
        std::string path = jsonBody.value("path", "");
        std::string userId = jsonBody.value("user_id", "default");
        bool createVersion = jsonBody.value("create_version", true);

        // Search for the file in all projects
        for (auto& [projectId, project] : impl_->projects_) {
            for (auto& file : project.files) {
                if (file.id == fileId) {
                    std::string oldContent = file.content;

                    // 更新内容（如果提供）
                    if (!content.empty()) {
                        file.content = content;
                    }

                    // 更新文件名和路径（重命名功能）
                    if (!name.empty()) {
                        file.name = name;
                        spdlog::info("[LatexApi] Renaming file {} to {}", file.name, name);
                    }
                    if (!path.empty()) {
                        file.path = path;
                    }

                    file.updatedAt = std::chrono::system_clock::now();

                    // 自动创建版本（如果内容有变化）
                    if (createVersion && !content.empty() && content != oldContent) {
                        saveVersion(fileId, project.id, userId, content, "文件更新", false);
                    }

                    return impl_->buildJsonResponse(200, true, "Project file updated");
                }
            }
        }

        return impl_->buildJsonResponse(404, false, "Project file not found");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleDeleteProjectFile(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing file ID");
    }

    try {
        int fileId = std::stoi(idIt->second);

        // Search for the file in all projects
        for (auto& [projectId, project] : impl_->projects_) {
            auto& files = project.files;
            auto it = std::find_if(files.begin(), files.end(), [fileId](const LatexProjectFile& f) {
                return f.id == fileId;
            });

            if (it != files.end()) {
                files.erase(it);
                return impl_->buildJsonResponse(200, true, "Project file deleted");
            }
        }

        return impl_->buildJsonResponse(404, false, "Project file not found");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleGetProjectFile(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing file ID");
    }

    try {
        int fileId = std::stoi(idIt->second);

        // Search for the file in all projects
        for (const auto& [projectId, project] : impl_->projects_) {
            for (const auto& file : project.files) {
                if (file.id == fileId) {
                    nlohmann::json result;
                    result["id"] = file.id;
                    result["project_id"] = file.projectId;
                    result["name"] = file.name;
                    result["path"] = file.path;
                    result["content"] = file.content;
                    result["type"] = file.type;
                    result["created_at"] = std::chrono::system_clock::to_time_t(file.createdAt);
                    result["updated_at"] = std::chrono::system_clock::to_time_t(file.updatedAt);

                    return impl_->buildJsonResponse(200, true, "Project file retrieved", result.dump());
                }
            }
        }

        return impl_->buildJsonResponse(404, false, "Project file not found");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// HTTP request handlers - File Upload, Batch Upload, Import
// ============================================================================

std::string LatexApiModule::handleListProjectFiles(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing project ID");
    }

    try {
        int projectId = std::stoi(idIt->second);
        auto project = getProject(projectId);

        if (!project) {
            return impl_->buildJsonResponse(404, false, "Project not found");
        }

        nlohmann::json result;
        result["items"] = nlohmann::json::array();
        for (const auto& file : project->files) {
            nlohmann::json fileObj;
            fileObj["id"] = file.id;
            fileObj["name"] = file.name;
            fileObj["path"] = file.path;
            fileObj["type"] = file.type;
            fileObj["size"] = file.content.length();
            fileObj["created_at"] = std::chrono::system_clock::to_time_t(file.createdAt);
            fileObj["updated_at"] = std::chrono::system_clock::to_time_t(file.updatedAt);
            result["items"].push_back(fileObj);
        }
        result["total"] = project->files.size();

        return impl_->buildJsonResponse(200, true, "Project files retrieved", result.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleUploadProjectFile(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing project ID");
    }

    try {
        int projectId = std::stoi(idIt->second);
        auto projectIt = impl_->projects_.find(projectId);
        if (projectIt == impl_->projects_.end()) {
            return impl_->buildJsonResponse(404, false, "Project not found");
        }

        // Parse request body (JSON format with base64 content)
        // Note: True multipart/form-data requires HTTP server support
        // This implementation accepts JSON with base64-encoded file content
        auto jsonBody = nlohmann::json::parse(body);

        std::string filename = jsonBody.value("filename", "");
        std::string path = jsonBody.value("path", "");
        std::string contentBase64 = jsonBody.value("content", "");
        std::string fileType = jsonBody.value("type", "other");

        if (filename.empty()) {
            return impl_->buildJsonResponse(400, false, "Missing filename");
        }

        // Decode base64 content (simplified - in production use proper base64 decoder)
        std::string content = contentBase64;

        // Create project file
        LatexProjectFile file;
        file.id = projectIt->second.files.size() + 1;
        file.projectId = projectId;
        file.name = filename;
        file.path = path.empty() ? filename : path;
        file.content = content;
        file.type = fileType;
        file.createdAt = std::chrono::system_clock::now();
        file.updatedAt = std::chrono::system_clock::now();

        projectIt->second.files.push_back(file);

        nlohmann::json result;
        result["id"] = file.id;
        result["name"] = file.name;
        result["path"] = file.path;
        result["project_id"] = projectId;
        result["size"] = content.length();

        spdlog::info("[LatexApi] File uploaded: {} to project {}", filename, projectId);

        return impl_->buildJsonResponse(201, true, "File uploaded successfully", result.dump());
    } catch (const nlohmann::json::exception& e) {
        return impl_->buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleBatchUploadProjectFiles(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing project ID");
    }

    try {
        int projectId = std::stoi(idIt->second);
        auto projectIt = impl_->projects_.find(projectId);
        if (projectIt == impl_->projects_.end()) {
            return impl_->buildJsonResponse(404, false, "Project not found");
        }

        auto jsonBody = nlohmann::json::parse(body);
        std::string targetPath = jsonBody.value("path", "");

        if (!jsonBody.contains("files") || !jsonBody["files"].is_array()) {
            return impl_->buildJsonResponse(400, false, "Missing files array");
        }

        nlohmann::json result;
        result["uploaded"] = nlohmann::json::array();
        result["failed"] = nlohmann::json::array();

        for (const auto& fileData : jsonBody["files"]) {
            try {
                std::string filename = fileData.value("filename", "");
                std::string contentBase64 = fileData.value("content", "");
                std::string fileType = fileData.value("type", "other");

                if (filename.empty()) {
                    result["failed"].push_back({{"filename", filename}, {"error", "Missing filename"}});
                    continue;
                }

                // Create project file
                LatexProjectFile file;
                file.id = projectIt->second.files.size() + 1;
                file.projectId = projectId;
                file.name = filename;
                file.path = targetPath.empty() ? filename : targetPath + "/" + filename;
                file.content = contentBase64;
                file.type = fileType;
                file.createdAt = std::chrono::system_clock::now();
                file.updatedAt = std::chrono::system_clock::now();

                projectIt->second.files.push_back(file);

                nlohmann::json uploadedFile;
                uploadedFile["id"] = file.id;
                uploadedFile["name"] = file.name;
                uploadedFile["path"] = file.path;
                uploadedFile["size"] = contentBase64.length();
                result["uploaded"].push_back(uploadedFile);

                spdlog::info("[LatexApi] Batch upload: {} to project {}", filename, projectId);
            } catch (const std::exception& e) {
                result["failed"].push_back({{"error", e.what()}});
            }
        }

        result["total_uploaded"] = result["uploaded"].size();
        result["total_failed"] = result["failed"].size();

        return impl_->buildJsonResponse(201, true, "Batch upload completed", result.dump());
    } catch (const nlohmann::json::exception& e) {
        return impl_->buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleImportFilesFromProject(const std::map<std::string, std::string>& params, const std::string& body) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing target project ID");
    }

    try {
        int targetProjectId = std::stoi(idIt->second);
        auto targetProjectIt = impl_->projects_.find(targetProjectId);
        if (targetProjectIt == impl_->projects_.end()) {
            return impl_->buildJsonResponse(404, false, "Target project not found");
        }

        auto jsonBody = nlohmann::json::parse(body);

        int sourceProjectId = jsonBody.value("source_project_id", 0);
        std::string targetPath = jsonBody.value("target_path", "");

        if (sourceProjectId == 0) {
            return impl_->buildJsonResponse(400, false, "Missing source_project_id");
        }

        auto sourceProjectIt = impl_->projects_.find(sourceProjectId);
        if (sourceProjectIt == impl_->projects_.end()) {
            return impl_->buildJsonResponse(404, false, "Source project not found");
        }

        // Get files to import
        std::vector<int> fileIds;
        if (jsonBody.contains("file_ids") && jsonBody["file_ids"].is_array()) {
            for (const auto& id : jsonBody["file_ids"]) {
                fileIds.push_back(id.get<int>());
            }
        } else {
            // Import all files if no specific IDs provided
            for (const auto& file : sourceProjectIt->second.files) {
                fileIds.push_back(file.id);
            }
        }

        nlohmann::json result;
        result["imported"] = nlohmann::json::array();
        result["failed"] = nlohmann::json::array();

        for (int fileId : fileIds) {
            bool found = false;
            for (const auto& sourceFile : sourceProjectIt->second.files) {
                if (sourceFile.id == fileId) {
                    // Create a copy in target project
                    LatexProjectFile newFile;
                    newFile.id = targetProjectIt->second.files.size() + 1;
                    newFile.projectId = targetProjectId;
                    newFile.name = sourceFile.name;
                    newFile.path = targetPath.empty() ? sourceFile.path : targetPath + "/" + sourceFile.name;
                    newFile.content = sourceFile.content;
                    newFile.type = sourceFile.type;
                    newFile.createdAt = std::chrono::system_clock::now();
                    newFile.updatedAt = std::chrono::system_clock::now();

                    targetProjectIt->second.files.push_back(newFile);

                    nlohmann::json importedFile;
                    importedFile["id"] = newFile.id;
                    importedFile["name"] = newFile.name;
                    importedFile["path"] = newFile.path;
                    importedFile["source_file_id"] = fileId;
                    result["imported"].push_back(importedFile);

                    spdlog::info("[LatexApi] Imported file {} from project {} to project {}",
                                 sourceFile.name, sourceProjectId, targetProjectId);
                    found = true;
                    break;
                }
            }

            if (!found) {
                result["failed"].push_back({{"file_id", fileId}, {"error", "File not found in source project"}});
            }
        }

        result["total_imported"] = result["imported"].size();
        result["total_failed"] = result["failed"].size();

        return impl_->buildJsonResponse(201, true, "Files imported successfully", result.dump());
    } catch (const nlohmann::json::exception& e) {
        return impl_->buildJsonResponse(400, false, "Invalid JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// HTTP request handlers - Templates
// ============================================================================

std::string LatexApiModule::handleListTemplates(const std::map<std::string, std::string>& params) {
    std::string category;
    auto catIt = params.find("category");
    if (catIt != params.end()) category = catIt->second;

    auto templates = listTemplates(category);

    nlohmann::json result;
    result["items"] = nlohmann::json::array();
    for (const auto& tmpl : templates) {
        nlohmann::json item;
        item["id"] = tmpl.id;
        item["name"] = tmpl.name;
        item["description"] = tmpl.description;
        item["category"] = tmpl.category;
        item["icon"] = tmpl.icon;
        item["is_built_in"] = tmpl.isBuiltIn;
        result["items"].push_back(item);
    }
    result["total"] = templates.size();

    return impl_->buildJsonResponse(200, true, "Templates retrieved", result.dump());
}

std::string LatexApiModule::handleGetTemplate(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing template ID");
    }

    try {
        int id = std::stoi(idIt->second);
        auto tmpl = getTemplate(id);
        if (!tmpl) {
            return impl_->buildJsonResponse(404, false, "Template not found");
        }

        nlohmann::json result;
        result["id"] = tmpl->id;
        result["name"] = tmpl->name;
        result["description"] = tmpl->description;
        result["category"] = tmpl->category;
        result["content"] = tmpl->content;

        return impl_->buildJsonResponse(200, true, "Template retrieved", result.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// HTTP request handlers - Stats
// ============================================================================

std::string LatexApiModule::handleStats() {
    auto stats = getStats();

    nlohmann::json result;
    result["total_documents"] = stats.totalDocuments;
    result["compiled_documents"] = stats.compiledDocuments;
    result["collaborative_documents"] = stats.collaborativeDocuments;
    result["total_words"] = stats.totalWords;
    result["total_characters"] = stats.totalCharacters;

    return impl_->buildJsonResponse(200, true, "Statistics retrieved", result.dump());
}

// ============================================================================
// HTTP request handlers - Cache Management
// ============================================================================

std::string LatexApiModule::handleGetCacheStats() {
    try {
        size_t cacheSize = impl_->getCacheSize();
        int cacheFileCount = 0;

        // 计算缓存文件数量
        for (const auto& entry : std::filesystem::directory_iterator(impl_->cacheDirectory_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".pdf") {
                cacheFileCount++;
            }
        }

        nlohmann::json result;
        result["cache_size_bytes"] = static_cast<long long>(cacheSize);
        result["cache_size_mb"] = static_cast<double>(cacheSize) / (1024 * 1024);
        result["cache_file_count"] = cacheFileCount;
        result["cache_directory"] = impl_->cacheDirectory_;

        // 格式化缓存大小
        size_t sizeMB = cacheSize / (1024 * 1024);
        size_t sizeGB = cacheSize / (1024 * 1024 * 1024);
        std::string sizeStr;
        if (sizeGB > 0) {
            sizeStr = std::to_string(sizeGB) + " GB";
        } else if (sizeMB > 0) {
            sizeStr = std::to_string(sizeMB) + " MB";
        } else {
            sizeStr = std::to_string(cacheSize / 1024) + " KB";
        }
        result["cache_size_formatted"] = sizeStr;

        return impl_->buildJsonResponse(200, true, "Cache statistics retrieved", result.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, "Failed to get cache stats: " + std::string(e.what()));
    }
}

std::string LatexApiModule::handleClearCache() {
    try {
        int deletedCount = 0;

        for (const auto& entry : std::filesystem::directory_iterator(impl_->cacheDirectory_)) {
            if (entry.is_regular_file()) {
                std::filesystem::remove(entry.path());
                deletedCount++;
            }
        }

        spdlog::info("[LatexApiModule] Cleared {} cache files", deletedCount);

        nlohmann::json result;
        result["deleted_count"] = deletedCount;
        result["message"] = "Cache cleared successfully";

        return impl_->buildJsonResponse(200, true, "Cache cleared", result.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, "Failed to clear cache: " + std::string(e.what()));
    }
}

// ============================================================================
// 版本控制辅助函数
// ============================================================================

namespace {
    // 辅助函数：序列化版本节点为JSON字符串
    std::string versionToJson(const LatexVersionNode& v) {
        nlohmann::json j;
        j["id"] = v.id;
        j["parentId"] = v.parentId;
        j["branchId"] = v.branchId;
        j["branchName"] = v.branchName;
        j["content"] = v.content;
        j["summary"] = v.summary;
        j["timestamp"] = std::chrono::system_clock::to_time_t(v.timestamp);
        j["author"] = v.author;
        j["isAutoSave"] = v.isAutoSave;
        j["changeCount"] = v.changeCount;
        j["totalLines"] = v.totalLines;
        j["fileId"] = v.fileId;
        j["projectId"] = v.projectId;
        j["userId"] = v.userId;
        j["position"] = v.position;
        j["depth"] = v.depth;
        j["isMerged"] = v.isMerged;
        return j.dump();
    }
}

// ============================================================================
// 版本控制HTTP请求处理器
// ============================================================================

std::string LatexApiModule::handleSaveVersion(const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);

        int fileId = jsonBody.value("file_id", 0);
        int projectId = jsonBody.value("project_id", 0);
        std::string userId = jsonBody.value("user_id", "");
        std::string content = jsonBody.value("content", "");
        std::string summary = jsonBody.value("summary", "");
        bool isAutoSave = jsonBody.value("is_auto_save", false);

        if (fileId == 0 || projectId == 0 || userId.empty()) {
            return impl_->buildJsonResponse(400, false, "Missing required fields: file_id, project_id, user_id");
        }

        auto version = saveVersion(fileId, projectId, userId, content, summary, isAutoSave);

        if (version) {
            nlohmann::json result;
            result["version"] = nlohmann::json::parse(versionToJson(*version));

            return impl_->buildJsonResponse(201, true, "Version saved", result.dump());
        }

        return impl_->buildJsonResponse(500, false, "Failed to save version");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleGetVersionHistory(const std::map<std::string, std::string>& params) {
    auto fileIdIt = params.find("file_id");
    auto projectIdIt = params.find("project_id");
    auto userIdIt = params.find("user_id");

    if (fileIdIt == params.end() || projectIdIt == params.end() || userIdIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing required parameters: file_id, project_id, user_id");
    }

    try {
        int fileId = std::stoi(fileIdIt->second);
        int projectId = std::stoi(projectIdIt->second);
        std::string userId = userIdIt->second;

        auto versions = getVersionHistory(fileId, projectId, userId);

        nlohmann::json result;
        result["versions"] = nlohmann::json::array();
        for (const auto& version : versions) {
            result["versions"].push_back(nlohmann::json::parse(versionToJson(version)));
        }
        result["total"] = versions.size();

        return impl_->buildJsonResponse(200, true, "Version history retrieved", result.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleGetVersionTree(const std::map<std::string, std::string>& params) {
    auto fileIdIt = params.find("file_id");
    auto projectIdIt = params.find("project_id");
    auto userIdIt = params.find("user_id");

    if (fileIdIt == params.end() || projectIdIt == params.end() || userIdIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing required parameters: file_id, project_id, user_id");
    }

    try {
        int fileId = std::stoi(fileIdIt->second);
        int projectId = std::stoi(projectIdIt->second);
        std::string userId = userIdIt->second;

        auto versions = getVersionTree(fileId, projectId, userId);

        nlohmann::json result;
        result["tree"] = nlohmann::json::array();
        for (const auto& version : versions) {
            result["tree"].push_back(nlohmann::json::parse(versionToJson(version)));
        }
        result["total"] = versions.size();

        return impl_->buildJsonResponse(200, true, "Version tree retrieved", result.dump());
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleRestoreVersion(const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string versionId = jsonBody.value("version_id", "");

        if (versionId.empty()) {
            return impl_->buildJsonResponse(400, false, "Missing required field: version_id");
        }

        auto newVersion = restoreVersion(versionId);

        if (newVersion) {
            nlohmann::json result;
            result["version"] = nlohmann::json::parse(versionToJson(*newVersion));

            return impl_->buildJsonResponse(200, true, "Version restored", result.dump());
        }

        return impl_->buildJsonResponse(404, false, "Version not found");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleCreateBranch(const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string parentVersionId = jsonBody.value("parent_version_id", "");
        std::string branchName = jsonBody.value("branch_name", "新分支");

        if (parentVersionId.empty()) {
            return impl_->buildJsonResponse(400, false, "Missing required field: parent_version_id");
        }

        auto branch = createBranch(parentVersionId, branchName);

        if (branch) {
            nlohmann::json result;
            result["branch"] = nlohmann::json::parse(versionToJson(*branch));

            return impl_->buildJsonResponse(201, true, "Branch created", result.dump());
        }

        return impl_->buildJsonResponse(404, false, "Parent version not found");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleMergeBranch(const std::string& body) {
    try {
        auto jsonBody = nlohmann::json::parse(body);
        std::string branchId = jsonBody.value("branch_id", "");

        if (branchId.empty()) {
            return impl_->buildJsonResponse(400, false, "Missing required field: branch_id");
        }

        auto mergedVersion = mergeBranch(branchId);

        if (mergedVersion) {
            nlohmann::json result;
            result["version"] = nlohmann::json::parse(versionToJson(*mergedVersion));

            return impl_->buildJsonResponse(200, true, "Branch merged", result.dump());
        }

        return impl_->buildJsonResponse(404, false, "Branch not found");
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

std::string LatexApiModule::handleDeleteVersion(const std::map<std::string, std::string>& params) {
    auto idIt = params.find("id");
    if (idIt == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing version ID");
    }

    std::string versionId = idIt->second;

    if (deleteVersion(versionId)) {
        return impl_->buildJsonResponse(200, true, "Version deleted");
    }

    return impl_->buildJsonResponse(404, false, "Version not found");
}

std::string LatexApiModule::handleCompareVersions(const std::map<std::string, std::string>& params) {
    auto v1It = params.find("version1");
    auto v2It = params.find("version2");

    if (v1It == params.end() || v2It == params.end()) {
        return impl_->buildJsonResponse(400, false, "Missing required parameters: version1, version2");
    }

    try {
        std::string version1 = v1It->second;
        std::string version2 = v2It->second;

        auto comparison = compareVersions(version1, version2);

        // 检查是否包含错误
        auto comparisonJson = nlohmann::json::parse(comparison);
        if (comparisonJson.contains("error")) {
            return impl_->buildJsonResponse(404, false, comparisonJson["error"]);
        }

        return impl_->buildJsonResponse(200, true, "Versions compared", comparison);
    } catch (const std::exception& e) {
        return impl_->buildJsonResponse(500, false, std::string("Error: ") + e.what());
    }
}

// ============================================================================
// Helper functions
// ============================================================================

std::string LatexApiModule::createErrorPDF(const std::string& outputPath, const std::string& title, const std::string& errorMessage) {
    // Create a PDF with error message
    std::ofstream pdf(outputPath, std::ios::binary);

    // Simple error PDF with multiple lines of text
    const char* errorPdf =
        "%PDF-1.4\n"
        "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n"
        "2 0 obj<</Type/Pages/Count 1/Kids[3 0 R]>>endobj\n"
        "3 0 obj<</Type/Page/MediaBox[0 0 612 792]/Parent 2 0 R/Resources<<"
        "/Font<<F1 4 0 R>>>>/Contents 5 0 R>>endobj\n"
        "4 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj\n"
        "5 0 obj<</Length 200>>stream\n"
        "BT\n"
        "/F1 14 Tf\n"
        "50 750 Td\n"
        "(LaTeX Compilation Failed) Tj\n"
        "0 -20 Td\n"
        "/F1 10 Tf\n"
        "(Please check your LaTeX syntax) Tj\n"
        "0 -30 Td\n"
        "(Error:) Tj\n"
        "ET\n"
        "endstream\n"
        "endobj\n"
        "xref\n"
        "0 6\n"
        "0000000000 65535 f\n"
        "0000000009 00000 n\n"
        "0000000058 00000 n\n"
        "0000000115 00000 n\n"
        "0000000262 00000 n\n"
        "0000000345 00000 n\n"
        "trailer<</Size 6/Root 1 0 R>>\n"
        "startxref\n"
        "443\n"
        "%%EOF\n";

    pdf.write(errorPdf, strlen(errorPdf));
    pdf.close();

    return outputPath;
}

LatexCompilationResult LatexApiModule::compileLatexContent(const std::string& content, const std::string& outputPath, const std::string& workDir) {
    LatexCompilationResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    // 计算内容哈希用于缓存
    std::string contentHash = impl_->computeContentHash(content);

    // 检查缓存
    std::string cachedPdfPath = impl_->checkPdfCache(contentHash);
    if (!cachedPdfPath.empty()) {
        // 使用缓存的PDF
        spdlog::info("[LatexApiModule] Using cached PDF: {}", cachedPdfPath);

        // 复制缓存的PDF到输出路径
        try {
            std::filesystem::copy_file(cachedPdfPath, outputPath, std::filesystem::copy_options::overwrite_existing);

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            result.success = true;
            result.pdfPath = outputPath;
            result.compileTime = duration.count();
            result.log = "Using cached PDF (content hash: " + contentHash + ")";

            return result;
        } catch (const std::exception& e) {
            spdlog::error("[LatexApiModule] Failed to copy cached PDF: {}", e.what());
            // 继续执行编译
        }
    } else {
        spdlog::info("[LatexApiModule] Cache miss for hash: {}, compiling...", contentHash);
    }

    // Write .tex file
    std::string texFile = workDir + "/document.tex";
    std::ofstream tex(texFile);
    tex << content;
    tex.close();

    // Compile using pdflatex or xelatex
    std::string outputFile = workDir + "/document.pdf";
    std::string logFile = workDir + "/document.log";

    // Try xelatex first (better for Chinese), then pdflatex
    std::vector<std::string> compilers = {"xelatex", "pdflatex"};
    bool compilationSuccess = false;
    std::string compileLog;

    for (const auto& compiler : compilers) {
        std::string cmd = compiler + " -interaction=nonstopmode -output-directory=\"" + workDir + "\" \"" + texFile + "\" 2>&1";
        compileLog = "Using compiler: " + compiler + "\n";

        FILE* pipe = popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                compileLog += buffer;
            }
            int returnCode = pclose(pipe);

            // Check if PDF was created
            if (returnCode == 0 && std::filesystem::exists(outputFile)) {
                compilationSuccess = true;
                break;
            } else {
                compileLog += "\nCompilation failed with return code: " + std::to_string(returnCode) + "\n";
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Read log file if it exists
    if (std::filesystem::exists(logFile)) {
        std::ifstream log(logFile);
        std::stringstream logBuffer;
        logBuffer << log.rdbuf();
        result.log = compileLog + "\n\n=== LaTeX Log ===\n" + logBuffer.str();
    } else {
        result.log = compileLog;
    }

    if (compilationSuccess) {
        result.success = true;
        result.pdfPath = outputFile;
        result.compileTime = duration.count();

        // 保存PDF到缓存（使用内容哈希）
        if (!contentHash.empty()) {
            impl_->savePdfToCache(outputFile, contentHash);
        }

        // 复制PDF到最终输出路径（如果不同）
        if (outputFile != outputPath) {
            try {
                std::filesystem::copy_file(outputFile, outputPath, std::filesystem::copy_options::overwrite_existing);
                result.pdfPath = outputPath;
            } catch (const std::exception& e) {
                spdlog::error("[LatexApiModule] Failed to copy PDF to output path: {}", e.what());
            }
        }
    } else {
        result.success = false;
        result.errorMessage = "LaTeX compilation failed. Please check your LaTeX syntax.";
        result.log = compileLog;
        result.pdfPath = createErrorPDF(workDir + "/error.pdf", "Compilation Failed", compileLog);
    }

    return result;
}

std::string LatexApiModule::getLatexTemplate() {
    return R"(%% !TeX program = xelatex
\documentclass[12pt,a4paper]{article}

% 中文支持
\usepackage{ctex}

% 页面设置
\usepackage{geometry}
\geometry{left=2.5cm,right=2.5cm,top=2.5cm,bottom=2.5cm}

% 数学公式
\usepackage{amsmath}
\usepackage{amssymb}

% 图片
\usepackage{graphicx}

% 表格
\usepackage{booktabs}
\usepackage{tabularx}

% 参考文献
\usepackage{cite}

% 超链接
\usepackage{hyperref}

% 代码高亮
\usepackage{listings}

\title{论文标题}
\author{作者姓名}
\date{\today}

\begin{document}

\maketitle

\begin{abstract}
这里是摘要内容。简要描述论文的主要内容和贡献。
\end{abstract}

\section{引言}
这是引言部分，介绍研究背景和动机。

\section{方法}
描述研究方法。

\section{结果}
展示研究结果。

\section{结论}
总结论文的主要发现和贡献。

\begin{thebibliography}{9}
\bibitem{lamport94}
  Leslie Lamport,
  \emph{\LaTeX: A Document Preparation System}.
  Addison Wesley, Massachusetts,
  2nd Edition,
  1994.
\end{thebibliography}

\end{document}
)";
}

std::string LatexApiModule::escapeJson(const std::string& str) {
    return impl_->escapeJson(str);
}

std::string LatexApiModule::buildJsonResponse(bool success, const std::string& message, const std::string& data) {
    return impl_->buildJsonResponse(success, message, data);
}

std::string LatexApiModule::buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data) {
    return impl_->buildJsonResponse(statusCode, success, message, data);
}

void LatexApiModule::initializeBuiltInTemplates() {
    // Article template
    LatexTemplate articleTmpl;
    articleTmpl.id = impl_->nextTemplateId_++;
    articleTmpl.name = "学术论文";
    articleTmpl.description = "标准学术论文模板";
    articleTmpl.category = "article";
    articleTmpl.icon = "document";
    articleTmpl.isBuiltIn = true;
    articleTmpl.content = getLatexTemplate();
    impl_->templates_[articleTmpl.id] = articleTmpl;

    // Book template
    LatexTemplate bookTmpl;
    bookTmpl.id = impl_->nextTemplateId_++;
    bookTmpl.name = "书籍";
    bookTmpl.description = "书籍编写模板";
    bookTmpl.category = "book";
    bookTmpl.icon = "book";
    bookTmpl.isBuiltIn = true;
    bookTmpl.content = getLatexTemplate();
    impl_->templates_[bookTmpl.id] = bookTmpl;

    // Presentation template
    LatexTemplate presTmpl;
    presTmpl.id = impl_->nextTemplateId_++;
    presTmpl.name = "演示文稿";
    presTmpl.description = "Beamer演示文稿模板";
    presTmpl.category = "presentation";
    presTmpl.icon = "presentation";
    presTmpl.isBuiltIn = true;
    presTmpl.content = getLatexTemplate();
    impl_->templates_[presTmpl.id] = presTmpl;
}

// ============================================================================
// 版本控制方法实现
// ============================================================================

std::string LatexApiModule::getStoragePath(const std::string& userId, int projectId, int fileId) {
    return impl_->pdfDirectory_ + "/versions/" + userId + "/project_" + std::to_string(projectId) + "/file_" + std::to_string(fileId);
}

std::optional<LatexVersionNode> LatexApiModule::saveVersion(int fileId, int projectId, const std::string& userId,
                                                             const std::string& content, const std::string& summary,
                                                             bool isAutoSave) {
    std::lock_guard<std::mutex> lock(saveMutex_);

    // 生成版本ID
    std::string versionId = "v_" + std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) + "_" + std::to_string(fileId);

    // 查找父版本（主线最新版本）
    std::string fileKey = userId + "_" + std::to_string(projectId) + "_" + std::to_string(fileId);
    std::string parentId;
    std::string branchId = "main";
    std::string branchName = "主线";

    auto it = impl_->fileVersions_.find(fileKey);
    if (it != impl_->fileVersions_.end() && !it->second.empty()) {
        parentId = it->second.back();
        auto parentIt = impl_->versions_.find(parentId);
        if (parentIt != impl_->versions_.end()) {
            branchId = parentIt->second.branchId;
            if (!parentIt->second.branchName.empty()) {
                branchName = parentIt->second.branchName;
            }
        }
    }

    // 创建版本节点
    LatexVersionNode version;
    version.id = versionId;
    version.parentId = parentId;
    version.branchId = branchId;
    version.branchName = branchName;
    version.content = content;
    version.summary = summary.empty() ? (isAutoSave ? "自动保存" : "手动保存") : summary;
    version.timestamp = std::chrono::system_clock::now();
    version.author = userId;
    version.isAutoSave = isAutoSave;
    version.fileId = std::to_string(fileId);
    version.projectId = std::to_string(projectId);
    version.userId = userId;
    version.position = impl_->nextVersionPosition_++;
    version.depth = (branchId == "main") ? 0 : 1;

    // 计算变更统计
    if (!parentId.empty()) {
        auto parentIt = impl_->versions_.find(parentId);
        if (parentIt != impl_->versions_.end()) {
            int changes = 0;
            for (size_t i = 0; i < std::min(content.size(), parentIt->second.content.size()); ++i) {
                if (content[i] != parentIt->second.content[i]) changes++;
            }
            version.changeCount = changes;
        }
    }

    version.totalLines = std::count(content.begin(), content.end(), '\n') + 1;

    // 保存版本
    impl_->versions_[versionId] = version;
    impl_->fileVersions_[fileKey].push_back(versionId);
    impl_->branchTips_[branchId] = versionId;

    spdlog::info("[LatexApiModule] Saved version {} for file {} (project {}, user {})",
                 versionId, fileId, projectId, userId);

    return version;
}

std::vector<LatexVersionNode> LatexApiModule::getVersionHistory(int fileId, int projectId, const std::string& userId) {
    std::vector<LatexVersionNode> result;
    std::string fileKey = userId + "_" + std::to_string(projectId) + "_" + std::to_string(fileId);

    auto it = impl_->fileVersions_.find(fileKey);
    if (it != impl_->fileVersions_.end()) {
        for (const auto& versionId : it->second) {
            auto vit = impl_->versions_.find(versionId);
            if (vit != impl_->versions_.end()) {
                result.push_back(vit->second);
            }
        }
    }

    return result;
}

std::vector<LatexVersionNode> LatexApiModule::getVersionTree(int fileId, int projectId, const std::string& userId) {
    std::vector<LatexVersionNode> result;
    std::string fileKey = userId + "_" + std::to_string(projectId) + "_" + std::to_string(fileId);

    auto it = impl_->fileVersions_.find(fileKey);
    if (it != impl_->fileVersions_.end()) {
        for (const auto& versionId : it->second) {
            auto vit = impl_->versions_.find(versionId);
            if (vit != impl_->versions_.end()) {
                result.push_back(vit->second);
            }
        }
    }

    // 按位置排序
    std::sort(result.begin(), result.end(), [](const LatexVersionNode& a, const LatexVersionNode& b) {
        return a.position < b.position;
    });

    return result;
}

std::optional<LatexVersionNode> LatexApiModule::restoreVersion(const std::string& versionId) {
    auto it = impl_->versions_.find(versionId);
    if (it == impl_->versions_.end()) {
        return std::nullopt;
    }

    // 解析文件和项目信息
    int fileId = std::stoi(it->second.fileId);
    int projectId = std::stoi(it->second.projectId);
    const std::string& userId = it->second.userId;

    // 创建恢复分支的新版本
    std::string branchName = "恢复分支_" + std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

    // 创建新分支
    auto branchVersion = saveVersion(fileId, projectId, userId, it->second.content,
                                      "恢复至版本: " + versionId, false);

    if (branchVersion) {
        branchVersion->parentId = versionId;
        branchVersion->branchName = branchName;
        branchVersion->depth = 1;

        // 更新存储
        impl_->versions_[branchVersion->id] = *branchVersion;

        spdlog::info("[LatexApiModule] Restored version {} as new version {}", versionId, branchVersion->id);
        return branchVersion;
    }

    return std::nullopt;
}

std::optional<LatexVersionNode> LatexApiModule::createBranch(const std::string& parentVersionId, const std::string& branchName) {
    auto it = impl_->versions_.find(parentVersionId);
    if (it == impl_->versions_.end()) {
        return std::nullopt;
    }

    // 创建新分支ID
    std::string newBranchId = "branch_" + std::to_string(impl_->branchCounters_["branch"]++);

    // 创建新版本节点（分支起点）
    LatexVersionNode newVersion = it->second;
    newVersion.id = "v_" + std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) + "_branch";
    newVersion.parentId = parentVersionId;
    newVersion.branchId = newBranchId;
    newVersion.branchName = branchName;
    newVersion.timestamp = std::chrono::system_clock::now();
    newVersion.position = impl_->nextVersionPosition_++;
    newVersion.depth = 1;
    newVersion.isMerged = false;

    impl_->versions_[newVersion.id] = newVersion;
    impl_->branchTips_[newBranchId] = newVersion.id;

    spdlog::info("[LatexApiModule] Created branch {} from version {}", newBranchId, parentVersionId);

    return newVersion;
}

std::optional<LatexVersionNode> LatexApiModule::mergeBranch(const std::string& branchId) {
    // 查找分支最新版本
    auto tipIt = impl_->branchTips_.find(branchId);
    if (tipIt == impl_->branchTips_.end()) {
        return std::nullopt;
    }

    auto branchVersionIt = impl_->versions_.find(tipIt->second);
    if (branchVersionIt == impl_->versions_.end()) {
        return std::nullopt;
    }

    // 创建合并后的主线版本
    int fileId = std::stoi(branchVersionIt->second.fileId);
    int projectId = std::stoi(branchVersionIt->second.projectId);
    const std::string& userId = branchVersionIt->second.userId;

    auto mergedVersion = saveVersion(fileId, projectId, userId, branchVersionIt->second.content,
                                      "合并分支: " + branchId, false);

    if (mergedVersion) {
        mergedVersion->branchId = "main";
        mergedVersion->branchName = "主线";
        mergedVersion->depth = 0;
        mergedVersion->isMerged = true;

        // 标记原分支为已合并
        impl_->versions_[tipIt->second].isMerged = true;

        impl_->versions_[mergedVersion->id] = *mergedVersion;
        impl_->branchTips_["main"] = mergedVersion->id;

        spdlog::info("[LatexApiModule] Merged branch {} into main as version {}", branchId, mergedVersion->id);
        return mergedVersion;
    }

    return std::nullopt;
}

bool LatexApiModule::deleteVersion(const std::string& versionId) {
    auto it = impl_->versions_.find(versionId);
    if (it == impl_->versions_.end()) {
        return false;
    }

    // 从文件版本列表中移除
    std::string fileKey = it->second.userId + "_" + it->second.projectId + "_" + it->second.fileId;
    auto fvIt = impl_->fileVersions_.find(fileKey);
    if (fvIt != impl_->fileVersions_.end()) {
        auto& versions = fvIt->second;
        versions.erase(std::remove(versions.begin(), versions.end(), versionId), versions.end());
    }

    // 删除版本
    impl_->versions_.erase(it);

    spdlog::info("[LatexApiModule] Deleted version {}", versionId);
    return true;
}

std::string LatexApiModule::compareVersions(const std::string& versionId1, const std::string& versionId2) {
    nlohmann::json result;

    auto it1 = impl_->versions_.find(versionId1);
    auto it2 = impl_->versions_.find(versionId2);

    if (it1 == impl_->versions_.end() || it2 == impl_->versions_.end()) {
        result["error"] = "One or both versions not found";
        return result.dump();
    }

    const auto& v1 = it1->second;
    const auto& v2 = it2->second;

    // 统计差异
    int additions = 0, deletions = 0, modifications = 0;

    std::vector<std::string> lines1, lines2;
    std::stringstream ss1(v1.content), ss2(v2.content);
    std::string line;

    while (std::getline(ss1, line)) lines1.push_back(line);
    while (std::getline(ss2, line)) lines2.push_back(line);

    // 简单行比较
    size_t maxLines = std::max(lines1.size(), lines2.size());

    for (size_t i = 0; i < maxLines; ++i) {
        if (i >= lines1.size()) {
            additions++;
        } else if (i >= lines2.size()) {
            deletions++;
        } else if (lines1[i] != lines2[i]) {
            modifications++;
        }
    }

    result["version1"] = v1.id;
    result["version2"] = v2.id;
    result["timestamp1"] = std::chrono::system_clock::to_time_t(v1.timestamp);
    result["timestamp2"] = std::chrono::system_clock::to_time_t(v2.timestamp);
    result["summary1"] = v1.summary;
    result["summary2"] = v2.summary;
    result["additions"] = additions;
    result["deletions"] = deletions;
    result["modifications"] = modifications;
    result["totalChanges"] = additions + deletions + modifications;
    result["lineCount1"] = lines1.size();
    result["lineCount2"] = lines2.size();

    return result.dump();
}

// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __attribute__((visibility("default")))

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
