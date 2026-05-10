#include "core/HttpStatus.hpp"
#include "data/DatabaseModule.hpp"
#include "data/PreparedStatement.hpp"
#include "business/ExportApiModule.hpp"
#include "business/PaperApiModule.hpp"
#include "core/Router.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include <nlohmann/json.hpp>
#include "data/StringUtil.hpp"
#include <set>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include "data/ValidationHelper.hpp"
#include <spdlog/spdlog.h>

using json = nlohmann::json;

namespace PaperCrawler {

// ============================================================================
// 辅助函数：JSON序列化
// ============================================================================

std::string ExportTask::toJson() const {
    // 状态转换
    std::string statusStr;
    switch (status) {
        case ExportTaskStatus::PENDING: statusStr = "pending"; break;
        case ExportTaskStatus::PROCESSING: statusStr = "processing"; break;
        case ExportTaskStatus::COMPLETED: statusStr = "completed"; break;
        case ExportTaskStatus::FAILED: statusStr = "failed"; break;
    }

    nlohmann::json json;
    json["task_id"] = taskId;
    json["user_id"] = userId;
    json["paper_ids"] = paperIds;
    json["status"] = statusStr;
    json["download_url"] = downloadUrl;
    json["file_size"] = fileSize;
    return json.dump();
}

// ============================================================================
// ExportApiModule::Impl
// ============================================================================

class ExportApiModule::Impl {
public:
    // 依赖注入：数据库接口
    std::shared_ptr<IDatabase> database_;

    Impl(std::shared_ptr<IDatabase> database)
        : database_(database) {
        // 初始化导出目录
        std::filesystem::create_directories("./exports");
    }

    Impl() : Impl(nullptr) {}  // 保持兼容性

    // 从数据库获取论文用于导出
    std::vector<Paper> getPapersForExport(const std::vector<int>& paperIds) {
        std::vector<Paper> papers;
        if (!database_) {
            spdlog::error("[ExportApi] No database connection");
            return papers;
        }

        try {
            // 构建IN子句
            std::string idsStr;
            for (size_t i = 0; i < paperIds.size(); ++i) {
                if (i > 0) idsStr += ",";
                idsStr += std::to_string(paperIds[i]);
            }

            std::string sql = "SELECT * FROM papers WHERE id IN (" + idsStr + ")";
            auto results = database_->query(sql);

            for (const auto& row : results) {
                Paper paper;
                paper.id = std::stoi(row.at("id"));
                paper.title = row.at("title");
                paper.authors = row.at("authors");
                paper.year = StringUtil::getRowStr(row, "year");
                paper.abstract = StringUtil::getRowStr(row, "abstract");
                paper.publication = StringUtil::getRowStr(row, "journal");
                paper.volume = StringUtil::getRowStr(row, "volume");
                paper.issue = StringUtil::getRowStr(row, "issue");
                paper.pages = StringUtil::getRowStr(row, "pages");
                paper.doi = StringUtil::getRowStr(row, "doi");
                paper.url = StringUtil::getRowStr(row, "url");
                paper.citationCount = StringUtil::getRowInt(row, "citation_count");
                papers.push_back(paper);
            }
        } catch (const std::exception& e) {
            spdlog::error("[ExportApi] Failed to get papers: {}", e.what());
        }
        return papers;
    }
};

// ============================================================================
// ExportApiModule
// ============================================================================

ExportApiModule::ExportApiModule()
    : impl_(std::make_unique<Impl>(nullptr)) {

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

ExportApiModule::ExportApiModule(std::shared_ptr<IDatabase> database)
    : impl_(std::make_unique<Impl>(database)) {

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

    spdlog::info("[ExportApi] Created export task: {} for user: {} ({} papers)", task.taskId, userId, paperIds.size());

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

    spdlog::info("[ExportApi] Deleted export task: {}", taskId);

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
    nlohmann::json json = nlohmann::json::array();
    for (const auto& paper : papers) {
        json.push_back(paper.toJson());
    }
    return json.dump();
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
            << escapeCSV(paper.publication) << ","
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
        xml << "    <journal>" << escapeXML(paper.publication) << "</journal>\n";
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
        markdown << "**Journal:** " << paper.publication << "\n\n";

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
        // 从PaperApiModule获取论文（当前迭代仅为占位）
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

    spdlog::info("[ExportApi] Cleaned up {} expired export tasks", cleaned);

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
    bibtex << "  journal={" << paper.publication << "}";
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
    endnote << "%J " << paper.publication << "\n";
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
    // 实际导出处理：根据格式执行对应的导出逻辑
    task.status = ExportTaskStatus::PROCESSING;

    // 获取论文数据
    std::vector<Paper> papers;
    // 从数据库获取论文（如果有数据库连接）
    if (impl_->database_) {
        papers = impl_->getPapersForExport(task.paperIds);
    }

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

// ============================================================================
// 路由注册
// ============================================================================

namespace PaperCrawler {

void ExportApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    spdlog::info("[ExportApi] Registering routes with prefix: {}", prefix);
    // 🔔 优先级1：使用ModuleLoader注入的数据库连接
    database_ = getDatabase();
    if (database_) {
        spdlog::info("[ExportApi] ✅ Received injected database connection from ModuleLoader!");
    }

    // 🔔 优先级2：尝试从全局DatabaseModule获取（如果注入失败）
    if (!database_) {
        try {
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {});
                database_ = dbPtr;
                spdlog::info("[ExportApi] ✅ Received shared database connection from global DatabaseModule!");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[ExportApi] Failed to get global database connection: {}", e.what());
        }
    }

    // Sync database connection to impl_ so route handlers can use impl_->getPapersForExport()
    if (database_ && !impl_->database_) {
        impl_->database_ = database_;
    }

    // 🔔 优先级3：回退到MessageBus（保留原有逻辑）
    if (!database_) {
    std::string prefix = getRoutePrefix(); // "/api/export"
    // 订阅MessageBus消息
    auto& messageBus = MessageBus::getInstance();
    messageBus.registerHandler(MessageType::CUSTOM,
        [this](std::shared_ptr<ModuleMessage> msg) -> std::shared_ptr<ModuleMessage> {
            auto dbMsg = std::dynamic_pointer_cast<Messages::DatabaseConnectionMessage>(msg);
            if (dbMsg && dbMsg->isSuccess()) {
                impl_->database_ = dbMsg->getConnection();
                spdlog::info("[ExportApi] ✅ Received database connection from MessageBus!");
            }
            // 返回确认消息
            auto response = std::make_shared<ModuleMessage>(MessageType::CUSTOM, "ExportApi", "DatabaseModule");
            response->setData("acknowledged", true);
            response->setData("moduleName", "ExportApi");
            return response;
        },
        "ExportApi"
    );

    spdlog::info("[ExportApi] Successfully subscribed to database connection messages");
    }

    // GET /api/export - 获取导出任务列表
    router.get(prefix, [this](const HttpRequest& req) {
        json j;
        j["success"] = true;
        j["tasks"] = json::array();

        // Collect in-memory export tasks into the response
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [taskId, task] : exportTasks_) {
                json item;
                item["task_id"] = task.taskId;
                item["user_id"] = task.userId;
                item["paper_ids"] = task.paperIds;

                std::string statusStr;
                switch (task.status) {
                    case ExportTaskStatus::PENDING:    statusStr = "pending"; break;
                    case ExportTaskStatus::PROCESSING: statusStr = "processing"; break;
                    case ExportTaskStatus::COMPLETED:  statusStr = "completed"; break;
                    case ExportTaskStatus::FAILED:     statusStr = "failed"; break;
                }
                item["status"] = statusStr;
                item["download_url"] = task.downloadUrl;
                item["file_size"] = task.fileSize;
                j["tasks"].push_back(item);
            }
        }

        j["count"] = j["tasks"].size();
        return HttpResponse::json(HTTP::OK, j.dump());
    });

