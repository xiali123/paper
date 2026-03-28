#include "business/ExportApiModule.hpp"
#include "business/PaperApiModule.hpp"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace PaperCrawler {

// ============================================================================
// 辅助函数：JSON序列化
// ============================================================================

std::string ExportTask::toJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"task_id\": \"" << taskId << "\",\n";
    json << "  \"user_id\": \"" << userId << "\",\n";
    json << "  \"paper_ids\": [";

    for (size_t i = 0; i < paperIds.size(); ++i) {
        if (i > 0) json << ",";
        json << paperIds[i];
    }

    json << "],\n";

    // 状态转换
    std::string statusStr;
    switch (status) {
        case ExportTaskStatus::PENDING: statusStr = "pending"; break;
        case ExportTaskStatus::PROCESSING: statusStr = "processing"; break;
        case ExportTaskStatus::COMPLETED: statusStr = "completed"; break;
        case ExportTaskStatus::FAILED: statusStr = "failed"; break;
    }
    json << "  \"status\": \"" << statusStr << "\",\n";
    json << "  \"download_url\": \"" << downloadUrl << "\",\n";
    json << "  \"file_size\": " << fileSize << "\n";
    json << "}";
    return json.str();
}

// ============================================================================
// ExportApiModule::Impl
// ============================================================================

class ExportApiModule::Impl {
public:
    Impl() {
        // 初始化导出目录
        std::filesystem::create_directories("./exports");
    }

    std::map<int, Paper> mockPapers;
};

// ============================================================================
// ExportApiModule
// ============================================================================

ExportApiModule::ExportApiModule()
    : impl_(std::make_unique<Impl>()) {

    // 初始化支持的导出格式
    supportedFormats_ = {
        ExportFormat::JSON,
        ExportFormat::BIBTEX,
        ExportFormat::ENDNOTE,
        ExportFormat::CSV,
        ExportFormat::XML,
        ExportFormat::MARKDOWN
    };

    // 初始化默认模板
    exportTemplates_["default_bibtex"] = "@article{id,\n  title={title},\n  author={author},\n  year={year}\n}";
    exportTemplates_["default_csv"] = "ID,Title,Author,Year\n";
}

ExportApiModule::~ExportApiModule() = default;

bool ExportApiModule::initialize() {
    std::cout << "ExportApiModule initialized" << std::endl;
    std::cout << "  - Export directory: " << exportDirectory_ << std::endl;
    std::cout << "  - Supported formats: " << supportedFormats_.size() << std::endl;
    return true;
}

bool ExportApiModule::start() {
    std::cout << "ExportApiModule started" << std::endl;
    return true;
}

bool ExportApiModule::stop() {
    std::cout << "ExportApiModule stopped" << std::endl;
    return true;
}

void ExportApiModule::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    exportTasks_.clear();
    userTasks_.clear();
}

std::string ExportApiModule::createExportTask(const std::string& userId,
                                             const std::vector<int>& paperIds,
                                             const ExportOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);

    ExportTask task;
    task.taskId = generateTaskId();
    task.userId = userId;
    task.paperIds = paperIds;
    task.options = options;
    task.status = ExportTaskStatus::PENDING;
    task.createdAt = std::chrono::system_clock::now();

    exportTasks_[task.taskId] = task;
    userTasks_[userId].push_back(task.taskId);
    taskQueue_.push_back(task.taskId);

    std::cout << "[ExportApi] Created export task: " << task.taskId
              << " for user: " << userId << " (" << paperIds.size() << " papers)" << std::endl;

    return task.taskId;
}

