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
#include <unordered_map>

namespace PaperCrawler {

// 前向声明
class Router;

/**
 * @brief 协作用户信息
 */
struct LatexCollaborationUser {
    std::string connectionId;
    std::string userId;
    std::string userName;
    std::string color;
    std::pair<int, int> cursorPosition;  // line, column
    std::pair<int, int> selectionStart;    // line, column
    std::pair<int, int> selectionEnd;      // line, column
    bool isActive;
    std::chrono::system_clock::time_point lastActivity;

    std::string toJSON() const {
        std::ostringstream json;
        json << "{";
        json << "\"connectionId\":\"" << connectionId << "\",";
        json << "\"userId\":\"" << userId << "\",";
        json << "\"userName\":\"" << userName << "\",";
        json << "\"color\":\"" << color << "\",";
        json << "\"cursorPosition\":{\"line\":" << cursorPosition.first << ",\"column\":" << cursorPosition.second << "},";
        json << "\"selectionStart\":{\"line\":" << selectionStart.first << ",\"column\":" << selectionStart.second << "},";
        json << "\"selectionEnd\":{\"line\":" << selectionEnd.first << ",\"column\":" << selectionEnd.second << "},";
        json << "\"isActive\":" << (isActive ? "true" : "false") << ",";
        json << "\"lastActivity\":" << std::chrono::system_clock::to_time_t(lastActivity);
        json << "}";
        return json.str();
    }
};

/**
 * @brief 协作会话
 */
struct LatexCollaborationSession {
    std::string sessionId;
    int documentId;
    std::string documentTitle;
    std::unordered_map<std::string, LatexCollaborationUser> users;
    std::string documentContent;  // 当前文档内容
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastActivity;

    size_t getActiveUserCount() const {
        auto now = std::chrono::system_clock::now();
        size_t count = 0;
        for (const auto& [id, user] : users) {
            auto inactiveTime = std::chrono::duration_cast<std::chrono::seconds>(now - user.lastActivity);
            if (inactiveTime.count() < 300) {  // 5分钟内活跃
                count++;
            }
        }
        return count;
    }
};

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
 * @brief LaTeX版本节点
 */
struct LatexVersionNode {
    std::string id;                // UUID
    std::string parentId;          // 父版本ID
    std::string branchId;          // 分支ID
    std::string branchName;        // 分支名称 ("主线" / "恢复分支_xxx")
    std::string content;           // 完整内容
    std::string summary;           // 摘要
    std::chrono::system_clock::time_point timestamp;
    std::string author;
    bool isAutoSave{false};
    int changeCount{0};
    int totalLines{0};
    std::string fileId;
    std::string projectId;
    std::string userId;
    int position{0};                // 时间线位置
    int depth{0};                   // 深度(主线=0, 分支=1)
    bool isMerged{false};          // 是否已合并
};

/**
 * @brief 用户编译配额
 */
struct LatexUserQuota {
    std::string userId;
    int dailyCompileLimit{10};      // 每日编译次数限制
    int monthlyCompileLimit{100};    // 每月编译次数限制
    int maxProjectCount{5};          // 最大项目数
    bool canUseAdvancedFeatures{false}; // 是否可以使用高级功能
    std::vector<std::string> allowedPackages; // 允许使用的LaTeX包
    std::chrono::system_clock::time_point dailyReset;
    std::chrono::system_clock::time_point monthlyReset;

    // 当前使用统计
    int dailyCompilesUsed{0};
    int monthlyCompilesUsed{0};
    int projectCountUsed{0};

    // 序列化为JSON
    std::string toJSON() const {
        std::ostringstream json;
        json << "{";
        json << "\"userId\":\"" << userId << "\",";
        json << "\"dailyCompileLimit\":" << dailyCompileLimit << ",";
        json << "\"monthlyCompileLimit\":" << monthlyCompileLimit << ",";
        json << "\"maxProjectCount\":" << maxProjectCount << ",";
        json << "\"canUseAdvancedFeatures\":" << (canUseAdvancedFeatures ? "true" : "false") << ",";
        json << "\"dailyCompilesUsed\":" << dailyCompilesUsed << ",";
        json << "\"monthlyCompilesUsed\":" << monthlyCompilesUsed << ",";
        json << "\"projectCountUsed\":" << projectCountUsed << ",";
        json << "\"dailyReset\":" << std::chrono::system_clock::to_time_t(dailyReset) << ",";
        json << "\"monthlyReset\":" << std::chrono::system_clock::to_time_t(monthlyReset);
        json << "}";
        return json.str();
    }
};

/**
 * @brief 编译记录
 */
struct LatexCompilationRecord {
    int id;
    std::string userId;
    int projectId;  // 0表示单文档
    std::string documentId;
    std::string contentHash;  // 内容哈希，用于去重
    bool success;
    std::string errorMessage;
    std::chrono::system_clock::time_point timestamp;

