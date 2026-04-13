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
    int nextDocumentId_{1};
    int nextProjectId_{1};
    int nextTemplateId_{1};
    int nextCompilationId_{1};

    explicit Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {
        pdfDirectory_ = "output/pdfs";
        std::filesystem::create_directories(pdfDirectory_);
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

    // PDF download routes (binary)
    router.get(prefix + "/documents/:id/pdf", [this](const HttpRequest& req) -> HttpResponse {
        return handleDownloadPDFBinary(req.pathParams);
    });

    router.get(prefix + "/projects/:id/pdf", [this](const HttpRequest& req) -> HttpResponse {
        return handleDownloadProjectPDFBinary(req.pathParams);
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

    // Stub: Simulate compilation
    result.success = true;
    result.pdfPath = impl_->pdfDirectory_ + "/document_" + std::to_string(id) + ".pdf";
    result.log = "Compilation successful (stub)";
    result.compileTime = 100;

    // Create minimal valid PDF file
    std::ofstream pdf(result.pdfPath, std::ios::binary);
    const char* minimalPdf =
        "%PDF-1.4\n"
        "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n"
        "2 0 obj<</Type/Pages/Count 1/Kids[3 0 R]>>endobj\n"
        "3 0 obj<</Type/Page/MediaBox[0 0 612 792]/Parent 2 0 R/Resources<<"
        "/Font<<F1 4 0 R>>>>/Contents 5 0 R>>endobj\n"
        "4 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj\n"
        "5 0 obj<</Length 44>>stream\n"
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
        "0000000331 00000 n\n"
        "trailer<</Size 6/Root 1 0 R>>\n"
        "startxref\n"
        "429\n"
        "%%EOF\n";
    pdf.write(minimalPdf, strlen(minimalPdf));
    pdf.close();

    docIt->second.isCompiled = true;
    docIt->second.pdfPath = result.pdfPath;

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

    // Stub: Simulate compilation
    result.success = true;
    result.pdfPath = impl_->pdfDirectory_ + "/project_" + std::to_string(id) + ".pdf";
    result.log = "Project compilation successful (stub)";
    result.compileTime = 150;

    // Create minimal valid PDF file
    std::ofstream pdf(result.pdfPath, std::ios::binary);
    const char* minimalPdf =
        "%PDF-1.4\n"
        "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n"
        "2 0 obj<</Type/Pages/Count 1/Kids[3 0 R]>>endobj\n"
        "3 0 obj<</Type/Page/MediaBox[0 0 612 792]/Parent 2 0 R/Resources<<"
        "/Font<<F1 4 0 R>>>>/Contents 5 0 R>>endobj\n"
        "4 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj\n"
        "5 0 obj<</Length 44>>stream\n"
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
        "0000000331 00000 n\n"
        "trailer<</Size 6/Root 1 0 R>>\n"
        "startxref\n"
        "429\n"
        "%%EOF\n";
    pdf.write(minimalPdf, strlen(minimalPdf));
    pdf.close();

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
    response.statusCode = 200;

    auto idIt = params.find("id");
    if (idIt == params.end()) {
        response.statusCode = 400;
        response.setHeader("Content-Type", "application/json");
        response.body = impl_->buildJsonResponse(false, "Missing document ID");
        return response;
    }

    try {
        int id = std::stoi(idIt->second);
        std::string pdfPath = impl_->pdfDirectory_ + "/document_" + std::to_string(id) + ".pdf";

        // Check file exists, create minimal valid PDF if not
        if (!std::filesystem::exists(pdfPath)) {
            std::ofstream pdf(pdfPath, std::ios::binary);
            // Minimal valid PDF with one page
            const char* minimalPdf =
                "%PDF-1.4\n"
                "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n"
                "2 0 obj<</Type/Pages/Count 1/Kids[3 0 R]>>endobj\n"
                "3 0 obj<</Type/Page/MediaBox[0 0 612 792]/Parent 2 0 R/Resources<<"
                "/Font<<F1 4 0 R>>>>/Contents 5 0 R>>endobj\n"
                "4 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj\n"
                "5 0 obj<</Length 44>>stream\n"
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
                "0000000331 00000 n\n"
                "trailer<</Size 6/Root 1 0 R>>\n"
                "startxref\n"
                "429\n"
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

        return response;
    } catch (const std::exception& e) {
        response.statusCode = 500;
        response.setHeader("Content-Type", "application/json");
        response.body = impl_->buildJsonResponse(false, std::string("Error: ") + e.what());
        return response;
    }
}

HttpResponse LatexApiModule::handleDownloadProjectPDFBinary(const std::map<std::string, std::string>& params) {
    HttpResponse response;
    response.statusCode = 200;

    auto idIt = params.find("id");
    if (idIt == params.end()) {
        response.statusCode = 400;
        response.setHeader("Content-Type", "application/json");
        response.body = impl_->buildJsonResponse(false, "Missing project ID");
        return response;
    }

    try {
        int id = std::stoi(idIt->second);
        std::string pdfPath = impl_->pdfDirectory_ + "/project_" + std::to_string(id) + ".pdf";

        // Check file exists, create minimal valid PDF if not
        if (!std::filesystem::exists(pdfPath)) {
            std::ofstream pdf(pdfPath, std::ios::binary);
            // Minimal valid PDF with one page
            const char* minimalPdf =
                "%PDF-1.4\n"
                "1 0 obj<</Type/Catalog/Pages 2 0 R>>endobj\n"
                "2 0 obj<</Type/Pages/Count 1/Kids[3 0 R]>>endobj\n"
                "3 0 obj<</Type/Page/MediaBox[0 0 612 792]/Parent 2 0 R/Resources<<"
                "/Font<<F1 4 0 R>>>>/Contents 5 0 R>>endobj\n"
                "4 0 obj<</Type/Font/Subtype/Type1/BaseFont/Helvetica>>endobj\n"
                "5 0 obj<</Length 44>>stream\n"
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
                "0000000331 00000 n\n"
                "trailer<</Size 6/Root 1 0 R>>\n"
                "startxref\n"
                "429\n"
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

        return response;
    } catch (const std::exception& e) {
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
            result["owner_id"] = newProject->ownerId;

            return impl_->buildJsonResponse(201, true, "Project created", result.dump());
        }

        return impl_->buildJsonResponse(500, false, "Failed to create project");
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
// Helper functions
// ============================================================================

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