std::optional<ExportTask> ExportApiModule::getExportTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = exportTasks_.find(taskId);
    if (it != exportTasks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string ExportApiModule::getExportFilePath(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = exportTasks_.find(taskId);
    if (it != exportTasks_.end()) {
        return exportDirectory_ + "/" + taskId + ".export";
    }
    return "";
}

std::vector<uint8_t> ExportApiModule::downloadExportFile(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = exportTasks_.find(taskId);
    if (it == exportTasks_.end()) {
        return {};
    }

    const ExportTask& task = it->second;
    if (task.status != ExportTaskStatus::COMPLETED) {
        return {};
    }

    std::string filePath = exportDirectory_ + "/" + taskId + ".export";

    // 读取文件
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());

    return buffer;
}

std::vector<ExportTask> ExportApiModule::getUserExportTasks(const std::string& userId, int limit) {
    std::vector<ExportTask> tasks;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = userTasks_.find(userId);
    if (it != userTasks_.end()) {
        const auto& taskIds = it->second;
        size_t start = 0;
        size_t end = std::min(limit, static_cast<int>(taskIds.size()));

        for (size_t i = start; i < end; ++i) {
            auto taskIt = exportTasks_.find(taskIds[i]);
            if (taskIt != exportTasks_.end()) {
                tasks.push_back(taskIt->second);
            }
        }
    }

    return tasks;
}

bool ExportApiModule::deleteExportTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = exportTasks_.find(taskId);
    if (it == exportTasks_.end()) {
        return false;
    }

    const ExportTask& task = it->second;

    // 删除文件
    std::string filePath = exportDirectory_ + "/" + taskId + ".export";
    std::filesystem::remove(filePath);

    // 从用户任务列表中移除
    auto userIt = userTasks_.find(task.userId);
    if (userIt != userTasks_.end()) {
        auto& taskIds = userIt->second;
        taskIds.erase(std::remove(taskIds.begin(), taskIds.end(), taskId), taskIds.end());
    }

    exportTasks_.erase(it);

    std::cout << "[ExportApi] Deleted export task: " << taskId << std::endl;

    return true;
}

ExportStats ExportApiModule::getStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

std::vector<ExportFormat> ExportApiModule::getSupportedFormats() {
    return supportedFormats_;
}

std::string ExportApiModule::exportToJSON(const std::vector<Paper>& papers, const ExportOptions& options) {
    std::ostringstream json;
    json << "[\n";

    for (size_t i = 0; i < papers.size(); ++i) {
        if (i > 0) json << ",\n";
        json << "  " << papers[i].toJSON();
    }

    json << "\n]";
    return json.str();
}

std::string ExportApiModule::exportToBibTeX(const std::vector<Paper>& papers, const ExportOptions& options) {
    std::ostringstream bibtex;

    for (const auto& paper : papers) {
        bibtex << formatBibTeXEntry(paper) << "\n\n";
    }

    return bibtex.str();
}

std::string ExportApiModule::exportToEndNote(const std::vector<Paper>& papers, const ExportOptions& options) {
    std::ostringstream endnote;

    for (const auto& paper : papers) {
        endnote << formatEndNoteEntry(paper) << "\n\n";
    }

    return endnote.str();
}

std::string ExportApiModule::exportToCSV(const std::vector<Paper>& papers, const ExportOptions& options) {
    std::ostringstream csv;

    // CSV头部
    csv << "ID,Title,Authors,Year,Journal,Citation Count\n";

    // 数据行
    for (const auto& paper : papers) {
        csv << paper.id << ","
            << escapeCSV(paper.title) << ","
            << escapeCSV(paper.authors) << ","
            << paper.year << ","
            << escapeCSV(paper.journal) << ","
            << paper.citationCount << "\n";
    }

    return csv.str();
}

std::string ExportApiModule::exportToXML(const std::vector<Paper>& papers, const ExportOptions& options) {
    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml << "<papers>\n";

    for (const auto& paper : papers) {
        xml << "  <paper>\n";
        xml << "    <id>" << paper.id << "</id>\n";
        xml << "    <title>" << escapeXML(paper.title) << "</title>\n";
        xml << "    <authors>" << escapeXML(paper.authors) << "</authors>\n";
        xml << "    <year>" << paper.year << "</year>\n";
        xml << "    <journal>" << escapeXML(paper.journal) << "</journal>\n";
        xml << "    <citationCount>" << paper.citationCount << "</citationCount>\n";
        xml << "  </paper>\n";
    }

    xml << "</papers>";
    return xml.str();
}

