#include "core/PluginManager.hpp"
#include <spdlog/spdlog.h>
#include <iostream>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
    #define LOAD_LIBRARY(path) reinterpret_cast<void*>(LoadLibraryA(path))
    #define FREE_LIBRARY(handle) FreeLibrary(reinterpret_cast<HMODULE>(handle))
    #define GET_SYMBOL(handle, name) GetProcAddress(reinterpret_cast<HMODULE>(handle), name)
#else
    #include <dlfcn.h>
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
    #define FREE_LIBRARY(handle) dlclose(handle)
    #define GET_SYMBOL(handle, name) dlsym(handle, name)
#endif

namespace PaperCrawler {

PluginManager& PluginManager::getInstance() {
    static PluginManager instance;
    return instance;
}

bool PluginManager::initialize() {
    spdlog::info("PluginManager initialized");
    return true;
}

bool PluginManager::loadModule(const std::string& moduleName, const std::string& modulePath) {
    std::lock_guard<std::mutex> lock(mutex_);

    spdlog::info("Loading module: {} from {}", moduleName, modulePath);

    // 加载动态库
    ModuleHandle handle = LOAD_LIBRARY(modulePath.c_str());
    if (!handle) {
        spdlog::error("Failed to load module library: {}", modulePath);
        return false;
    }

    // 获取导出函数
    auto createFunc = reinterpret_cast<CreateModuleFunc>(
        GET_SYMBOL(handle, "createModule")
    );
    auto getVersionFunc = reinterpret_cast<GetModuleVersionFunc>(
        GET_SYMBOL(handle, "getModuleVersion")
    );

    if (!createFunc) {
        spdlog::error("Module {} does not export createModule function", moduleName);
        FREE_LIBRARY(handle);
        return false;
    }

    // 创建模块实例
    void* modulePtr = createFunc();
    if (!modulePtr) {
        spdlog::error("Failed to create module instance: {}", moduleName);
        FREE_LIBRARY(handle);
        return false;
    }

    auto* module = static_cast<IModule*>(modulePtr);

    // 初始化模块
    if (!module->initialize()) {
        spdlog::error("Failed to initialize module: {}", moduleName);
        auto destroyFunc = reinterpret_cast<DestroyModuleFunc>(
            GET_SYMBOL(handle, "destroyModule")
        );
        if (destroyFunc) {
            destroyFunc(modulePtr);
        }
        FREE_LIBRARY(handle);
        return false;
    }

    // 存储模块
    modules_[moduleName] = std::unique_ptr<IModule>(module);
    handles_[moduleName] = handle;

    spdlog::info("Module {} loaded successfully (version: {})",
        moduleName, module->getVersion());

    return true;
}

bool PluginManager::unloadModule(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = modules_.find(moduleName);
    if (it == modules_.end()) {
        spdlog::warn("Module not found: {}", moduleName);
        return false;
    }

    spdlog::info("Unloading module: {}", moduleName);

    // 停止模块
    it->second->stop();
    it->second->cleanup();

    // 释放动态库
    auto handleIt = handles_.find(moduleName);
    if (handleIt != handles_.end()) {
        auto destroyFunc = reinterpret_cast<DestroyModuleFunc>(
            GET_SYMBOL(handleIt->second, "destroyModule")
        );
        if (destroyFunc) {
            destroyFunc(it->second.get());
        }

        FREE_LIBRARY(handleIt->second);
        handles_.erase(handleIt);
    }

    modules_.erase(it);

    spdlog::info("Module {} unloaded", moduleName);
    return true;
}

IModule* PluginManager::getModule(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<IModule*> PluginManager::getAllModules() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<IModule*> result;
    for (auto& pair : modules_) {
        result.push_back(pair.second.get());
    }
    return result;
}

std::vector<IModule*> PluginManager::getBusinessModules() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<IModule*> result;
    for (auto& pair : modules_) {
        if (pair.second->getModuleType() == ModuleType::BUSINESS) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

std::vector<IModule*> PluginManager::getServerModules() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<IModule*> result;
    for (auto& pair : modules_) {
        if (pair.second->getModuleType() == ModuleType::SERVER) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

bool PluginManager::startAllModules() {
    std::lock_guard<std::mutex> lock(mutex_);

    spdlog::info("Starting all modules...");

    // 先启动SERVER模块
    for (auto& pair : modules_) {
        if (pair.second->getModuleType() == ModuleType::SERVER) {
            if (!pair.second->start()) {
                spdlog::error("Failed to start SERVER module: {}", pair.first);
                return false;
            }
        }
    }

    // 再启动BUSINESS模块
    for (auto& pair : modules_) {
        if (pair.second->getModuleType() == ModuleType::BUSINESS) {
            if (!pair.second->start()) {
                spdlog::error("Failed to start BUSINESS module: {}", pair.first);
                return false;
            }
        }
    }

    spdlog::info("All modules started successfully");
    return true;
}

bool PluginManager::stopAllModules() {
    std::lock_guard<std::mutex> lock(mutex_);

    spdlog::info("Stopping all modules...");

    for (auto& pair : modules_) {
        pair.second->stop();
        pair.second->cleanup();
    }

    spdlog::info("All modules stopped");
    return true;
}

std::vector<std::string> PluginManager::getLoadedModules() const {
    std::vector<std::string> result;
    for (const auto& pair : modules_) {
        result.push_back(pair.first);
    }
    return result;
}

bool PluginManager::scanAndLoadModules(const std::string& modulesDir) {
    std::lock_guard<std::mutex> lock(mutex_);

    spdlog::info("Scanning modules directory: {}", modulesDir);

    // 检查目录是否存在
    namespace fs = std::filesystem;
    if (!fs::exists(modulesDir)) {
        spdlog::warn("Modules directory does not exist: {}", modulesDir);
        return false;
    }

    size_t loadedCount = 0;
    size_t failedCount = 0;

    // 递归扫描所有子目录
    for (const auto& entry : fs::recursive_directory_iterator(modulesDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        std::string path = entry.path().string();
        std::string filename = entry.path().filename().string();

        // 检查文件扩展名
        #ifdef _WIN32
            if (filename.find(".dll") == std::string::npos) {
                continue;
            }
        #else
            if (filename.find(".so") == std::string::npos) {
                continue;
            }
        #endif

        // 从文件名提取模块名
        // 例如：libpaperapi.dll → PaperApi
        std::string moduleName = filename;

        // 移除lib前缀
        if (moduleName.find("lib") == 0) {
            moduleName = moduleName.substr(3);
        }

        // 移除扩展名
        size_t dotPos = moduleName.find('.');
        if (dotPos != std::string::npos) {
            moduleName = moduleName.substr(0, dotPos);
        }

        // 首字母大写
        if (!moduleName.empty()) {
            moduleName[0] = std::toupper(moduleName[0]);
        }

        spdlog::info("Found module library: {} -> {}", filename, moduleName);

        // 加载模块
        if (loadModule(moduleName, path)) {
            loadedCount++;
        } else {
            failedCount++;
        }
    }

    spdlog::info("Module scan complete: {} loaded, {} failed",
                 loadedCount, failedCount);

    return (failedCount == 0);
}

PluginManager::~PluginManager() {
    stopAllModules();

    for (auto& pair : handles_) {
        FREE_LIBRARY(pair.second);
    }
    handles_.clear();
    modules_.clear();
}

} // namespace PaperCrawler