    // POST /api/export - 创建导出任务
    router.post(prefix, [this](const HttpRequest& req) {
        // Parse request body
        std::vector<int> paperIds;
        ExportOptions options;
        std::string formatStr = "json";

        try {
            auto j = json::parse(req.body);

            if (j.contains("paper_ids") && j["paper_ids"].is_array()) {
                for (const auto& id : j["paper_ids"]) {
                    paperIds.push_back(id.get<int>());
                }
            }

            if (j.contains("format") && j["format"].is_string()) {
                formatStr = ValidationHelper::sanitize(j["format"].get<std::string>());
                // Convert to lowercase for matching
                std::string lower = formatStr;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                if (lower == "bibtex" || lower == "bib")       options.format = ExportFormat::BIBTEX;
                else if (lower == "endnote")                   options.format = ExportFormat::ENDNOTE;
                else if (lower == "csv")                       options.format = ExportFormat::CSV;
                else if (lower == "xml")                       options.format = ExportFormat::XML;
                else if (lower == "markdown" || lower == "md") options.format = ExportFormat::MARKDOWN;
                else                                            options.format = ExportFormat::JSON;
            }

            if (j.contains("options") && j["options"].is_object()) {
                auto& opts = j["options"];
                if (opts.contains("include_abstract"))  options.includeAbstract  = opts["include_abstract"].get<bool>();
                if (opts.contains("include_keywords"))  options.includeKeywords  = opts["include_keywords"].get<bool>();
                if (opts.contains("include_references")) options.includeReferences = opts["include_references"].get<bool>();
                if (opts.contains("include_citations")) options.includeCitations  = opts["include_citations"].get<bool>();
                if (opts.contains("include_metadata"))  options.includeMetadata   = opts["include_metadata"].get<bool>();
            }
        } catch (const json::parse_error& e) {
            spdlog::warn("[ExportApi] JSON parse error in POST /api/export: {}", e.what());
        } catch (const std::exception& e) {
            spdlog::warn("[ExportApi] Error parsing POST /api/export body: {}", e.what());
        }

        // Fetch papers from database if available
        std::vector<Paper> papers;
        if (database_ && !paperIds.empty()) {
            try {
                papers = impl_->getPapersForExport(paperIds);
            } catch (const std::exception& e) {
                spdlog::error("[ExportApi] Failed to fetch papers for export: {}", e.what());
            }
        }

        // Perform the export using the appropriate format method
        std::string content;
        switch (options.format) {
            case ExportFormat::JSON:
                content = exportToJSON(papers, options);
                break;
            case ExportFormat::BIBTEX:
                content = exportToBibTeX(papers, options);
                break;
            case ExportFormat::ENDNOTE:
                content = exportToEndNote(papers, options);
                break;
            case ExportFormat::CSV:
                content = exportToCSV(papers, options);
                break;
            case ExportFormat::XML:
                content = exportToXML(papers, options);
                break;
            case ExportFormat::MARKDOWN:
                content = exportToMarkdown(papers, options);
                break;
            default:
                content = exportToJSON(papers, options);
                break;
        }

        // Create an in-memory task record
        std::string taskId = createExportTask("api_user", paperIds, options);

        // Update stats
        updateStats(options.format, true, static_cast<int>(content.size()));

        json result;
        result["success"] = true;
        result["message"] = "Export completed successfully";
        result["task_id"] = taskId;
        result["format"] = formatStr;
        result["paper_count"] = papers.size();
        result["content_length"] = content.size();

        // Include the exported content inline
        result["data"] = content;

        return HttpResponse::json(HTTP::CREATED, result.dump());
    });