std::string ExportApiModule::exportToMarkdown(const std::vector<Paper>& papers, const ExportOptions& options) {
    std::ostringstream markdown;

    for (const auto& paper : papers) {
        markdown << "# " << paper.title << "\n\n";
        markdown << "**Authors:** " << paper.authors << "\n\n";
        markdown << "**Year:** " << paper.year << "\n\n";
        markdown << "**Journal:** " << paper.journal << "\n\n";

        if (options.includeAbstract && !paper.abstract.empty()) {
            markdown << "**Abstract:** " << paper.abstract << "\n\n";
        }

        if (options.includeCitations) {
            markdown << "**Citations:** " << paper.citationCount << "\n\n";
        }

        markdown << "---\n\n";
    }

    return markdown.str();
}

std::string ExportApiModule::previewExport(const std::vector<int>& paperIds,
                                           const ExportOptions& options,
                                           int previewCount) {
    // 获取论文（简化）
    std::vector<Paper> papers;
    int count = 0;
    for (int id : paperIds) {
        if (count >= previewCount) break;
        // TODO: 从PaperApiModule获取论文
        count++;
    }

    // 导出预览
    switch (options.format) {
        case ExportFormat::JSON:
            return exportToJSON(papers, options);
        case ExportFormat::BIBTEX:
            return exportToBibTeX(papers, options);
        case ExportFormat::CSV:
            return exportToCSV(papers, options);
        default:
            return exportToJSON(papers, options);
    }
}

std::map<std::string, std::string> ExportApiModule::getExportTemplates() {
    return exportTemplates_;
}

bool ExportApiModule::createExportTemplate(const std::string& name, const std::string& templateStr) {
    std::lock_guard<std::mutex> lock(mutex_);
    exportTemplates_[name] = templateStr;
    return true;
}

std::string ExportApiModule::batchExport(const std::vector<std::vector<int>>& paperGroups,
                                        const std::vector<ExportOptions>& options) {
    std::ostringstream result;
    result << "{\n";
    result << "  \"tasks\": [";

    for (size_t i = 0; i < paperGroups.size(); ++i) {
        if (i > 0) result << ",";
        result << "\n    \"" << createExportTask("batch", paperGroups[i], options[i]) << "\"";
    }

    result << "\n  ]\n}";
    return result.str();
}

size_t ExportApiModule::cleanupExpiredExports(std::chrono::hours maxAge) {
    size_t cleaned = 0;
    auto now = std::chrono::system_clock::now();

    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> toDelete;

    for (const auto& pair : exportTasks_) {
        const ExportTask& task = pair.second;
        auto age = now - task.createdAt;

        if (age > maxAge) {
            toDelete.push_back(pair.first);
        }
    }

    for (const auto& taskId : toDelete) {
        deleteExportTask(taskId);
        cleaned++;
    }

    std::cout << "[ExportApi] Cleaned up " << cleaned << " expired export tasks" << std::endl;

    return cleaned;
}

// ============================================================================
// 私有辅助方法
// ============================================================================

std::string ExportApiModule::generateTaskId() {
    std::ostringstream oss;
    oss << "export_" << std::chrono::system_clock::now().time_since_epoch().count()
        << "_" << nextTaskId_++;
    return oss.str();
}

std::string ExportApiModule::formatAuthors(const std::string& authors) {
    // 简化实现：保持原样
    return authors;
}

