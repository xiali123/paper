#include "core/HttpStatus.hpp"
#include "business/StatsApiModule.hpp"
#include "data/DatabaseModule.hpp"
#include "data/IDatabase.hpp"
#include "data/QueryCache.hpp"
#include "core/ModuleRegistry.hpp"
#include "core/Router.hpp"
#include "core/MessageBus.hpp"
#include "messages/DatabaseConnectionMessage.hpp"
#include "business/JsonHelper.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <map>
#include <chrono>
#include <spdlog/spdlog.h>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #pragma comment(lib, "psapi.lib")
#else
    #include <sys/sysinfo.h>
    #include <sys/resource.h>
    #include <sys/utsname.h>
    #include <sys/statvfs.h>
    #include <unistd.h>
#endif

namespace PaperCrawler {

// ============================================================================
// SystemResources JSON 序列化
// ============================================================================

std::string SystemResources::toJSON() const {
    nlohmann::json json;
    json["cpu_usage_percent"] = cpuUsagePercent;
    json["memory_usage_percent"] = memoryUsagePercent;
    json["memory_total"] = memoryTotal;
    json["memory_used"] = memoryUsed;
    json["memory_available"] = memoryAvailable;
    json["disk_usage_percent"] = diskUsagePercent;
    json["disk_total"] = diskTotal;
    json["disk_used"] = diskUsed;
    json["disk_available"] = diskAvailable;
    json["load_average_1m"] = loadAverage1m;
    json["load_average_5m"] = loadAverage5m;
    json["load_average_15m"] = loadAverage15m;
    return json.dump();
}

std::string SystemInfo::toJSON() const {
    nlohmann::json json;
    json["hostname"] = hostname;
    json["os_type"] = osType;
    json["os_version"] = osVersion;
    json["os_architecture"] = osArchitecture;
    json["cpu_model"] = cpuModel;
    json["cpu_cores"] = cpuCores;
    json["cpu_frequency"] = cpuFrequency;
    json["total_memory"] = totalMemory;
    json["kernel_version"] = kernelVersion;
    json["cpp_version"] = cppVersion;
    return json.dump();
}

// ============================================================================
// SystemUptime JSON 序列化
// ============================================================================

std::string SystemUptime::format() const {
    std::ostringstream oss;
    if (days > 0) {
        oss << days << "d ";
    }
    if (hours > 0 || days > 0) {
        oss << hours << "h ";
    }
    if (minutes > 0 || hours > 0 || days > 0) {
        oss << minutes << "m ";
    }
    oss << seconds << "s";
    return oss.str();
}

std::string SystemUptime::toJSON() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"total_seconds\": " << totalSeconds << ",\n";
    json << "  \"days\": " << days << ",\n";
    json << "  \"hours\": " << hours << ",\n";
    json << "  \"minutes\": " << minutes << ",\n";
    json << "  \"seconds\": " << seconds << ",\n";
    json << "  \"formatted\": \"" << format() << "\"\n";
    json << "}";
    return json.str();
}

// ============================================================================
// PerformanceMetrics JSON 序列化
// ============================================================================

std::string PerformanceMetrics::toJSON() const {
    std::ostringstream json;
    json << "{\n";

    // Request counts
    json << "  \"request_counts\": {\n";
    bool first = true;
    for (const auto& pair : requestCounts) {
        if (!first) json << ",\n";
        first = false;
        json << "    \"" << pair.first << "\": " << pair.second;
    }
    json << "\n  },\n";

    // Average response times
    json << "  \"average_response_times\": {\n";
    first = true;
    for (const auto& pair : averageResponseTimes) {
        if (!first) json << ",\n";
        first = false;
        json << "    \"" << pair.first << "\": " << pair.second.count();
    }
    json << "\n  },\n";

    // Throughput
    json << "  \"throughput\": {\n";
    first = true;
    for (const auto& pair : throughput) {
        if (!first) json << ",\n";
        first = false;
        json << "    \"" << pair.first << "\": " << pair.second;
    }
    json << "\n  },\n";

    // Error counts
    json << "  \"error_counts\": {\n";
    first = true;
    for (const auto& pair : errorCounts) {
        if (!first) json << ",\n";
        first = false;
        json << "    \"" << pair.first << "\": " << pair.second;
    }
    json << "\n  }\n";

    json << "}";
    return json.str();
}