    std::string toJSON() const {
        std::ostringstream json;
        json << "{";
        json << "\"id\":" << id << ",";
        json << "\"userId\":\"" << userId << "\",";
        json << "\"projectId\":" << projectId << ",";
        json << "\"documentId\":\"" << documentId << "\",";
        json << "\"contentHash\":\"" << contentHash << "\",";
        json << "\"success\":" << (success ? "true" : "false") << ",";
        json << "\"errorMessage\":\"" << errorMessage << "\",";
        json << "\"timestamp\":" << std::chrono::system_clock::to_time_t(timestamp);
        json << "}";
        return json.str();
    }
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
 * @brief LaTeX项目中的文件
 */
struct LatexProjectFile {
    int id;
    int projectId;
    std::string name;
    std::string path;  // 相对路径，如 "main.tex", "chapters/chapter1.tex"
    std::string content;
    std::string type;  // "main", "included", "bibliography", "image", "other"
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;
};

/**
 * @brief LaTeX项目（支持多文件）
 */
struct LatexProject {
    int id;
    std::string name;
    std::string ownerId;
    std::string mainFile;  // 主文件路径，如 "main.tex"
    std::string description;
    bool isPublic{false};
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;
    int version{1};

    // 项目中的文件列表
    std::vector<LatexProjectFile> files;
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
     * @param id 文档ID
     * @param userId 用户ID（用于配额检查，为空则不检查）
     */
    LatexCompilationResult compileDocument(int id, const std::string& userId = "");

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

    /**
     * @brief 加入协作会话
     */
    std::string joinCollaboration(int documentId, const std::string& userId, const std::string& userName);

    /**
     * @brief 离开协作会话
     */
    bool leaveCollaboration(const std::string& sessionId, const std::string& userId);

    /**
     * @brief 更新光标位置
     */
    bool updateCursorPosition(const std::string& sessionId, const std::string& userId,
                               int line, int column,
                               int selectionStartLine, int selectionStartColumn,
                               int selectionEndLine, int selectionEndColumn);

    /**
     * @brief 广播文档内容更新
     */
    bool broadcastDocumentUpdate(const std::string& sessionId, const std::string& content,
                                 const std::string& excludeUserId = "");

    /**
     * @brief 获取协作会话信息
     */
    std::optional<LatexCollaborationSession> getCollaborationSession(const std::string& sessionId);

    /**
     * @brief 获取所有活跃协作会话
     */
    std::vector<LatexCollaborationSession> getActiveCollaborationSessions();

    // ==========================================
    // 项目管理方法（多文件支持）
    // ==========================================

    /**
     * @brief 获取项目列表
     */
    std::vector<LatexProject> listProjects(int page = 1, int limit = 20, const std::string& ownerId = "");

    /**
     * @brief 获取项目详情
     */
    std::optional<LatexProject> getProject(int id);

    /**
     * @brief 创建项目
     */
    std::optional<LatexProject> createProject(const LatexProject& project);

    /**
     * @brief 更新项目
     */
    bool updateProject(int id, const LatexProject& project);

    /**
     * @brief 删除项目
     */
    bool deleteProject(int id);

    /**
     * @brief 编译项目（使用主文件）
     * @param id 项目ID
     * @param userId 用户ID（用于配额检查，为空则不检查）
     */
    LatexCompilationResult compileProject(int id, const std::string& userId = "");

    /**
     * @brief 添加文件到项目
     */
    std::optional<LatexProjectFile> addProjectFile(int projectId, const LatexProjectFile& file);

    /**
     * @brief 更新项目文件
     */
    bool updateProjectFile(int fileId, const LatexProjectFile& file);

    /**
     * @brief 删除项目文件
     */
    bool deleteProjectFile(int fileId);

    /**
     * @brief 获取项目文件内容
     */
    std::optional<LatexProjectFile> getProjectFile(int fileId);

    // ==========================================
    // 用户配额和权限管理方法
    // ==========================================

    /**
     * @brief 获取用户配额信息
     */
    std::optional<LatexUserQuota> getUserQuota(const std::string& userId);

    /**
     * @brief 设置用户配额
     */
    bool setUserQuota(const LatexUserQuota& quota);

    /**
     * @brief 初始化用户默认配额
     * @param userId 用户ID
     * @param tier 套餐类型: "free", "pro", "admin"
     */
    void initializeUserQuota(const std::string& userId, const std::string& tier = "free");

    /**
     * @brief 检查用户是否可以编译
     */
    bool canUserCompile(const std::string& userId);

    /**
     * @brief 获取用户编译记录
     */
    std::vector<LatexCompilationRecord> getUserCompilationRecords(const std::string& userId, int limit = 100);

    // ==========================================
    // 版本控制方法
    // ==========================================

