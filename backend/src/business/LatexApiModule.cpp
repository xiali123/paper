#include "business/LatexApiModule.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace PaperCrawler {

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
    spdlog::info("[LatexApi] Registering routes...");

    // 文档CRUD
    addRoute("/api/latex/documents", [this](const HttpRequest& req) -> HttpResponse {
        if (req.method == "GET") {
            auto params = parseQueryParams(req.queryString);
            std::string result = handleListDocuments(params);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        } else if (req.method == "POST") {
            std::string result = handleCreateDocument(req.body);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        }
        HttpResponse response;
        response.statusCode = 405;
        response.body = R"({"error":"Method Not Allowed"})";
        return response;
    });

    // 文档详情/更新/删除
    addRoute("/api/latex/documents/:id", [this](const HttpRequest& req) -> HttpResponse {
        auto params = req.pathParams;
        if (req.method == "GET") {
            std::string result = handleGetDocument(params);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        } else if (req.method == "PUT") {
            std::string result = handleUpdateDocument(params, req.body);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        } else if (req.method == "DELETE") {
            std::string result = handleDeleteDocument(params);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        }
        HttpResponse response;
        response.statusCode = 405;
        response.body = R"({"error":"Method Not Allowed"})";
        return response;
    });

    // 编译
    addRoute("/api/latex/documents/:id/compile", [this](const HttpRequest& req) -> HttpResponse {
        if (req.method == "POST") {
            auto params = req.pathParams;
            std::string result = handleCompile(params);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        }
        HttpResponse response;
        response.statusCode = 405;
        response.body = R"({"error":"Method Not Allowed"})";
        return response;
    });

    // 自动保存
    addRoute("/api/latex/documents/:id/autosave", [this](const HttpRequest& req) -> HttpResponse {
        if (req.method == "POST") {
            auto params = req.pathParams;
            std::string result = handleAutoSave(params, req.body);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        }
        HttpResponse response;
        response.statusCode = 405;
        response.body = R"({"error":"Method Not Allowed"})";
        return response;
    });

    // 模板列表
    addRoute("/api/latex/templates", [this](const HttpRequest& req) -> HttpResponse {
        if (req.method == "GET") {
            auto params = parseQueryParams(req.queryString);
            std::string result = handleListTemplates(params);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        } else if (req.method == "POST") {
            std::string result = handleCreateFromTemplate(req.body);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        }
        HttpResponse response;
        response.statusCode = 405;
        response.body = R"({"error":"Method Not Allowed"})";
        return response;
    });

    // 模板详情
    addRoute("/api/latex/templates/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (req.method == "GET") {
            auto params = req.pathParams;
            std::string result = handleGetTemplate(params);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        }
        HttpResponse response;
        response.statusCode = 405;
        response.body = R"({"error":"Method Not Allowed"})";
        return response;
    });

    // 统计信息
    addRoute("/api/latex/stats", [this](const HttpRequest& req) -> HttpResponse {
        if (req.method == "GET") {
            std::string result = handleStats();
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        }
        HttpResponse response;
        response.statusCode = 405;
        response.body = R"({"error":"Method Not Allowed"})";
        return response;
    });

    // PDF下载
    addRoute("/api/latex/documents/:id/pdf", [this](const HttpRequest& req) -> HttpResponse {
        if (req.method == "GET") {
            auto params = req.pathParams;
            std::string result = handleDownloadPDF(params);
            HttpResponse response;
            response.statusCode = 200;
            response.setHeader("Content-Type", "application/json");
            response.body = result;
            return response;
        }
        HttpResponse response;
        response.statusCode = 405;
        response.body = R"({"error":"Method Not Allowed"})";
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
            return buildJsonResponse(false, "Document not found", "", 404);
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
            return buildJsonResponse(false, "Failed to update document", "", 404);
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
            return buildJsonResponse(false, "Failed to delete document", "", 404);
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
            return buildJsonResponse(false, "Failed to auto-save", "", 404);
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
            return buildJsonResponse(false, "Template not found", "", 404);
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
            return buildJsonResponse(false, "PDF not found", "", 404);
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
// 业务逻辑实现
// ==========================================

std::vector<LatexDocument> LatexApiModule::listDocuments(int page, int limit, const std::string& ownerId) {
    // TODO: 实现数据库查询
    spdlog::info("[LatexApi] Listing documents: page={}, limit={}, owner={}", page, limit, ownerId);
    return {};
}

std::optional<LatexDocument> LatexApiModule::getDocument(int id) {
    spdlog::info("[LatexApi] Getting document: id={}", id);

    // TODO: 从数据库查询
    // 返回一个示例文档
    LatexDocument doc;
    doc.id = id;
    doc.title = "Example Document";
    doc.content = "\\documentclass{article}\n\\begin{document}\nHello, World!\n\\end{document}";
    doc.ownerId = "user1";
    doc.createdAt = std::chrono::system_clock::now();
    doc.updatedAt = std::chrono::system_clock::now();
    doc.lastAutoSave = std::chrono::system_clock::now();
    doc.version = 1;
    doc.isCompiled = false;
    return doc;
}

std::optional<LatexDocument> LatexApiModule::createDocument(const LatexDocument& document) {
    spdlog::info("[LatexApi] Creating document: title={}", document.title);

    // TODO: 保存到数据库
    LatexDocument created = document;
    created.id = 1; // 模拟生成的ID
    return created;
}

bool LatexApiModule::updateDocument(int id, const LatexDocument& document) {
    spdlog::info("[LatexApi] Updating document: id={}", id);
    // TODO: 更新数据库
    return true;
}

bool LatexApiModule::deleteDocument(int id) {
    spdlog::info("[LatexApi] Deleting document: id={}", id);
    // TODO: 从数据库删除
    return true;
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

std::string LatexApiModule::buildJsonResponse(bool success, const::string& message, const std::string& data, int statusCode) {
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

std::string LatexApiModule::escapeJson(const std::string& str) {
    // 简单的JSON转义
    std::string result = str;
    // TODO: 完整的JSON转义
    return result;
}

LatexCompilationResult LatexApiModule::compileLatex(const std::string& content, const std::string& outputPath) {
    LatexCompilationResult result;

    try {
        // 保存.tex文件
        std::string texPath = outputPath.substr(0, outputPath.find_last_of('.')) + ".tex";
        std::ofstream texFile(texPath);
        texFile << content;
        texFile.close();

        // TODO: 调用pdflatex编译
        // 现在返回stub结果
        result.success = true;
        result.pdfPath = outputPath;
        result.compileTime = 1000;
        result.log = "LaTeX compilation successful (stub)";

        spdlog::info("[LatexApi] LaTeX compilation stub: {} -> {}", texPath, outputPath);
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
        spdlog::error("[LatexApi] LaTeX compilation failed: {}", e.what());
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
// DLL导出函数
// ==========================================

#define EXPORT __declspec(dllexport)

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