// ============================================================================
// StatsApiModule 实现
// ============================================================================

class StatsApiModule::Impl {
public:
    // 依赖注入：数据库接口
    std::shared_ptr<IDatabase> database_;

    SystemInfo systemInfo;
    SystemResources currentResources;
    std::chrono::system_clock::time_point startTime_;

    Impl(std::shared_ptr<IDatabase> database)
        : database_(database), startTime_(std::chrono::system_clock::now()) {
        collectSystemInfo();
    }

    Impl() : Impl(nullptr) {}  // 保持兼容性

    // 从数据库获取统计信息
    int getUserCount() {
        if (!database_) return 0;
        try {
            auto results = database_->query("SELECT COUNT(*) as count FROM users");
            if (!results.empty()) {
                return std::stoi(results[0]["count"]);
            }
        } catch (...) { spdlog::warn("[StatsApi] Failed to parse numeric parameter"); }
        return 0;
    }

    int getPaperCount() {
        if (!database_) return 0;
        try {
            auto results = database_->query("SELECT COUNT(*) as count FROM papers");
            if (!results.empty()) {
                return std::stoi(results[0]["count"]);
            }
        } catch (...) { spdlog::warn("[StatsApi] Failed to parse numeric parameter"); }
        return 0;
    }

    int getSessionCount() {
        if (!database_) return 0;
        try {
            auto results = database_->query("SELECT COUNT(*) as count FROM user_sessions WHERE expires_at > NOW()");
            if (!results.empty()) {
                return std::stoi(results[0]["count"]);
            }
        } catch (...) { spdlog::warn("[StatsApi] Failed to parse numeric parameter"); }
        return 0;
    }

    void collectSystemInfo() {
        // 获取主机名
        char hostname[256];
#ifdef _WIN32
        DWORD size = sizeof(hostname);
        GetComputerNameA(hostname, &size);
#else
        gethostname(hostname, sizeof(hostname));
#endif
        systemInfo.hostname = hostname;

        // 获取操作系统信息
        systemInfo.osType = "Unknown";
        systemInfo.osVersion = "Unknown";
        systemInfo.osArchitecture = "Unknown";

#ifdef _WIN32
        systemInfo.osType = "Windows";
        OSVERSIONINFO osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionExA(&osvi);
        std::ostringstream version;
        version << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << "." << osvi.dwBuildNumber;
        systemInfo.osVersion = version.str();

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        systemInfo.cpuCores = sysInfo.dwNumberOfProcessors;

        // Get total memory using MEMORYSTATUSEX
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(memInfo);
        GlobalMemoryStatusEx(&memInfo);
        systemInfo.totalMemory = memInfo.ullTotalPhys;

        // CPU架构
        systemInfo.osArchitecture = "x64";
#else
        systemInfo.osType = "Linux";
        struct utsname unameInfo;
        uname(&unameInfo);
        systemInfo.osVersion = unameInfo.release;
        systemInfo.osArchitecture = unameInfo.machine;

        systemInfo.cpuCores = sysconf(_SC_NPROCESSORS_ONLN);
        systemInfo.totalMemory = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE);
#endif