std::string ExportApiModule::formatBibTeXEntry(const Paper& paper) {
    std::ostringstream bibtex;
    bibtex << "@article{paper" << paper.id << ",\n";
    bibtex << "  title={" << paper.title << "},\n";
    bibtex << "  author={" << paper.authors << "},\n";
    bibtex << "  year={" << paper.year << "},\n";
    bibtex << "  journal={" << paper.journal << "}";
    if (!paper.doi.empty()) {
        bibtex << ",\n  doi={" << paper.doi << "}";
    }
    bibtex << "\n}";
    return bibtex.str();
}

std::string ExportApiModule::formatEndNoteEntry(const Paper& paper) {
    std::ostringstream endnote;
    endnote << "%0 Journal Article\n";
    endnote << "%T " << paper.title << "\n";
    endnote << "%A " << paper.authors << "\n";
    endnote << "%J " << paper.journal << "\n";
    endnote << "%D " << paper.year << "\n";
    if (!paper.doi.empty()) {
        endnote << "%R " << paper.doi << "\n";
    }
    return endnote.str();
}

std::string ExportApiModule::escapeCSV(const std::string& value) {
    std::string escaped = value;
    // 如果包含逗号、引号或换行，用引号包裹
    if (escaped.find(',') != std::string::npos ||
        escaped.find('"') != std::string::npos ||
        escaped.find('\n') != std::string::npos) {
        // 转义引号
        size_t pos = 0;
        while ((pos = escaped.find('"', pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\"\"");
            pos += 2;
        }
        // 用引号包裹
        escaped = "\"" + escaped + "\"";
    }
    return escaped;
}

std::string ExportApiModule::escapeXML(const std::string& value) {
    std::string escaped = value;
    // 转义XML特殊字符
    size_t pos = 0;
    while ((pos = escaped.find('&', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&amp;");
        pos += 5;
    }
    pos = 0;
    while ((pos = escaped.find('<', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&lt;");
        pos += 4;
    }
    pos = 0;
    while ((pos = escaped.find('>', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&gt;");
        pos += 4;
    }
    pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "&quot;");
        pos += 6;
    }
    return escaped;
}

std::string ExportApiModule::sanitizeFileName(const std::string& name) {
    std::string sanitized = name;
    // 移除或替换不安全的文件名字符
    const std::string unsafeChars = "<>:\"/\\|?*";
    for (char c : unsafeChars) {
        sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), c), sanitized.end());
    }
    return sanitized;
}

bool ExportApiModule::processExportTask(ExportTask& task) {
    // TODO: 实现实际的导出处理
    task.status = ExportTaskStatus::PROCESSING;

    // 获取论文数据
    std::vector<Paper> papers;
    // TODO: 从PaperApiModule获取论文

    // 执行导出
    std::string content;
    switch (task.options.format) {
        case ExportFormat::JSON:
            content = exportToJSON(papers, task.options);
            break;
        case ExportFormat::BIBTEX:
            content = exportToBibTeX(papers, task.options);
            break;
        case ExportFormat::CSV:
            content = exportToCSV(papers, task.options);
            break;
        case ExportFormat::XML:
            content = exportToXML(papers, task.options);
            break;
        case ExportFormat::MARKDOWN:
            content = exportToMarkdown(papers, task.options);
            break;
        default:
            return false;
    }

    // 保存文件
    std::string filePath = exportDirectory_ + "/" + task.taskId + ".export";
    std::ofstream file(filePath);
    if (!file.is_open()) {
        task.status = ExportTaskStatus::FAILED;
        task.errorMessage = "Failed to create export file";
        return false;
    }

    file << content;
    file.close();

    task.status = ExportTaskStatus::COMPLETED;
    task.completedAt = std::chrono::system_clock::now();
    task.downloadUrl = "/api/export/" + task.taskId + "/download";
    task.fileSize = content.length();

    return true;
}

void ExportApiModule::updateStats(ExportFormat format, bool success, int bytes) {
    stats_.totalExports++;

    if (success) {
        stats_.successfulExports++;
        stats_.totalBytesExported += bytes;
        stats_.exportsByFormat[format]++;
    } else {
        stats_.failedExports++;
    }
}

} // namespace PaperCrawler
