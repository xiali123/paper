#include "core/ModuleLoader.hpp"
#include "core/IModule.hpp"
#include "core/ModuleBase.hpp"
#include "core/Router.hpp"
#include "data/DatabaseModule.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <sstream>
#include "../../core/external/nlohmann/json.hpp"

#ifdef _WIN32
    #define LOAD_LIBRARY(path) LoadLibraryA(path)
    #define FREE_LIBRARY(handle) FreeLibrary(reinterpret_cast<HMODULE>(handle))
    #define GET_SYMBOL(handle, name) GetProcAddress(reinterpret_cast<HMODULE>(handle), name)
#else
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
    #define FREE_LIBRARY(handle) dlclose(handle)
    #define GET_SYMBOL(handle, name) dlsym(handle, name)
#endif

using json = nlohmann::json;

namespace PaperCrawler {

ModuleLoader& ModuleLoader::getInstance() {
    static ModuleLoader instance;
    return instance;
}

ModuleLoader::~ModuleLoader() {
    cleanup();
}

bool ModuleLoader::initialize(const std::string& modulesConfigPath) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    spdlog::info("[ModuleLoader] Initializing with config: {}", modulesConfigPath);
    configPath_ = modulesConfigPath;

    // 从配置文件加载模块定义
    if (!loadConfigFromJson(modulesConfigPath)) {
        spdlog::warn("[ModuleLoader] Failed to load config from {}, will use auto-discovery", modulesConfigPath);
    }

    spdlog::info("[ModuleLoader] Initialized successfully");
    return true;
}

bool ModuleLoader::loadConfigFromJson(const std::string& path) {
    try {
        std::ifstream configFile(path);
        if (!configFile.is_open()) {
            spdlog::warn("[ModuleLoader] Config file not found: {}", path);
            return false;
        }

        json config;
        configFile >> config;

        // 读取全局配置
        if (config.contains("modulesDirectory")) {
            modulesDirectory_ = config["modulesDirectory"];
            spdlog::info("[ModuleLoader] Modules directory: {}", modulesDirectory_);
        }

        if (config.contains("healthCheckInterval")) {
            healthCheckInterval_ = config["healthCheckInterval"];
            spdlog::info("[ModuleLoader] Health check interval: {}s", healthCheckInterval_);
        }

        // 读取模块配置
        if (config.contains("modules")) {
            for (const auto& moduleConfig : config["modules"]) {
                ModuleMetadata metadata;

                if (moduleConfig.contains("name")) metadata.name = moduleConfig["name"];
                if (moduleConfig.contains("version")) metadata.version = moduleConfig["version"];
                if (moduleConfig.contains("description")) metadata.description = moduleConfig["description"];
                if (moduleConfig.contains("author")) metadata.author = moduleConfig["author"];
                if (moduleConfig.contains("license")) metadata.license = moduleConfig["license"];
                if (moduleConfig.contains("routePrefix")) metadata.routePrefix = moduleConfig["routePrefix"];
                if (moduleConfig.contains("libraryPath")) metadata.libraryPath = moduleConfig["libraryPath"];
                if (moduleConfig.contains("loadPriority")) metadata.loadPriority = moduleConfig["loadPriority"];

                // 读取模块类型
                if (moduleConfig.contains("type")) {
                    std::string typeStr = moduleConfig["type"];
                    metadata.type = (typeStr == "BUSINESS") ? ModuleType::BUSINESS : ModuleType::SERVER;
                }

                // 读取端点列表
                if (moduleConfig.contains("endpoints")) {
                    for (const auto& endpoint : moduleConfig["endpoints"]) {
                        metadata.endpoints.push_back(endpoint);
                    }
                }

                // 读取依赖
                if (moduleConfig.contains("dependencies")) {
                    for (const auto& dep : moduleConfig["dependencies"]) {
                        std::string depName = dep["module"];
                        std::string minVersion = dep.value("minVersion", "1.0.0");
                        bool optional = dep.value("optional", false);
                        metadata.dependencies.emplace_back(depName, minVersion, optional);
                    }
                }

                // 读取配置
                if (moduleConfig.contains("config")) {
                    for (auto& [key, value] : moduleConfig["config"].items()) {
                        metadata.config[key] = value.get<std::string>();
                    }
                }

                // 如果没有指定libraryPath，尝试自动推断
                if (metadata.libraryPath.empty() && !modulesDirectory_.empty()) {
                    std::string libName = "lib" + metadata.name;
                    #ifdef _WIN32
                        libName += ".dll";
                    #else
                        libName += ".so";
                    #endif
                    metadata.libraryPath = modulesDirectory_ + "/" + libName;
                }

                modulesMetadata_[metadata.name] = metadata;
                spdlog::info("[ModuleLoader] Loaded metadata for module: {}", metadata.name);
            }
        }

        spdlog::info("[ModuleLoader] Config loaded successfully, {} modules configured", modulesMetadata_.size());
        return true;

    } catch (const std::exception& e) {
        spdlog::error("[ModuleLoader] Failed to parse config: {}", e.what());
        return false;
    }
}