        systemInfo.cppVersion = __cplusplus;
    }

    SystemResources collectResources() {
        SystemResources resources;

#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(memInfo);
        GlobalMemoryStatusEx(&memInfo);

        resources.memoryTotal = memInfo.ullTotalPhys;
        resources.memoryAvailable = memInfo.ullAvailPhys;
        resources.memoryUsed = resources.memoryTotal - resources.memoryAvailable;
        resources.memoryUsagePercent = (static_cast<double>(resources.memoryUsed) / resources.memoryTotal) * 100.0;

        // CPU使用率（简化版）
        FILETIME idleTime, kernelTime, userTime;
        if (GetSystemTimes((FILETIME*)&idleTime, (FILETIME*)&kernelTime, (FILETIME*)&userTime)) {
            ULONGLONG totalTime = idleTime.dwLowDateTime + kernelTime.dwLowDateTime + userTime.dwLowDateTime;
            if (totalTime > 0) {
                resources.cpuUsagePercent = ((totalTime - idleTime.dwLowDateTime) * 100.0) / totalTime;
            }
        }

        // 磁盘使用率
        ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
        if (GetDiskFreeSpaceExA("C:\\", &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
            resources.diskTotal = totalNumberOfBytes.QuadPart;
            resources.diskAvailable = totalNumberOfFreeBytes.QuadPart;
            resources.diskUsed = resources.diskTotal - resources.diskAvailable;
            resources.diskUsagePercent = (static_cast<double>(resources.diskUsed) / resources.diskTotal) * 100.0;
        }

        // 负载平均值（Windows不支持，设置为0）
        resources.loadAverage1m = 0;
        resources.loadAverage5m = 0;
        resources.loadAverage15m = 0;
#else
        // Linux实现
        struct sysinfo info;
        sysinfo(&info);

        resources.loadAverage1m = info.loads[0];
        resources.loadAverage5m = info.loads[1];
        resources.loadAverage15m = info.loads[2];

        // 内存使用率
        resources.memoryTotal = info.totalram * info.mem_unit;
        resources.memoryAvailable = info.freeram * info.mem_unit;
        resources.memoryUsed = resources.memoryTotal - resources.memoryAvailable;
        resources.memoryUsagePercent = (static_cast<double>(resources.memoryUsed) / resources.memoryTotal) * 100.0;

        // 磁盘使用率（简化，只检查根分区）
        struct statvfs stat;
        if (statvfs("/", &stat) == 0) {
            resources.diskTotal = stat.f_blocks * stat.f_frsize;
            resources.diskAvailable = stat.f_bavail * stat.f_frsize;
            resources.diskUsed = resources.diskTotal - resources.diskAvailable;
            resources.diskUsagePercent = (static_cast<double>(resources.diskUsed) / resources.diskTotal) * 100.0;
        }

        // CPU使用率（简化）
        resources.cpuUsagePercent = 0.0; // Linux需要更复杂的实现
#endif

        return resources;
    }
};

// ============================================================================

StatsApiModule::StatsApiModule()
    : StatsApiModule(nullptr) {
}

StatsApiModule::StatsApiModule(std::shared_ptr<IDatabase> database)
    : impl_(std::make_unique<Impl>()) {
    // 接收database参数并保存到impl_（后续启用数据库集成时取消注释）
    // impl_->database_ = database;
}

StatsApiModule::~StatsApiModule() = default;

SystemInfo StatsApiModule::getSystemInfo() {
    return impl_->systemInfo;
}

SystemResources StatsApiModule::getResources() {
    return impl_->collectResources();
}

SystemUptime StatsApiModule::getUptime() {
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - impl_->startTime_);

    SystemUptime systemUptime;
    systemUptime.totalSeconds = uptime.count();
    systemUptime.days = uptime.count() / 86400;
    systemUptime.hours = (uptime.count() % 86400) / 3600;
    systemUptime.minutes = (uptime.count() % 3600) / 60;
    systemUptime.seconds = uptime.count() % 60;
    systemUptime.startTime = impl_->startTime_;

    return systemUptime;
}

std::vector<ModuleInfo> StatsApiModule::getAllModules() {
    return ModuleRegistry::getInstance().getAllModules();
}

std::optional<ModuleInfo> StatsApiModule::getModule(const std::string& moduleName) {
    auto modules = getAllModules();
    for (const auto& module : modules) {
        if (module.name == moduleName) {
            return module;
        }
    }
    return std::nullopt;
}

PerformanceMetrics StatsApiModule::getPerformanceMetrics() {
    PerformanceMetrics metrics;

    // 使用真实数据库数据（如果有数据库连接）
    if (impl_->database_) {
        metrics.requestCounts["Users"] = impl_->getUserCount();
        metrics.requestCounts["Papers"] = impl_->getPaperCount();
        metrics.requestCounts["ActiveSessions"] = impl_->getSessionCount();
    } else {
        // 降级到Mock数据
        metrics.requestCounts["HttpServer"] = 1000;
        metrics.requestCounts["PaperApi"] = 500;
        metrics.requestCounts["AuthApi"] = 200;
    }

    metrics.averageResponseTimes["HttpServer"] = std::chrono::microseconds(15000);
    metrics.averageResponseTimes["PaperApi"] = std::chrono::microseconds(25000);
    metrics.averageResponseTimes["AuthApi"] = std::chrono::microseconds(10000);

    metrics.throughput["HttpServer"] = 100.0;
    metrics.throughput["PaperApi"] = 50.0;
    metrics.throughput["AuthApi"] = 20.0;

    metrics.errorCounts["HttpServer"] = 5;
    metrics.errorCounts["PaperApi"] = 10;
    metrics.errorCounts["AuthApi"] = 2;

    return metrics;
}