    /**
     * @brief 保存版本
     * @param fileId 文件ID
     * @param projectId 项目ID
     * @param userId 用户ID
     * @param content 内容
     * @param summary 摘要
     * @param isAutoSave 是否自动保存
     * @return 保存的版本节点
     */
    std::optional<LatexVersionNode> saveVersion(int fileId, int projectId, const std::string& userId,
                                                  const std::string& content, const std::string& summary,
                                                  bool isAutoSave = false);

    /**
     * @brief 获取版本历史
     */
    std::vector<LatexVersionNode> getVersionHistory(int fileId, int projectId, const std::string& userId);

    /**
     * @brief 获取版本树（用于可视化分支）
     */
    std::vector<LatexVersionNode> getVersionTree(int fileId, int projectId, const std::string& userId);

    /**
     * @brief 恢复版本
     * @param versionId 版本ID
     * @return 新版本节点（恢复后的版本）
     */
    std::optional<LatexVersionNode> restoreVersion(const std::string& versionId);

    /**
     * @brief 创建分支
     */
    std::optional<LatexVersionNode> createBranch(const std::string& parentVersionId, const std::string& branchName);

    /**
     * @brief 合并分支到主线
     */
    std::optional<LatexVersionNode> mergeBranch(const std::string& branchId);

    /**
     * @brief 删除版本
     */
    bool deleteVersion(const std::string& versionId);

    /**
     * @brief 比较两个版本
     */
    std::string compareVersions(const std::string& versionId1, const std::string& versionId2);

    /**
     * @brief 获取存储路径
     */
    std::string getStoragePath(const std::string& userId, int projectId, int fileId);

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
    std::string handleCompileDocument(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleCompileProject(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleAutoSave(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleListTemplates(const std::map<std::string, std::string>& params);
    std::string handleGetTemplate(const std::map<std::string, std::string>& params);
    std::string handleCreateFromTemplate(const std::string& body);
    std::string handleStats();
    std::string handleDownloadPDF(const std::map<std::string, std::string>& params);
    HttpResponse handleDownloadPDFBinary(const std::map<std::string, std::string>& params);
    HttpResponse handleDownloadProjectPDFBinary(const std::map<std::string, std::string>& params);

    // 协作HTTP请求处理器
    std::string handleJoinCollaboration(const std::string& body);
    std::string handleLeaveCollaboration(const std::string& body);
    std::string handleUpdateCursor(const std::string& body);
    std::string handleBroadcastUpdate(const std::string& body);
    std::string handleListCollaborationSessions();

    // 项目HTTP请求处理器（多文件支持）
    std::string handleListProjects(const std::map<std::string, std::string>& params);
    std::string handleGetProject(const std::map<std::string, std::string>& params);
    std::string handleCreateProject(const std::string& body);
    std::string handleUpdateProject(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteProject(const std::map<std::string, std::string>& params);
    std::string handleCompileProject(const std::map<std::string, std::string>& params);
    std::string handleAddProjectFile(const std::string& body);
    std::string handleUpdateProjectFile(const std::map<std::string, std::string>& params, const std::string& body);
    std::string handleDeleteProjectFile(const std::map<std::string, std::string>& params);
    std::string handleGetProjectFile(const std::map<std::string, std::string>& params);

    // 用户配额HTTP请求处理器
    std::string handleGetUserQuota(const std::map<std::string, std::string>& params);
    std::string handleSetUserQuota(const std::string& body);
    std::string handleInitializeUserQuota(const std::string& body);
    std::string handleGetUserCompilationRecords(const std::map<std::string, std::string>& params);

    // 版本控制HTTP请求处理器
    std::string handleSaveVersion(const std::string& body);
    std::string handleGetVersionHistory(const std::map<std::string, std::string>& params);
    std::string handleGetVersionTree(const std::map<std::string, std::string>& params);
    std::string handleRestoreVersion(const std::string& body);
    std::string handleCreateBranch(const std::string& body);
    std::string handleMergeBranch(const std::string& body);
    std::string handleDeleteVersion(const std::map<std::string, std::string>& params);
    std::string handleCompareVersions(const std::map<std::string, std::string>& params);

    // 辅助函数
    std::string buildJsonResponse(bool success, const std::string& message, const std::string& data = "");
    std::string buildJsonResponse(int statusCode, bool success, const std::string& message, const std::string& data = "");
    std::string escapeJson(const std::string& str);
    LatexCompilationResult compileLatex(const std::string& content, const std::string& outputPath);
    LatexCompilationResult compileLatexContent(const std::string& content, const std::string& outputPath, const std::string& workDir);
    std::string createErrorPDF(const std::string& outputPath, const std::string& title, const std::string& errorMessage);
    void initializeBuiltInTemplates();
    std::string getLatexTemplate();
};

} // namespace PaperCrawler