std::vector<ModuleMetadata> ModuleLoader::scanDirectory(const std::string& directory) {
    std::vector<ModuleMetadata> discoveredModules;

    spdlog::info("[ModuleLoader] Scanning directory: {}", directory);

    namespace fs = std::filesystem;
    if (!fs::exists(directory)) {
        spdlog::warn("[ModuleLoader] Directory does not exist: {}", directory);
        return discoveredModules;
    }

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (!entry.is_regular_file()) continue;

        std::string path = entry.path().string();
        std::string filename = entry.path().filename().string();

        #ifdef _WIN32
            if (filename.find(".dll") == std::string::npos) continue;
        #else
            if (filename.find(".so") == std::string::npos) continue;
        #endif

        // 尝试读取模块元数据
        ModuleMetadata metadata;
        metadata.libraryPath = path;

        if (readModuleMetadata(path, metadata)) {
            discoveredModules.push_back(metadata);
            spdlog::info("[ModuleLoader] Discovered module: {} ({})", metadata.name, metadata.version);
        } else {
            spdlog::warn("[ModuleLoader] Failed to read metadata from: {}", filename);
        }
    }

    spdlog::info("[ModuleLoader] Discovered {} modules", discoveredModules.size());
    return discoveredModules;
}

bool ModuleLoader::readModuleMetadata(const std::string& libraryPath, ModuleMetadata& metadata) {
    // 加载动态库
    ModuleHandle handle = LOAD_LIBRARY(libraryPath.c_str());
    if (!handle) {
        spdlog::error("[ModuleLoader] Failed to load library: {}", libraryPath);
        return false;
    }

    // 获取模块信息函数
    auto getNameFunc = reinterpret_cast<const char* (*)()>(GET_SYMBOL(handle, "getModuleName"));
    auto getVersionFunc = reinterpret_cast<const char* (*)()>(GET_SYMBOL(handle, "getModuleVersion"));
    auto getDescFunc = reinterpret_cast<const char* (*)()>(GET_SYMBOL(handle, "getModuleDescription"));
    auto getTypeFunc = reinterpret_cast<const char* (*)()>(GET_SYMBOL(handle, "getModuleType"));
    auto getRoutePrefixFunc = reinterpret_cast<const char* (*)()>(GET_SYMBOL(handle, "getRoutePrefix"));

    if (!getNameFunc || !getVersionFunc) {
        spdlog::error("[ModuleLoader] Library {} does not export required symbols", libraryPath);
        FREE_LIBRARY(handle);
        return false;
    }

    // 读取基本信息
    metadata.name = getNameFunc();
    metadata.version = getVersionFunc();
    if (getDescFunc) metadata.description = getDescFunc();
    if (getTypeFunc) {
        std::string typeStr = getTypeFunc();
        metadata.type = (typeStr == "BUSINESS") ? ModuleType::BUSINESS : ModuleType::SERVER;
    }
    if (getRoutePrefixFunc) {
        metadata.routePrefix = getRoutePrefixFunc();
    }

    // 如果没有路由前缀，尝试自动推断
    if (metadata.routePrefix.empty() && metadata.type == ModuleType::BUSINESS) {
        metadata.routePrefix = inferRoutePrefix(metadata.name);
    }

    // 设置默认值
    if (metadata.description.empty()) {
        metadata.description = "No description available";
    }

    metadata.libraryPath = libraryPath;
    metadata.handle = handle;

    FREE_LIBRARY(handle);
    return true;
}

