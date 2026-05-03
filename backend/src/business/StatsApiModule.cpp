#include <iostream>
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
    std::ostringstream json;
    json << "{\n";
    json << "  \"cpu_usage_percent\": " << cpuUsagePercent << ",\n";
    json << "  \"memory_usage_percent\": " << memoryUsagePercent << ",\n";
    json << "  \"memory_total\": " << memoryTotal << ",\n";
    json << "  \"memory_used\": " << memoryUsed << ",\n";
    json << "  \"memory_available\": " << memoryAvailable << ",\n";
    json << "  \"disk_usage_percent\": " << diskUsagePercent << ",\n";
    json << "  \"disk_total\": " << diskTotal << ",\n";
    json << "  \"disk_used\": " << diskUsed << ",\n";
    json << "  \"disk_available\": " << diskAvailable << ",\n";
    json << "  \"load_average_1m\": " << loadAverage1m << ",\n";
    json << "  \"load_average_5m\": " << loadAverage5m << ",\n";
    json << "  \"load_average_15m\": " << loadAverage15m << "\n";
    json << "}";
    return json.str();
}

std::string SystemInfo::toJSON() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"hostname\": \"" << hostname << "\",\n";
    json << "  \"os_type\": \"" << osType << "\",\n";
    json << "  \"os_version\": \"" << osVersion << "\",\n";
    json << "  \"os_architecture\": \"" << osArchitecture << "\",\n";
    json << "  \"cpu_model\": \"" << cpuModel << "\",\n";
    json << "  \"cpu_cores\": " << cpuCores << ",\n";
    json << "  \"cpu_frequency\": " << cpuFrequency << ",\n";
    json << "  \"total_memory\": " << totalMemory << ",\n";
    json << "  \"kernel_version\": \"" << kernelVersion << "\",\n";
    json << "  \"cpp_version\": \"" << cppVersion << "\"\n";
    json << "}";
    return json.str();
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
        } catch (...) {}
        return 0;
    }

    int getPaperCount() {
        if (!database_) return 0;
        try {
            auto results = database_->query("SELECT COUNT(*) as count FROM papers");
            if (!results.empty()) {
                return std::stoi(results[0]["count"]);
            }
        } catch (...) {}
        return 0;
    }

    int getSessionCount() {
        if (!database_) return 0;
        try {
            auto results = database_->query("SELECT COUNT(*) as count FROM user_sessions WHERE expires_at > NOW()");
            if (!results.empty()) {
                return std::stoi(results[0]["count"]);
            }
        } catch (...) {}
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

    spdlog::info("[StatsApiModule] Registering routes with prefix: {}", prefix);

    // 🔔 优先级1：使用ModuleLoader注入的数据库连接
    database_ = getDatabase();
    if (database_) {
        spdlog::info("[StatsApiModule] ✅ Received injected database connection from ModuleLoader!");
    }

    // 🔔 优先级2：尝试从全局DatabaseModule获取（如果注入失败）
    if (!database_) {
        try {
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {});
                database_ = dbPtr;
                spdlog::info("[StatsApiModule] ✅ Received shared database connection from global DatabaseModule!");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[StatsApiModule] Failed to get global database connection: {}", e.what());
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
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.headers["Content-Type"] = "application/json";
        response.body = handleStats();
        return response;
    });

    // GET /api/stats/system - 系统信息
    router.get(prefix + "/system", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.headers["Content-Type"] = "application/json";
        response.body = handleSystemInfo();
        return response;
    });

    // GET /api/stats/resources - 资源使用情况
    router.get(prefix + "/resources", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.headers["Content-Type"] = "application/json";
        response.body = handleResources();
        return response;
    });

    // GET /api/stats/uptime - 运行时间
    router.get(prefix + "/uptime", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.headers["Content-Type"] = "application/json";
        response.body = handleUptime();
        return response;
    });

    // GET /api/stats/modules - 模块状态
    router.get(prefix + "/modules", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.headers["Content-Type"] = "application/json";
        response.body = handleModules();
        return response;
    });

    // GET /api/stats/performance - 性能指标
    router.get(prefix + "/performance", [this](const HttpRequest& req) {
        HttpResponse response;
        response.statusCode = HTTP::OK;
        response.headers["Content-Type"] = "application/json";
        response.body = handlePerformance();
        return response;
    });

    spdlog::info("[StatsApiModule] Registered 6 routes");
}

std::string StatsApiModule::handleStats() {
    // 查询缓存
    std::string cacheKey = CacheKeys::stats("overview");
    auto cached = QueryCache::instance().get(cacheKey);
    if (cached) {
        spdlog::debug("[StatsApiModule] Stats overview cache HIT");
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

        // 手动构建 JSON 响应
        std::ostringstream json;
        json << "{\n";
        json << "  \"success\": true,\n";
        json << "  \"stats\": {\n";
        json << "    \"totalPapers\": " << totalPapers << ",\n";
        json << "    \"totalJournals\": " << totalJournals << ",\n";
        json << "    \"totalAuthors\": " << totalAuthors << ",\n";
        json << "    \"totalCollections\": " << totalCollections << ",\n";
        json << "    \"recentPapers\": " << recentPapersCount << "\n";
        json << "  }\n";
        json << "}";

        std::string responseBody = json.str();
        QueryCache::instance().put(cacheKey, responseBody, CacheTTL::STATS);
        return responseBody;
    } catch (const std::exception& e) {
        spdlog::error("[StatsApiModule] Error in handleStats: {}", e.what());
        std::ostringstream json;
        json << "{\n";
        json << "  \"success\": false,\n";
        json << "  \"error\": \"" << e.what() << "\"\n";
        json << "}";
        return json.str();
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

    std::ostringstream json;
    json << "[";
    bool first = true;
    for (const auto& module : modules) {
        if (!first) json << ",";
        first = false;

        json << "{\n";
        json << "  \"name\": \"" << module.name << "\",\n";
        json << "  \"version\": \"" << module.version << "\",\n";
        json << "  \"type\": \"" << (module.type == ModuleType::SERVER ? "SERVER" : "BUSINESS") << "\",\n";
        json << "  \"state\": \"" << (module.state == ModuleState::STARTED ? "STARTED" :
                  module.state == ModuleState::STOPPED ? "STOPPED" : "UNLOADED") << "\",\n";
        json << "  \"reference_count\": " << module.referenceCount.load() << "\n";
        json << "}";
    }
    json << "]";

    return JsonHelper::buildJsonResponse({
        {"modules", json.str()}
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

    std::ostringstream json;
    json << "{\n";
    json << "  \"request_counts\": {\n";

    bool first = true;
    for (const auto& pair : metrics.requestCounts) {
        if (!first) json << ",\n";
        first = false;
        json << "    \"" << pair.first << "\": " << pair.second;
    }

    json << "\n  },\n";
    json << "  \"throughput\": {\n";

    first = true;
    for (const auto& pair : metrics.throughput) {
        if (!first) json << ",\n";
        first = false;
        json << "    \"" << pair.first << "\": " << pair.second;
    }

    json << "\n  }\n";
    json << "}";

    return JsonHelper::buildJsonResponse({
        {"performance_metrics", json.str()}
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