std::string StatsApiModule::getRealtimeStats() {
    std::ostringstream json;
    json << "{\n";
    json << "  \"timestamp\": " << std::chrono::system_clock::now().time_since_epoch().count() << ",\n";
    json << "  \"system\": " << getSystemInfo().toJSON() << ",\n";
    json << "  \"resources\": " << getResources().toJSON() << ",\n";
    json << "  \"uptime\": " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - impl_->startTime_).count() << "\n";
    json << "}";
    return json.str();
}

bool StatsApiModule::isHealthy() {
    // 检查关键组件是否健康
    // 实际应检查各模块的健康状态（当前为简化版本）
    return true;
}

std::map<std::string, bool> StatsApiModule::getHealthDetails() {
    std::map<std::string, bool> health;

    health["HttpServer"] = true;
    health["PaperApi"] = true;
    health["AuthApi"] = true;
    health["StatsApi"] = true;
    health["Database"] = true;
    health["Cache"] = true;

    return health;
}

void StatsApiModule::updatePerformanceMetrics(const std::string& moduleName,
                                               uint64_t requestCount,
                                               std::chrono::microseconds responseTime,
                                               bool success) {
    // 更新性能指标（预留接口）
}

// ============================================================================
// 路由处理
// ============================================================================

void StatsApiModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix(); // "/api/stats"

    spdlog::info("[StatsApi] Registering routes with prefix: {}", prefix);

    // 🔔 优先级1：使用ModuleLoader注入的数据库连接
    database_ = getDatabase();
    if (database_) {
        spdlog::info("[StatsApi] ✅ Received injected database connection from ModuleLoader!");
    }

    // 🔔 优先级2：尝试从全局DatabaseModule获取（如果注入失败）
    if (!database_) {
        try {
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {});
                database_ = dbPtr;
                spdlog::info("[StatsApi] ✅ Received shared database connection from global DatabaseModule!");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[StatsApi] Failed to get global database connection: {}", e.what());
        }
    }

    // 🔔 优先级3：回退到MessageBus（保留原有逻辑）
    if (!database_) {
        // 订阅MessageBus消息
        auto& messageBus = MessageBus::getInstance();
        messageBus.registerHandler(MessageType::CUSTOM,
            [this](std::shared_ptr<ModuleMessage> msg) -> std::shared_ptr<ModuleMessage> {
                auto dbMsg = std::dynamic_pointer_cast<Messages::DatabaseConnectionMessage>(msg);
                if (dbMsg && dbMsg->isSuccess()) {
                    impl_->database_ = dbMsg->getConnection();
                    spdlog::info("[StatsApi] ✅ Received database connection from MessageBus!");
                }
                // 返回确认消息
                auto response = std::make_shared<ModuleMessage>(MessageType::CUSTOM, "StatsApi", "DatabaseModule");
                response->setData("acknowledged", true);
                response->setData("moduleName", "StatsApi");
                return response;
            },
            "StatsApi"
        );

        spdlog::info("[StatsApi] Successfully subscribed to database connection messages");
    }

    // GET /api/stats - 论文统计信息（根路由）
    router.get(prefix, [this](const HttpRequest& req) {
        return HttpResponse::json(HTTP::OK, handleStats());
    });

    // GET /api/stats/system - 系统信息
    router.get(prefix + "/system", [this](const HttpRequest& req) {
        return HttpResponse::json(HTTP::OK, handleSystemInfo());
    });

    // GET /api/stats/resources - 资源使用情况
    router.get(prefix + "/resources", [this](const HttpRequest& req) {
        return HttpResponse::json(HTTP::OK, handleResources());
    });

    // GET /api/stats/uptime - 运行时间
    router.get(prefix + "/uptime", [this](const HttpRequest& req) {
        return HttpResponse::json(HTTP::OK, handleUptime());
    });

    // GET /api/stats/modules - 模块状态
    router.get(prefix + "/modules", [this](const HttpRequest& req) {
        return HttpResponse::json(HTTP::OK, handleModules());
    });

    // GET /api/stats/performance - 性能指标
    router.get(prefix + "/performance", [this](const HttpRequest& req) {
        return HttpResponse::json(HTTP::OK, handlePerformance());
    });

    // 聚合所有统计
    router.get(prefix + "/all", [this](const HttpRequest& req) {
        nlohmann::json resp;
        try {
            resp["stats"] = nlohmann::json::parse(handleStats());
            resp["system"] = nlohmann::json::parse(handleSystemInfo());
            resp["resources"] = nlohmann::json::parse(handleResources());
            resp["modules"] = nlohmann::json::parse(handleModules());
        } catch (...) {
            resp["error"] = "Partial data unavailable";
        }
        return HttpResponse::json(HTTP::OK, resp.dump());
    });

    // 按期刊统计
    router.get(prefix + "/journals", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"journals\":[],\"total\":0}");

        try {
            auto results = database_->query(
                "SELECT journal, COUNT(*) as count FROM papers WHERE journal IS NOT NULL AND journal != '' "
                "GROUP BY journal ORDER BY count DESC LIMIT 20");
            nlohmann::json arr = nlohmann::json::array();
            int total = 0;
            for (auto& row : results) {
                nlohmann::json item;
                item["journal"] = row.count("journal") ? row.at("journal") : "";
                item["count"] = row.count("count") ? std::stoi(row.at("count")) : 0;
                total += item["count"].get<int>();
                arr.push_back(item);
            }
            // 补充百分比
            for (auto& item : arr) {
                item["percentage"] = total > 0 ? (item["count"].get<int>() * 100.0 / total) : 0;
            }
            nlohmann::json resp;
            resp["journals"] = arr;
            resp["total"] = total;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // 按年份统计
    router.get(prefix + "/years", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"years\":[],\"total\":0}");

        try {
            auto results = database_->query(
                "SELECT YEAR(publication_date) as year, COUNT(*) as count "
                "FROM papers WHERE publication_date IS NOT NULL "
                "GROUP BY YEAR(publication_date) ORDER BY year DESC");
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["year"] = row.count("year") ? std::stoi(row.at("year")) : 0;
                item["count"] = row.count("count") ? std::stoi(row.at("count")) : 0;
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["years"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // 按作者统计
    router.get(prefix + "/authors", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"authors\":[],\"total\":0}");

        try {
            auto results = database_->query(
                "SELECT authors, COUNT(*) as count FROM papers "
                "WHERE authors IS NOT NULL AND authors != '' "
                "GROUP BY authors ORDER BY count DESC LIMIT 20");
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["authors"] = row.count("authors") ? row.at("authors") : "";
                item["count"] = row.count("count") ? std::stoi(row.at("count")) : 0;
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["authors"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // 引用趋势
    router.get(prefix + "/citation-trends", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"trends\":[],\"total\":0}");

        try {
            auto results = database_->query(
                "SELECT YEAR(publication_date) as year, SUM(citation_count) as citations, "
                "AVG(citation_count) as avg_citations, COUNT(*) as papers "
                "FROM papers WHERE publication_date IS NOT NULL "
                "GROUP BY YEAR(publication_date) ORDER BY year DESC LIMIT 20");
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["year"] = row.count("year") ? std::stoi(row.at("year")) : 0;
                item["citations"] = row.count("citations") ? std::stoi(row.at("citations")) : 0;
                item["avgCitations"] = row.count("avg_citations") ? std::stod(row.at("avg_citations")) : 0.0;
                item["papers"] = row.count("papers") ? std::stoi(row.at("papers")) : 0;
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["trends"] = arr;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Top authors by paper count
    router.get(prefix + "/top-authors", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"authors\":[],\"total\":0}");

        try {
            int limit = 20;
            auto it = req.queryParams.find("limit");
            if (it != req.queryParams.end()) limit = std::stoi(it->second);

            auto results = database_->query(
                "SELECT authors, COUNT(*) as paper_count, SUM(citation_count) as total_citations "
                "FROM papers GROUP BY authors ORDER BY paper_count DESC LIMIT " + std::to_string(limit));
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["name"] = row.count("authors") ? row.at("authors") : "";
                item["paperCount"] = row.count("paper_count") ? std::stoi(row.at("paper_count")) : 0;
                item["totalCitations"] = row.count("total_citations") ? std::stoi(row.at("total_citations")) : 0;
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["authors"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Keyword research trends over years
    router.get(prefix + "/research-trends", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"trends\":[],\"total\":0}");

        try {
            auto results = database_->query(
                "SELECT keywords, year, COUNT(*) as count FROM papers "
                "WHERE keywords IS NOT NULL AND keywords != '' "
                "GROUP BY keywords, year ORDER BY count DESC LIMIT 50");
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["keyword"] = row.count("keywords") ? row.at("keywords") : "";
                item["year"] = row.count("year") && !row.at("year").empty() ? std::stoi(row.at("year")) : 0;
                item["count"] = row.count("count") ? std::stoi(row.at("count")) : 0;
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["trends"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Growth timeline — papers per month
    router.get(prefix + "/growth", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"growth\":[],\"total\":0}");

        try {
            int months = 12;
            auto it = req.queryParams.find("months");
            if (it != req.queryParams.end()) months = std::stoi(it->second);

            auto results = database_->query(
                "SELECT DATE_FORMAT(created_at, '%Y-%m') as month, COUNT(*) as count "
                "FROM papers WHERE created_at >= DATE_SUB(NOW(), INTERVAL " + std::to_string(months) + " MONTH) "
                "GROUP BY month ORDER BY month");
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["month"] = row.count("month") ? row.at("month") : "";
                item["count"] = row.count("count") ? std::stoi(row.at("count")) : 0;
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["growth"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // CCF level distribution
    router.get(prefix + "/ccf", [this](const HttpRequest& req) -> HttpResponse {
        if (!database_)
            return HttpResponse::json(HTTP::OK, "{\"distribution\":[]}");

        try {
            auto results = database_->query(
                "SELECT j.ccf_level, COUNT(p.id) as count "
                "FROM papers p LEFT JOIN journals j ON p.journal_id = j.id "
                "GROUP BY j.ccf_level ORDER BY count DESC");
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["level"] = row.count("ccf_level") && !row.at("ccf_level").empty() ? row.at("ccf_level") : "Unknown";
                item["count"] = row.count("count") ? std::stoi(row.at("count")) : 0;
                arr.push_back(item);
            }
            nlohmann::json resp;
            resp["distribution"] = arr;
            return HttpResponse::json(HTTP::OK, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(HTTP::INTERNAL_ERROR, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    spdlog::info("[StatsApi] Registered 15 routes");
}

std::string StatsApiModule::handleStats() {
    // 查询缓存
    std::string cacheKey = CacheKeys::stats("overview");
    auto cached = QueryCache::instance().get(cacheKey);
    if (cached) {
        spdlog::debug("[StatsApi] Stats overview cache HIT");
        return *cached;
    }

    try {
        size_t totalPapers = 0;
        size_t totalJournals = 0;
        size_t totalAuthors = 0;
        size_t totalCollections = 0;
        size_t recentPapersCount = 0;

        if (database_) {
            // 查询统计数据
            auto papers = database_->query("SELECT COUNT(*) as count FROM papers");
            auto journals = database_->query("SELECT COUNT(*) as count FROM journals");
            auto authors = database_->query("SELECT COUNT(*) as count FROM authors");
            auto collections = database_->query("SELECT COUNT(*) as count FROM collections");

            totalPapers = papers.empty() ? 0 : std::stoul(papers[0].at("count"));
            totalJournals = journals.empty() ? 0 : std::stoul(journals[0].at("count"));
            totalAuthors = authors.empty() ? 0 : std::stoul(authors[0].at("count"));
            totalCollections = collections.empty() ? 0 : std::stoul(collections[0].at("count"));

            // 获取最近的论文数量（最近7天）
            auto recentPapers = database_->query(
                "SELECT COUNT(*) as count FROM papers WHERE created_at >= DATE_SUB(NOW(), INTERVAL 7 DAY)"
            );
            recentPapersCount = recentPapers.empty() ? 0 : std::stoul(recentPapers[0].at("count"));
        }

        // 构建 JSON 响应
        nlohmann::json json;
        json["success"] = true;
        json["stats"]["totalPapers"] = totalPapers;
        json["stats"]["totalJournals"] = totalJournals;
        json["stats"]["totalAuthors"] = totalAuthors;
        json["stats"]["totalCollections"] = totalCollections;
        json["stats"]["recentPapers"] = recentPapersCount;

        std::string responseBody = json.dump();
        QueryCache::instance().put(cacheKey, responseBody, CacheTTL::STATS);
        return responseBody;
    } catch (const std::exception& e) {
        spdlog::error("[StatsApi] Error in handleStats: {}", e.what());
        nlohmann::json json;
        json["success"] = false;
        json["error"] = e.what();
        return json.dump();
    }
}

std::string StatsApiModule::handleSystemInfo() {
    auto info = getSystemInfo();
    return JsonHelper::buildJsonResponse({
        {"system_info", info.toJSON()}
    });
}

std::string StatsApiModule::handleResources() {
    auto resources = getResources();
    return JsonHelper::buildJsonResponse({
        {"resources", resources.toJSON()}
    });
}

std::string StatsApiModule::handleUptime() {
    auto uptime = getUptime();
    std::map<std::string, std::string> data;
    data["total_seconds"] = std::to_string(uptime.totalSeconds);
    data["days"] = std::to_string(uptime.days);
    data["hours"] = std::to_string(uptime.hours);
    data["minutes"] = std::to_string(uptime.minutes);
    data["seconds"] = std::to_string(uptime.seconds);
    data["formatted"] = uptime.format();

    return JsonHelper::buildJsonResponse(data);
}

std::string StatsApiModule::handleModules() {
    auto modules = getAllModules();

    nlohmann::json modulesArray = nlohmann::json::array();
    for (const auto& module : modules) {
        nlohmann::json mod;
        mod["name"] = module.name;
        mod["version"] = module.version;
        mod["type"] = (module.type == ModuleType::SERVER ? "SERVER" : "BUSINESS");
        mod["state"] = (module.state == ModuleState::STARTED ? "STARTED" :
                  module.state == ModuleState::STOPPED ? "STOPPED" : "UNLOADED");
        mod["reference_count"] = module.referenceCount.load();
        modulesArray.push_back(mod);
    }

    return JsonHelper::buildJsonResponse({
        {"modules", modulesArray.dump()}
    });
}

std::string StatsApiModule::handleModule(const std::string& moduleName) {
    auto module = getModule(moduleName);
    if (!module.has_value()) {
        return JsonHelper::buildJsonResponse({
            {"error", "Module not found"}
        }, HTTP::NOT_FOUND);
    }

    std::map<std::string, std::string> data;
    const ModuleInfo& info = module.value();
    data["name"] = info.name;
    data["version"] = info.version;
    data["type"] = (info.type == ModuleType::SERVER ? "SERVER" : "BUSINESS");
    data["state"] = (info.state == ModuleState::STARTED ? "STARTED" :
                  info.state == ModuleState::STOPPED ? "STOPPED" : "UNLOADED");
    data["reference_count"] = std::to_string(info.referenceCount.load());

    return JsonHelper::buildJsonResponse(data);
}

std::string StatsApiModule::handlePerformance() {
    auto metrics = getPerformanceMetrics();

    nlohmann::json json;
    nlohmann::json requestCounts;
    for (const auto& pair : metrics.requestCounts) {
        requestCounts[pair.first] = pair.second;
    }

    nlohmann::json throughput;
    for (const auto& pair : metrics.throughput) {
        throughput[pair.first] = pair.second;
    }

    json["request_counts"] = requestCounts;
    json["throughput"] = throughput;

    return JsonHelper::buildJsonResponse({
        {"performance_metrics", json.dump()}
    });
}

std::string StatsApiModule::handleRealtime() {
    return JsonHelper::buildJsonResponse({
        {"realtime_stats", getRealtimeStats()}
    });
}

void StatsApiModule::monitorLoop() {
    while (streaming_) {
        // 收集实时统计数据（当前为空循环占位）
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================


extern "C" {

PAPERCRAWLER_API void* createModule() {
    return new PaperCrawler::StatsApiModule();
}

PAPERCRAWLER_API void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::StatsApiModule*>(ptr);
}

PAPERCRAWLER_API const char* getModuleVersion() {
    return "1.0.0";
}

}