std::string ModuleLoader::inferRoutePrefix(const std::string& moduleName) {
    // 模块名到路由前缀的映射规则
    // 例如：AuthApi -> /api/auth, UserApiModule -> /api/users

    std::string lowerName = moduleName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    // 移除常见的后缀
    std::string baseName = lowerName;
    std::vector<std::string> suffixes = {"module", "api", "service"};

    for (const auto& suffix : suffixes) {
        if (baseName.length() > suffix.length()) {
            size_t pos = baseName.rfind(suffix);
            if (pos != std::string::npos && pos == baseName.length() - suffix.length()) {
                baseName = baseName.substr(0, pos);
                break;
            }
        }
    }

    return "/api/" + baseName;
}

bool ModuleLoader::loadAllModules() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    spdlog::info("[ModuleLoader] Loading all modules...");

    // 如果没有配置的模块，尝试自动发现
    if (modulesMetadata_.empty() && !modulesDirectory_.empty()) {
        auto discovered = scanDirectory(modulesDirectory_);
        for (auto& metadata : discovered) {
            modulesMetadata_[metadata.name] = metadata;
        }
    }

    if (modulesMetadata_.empty()) {
        spdlog::warn("[ModuleLoader] No modules to load");
        return true;
    }

    // 按优先级排序
    std::vector<ModuleMetadata> sortedModules;
    for (auto& [name, metadata] : modulesMetadata_) {
        sortedModules.push_back(metadata);
    }
    sortedModules = sortModulesByPriority(sortedModules);

    size_t successCount = 0;
    size_t failCount = 0;

    // 逐个加载模块
    for (auto& metadata : sortedModules) {
        spdlog::info("[ModuleLoader] Loading module: {} (priority: {})", metadata.name, metadata.loadPriority);

        if (loadModule(metadata)) {
            successCount++;
            triggerEvent("module_loaded", metadata.name, "Module loaded successfully");
        } else {
            failCount++;
            triggerEvent("module_failed", metadata.name, "Failed to load module");
        }
    }

    spdlog::info("[ModuleLoader] Loading complete: {} succeeded, {} failed", successCount, failCount);
    return (failCount == 0);
}

bool ModuleLoader::loadModule(const ModuleMetadata& metadata) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    spdlog::info("[ModuleLoader] Loading module: {} from {}", metadata.name, metadata.libraryPath);

    // 检查依赖
    if (!checkDependencies(metadata)) {
        spdlog::error("[ModuleLoader] Dependency check failed for module: {}", metadata.name);
        return false;
    }

    // 加载动态库
    ModuleHandle handle = LOAD_LIBRARY(metadata.libraryPath.c_str());
    if (!handle) {
        spdlog::error("[ModuleLoader] Failed to load library: {}", metadata.libraryPath);
        return false;
    }

    // 获取导出函数
    auto createFunc = reinterpret_cast<void* (*)()>(GET_SYMBOL(handle, "createModule"));
    auto destroyFunc = reinterpret_cast<void (*)(void*)>(GET_SYMBOL(handle, "destroyModule"));

    if (!createFunc) {
        spdlog::error("[ModuleLoader] Module {} does not export createModule", metadata.name);
        FREE_LIBRARY(handle);
        return false;
    }

    // 创建模块实例
    void* modulePtr = createFunc();
    if (!modulePtr) {
        spdlog::error("[ModuleLoader] Failed to create module instance: {}", metadata.name);
        FREE_LIBRARY(handle);
        return false;
    }

    auto* module = static_cast<IModule*>(modulePtr);

    // 注册路由（如果是业务模块）- 必须在initialize()之前
    // 因为BusinessModuleBase::initialize()会调用registerRoutes()
    ModuleMetadata mutableMetadata = metadata;
    mutableMetadata.handle = handle;
    mutableMetadata.loadTime = std::chrono::system_clock::now();
    mutableMetadata.healthStatus = ModuleHealthStatus::HEALTHY;

    if (metadata.type == ModuleType::BUSINESS) {
        // 先设置路由前缀和Router实例
        if (auto* businessModule = dynamic_cast<BusinessModuleBase*>(module)) {
            // 设置Router实例（确保使用主程序的Router）
            businessModule->setRouter(&Router::getInstance());
            // 设置路由前缀
            businessModule->setRoutePrefix(metadata.routePrefix);
            spdlog::info("[ModuleLoader] Set Router instance and route prefix for module {}: {}",
                        metadata.name, metadata.routePrefix);

            // 🔔 注入数据库连接（直接从DatabaseModule获取IDatabase接口）
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                // 创建shared_ptr，但不拥有所有权（DatabaseModule负责生命周期）
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {
                    // 空删除器
                });
                businessModule->setDatabase(dbPtr);
                spdlog::info("[ModuleLoader] ✅ Injected database connection to module: {}", metadata.name);
            } else {
                spdlog::warn("[ModuleLoader] ⚠️ No global DatabaseModule instance, module {} will use fallback connection", metadata.name);
            }
        }
    }

    // 初始化模块（这里会调用registerRoutes()，此时Router和前缀都已设置）
    if (!module->initialize()) {
        spdlog::error("[ModuleLoader] Failed to initialize module: {}", metadata.name);
        if (destroyFunc) {
            destroyFunc(modulePtr);
        }
        FREE_LIBRARY(handle);
        return false;
    }

    // 存储模块（使用自定义deleter确保DLL的destroyFunc被调用）
    modules_[metadata.name] = std::unique_ptr<IModule, std::function<void(IModule*)>>(
        module,
        [destroyFunc](IModule* ptr) {
            if (destroyFunc) {
                destroyFunc(ptr);
            }
        }
    );
    modulesMetadata_[metadata.name] = mutableMetadata;

    spdlog::info("[ModuleLoader] Module {} loaded successfully", metadata.name);
    return true;
}