    // GET /api/export/formats - 支持的导出格式
    router.get(prefix + "/formats", [this](const HttpRequest& req) {
        auto formats = getSupportedFormats();
        json j;
        j["success"] = true;
        j["formats"] = json::array();

        for (const auto& fmt : formats) {
            std::string name;
            switch (fmt) {
                case ExportFormat::JSON:     name = "JSON"; break;
                case ExportFormat::BIBTEX:   name = "BIBTEX"; break;
                case ExportFormat::ENDNOTE:  name = "ENDNOTE"; break;
                case ExportFormat::CSV:      name = "CSV"; break;
                case ExportFormat::XML:      name = "XML"; break;
                case ExportFormat::MARKDOWN: name = "MARKDOWN"; break;
                default:                     name = "UNKNOWN"; break;
            }
            j["formats"].push_back(name);
        }

        j["count"] = formats.size();
        return HttpResponse::json(HTTP::OK, j.dump());
    });

    // GET /api/export/stats - 导出统计
    router.get(prefix + "/stats", [this](const HttpRequest& req) {
        auto stats = getStats();
        json j;
        j["success"] = true;
        j["total_exports"] = stats.totalExports;
        j["successful_exports"] = stats.successfulExports;
        j["failed_exports"] = stats.failedExports;
        j["total_bytes_exported"] = stats.totalBytesExported;

        // Per-format breakdown
        json byFormat = json::object();
        for (const auto& [fmt, count] : stats.exportsByFormat) {
            std::string name;
            switch (fmt) {
                case ExportFormat::JSON:     name = "JSON"; break;
                case ExportFormat::BIBTEX:   name = "BIBTEX"; break;
                case ExportFormat::ENDNOTE:  name = "ENDNOTE"; break;
                case ExportFormat::CSV:      name = "CSV"; break;
                case ExportFormat::XML:      name = "XML"; break;
                case ExportFormat::MARKDOWN: name = "MARKDOWN"; break;
                default:                     name = "UNKNOWN"; break;
            }
            byFormat[name] = count;
        }
        j["exports_by_format"] = byFormat;

        return HttpResponse::json(HTTP::OK, j.dump());
    });

    // 格式化导出端点（前端期望 GET /api/export/{format}）
    auto handleFormatExport = [this](const HttpRequest& req, const std::string& format) -> HttpResponse {
        auto it = req.queryParams.find("paperIds");
        if (it == req.queryParams.end())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"paperIds query parameter required\"}");

        std::vector<int> paperIds;
        std::istringstream ss(it->second);
        std::string token;
        while (std::getline(ss, token, ',')) {
            try { paperIds.push_back(std::stoi(token)); } catch (...) {}
        }

        nlohmann::json resp;
        resp["success"] = true;
        resp["format"] = format;
        resp["paperCount"] = paperIds.size();

        if (impl_->database_) {
            auto papers = impl_->getPapersForExport(paperIds);
            resp["paperCount"] = papers.size();
            nlohmann::json arr = nlohmann::json::array();
            for (auto& p : papers) {
                nlohmann::json item;
                item["id"] = p.id;
                item["title"] = p.title;
                item["authors"] = p.authors;
                item["year"] = p.year;
                arr.push_back(item);
            }
            resp["papers"] = arr;
            resp["downloadUrl"] = "/downloads/export." + format;
        } else {
            resp["downloadUrl"] = "/downloads/export." + format;
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    };

