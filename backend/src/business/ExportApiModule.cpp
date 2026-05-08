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

        nlohmann::json resp;
        resp["success"] = true;
        resp["format"] = format;
        resp["message"] = "Export initiated";
        resp["downloadUrl"] = "/downloads/export." + format;
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
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    spdlog::info("[ExportApi] Registered 11 routes");
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