bool ModuleLoader::registerModuleRoutes(IModule* module, const ModuleMetadata& metadata) {
    if (!module || metadata.type != ModuleType::BUSINESS) {
        return false;
    }

    spdlog::info("[ModuleLoader] Registering routes for module: {} -> {}", metadata.name, metadata.routePrefix);

    try {
        // 如果是业务模块，先设置路由前缀
        if (auto* businessModule = dynamic_cast<BusinessModuleBase*>(module)) {
            businessModule->setRoutePrefix(metadata.routePrefix);
            spdlog::info("[ModuleLoader] Set route prefix for module {}: {}", metadata.name, metadata.routePrefix);
        }

        // 调用模块的registerRoutes方法
        // 这里假设模块实现了registerRoutes方法
        // 或者我们可以通过Router的registerModuleRoutes方法来注册

        auto& router = Router::getInstance();
        router.registerModuleRoutes(metadata.routePrefix, module);

        spdlog::info("[ModuleLoader] Routes registered for module: {}", metadata.name);
        return true;

    } catch (const std::exception& e) {
        spdlog::error("[ModuleLoader] Failed to register routes for module {}: {}", metadata.name, e.what());
        return false;
    }
}

bool ModuleLoader::checkDependencies(const ModuleMetadata& metadata) {
    for (const auto& dep : metadata.dependencies) {
        // 检查依赖模块是否已加载
        if (modules_.find(dep.moduleName) == modules_.end()) {
            if (dep.optional) {
                spdlog::warn("[ModuleLoader] Optional dependency {} not found for module {}", dep.moduleName, metadata.name);
            } else {
                spdlog::error("[ModuleLoader] Required dependency {} not found for module {}", dep.moduleName, metadata.name);
                return false;
            }
        }
    }
    return true;
}

std::vector<ModuleMetadata> ModuleLoader::sortModulesByPriority(std::vector<ModuleMetadata> modules) {
    std::sort(modules.begin(), modules.end(),
        [](const ModuleMetadata& a, const ModuleMetadata& b) {
            return a.loadPriority > b.loadPriority;
        });
    return modules;
}

bool ModuleLoader::unloadModule(const std::string& moduleName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    spdlog::info("[ModuleLoader] Unloading module: {}", moduleName);

    auto it = modules_.find(moduleName);
    if (it == modules_.end()) {
        spdlog::warn("[ModuleLoader] Module not found: {}", moduleName);
        return false;
    }

    // 停止模块
    it->second->stop();
    it->second->cleanup();

    // 释放动态库
    auto metaIt = modulesMetadata_.find(moduleName);
    if (metaIt != modulesMetadata_.end() && metaIt->second.handle) {
        auto destroyFunc = reinterpret_cast<void (*)(void*)>(
            GET_SYMBOL(metaIt->second.handle, "destroyModule")
        );
        if (destroyFunc) {
            destroyFunc(it->second.get());
        }

        FREE_LIBRARY(metaIt->second.handle);
        modulesMetadata_.erase(metaIt);
    }

    modules_.erase(it);

    spdlog::info("[ModuleLoader] Module {} unloaded", moduleName);
    triggerEvent("module_unloaded", moduleName, "Module unloaded successfully");
    return true;
}