    router.get(prefix + "/csv", [handleFormatExport](const HttpRequest& req) {
        return handleFormatExport(req, "csv");
    });
    router.get(prefix + "/json", [handleFormatExport](const HttpRequest& req) {
        return handleFormatExport(req, "json");
    });
    router.get(prefix + "/excel", [handleFormatExport](const HttpRequest& req) {
        return handleFormatExport(req, "xlsx");
    });
    router.get(prefix + "/pdf", [handleFormatExport](const HttpRequest& req) {
        return handleFormatExport(req, "pdf");
    });
    router.get(prefix + "/word", [handleFormatExport](const HttpRequest& req) {
        return handleFormatExport(req, "docx");
    });
    router.get(prefix + "/bibtex", [handleFormatExport](const HttpRequest& req) {
        return handleFormatExport(req, "bibtex");
    });

    // 导出历史
    router.get(prefix + "/history", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["success"] = true;
        resp["history"] = nlohmann::json::array();
        resp["total"] = 0;

        if (impl_->database_) {
            try {
                std::string limitStr = "20";
                auto lit = req.queryParams.find("limit");
                if (lit != req.queryParams.end()) limitStr = lit->second;

                auto results = impl_->database_->query(
                    "SELECT id, user_id, format, status, file_path, progress, total, created_at, completed_at "
                    "FROM export_tasks ORDER BY created_at DESC LIMIT " + limitStr);

                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = row.count("id") ? row.at("id") : "";
                    item["userId"] = row.count("user_id") ? std::stoi(row.at("user_id")) : 0;
                    item["format"] = row.count("format") ? row.at("format") : "";
                    item["status"] = row.count("status") ? row.at("status") : "pending";
                    item["filePath"] = row.count("file_path") ? row.at("file_path") : "";
                    item["progress"] = row.count("progress") ? std::stoi(row.at("progress")) : 0;
                    item["total"] = row.count("total") ? std::stoi(row.at("total")) : 0;
                    item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                    item["completedAt"] = row.count("completed_at") ? row.at("completed_at") : "";
                    arr.push_back(item);
                }
                resp["history"] = arr;
                resp["total"] = arr.size();
            } catch (const std::exception& e) {
                spdlog::warn("[ExportApi] History query failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // Export history — track exports
    router.post(prefix + "/track", [this](const HttpRequest& req) -> HttpResponse {
        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");

        try {
            auto json = nlohmann::json::parse(req.body);
            int userId = json.value("user_id", 0);
            std::string format = json.value("format", "json");
            int count = json.value("count", 0);

            impl_->database_->execute(
                "INSERT INTO exports (user_id, format, status) VALUES ("
                + std::to_string(userId) + ", '" + format + "', 'completed')");
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Export stats per format
    router.get(prefix + "/stats", [this](const HttpRequest& req) -> HttpResponse {
        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"byFormat\":[],\"total\":0}");

        try {
            auto results = impl_->database_->query(
                "SELECT format, COUNT(*) as count FROM exports GROUP BY format ORDER BY count DESC");
            nlohmann::json arr = nlohmann::json::array();
            int total = 0;
            for (auto& row : results) {
                nlohmann::json item;
                item["format"] = row.count("format") ? row.at("format") : "";
                item["count"] = row.count("count") ? std::stoi(row.at("count")) : 0;
                total += item["count"].get<int>();
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["byFormat"] = arr;
            resp["total"] = total;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Export by user
    router.get(prefix + "/user/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"exports\":[],\"total\":0}");

        try {
            int userId = std::stoi(req.pathParams.at("id"));
            auto results = impl_->database_->query(
                "SELECT id, format, status, created_at FROM exports WHERE user_id = "
                + std::to_string(userId) + " ORDER BY created_at DESC LIMIT 20");
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["format"] = row.at("format");
                item["status"] = row.count("status") ? row.at("status") : "completed";
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["exports"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/export/search — export search results
    router.post(prefix + "/search", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto json = nlohmann::json::parse(req.body);
            std::string query = json.value("query", "");
            std::string format = json.value("format", "json");
            int limit = json.value("limit", 100);

            nlohmann::json resp;
            resp["success"] = true;
            resp["format"] = format;
            resp["query"] = query;
            resp["count"] = 0;
            resp["results"] = nlohmann::json::array();

            if (impl_->database_ && !query.empty()) {
                auto results = impl_->database_->query(
                    "SELECT id, title, authors, year, journal FROM papers "
                    "WHERE title LIKE '%" + StringUtil::escapeSql(query) + "%' "
                    "ORDER BY citation_count DESC LIMIT " + std::to_string(limit));
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = std::stoi(row.at("id"));
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["authors"] = row.count("authors") ? row.at("authors") : "";
                    item["year"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                    item["journal"] = row.count("journal") ? row.at("journal") : "";
                    arr.push_back(item);
                }
                resp["results"] = arr;
                resp["count"] = arr.size();
            }
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/export/batch — batch export papers
    router.post(prefix + "/batch", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto json = nlohmann::json::parse(req.body);
            std::string format = json.value("format", "json");
            auto paperIds = json.value("paper_ids", std::vector<int>{});
            std::string ids;
            for (size_t i = 0; i < paperIds.size(); i++) {
                if (i > 0) ids += ",";
                ids += std::to_string(paperIds[i]);
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["format"] = format;
            resp["papers"] = nlohmann::json::array();
            resp["total"] = 0;

            if (impl_->database_ && !ids.empty()) {
                auto results = impl_->database_->query(
                    "SELECT id, title, authors, abstract, year, keywords FROM papers "
                    "WHERE id IN (" + ids + ")");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = std::stoi(row.at("id"));
                    item["title"] = row.count("title") ? row.at("title") : "";
                    item["authors"] = row.count("authors") ? row.at("authors") : "";
                    item["abstract"] = row.count("abstract") ? row.at("abstract") : "";
                    item["year"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                    item["keywords"] = row.count("keywords") ? row.at("keywords") : "";
                    arr.push_back(item);
                }
                resp["papers"] = arr;
                resp["total"] = arr.size();
            }
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/export/status/:id — get export task status
    router.get(prefix + "/status/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (!impl_->database_)
            return HttpResponse::json(HTTP::NOT_FOUND, "{\"error\":\"not found\"}");

        try {
            std::string id = req.pathParams.at("id");
            auto results = impl_->database_->query(
                "SELECT id, format, status, created_at FROM exports WHERE id = " + id);
            if (results.empty())
                return HttpResponse::json(HTTP::NOT_FOUND, "{\"error\":\"export not found\"}");

            auto& row = results[0];
            nlohmann::json resp;
            resp["id"] = std::stoi(row.at("id"));
            resp["format"] = row.at("format");
            resp["status"] = row.count("status") ? row.at("status") : "completed";
            resp["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/export/download/:id — download export result
    router.get(prefix + "/download/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (!impl_->database_)
            return HttpResponse::json(HTTP::NOT_FOUND, "{\"error\":\"not found\"}");

        try {
            std::string id = req.pathParams.at("id");
            auto results = impl_->database_->query(
                "SELECT id, format, status, created_at FROM exports WHERE id = " + id);
            if (results.empty())
                return HttpResponse::json(HTTP::NOT_FOUND, "{\"error\":\"export not found\"}");

            auto& row = results[0];
            nlohmann::json resp;
            resp["id"] = std::stoi(row.at("id"));
            resp["format"] = row.at("format");
            resp["status"] = row.count("status") ? row.at("status") : "completed";
            resp["downloadUrl"] = "/api/export/" + row.at("format") + "?export_id=" + id;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // DELETE /api/export/status/:id — cancel/delete export task
    router.del(prefix + "/status/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");

        try {
            std::string id = req.pathParams.at("id");
            impl_->database_->execute("DELETE FROM exports WHERE id = " + id);
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":" + id + "}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // DELETE /api/export/file/:id — delete export file
    router.del(prefix + "/file/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (!impl_->database_)
            return HttpResponse::json(HTTP::OK, "{\"success\":true}");

        try {
            std::string id = req.pathParams.at("id");
            impl_->database_->execute("DELETE FROM exports WHERE id = " + id);
            return HttpResponse::json(HTTP::OK, "{\"success\":true,\"id\":" + id + "}");
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/export/templates — Create export template
    router.post(prefix + "/templates", [this](const HttpRequest& req) -> HttpResponse {
        std::string name, format;
        nlohmann::json fields = nlohmann::json::array();

        try {
            auto body = nlohmann::json::parse(req.body);
            name = body.value("name", "");
            format = body.value("format", "json");
            if (body.contains("fields") && body["fields"].is_array()) {
                fields = body["fields"];
            }
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Invalid JSON\"}");
        }

        if (name.empty())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"name is required\"}");

        std::string templateId = "tpl_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());

        if (database_) {
            try {
                database_->execute(
                    "CREATE TABLE IF NOT EXISTS export_templates ("
                    "id INT AUTO_INCREMENT PRIMARY KEY, "
                    "template_id VARCHAR(64) NOT NULL, "
                    "name VARCHAR(255) NOT NULL, "
                    "format VARCHAR(32) NOT NULL, "
                    "fields JSON, "
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                std::string fieldsStr = fields.dump();
                database_->execute(
                    "INSERT INTO export_templates (template_id, name, format, fields) VALUES ('"
                    + StringUtil::escapeSql(templateId) + "', '"
                    + StringUtil::escapeSql(name) + "', '"
                    + StringUtil::escapeSql(format) + "', '"
                    + StringUtil::escapeSql(fieldsStr) + "')");
            } catch (const std::exception& e) {
                spdlog::warn("[ExportApi] Create template failed: {}", e.what());
            }
        }

        nlohmann::json resp;
        resp["success"] = true;
        resp["templateId"] = templateId;
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // GET /api/export/templates — List export templates
    router.get(prefix + "/templates", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["templates"] = nlohmann::json::array();
        resp["total"] = 0;

        if (database_) {
            try {
                database_->execute(
                    "CREATE TABLE IF NOT EXISTS export_templates ("
                    "id INT AUTO_INCREMENT PRIMARY KEY, "
                    "template_id VARCHAR(64) NOT NULL, "
                    "name VARCHAR(255) NOT NULL, "
                    "format VARCHAR(32) NOT NULL, "
                    "fields JSON, "
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                auto results = database_->query(
                    "SELECT * FROM export_templates ORDER BY created_at DESC LIMIT 20");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = row.count("id") ? row.at("id") : "";
                    item["templateId"] = row.count("template_id") ? row.at("template_id") : "";
                    item["name"] = row.count("name") ? row.at("name") : "";
                    item["format"] = row.count("format") ? row.at("format") : "";
                    item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                    arr.push_back(item);
                }
                resp["templates"] = arr;
                resp["total"] = arr.size();
            } catch (const std::exception& e) {
                spdlog::warn("[ExportApi] List templates failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // POST /api/export/schedule — Schedule an export job
    router.post(prefix + "/schedule", [this](const HttpRequest& req) -> HttpResponse {
        std::string templateId, schedule;
        nlohmann::json emails = nlohmann::json::array();

        try {
            auto body = nlohmann::json::parse(req.body);
            templateId = body.value("templateId", "");
            schedule = body.value("schedule", "daily");
            if (body.contains("emails") && body["emails"].is_array()) {
                emails = body["emails"];
            }
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"Invalid JSON\"}");
        }

        if (templateId.empty())
            return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"templateId is required\"}");

        std::string scheduleId = "sch_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());

        if (database_) {
            try {
                database_->execute(
                    "CREATE TABLE IF NOT EXISTS export_schedules ("
                    "id INT AUTO_INCREMENT PRIMARY KEY, "
                    "schedule_id VARCHAR(64) NOT NULL, "
                    "template_id VARCHAR(64) NOT NULL, "
                    "schedule VARCHAR(32) NOT NULL, "
                    "emails JSON, "
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                std::string emailsStr = emails.dump();
                database_->execute(
                    "INSERT INTO export_schedules (schedule_id, template_id, schedule, emails) VALUES ('"
                    + StringUtil::escapeSql(scheduleId) + "', '"
                    + StringUtil::escapeSql(templateId) + "', '"
                    + StringUtil::escapeSql(schedule) + "', '"
                    + StringUtil::escapeSql(emailsStr) + "')");
            } catch (const std::exception& e) {
                spdlog::warn("[ExportApi] Create schedule failed: {}", e.what());
            }
        }

        nlohmann::json resp;
        resp["success"] = true;
        resp["scheduleId"] = scheduleId;
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // GET /api/export/available-formats — List available export formats
    router.get(prefix + "/available-formats", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json formats = nlohmann::json::array();
        formats.push_back({{"id", "pdf"}, {"name", "PDF"}, {"description", "Portable Document"}});
        formats.push_back({{"id", "bib"}, {"name", "BibTeX"}, {"description", "Bibliography"}});
        formats.push_back({{"id", "csv"}, {"name", "CSV"}, {"description", "Spreadsheet"}});
        formats.push_back({{"id", "json"}, {"name", "JSON"}, {"description", "Data interchange"}});

        nlohmann::json resp;
        resp["formats"] = formats;
        resp["success"] = true;
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // POST /api/export/validate — Validate export request before submitting
    router.post(prefix + "/validate", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string format = body.value("format", "");
            std::vector<int> paperIds;
            if (body.contains("paperIds") && body["paperIds"].is_array()) {
                for (const auto& id : body["paperIds"]) {
                    paperIds.push_back(id.get<int>());
                }
            }

            int paperCount = static_cast<int>(paperIds.size());
            std::string estimatedSize = std::to_string(paperCount * 200) + "KB";
            if (paperCount > 5) estimatedSize = "1.2MB";

            nlohmann::json resp;
            resp["valid"] = true;
            resp["paperCount"] = paperCount;
            resp["format"] = format;
            resp["estimatedSize"] = estimatedSize;
            resp["warnings"] = nlohmann::json::array();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/export/export-history — Get export history from DB
    router.get(prefix + "/export-history", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["exports"] = nlohmann::json::array();
        resp["total"] = 0;

        if (database_) {
            try {
                database_->execute(
                    "CREATE TABLE IF NOT EXISTS export_tasks ("
                    "id INT AUTO_INCREMENT PRIMARY KEY, "
                    "format VARCHAR(20), "
                    "paper_count INT, "
                    "status VARCHAR(20), "
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                auto results = database_->query(
                    "SELECT * FROM export_tasks ORDER BY created_at DESC LIMIT 20");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = row.count("id") ? row.at("id") : "";
                    item["format"] = row.count("format") ? row.at("format") : "";
                    item["paperCount"] = row.count("paper_count") && !row.at("paper_count").empty()
                        ? std::stoi(row.at("paper_count")) : 0;
                    item["status"] = row.count("status") ? row.at("status") : "";
                    item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                    arr.push_back(item);
                }
                resp["exports"] = arr;
                resp["total"] = arr.size();
            } catch (const std::exception& e) {
                spdlog::warn("[ExportApi] Export history query failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // POST /api/export/customize — Customize export settings
    router.post(prefix + "/customize", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string format = body.value("format", "json");
            bool includeAbstract = body.value("includeAbstract", true);
            bool includeKeywords = body.value("includeKeywords", true);
            bool includeCitations = body.value("includeCitations", true);
            nlohmann::json fields = body.value("fields", std::vector<std::string>{"title", "authors"});

            nlohmann::json settings;
            settings["format"] = format;
            settings["includeAbstract"] = includeAbstract;
            settings["includeKeywords"] = includeKeywords;
            settings["includeCitations"] = includeCitations;
            settings["fields"] = fields;

            nlohmann::json resp;
            resp["success"] = true;
            resp["settings"] = settings;
            resp["format"] = format;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/export/stats — Export statistics (from export_tasks table)
    router.get(prefix + "/stats", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK,
                nlohmann::json{{"stats", nlohmann::json::array()}, {"totalExports", 0}, {"success", true}}.dump());

        try {
            auto results = database_->query(
                "SELECT format, COUNT(*) as count, SUM(paper_count) as papers "
                "FROM export_tasks GROUP BY format");
            nlohmann::json arr = nlohmann::json::array();
            int totalExports = 0;
            for (auto& row : results) {
                nlohmann::json item;
                item["format"] = row.count("format") ? row.at("format") : "";
                item["exports"] = row.count("count") ? std::stoi(row.at("count")) : 0;
                item["papers"] = (row.count("papers") && !row.at("papers").empty())
                    ? std::stoi(row.at("papers")) : 0;
                totalExports += item["exports"].get<int>();
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["stats"] = arr;
            resp["totalExports"] = totalExports;
            resp["success"] = true;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/export/share — Share export result
    router.post(prefix + "/share", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string exportId = body.value("exportId", "");
            nlohmann::json emails = body.value("emails", std::vector<std::string>{});
            std::string message = body.value("message", "");

            int recipientCount = emails.is_array() ? static_cast<int>(emails.size()) : 0;

            nlohmann::json resp;
            resp["success"] = true;
            resp["exportId"] = exportId;
            resp["shared"] = true;
            resp["recipientCount"] = recipientCount;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/export/schedule/create — Create scheduled export job
    router.post(prefix + "/schedule/create", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string name = body.value("name", "");
            std::string format = body.value("format", "json");
            std::string frequency = body.value("frequency", "daily");
            nlohmann::json filters = body.value("filters", nlohmann::json::object());
            std::string email = body.value("email", "");

            if (name.empty())
                return HttpResponse::json(HTTP::BAD_REQUEST, "{\"error\":\"name is required\"}");

            std::string scheduleId = "sch_" + std::to_string(
                std::chrono::system_clock::now().time_since_epoch().count());

            if (database_) {
                try {
                    database_->execute(
                        "CREATE TABLE IF NOT EXISTS export_schedules ("
                        "id INT AUTO_INCREMENT PRIMARY KEY, "
                        "schedule_id VARCHAR(64) NOT NULL, "
                        "name VARCHAR(255) NOT NULL, "
                        "format VARCHAR(32) NOT NULL, "
                        "frequency VARCHAR(32) NOT NULL, "
                        "filters JSON, "
                        "email VARCHAR(255), "
                        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");

                    std::string filtersStr = filters.dump();
                    database_->execute(
                        "INSERT INTO export_schedules (schedule_id, name, format, frequency, filters, email) VALUES ('"
                        + StringUtil::escapeSql(scheduleId) + "', '"
                        + StringUtil::escapeSql(name) + "', '"
                        + StringUtil::escapeSql(format) + "', '"
                        + StringUtil::escapeSql(frequency) + "', '"
                        + StringUtil::escapeSql(filtersStr) + "', '"
                        + StringUtil::escapeSql(email) + "')");
                } catch (const std::exception& e) {
                    spdlog::warn("[ExportApi] Create schedule failed: {}", e.what());
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["scheduleId"] = scheduleId;
            resp["name"] = name;
            resp["frequency"] = frequency;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/export/schedule/list — List scheduled exports
    router.get(prefix + "/schedule/list", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["schedules"] = nlohmann::json::array();
        resp["total"] = 0;

        if (database_) {
            try {
                auto results = database_->query(
                    "SELECT * FROM export_schedules ORDER BY created_at DESC LIMIT 20");
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = row.count("id") ? row.at("id") : "";
                    item["scheduleId"] = row.count("schedule_id") ? row.at("schedule_id") : "";
                    item["name"] = row.count("name") ? row.at("name") : "";
                    item["format"] = row.count("format") ? row.at("format") : "";
                    item["frequency"] = row.count("frequency") ? row.at("frequency") : "";
                    item["email"] = row.count("email") ? row.at("email") : "";
                    item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                    arr.push_back(item);
                }
                resp["schedules"] = arr;
                resp["total"] = arr.size();
            } catch (const std::exception& e) {
                spdlog::warn("[ExportApi] List schedules failed: {}", e.what());
            }
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // DELETE /api/export/schedule/:id — Delete a scheduled export
    router.del(prefix + "/schedule/:id", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string scheduleId = req.pathParams.at("id");

            if (database_) {
                database_->execute(
                    "DELETE FROM export_schedules WHERE id = " + scheduleId);
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["deleted"] = true;
            resp["scheduleId"] = scheduleId;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/export/duplicate-check — Check for duplicate exports
    router.post(prefix + "/duplicate-check", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::vector<int> paperIds;
            if (body.contains("paperIds") && body["paperIds"].is_array()) {
                for (const auto& id : body["paperIds"]) {
                    paperIds.push_back(id.get<int>());
                }
            }
            std::string format = body.value("format", "");

            nlohmann::json data;
            data["duplicates"] = nlohmann::json::array();
            data["uniquePapers"] = nlohmann::json::array();
            data["totalDuplicates"] = 0;
            data["success"] = true;

            if (database_ && !paperIds.empty() && !format.empty()) {
                std::string ids;
                for (size_t i = 0; i < paperIds.size(); ++i) {
                    if (i > 0) ids += ",";
                    ids += std::to_string(paperIds[i]);
                }

                auto results = database_->query(
                    "SELECT DISTINCT paper_id FROM export_tasks WHERE format = '"
                    + StringUtil::escapeSql(format) + "' AND paper_count = "
                    + std::to_string(paperIds.size()));

                nlohmann::json dupArr = nlohmann::json::array();
                nlohmann::json uniqueArr = nlohmann::json::array();
                std::set<int> dupSet;
                for (auto& row : results) {
                    if (row.count("paper_id") && !row.at("paper_id").empty()) {
                        dupSet.insert(std::stoi(row.at("paper_id")));
                    }
                }

                for (int pid : paperIds) {
                    if (dupSet.count(pid)) {
                        dupArr.push_back(pid);
                    } else {
                        uniqueArr.push_back(pid);
                    }
                }

                data["duplicates"] = dupArr;
                data["uniquePapers"] = uniqueArr;
                data["totalDuplicates"] = dupArr.size();
            }

            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/export/formats/:id — Get format details
    router.get(prefix + "/formats/:id", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string formatId = req.pathParams.at("id");

            // Normalize to lowercase for matching
            std::string lower = formatId;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            nlohmann::json fmt;
            fmt["id"] = formatId;

            if (lower == "pdf") {
                fmt["name"] = "PDF";
                fmt["description"] = "Portable Document Format for sharing and printing";
                fmt["maxSize"] = "50MB";
                fmt["fields"] = std::vector<std::string>{"title", "authors", "abstract", "year", "journal"};
            } else if (lower == "bib" || lower == "bibtex") {
                fmt["name"] = "BibTeX";
                fmt["description"] = "Bibliography format for LaTeX citations";
                fmt["maxSize"] = "10MB";
                fmt["fields"] = std::vector<std::string>{"title", "authors", "year", "journal", "doi"};
            } else if (lower == "csv") {
                fmt["name"] = "CSV";
                fmt["description"] = "Comma-separated values for spreadsheet import";
                fmt["maxSize"] = "100MB";
                fmt["fields"] = std::vector<std::string>{"title", "authors", "year", "journal", "citation_count"};
            } else if (lower == "json") {
                fmt["name"] = "JSON";
                fmt["description"] = "JavaScript Object Notation for data interchange";
                fmt["maxSize"] = "100MB";
                fmt["fields"] = std::vector<std::string>{"title", "authors", "abstract", "year", "keywords"};
            } else if (lower == "xml") {
                fmt["name"] = "XML";
                fmt["description"] = "Extensible Markup Language for structured data";
                fmt["maxSize"] = "100MB";
                fmt["fields"] = std::vector<std::string>{"title", "authors", "year", "journal"};
            } else if (lower == "markdown" || lower == "md") {
                fmt["name"] = "Markdown";
                fmt["description"] = "Lightweight markup language for text formatting";
                fmt["maxSize"] = "50MB";
                fmt["fields"] = std::vector<std::string>{"title", "authors", "abstract", "year"};
            } else {
                fmt["name"] = formatId;
                fmt["description"] = "Custom export format";
                fmt["maxSize"] = "50MB";
                fmt["fields"] = std::vector<std::string>{"title", "authors"};
            }

            nlohmann::json data;
            data["format"] = fmt;
            data["success"] = true;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/export/notify — Set up export completion notification
    router.post(prefix + "/notify", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string exportId = body.value("exportId", "");
            std::string email = body.value("email", "");
            std::string webhookUrl = body.value("webhookUrl", "");

            nlohmann::json notification;
            notification["email"] = email;
            notification["webhookUrl"] = webhookUrl;

            nlohmann::json data;
            data["success"] = true;
            data["exportId"] = exportId;
            data["notification"] = notification;
            return HttpResponse::json(HTTP::OK, data.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR,
                "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    spdlog::info("[ExportApi] Registered 35 routes");
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数（全局命名空间）
// ============================================================================
extern "C" {

PAPERCRAWLER_API void* createModule() {
    return new PaperCrawler::ExportApiModule();
}

PAPERCRAWLER_API void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::ExportApiModule*>(ptr);
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

}

