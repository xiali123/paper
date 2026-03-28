#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace PaperCrawler {

// 前向声明
struct Paper;

/**
 * @brief 导出格式
 */
enum class ExportFormat {
    JSON,          // JSON格式
    BIBTEX,        // BibTeX格式
    ENDNOTE,       // EndNote格式
    CSV,           // CSV格式
    XML,           // XML格式
    PDF,           // PDF格式
    MARKDOWN       // Markdown格式
};

/**
 * @brief 导出选项
 */
struct ExportOptions {
    ExportFormat format{ExportFormat::JSON};
    bool includeAbstract{true};
    bool includeKeywords{true};
    bool includeReferences{false};
    bool includeCitations{true};
    bool includeMetadata{true};
    std::string locale{"en"};        // 语言/地区
    std::string templateName;        // 自定义模板
};

/**
 * @brief 导出任务状态
 */
enum class ExportTaskStatus {
    PENDING,
    PROCESSING,
    COMPLETED,
    FAILED
};

/**
 * @brief 导出任务
 */
struct ExportTask {
    std::string taskId;
    std::string userId;
    std::vector<int> paperIds;
    ExportOptions options;
    ExportTaskStatus status{ExportTaskStatus::PENDING};
    std::string downloadUrl;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point completedAt;
    std::string errorMessage;
    int fileSize{0};

    std::string toJson() const;
};

/**
 * @brief 导出统计
 */
struct ExportStats {
    uint64_t totalExports;
    uint64_t successfulExports;
    uint64_t failedExports;
    uint64_t totalBytesExported;
    std::map<ExportFormat, uint64_t> exportsByFormat;
    std::map<std::string, uint64_t> exportsByUser;
};

/**
 * @brief 导出API模块
 *
 * 路由：
 * - POST /api/export              - 创建导出任务
 * - GET  /api/export/:taskId      - 获取导出任务状态
 * - GET  /api/export/:taskId/download - 下载导出文件
 * - GET  /api/export/tasks        - 获取导出任务列表
 * - DELETE /api/export/:taskId    - 删除导出任务
 * - GET  /api/export/stats        - 导出统计
 * - GET  /api/export/formats      - 支持的导出格式
 * - POST /api/export/preview      - 预览导出结果
 * - GET  /api/export/templates    - 获取导出模板
 */
class ExportApiModule : public IModule {
public:
    ExportApiModule();
    ~ExportApiModule() override;

    std::string getName() const override { return "ExportApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Export and download API with multiple format support";
    }
    ModuleType getModuleType() const override { return ModuleType::BUSINESS; }
    std::string getRoutePrefix() const override { return "/api/export"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 创建导出任务
     */
    std::string createExportTask(const std::string& userId,
                                const std::vector<int>& paperIds,
                                const ExportOptions& options);

    /**
     * @brief 获取导出任务状态
     */
    std::optional<ExportTask> getExportTask(const std::string& taskId);

    /**
     * @brief 获取导出文件路径
     */
    std::string getExportFilePath(const std::string& taskId);

    /**
     * @brief 下载导出文件
     */
    std::vector<uint8_t> downloadExportFile(const std::string& taskId);

    /**
     * @brief 获取用户导出任务列表
     */
    std::vector<ExportTask> getUserExportTasks(const std::string& userId, int limit);

    /**
     * @brief 删除导出任务
     */
    bool deleteExportTask(const std::string& taskId);

    /**
     * @brief 获取导出统计
     */
    ExportStats getStats();

    /**
     * @brief 获取支持的导出格式
     */
    std::vector<ExportFormat> getSupportedFormats();

    /**
     * @brief 导出为JSON
     */
    std::string exportToJSON(const std::vector<Paper>& papers, const ExportOptions& options);

    /**
     * @brief 导出为BibTeX
     */
    std::string exportToBibTeX(const std::vector<Paper>& papers, const ExportOptions& options);

    /**
     * @brief 导出为EndNote
     */
    std::string exportToEndNote(const std::vector<Paper>& papers, const ExportOptions& options);

    /**
     * @brief 导出为CSV
     */
    std::string exportToCSV(const std::vector<Paper>& papers, const ExportOptions& options);

    /**
     * @brief 导出为XML
     */
    std::string exportToXML(const std::vector<Paper>& papers, const ExportOptions& options);

    /**
     * @brief 导出为Markdown
     */
    std::string exportToMarkdown(const std::vector<Paper>& papers, const ExportOptions& options);

    /**
     * @brief 预览导出结果（前N条）
     */
    std::string previewExport(const std::vector<int>& paperIds,
                             const ExportOptions& options,
                             int previewCount);

    /**
     * @brief 获取导出模板
     */
    std::map<std::string, std::string> getExportTemplates();

    /**
     * @brief 创建自定义导出模板
     */
    bool createExportTemplate(const std::string& name, const std::string& template);

    /**
     * @brief 批量导出
     */
    std::string batchExport(const std::vector<std::vector<int>>& paperGroups,
                          const std::vector<ExportOptions>& options);

    /**
     * @brief 清理过期导出文件
     */
    size_t cleanupExpiredExports(std::chrono::hours maxAge);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // 导出任务存储
    std::map<std::string, ExportTask> exportTasks_;
    std::map<std::string, std::vector<std::string>> userTasks_;  // userId -> taskIds
    std::vector<std::string> taskQueue_;
    int nextTaskId_{1};

    // 导出统计
    ExportStats stats_{};

    // 导出文件存储路径
    std::string exportDirectory_{"./exports"};

    // 支持的导出格式
    std::vector<ExportFormat> supportedFormats_;

    // 导出模板
    std::map<std::string, std::string> exportTemplates_;

    mutable std::mutex mutex_;

    // 辅助方法
    std::string generateTaskId();
    std::string formatAuthors(const std::string& authors);
    std::string formatBibTeXEntry(const Paper& paper);
    std::string formatEndNoteEntry(const Paper& paper);
    std::string escapeCSV(const std::string& value);
    std::string escapeXML(const std::string& value);
    std::string sanitizeFileName(const std::string& name);
    bool processExportTask(ExportTask& task);
    void updateStats(ExportFormat format, bool success, int bytes);
};

} // namespace PaperCrawler