bool ModuleLoader::reloadModule(const std::string& moduleName) {
    spdlog::info("[ModuleLoader] Reloading module: {}", moduleName);

    // 保存元数据
    ModuleMetadata metadata;
    auto metaIt = modulesMetadata_.find(moduleName);
    if (metaIt != modulesMetadata_.end()) {
        metadata = metaIt->second;
    }

    // 卸载模块
    if (!unloadModule(moduleName)) {
        spdlog::error("[ModuleLoader] Failed to unload module for reload: {}", moduleName);
        return false;
    }

    // 重新加载
    if (loadModule(metadata)) {
        spdlog::info("[ModuleLoader] Module {} reloaded successfully", moduleName);
        triggerEvent("module_reloaded", moduleName, "Module reloaded successfully");
        return true;
    }

    return false;
}

bool ModuleLoader::startAllModules() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    spdlog::info("[ModuleLoader] Starting all modules...");

    // 先启动SERVER模块
    for (auto& [name, module] : modules_) {
        auto& metadata = modulesMetadata_[name];
        if (metadata.type == ModuleType::SERVER) {
            if (!module->start()) {
                spdlog::error("[ModuleLoader] Failed to start SERVER module: {}", name);
                return false;
            }
            metadata.startTime = std::chrono::system_clock::now();
            spdlog::info("[ModuleLoader] Started SERVER module: {}", name);
        }
    }

    // 再启动BUSINESS模块
    for (auto& [name, module] : modules_) {
        auto& metadata = modulesMetadata_[name];
        if (metadata.type == ModuleType::BUSINESS) {
            if (!module->start()) {
                spdlog::error("[ModuleLoader] Failed to start BUSINESS module: {}", name);
                return false;
            }
            metadata.startTime = std::chrono::system_clock::now();
            spdlog::info("[ModuleLoader] Started BUSINESS module: {}", name);
        }
    }

    spdlog::info("[ModuleLoader] All modules started successfully");
    return true;
}

bool ModuleLoader::stopAllModules() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    spdlog::info("[ModuleLoader] Stopping all modules...");

    for (auto& [name, module] : modules_) {
        module->stop();
        module->cleanup();
    }

    spdlog::info("[ModuleLoader] All modules stopped");
    return true;
}

void ModuleLoader::startHealthCheckThread(int intervalSeconds) {
    if (healthCheckRunning_.load()) {
        spdlog::warn("[ModuleLoader] Health check thread already running");
        return;
    }

    healthCheckInterval_ = intervalSeconds;
    healthCheckRunning_.store(true);
    healthCheckThread_ = std::thread(&ModuleLoader::healthCheckThreadFunc, this);

    spdlog::info("[ModuleLoader] Health check thread started (interval: {}s)", intervalSeconds);
}

void ModuleLoader::stopHealthCheckThread() {
    if (!healthCheckRunning_.load()) {
        return;
    }

    healthCheckRunning_.store(false);
    healthCheckCV_.notify_all();

    if (healthCheckThread_.joinable()) {
        healthCheckThread_.join();
    }

    spdlog::info("[ModuleLoader] Health check thread stopped");
}

void ModuleLoader::healthCheckThreadFunc() {
    while (healthCheckRunning_.load()) {
        // 执行健康检查
        performHealthCheck();

        // 等待指定间隔
        std::unique_lock<std::recursive_mutex> lock(mutex_);
        healthCheckCV_.wait_for(lock, std::chrono::seconds(healthCheckInterval_),
            [this] { return !healthCheckRunning_.load(); });
    }
}

void ModuleLoader::performHealthCheck() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    spdlog::debug("[ModuleLoader] Performing health check...");

    for (auto& [name, metadata] : modulesMetadata_) {
        checkModuleHealth(name);
    }
}

