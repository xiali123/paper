#include <iostream>
#include "business/StatsApiModule.hpp"
#include "data/IDatabase.hpp"
#include "core/ModuleRegistry.hpp"
#include "business/JsonHelper.hpp"
#include <sstream>
#include <map>
#include <chrono>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #pragma comment(lib, "psapi.lib")
#else
    #include <sys/sysinfo.h>
    #include <sys/resource.h>
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
    : impl_(std::make_unique<Impl>()) {
}

StatsApiModule::~StatsApiModule() = default;

bool StatsApiModule::initialize() {
    registerRoutes();
    std::cout << "StatsApiModule initialized" << std::endl;
    return true;
}

bool StatsApiModule::start() {
    std::cout << "StatsApiModule started" << std::endl;
    return true;
}

bool StatsApiModule::stop() {
    std::cout << "StatsApiModule stopped" << std::endl;
    return true;
}

void StatsApiModule::cleanup() {
    // 清理资源
}

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
    // TODO: 从ModuleRegistry获取所有模块信息
    std::vector<ModuleInfo> modules;

    // Mock数据
    ModuleInfo module1;
    module1.name = "HttpServer";
    module1.version = "1.0.0";
    module1.type = ModuleType::SERVER;
    module1.state = ModuleState::STARTED;
    module1.loadedAt = std::chrono::system_clock::now() - std::chrono::milliseconds(5000);
    modules.push_back(module1);

    ModuleInfo module2;
    module2.name = "PaperApi";
    module2.version = "1.0.0";
    module2.type = ModuleType::BUSINESS;
    module2.state = ModuleState::STARTED;
    module2.loadedAt = std::chrono::system_clock::now() - std::chrono::milliseconds(3000);
    modules.push_back(module2);

    return modules;
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
    // TODO: 实际应该检查各模块的健康状态
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
    // TODO: 更新性能指标
}

// ============================================================================
// 路由处理
// ============================================================================

void StatsApiModule::registerRoutes() {
    // TODO: 注册路由到 Router
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
        }, 404);
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
        // TODO: 收集实时统计数据
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

} // namespace PaperCrawler

// ============================================================================
// DLL导出函数
// ============================================================================

#define EXPORT __declspec(dllexport)

extern "C" {

EXPORT void* createModule() {
    return new PaperCrawler::StatsApiModule();
}

EXPORT void destroyModule(void* ptr) {
    delete static_cast<PaperCrawler::StatsApiModule*>(ptr);
}

EXPORT const char* getModuleVersion() {
    return "1.0.0";
}

}

