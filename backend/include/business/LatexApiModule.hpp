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
#include <sstream>
#include <memory>

namespace PaperCrawler {

/**
 * @brief LaTeX文档信息
 */
struct LatexDocument {
    int id;
    std::string title;
    std::string content;
    std::string ownerId;
    bool isCollaborative{false};
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;
    std::chrono::system_clock::time_point lastAutoSave;
    int version{1};
    bool isCompiled{false};
    std::string pdfPath;

    // 序列化为JSON
    std::string toJSON() const {
        std::ostringstream json;
        json << "{\n";
        json << "  \"id\": " << id << ",\n";
        json << "  \"title\": \"" << title << "\",\n";
        json << "  \"content\": ";
        // 转义JSON字符串中的特殊字符
        std::string escapedContent = content;
        // 简单转义（生产环境应使用nlohmann/json）
        size_t pos = 0;
        while ((pos = escapedContent.find('\n', pos)) != std::string::npos) {
            escapedContent.replace(pos, 1, "\\n");
            pos += 2;
        }
        json << "\"" << escapedContent.substr(0, 1000) << (content.length() > 1000 ? "..." : "") << "\",\n";
        json << "  \"owner_id\": \"" << ownerId << "\",\n";
        json << "  \"is_collaborative\": " << (isCollaborative ? "true" : "false") << ",\n";
        json << "  \"version\": " << version << ",\n";
        json << "  \"is_compiled\": " << (isCompiled ? "true" : "false") << ",\n";
        json << "  \"pdf_path\": \"" << pdfPath << "\",\n";
        json << "  \"created_at\": \""
            << std::chrono::system_clock::to_time_t(createdAt) << "\",\n";
        json << "  \"updated_at\": \""
            << std::chrono::system_clock::to_time_t(updatedAt) << "\",\n";
        json << "  \"last_auto_save\": \""
            << std::chrono::system_clock::to_time_t(lastAutoSave) << "\"\n";
        json << "}";
        return json.str();
    }
};

/**
 * @brief LaTeX模板
 */
struct LatexTemplate {
    int id;
    std::string name;
    std::string description;
    std::string category;
    std::string content;
    std::string icon;
    bool isBuiltIn{true};
};

/**
 * @brief LaTeX编译结果
 */
struct LatexCompilationResult {
    bool success{false};
    std::string pdfPath;
    std::string log;
    std::string errorMessage;
    int compileTime{0};  // 毫秒
};

/**
 * @brief LaTeX文档统计
 */
struct LatexDocumentStats {
    int totalDocuments{0};
    int compiledDocuments{0};
    int collaborativeDocuments{0};
    int totalWords{0};
    int totalCharacters{0};
};

/**
 * @brief LaTeX API模块
 *
 * 功能：
 * 1. LaTeX文档CRUD操作
 * 2. LaTeX编译（LaTeX -> PDF）
 * 3. 自动保存
 * 4. 版本历史
 * 5. 模板管理
 * 6. 协作编辑支持
 *
 * 端点：
 * - GET    /api/latex/documents       - 列表（分页）
 * - GET    /api/latex/documents/:id   - 详情
 * - POST   /api/latex/documents       - 创建
 * - PUT    /api/latex/documents/:id   - 更新
 * - DELETE /api/latex/documents/:id   - 删除
 * - POST   /api/latex/documents/:id/compile - 编译
 * - POST   /api/latex/documents/:id/autosave - 自动保存
 * - GET    /api/latex/templates       - 模板列表
 * - GET    /api/latex/templates/:id   - 模板详情
 * - GET    /api/latex/stats           - 统计信息
 * - GET    /api/latex/documents/:id/pdf - 下载PDF
 */
class LatexApiModule : public BusinessModuleBase {
public:
    // 默认构造函数（用于DLL导出）
    LatexApiModule();

    // 构造函数：注入IDatabase依赖
    explicit LatexApiModule(std::shared_ptr<IDatabase> database);
    ~LatexApiModule() override;

    std::string getName() const override { return "LatexApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "LaTeX editor API with compilation and collaboration support";
    }

    /**
     * @brief 获取文档列表（分页）
     */
    std::vector<LatexDocument> listDocuments(int page = 1, int limit = 20, const std::string& ownerId = "");

    /**
     * @brief 获取文档详情
     */
    std::optional<LatexDocument> getDocument(int id);

    /**
     * @brief 创建文档
     */
    std::optional<LatexDocument> createDocument(const LatexDocument& document);

    /**
     * @brief 更新文档
     */
    bool updateDocument(int id, const LatexDocument& document);

    /**
     * @brief 删除文档
     */
    bool deleteDocument(int id);

    /**
     * @brief 编译文档（LaTeX -> PDF）
     */
    LatexCompilationResult compileDocument(int id);

    /**
     * @brief 自动保存文档
     */
    bool autoSaveDocument(int id, const std::string& content);

    /**
     * @brief 获取模板列表
     */
    std::vector<LatexTemplate> listTemplates(const std::string& category = "");

    /**
     * @brief 获取模板详情
     */
    std::optional<LatexTemplate> getTemplate(int id);

    /**
     * @brief 从模板创建文档
     */
    std::optional<LatexDocument> createFromTemplate(int templateId, const std::string& title, const std::string& ownerId);

    /**
     * @brief 获取统计信息
     */
    LatexDocumentStats getStats();

    /**
     * @brief 获取PDF路径
     */
    std::string getPDFPath(int id);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // 依赖注入：数据库接口（允许Mock测试）
    std::shared_ptr<IDatabase> database_;

    // 互斥锁
    std::mutex compileMutex_;
    std::mutex saveMutex_;

    void registerRoutes() override;  // BusinessModuleBase要求实现

    // HTTP请求处理器
    std::string handleListDocuments(const std::map<std::string, std::string>& params);
    std::string handleGetDocument(const std::map<std::string, std::string>& params);
    std::string handleCreateDocument(const std::string& body);
    std::string handleUpdateDocument(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteDocument(const std::map<std::string, std::string>& params);
    std::string handleCompile(const std::map<std::string, std::string>& params);
    std::string handleAutoSave(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleListTemplates(const std::map<std::string, std::string>& params);
    std::string handleGetTemplate(const std::map<std::string, std::string>& params);
    std::string handleCreateFromTemplate(const std::string& body);
    std::string handleStats();
    std::string handleDownloadPDF(const std::map<std::string, std::string>& params);

    // 辅助函数
    std::string buildJsonResponse(bool success, const std::string& message, const std::string& data = "");
    std::string escapeJson(const std::string& str);
    LatexCompilationResult compileLatex(const std::string& content, const std::string& outputPath);
    void initializeBuiltInTemplates();
};

} // namespace PaperCrawler