void ModuleLoader::checkModuleHealth(const std::string& moduleName) {
    auto metaIt = modulesMetadata_.find(moduleName);
    if (metaIt == modulesMetadata_.end()) {
        return;
    }

    auto& metadata = metaIt->second;

    // 检查模块是否存在
    auto moduleIt = modules_.find(moduleName);
    if (moduleIt == modules_.end()) {
        metadata.updateHealthStatus(ModuleHealthStatus::FAILED, "Module instance not found");
        triggerEvent("module_unhealthy", moduleName, "Module instance not found");
        return;
    }

    // 检查错误率
    double errorRate = metadata.getErrorRate();
    if (errorRate > 0.5) {  // 错误率超过50%
        metadata.updateHealthStatus(ModuleHealthStatus::UNHEALTHY,
            "High error rate: " + std::to_string(errorRate));
        triggerEvent("module_unhealthy", moduleName, "High error rate");
    } else if (errorRate > 0.1) {  // 错误率超过10%
        metadata.updateHealthStatus(ModuleHealthStatus::DEGRADED,
            "Elevated error rate: " + std::to_string(errorRate));
    } else {
        metadata.updateHealthStatus(ModuleHealthStatus::HEALTHY);
    }
}

IModule* ModuleLoader::getModule(const std::string& moduleName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        return it->second.get();
    }
    return nullptr;
}

ModuleMetadata* ModuleLoader::getModuleMetadata(const std::string& moduleName) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    auto it = modulesMetadata_.find(moduleName);
    if (it != modulesMetadata_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<ModuleMetadata> ModuleLoader::getAllModulesMetadata() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    std::vector<ModuleMetadata> result;
    for (const auto& [name, metadata] : modulesMetadata_) {
        result.push_back(metadata);
    }
    return result;
}

std::map<std::string, ModuleMetadata> ModuleLoader::getModuleStats() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return modulesMetadata_;
}

void ModuleLoader::registerEventListener(const std::string& eventType, ModuleEventCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    eventListeners_[eventType].push_back(callback);
}

void ModuleLoader::triggerEvent(const std::string& eventType, const std::string& moduleName, const std::string& message) {
    auto it = eventListeners_.find(eventType);
    if (it != eventListeners_.end()) {
        for (auto& callback : it->second) {
            try {
                callback(moduleName, message);
            } catch (const std::exception& e) {
                spdlog::error("[ModuleLoader] Event callback error: {}", e.what());
            }
        }
    }

    spdlog::info("[ModuleLoader] Event: {} - {} - {}", eventType, moduleName, message);
}

bool ModuleLoader::saveConfig(const std::string& path) {
    try {
        json config;

        config["modulesDirectory"] = modulesDirectory_;
        config["healthCheckInterval"] = healthCheckInterval_;
        config["modules"] = json::array();

        for (const auto& [name, metadata] : modulesMetadata_) {
            json moduleConfig;
            moduleConfig["name"] = metadata.name;
            moduleConfig["version"] = metadata.version;
            moduleConfig["description"] = metadata.description;
            moduleConfig["type"] = (metadata.type == ModuleType::BUSINESS) ? "BUSINESS" : "SERVER";
            moduleConfig["author"] = metadata.author;
            moduleConfig["license"] = metadata.license;
            moduleConfig["routePrefix"] = metadata.routePrefix;
            moduleConfig["libraryPath"] = metadata.libraryPath;
            moduleConfig["loadPriority"] = metadata.loadPriority;

            if (!metadata.endpoints.empty()) {
                moduleConfig["endpoints"] = metadata.endpoints;
            }

            if (!metadata.dependencies.empty()) {
                moduleConfig["dependencies"] = json::array();
                for (const auto& dep : metadata.dependencies) {
                    json depConfig;
                    depConfig["module"] = dep.moduleName;
                    depConfig["minVersion"] = dep.minVersion;
                    depConfig["optional"] = dep.optional;
                    moduleConfig["dependencies"].push_back(depConfig);
                }
            }

            config["modules"].push_back(moduleConfig);
        }

        std::ofstream outFile(path);
        outFile << config.dump(4);

        spdlog::info("[ModuleLoader] Config saved to: {}", path);
        return true;

    } catch (const std::exception& e) {
        spdlog::error("[ModuleLoader] Failed to save config: {}", e.what());
        return false;
    }
}

void ModuleLoader::cleanup() {
    spdlog::info("[ModuleLoader] Cleaning up...");

    stopHealthCheckThread();
    stopAllModules();

    for (auto& [name, module] : modules_) {
        module->cleanup();
    }
    modules_.clear();
    modulesMetadata_.clear();

    spdlog::info("[ModuleLoader] Cleanup complete");
}

} // namespace PaperCrawler
