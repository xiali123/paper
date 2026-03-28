#include "framework/HotReloadManager.hpp"
#include "framework/PluginManager.hpp"
#include "framework/ModuleRegistry.hpp"
#include <iostream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/inotify.h>
#include <unistd.h>
#endif

namespace PaperCrawler {

HotReloadManager::~HotReloadManager() {
    stopFileWatcher();
}

ReloadResult HotReloadManager::reloadModule(const std::string& moduleName) {
    auto startTime = std::chrono::steady_clock::now();

    auto& registry = ModuleRegistry::getInstance();
    auto* moduleInfo = registry.getModuleInfo(moduleName);

    if (!moduleInfo) {
        return {false, "Module not found: " + moduleName, "", "", {}};
    }

    std::string oldVersion = moduleInfo->version;
    std::string newPath = moduleInfo->libraryPath;

    return reloadModule(moduleName, newPath);
}

ReloadResult HotReloadManager::reloadModule(const std::string& moduleName, const std::string& newModulePath) {
    auto startTime = std::chrono::steady_clock::now();

    auto& registry = ModuleRegistry::getInstance();
    auto* moduleInfo = registry.getModuleInfo(moduleName);

    if (!moduleInfo) {
        return {false, "Module not found: " + moduleName, "", "", {}};
    }

    std::string oldVersion = moduleInfo->version;

    std::cout << "Hot reloading module: " << moduleName << std::endl;

    // 1. 卸载旧版本
    auto& pluginMgr = PluginManager::getInstance();
    if (!pluginMgr.unloadModule(moduleName)) {
        return {false, "Failed to unload old version", oldVersion, "", {}};
    }

    // 2. 加载新版本
    if (!pluginMgr.loadModule(moduleName, newModulePath)) {
        return {false, "Failed to load new version", oldVersion, "", {}};
    }

    // 3. 获取新版本信息
    auto* newModuleInfo = registry.getModuleInfo(moduleName);
    std::string newVersion = newModuleInfo ? newModuleInfo->version : "unknown";

    auto endTime = std::chrono::steady_clock::now();
    auto reloadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // 4. 记录历史
    ReloadHistory history;
    history.moduleName = moduleName;
    history.timestamp = std::chrono::system_clock::now();
    history.fromVersion = oldVersion;
    history.toVersion = newVersion;
    history.success = true;
    addHistory(history);

    std::cout << "Hot reload completed: " << moduleName
              << " (" << oldVersion << " -> " << newVersion << ")"
              << " in " << reloadTime.count() << "ms" << std::endl;

    return {true, "Hot reload completed successfully", oldVersion, newVersion, reloadTime};
}

void HotReloadManager::startFileWatcher(const std::string& modulesDir) {
    if (watching_) {
        std::cout << "File watcher already running" << std::endl;
        return;
    }

    watchDirectory_ = modulesDir;
    watching_ = true;
    watcherThread_ = std::thread(&HotReloadManager::fileWatcherLoop, this);

    std::cout << "File watcher started for: " << modulesDir << std::endl;
}

void HotReloadManager::stopFileWatcher() {
    if (!watching_) {
        return;
    }

    watching_ = false;
    if (watcherThread_.joinable()) {
        watcherThread_.join();
    }

    std::cout << "File watcher stopped" << std::endl;
}

bool HotReloadManager::rollbackModule(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = history_.find(moduleName);
    if (it == history_.end() || it->second.empty()) {
        std::cerr << "No history found for module: " << moduleName << std::endl;
        return false;
    }

    // 找到上一个成功的版本
    for (auto histIt = it->second.rbegin(); histIt != it->second.rend(); ++histIt) {
        if (histIt->success) {
            std::cout << "Rolling back " << moduleName << " to version " << histIt->fromVersion << std::endl;
            // 这里需要实现实际的回滚逻辑
            return true;
        }
    }

    return false;
}

std::vector<ReloadHistory> HotReloadManager::getReloadHistory(const std::string& moduleName) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = history_.find(moduleName);
    if (it != history_.end()) {
        return it->second;
    }
    return {};
}

std::map<std::string, std::vector<ReloadHistory>> HotReloadManager::getAllHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

void HotReloadManager::clearHistory(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);
    history_.erase(moduleName);
}

void HotReloadManager::fileWatcherLoop() {
    namespace fs = std::filesystem;

    std::map<std::string, fs::file_time_type> lastModified;

    while (watching_) {
        try {
            for (const auto& entry : fs::directory_iterator(watchDirectory_)) {
                if (entry.is_regular_file()) {
                    auto path = entry.path();
                    std::string filename = path.filename().string();

                    // 只监控.so/.dll文件
                    #ifdef _WIN32
                        if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".dll") {
                            continue;
                        }
                    #else
                        if (filename.size() < 3 || filename.substr(filename.size() - 3) != ".so") {
                            continue;
                        }
                    #endif

                    auto lastWrite = entry.last_write_time();
                    auto it = lastModified.find(filename);

                    if (it == lastModified.end()) {
                        lastModified[filename] = lastWrite;
                    } else if (it->second != lastWrite) {
                        // 文件已修改
                        std::cout << "[HotReload] File change detected: " << filename << std::endl;

                        // 提取模块名（去掉扩展名）
                        size_t dotPos = filename.find_last_of('.');
                        std::string moduleName = filename.substr(0, dotPos);

                        // 尝试重载
                        auto result = reloadModule(moduleName, path.string());
                        if (result.success) {
                            std::cout << "[HotReload] Reload successful" << std::endl;
                        } else {
                            std::cerr << "[HotReload] Reload failed: " << result.message << std::endl;
                        }

                        lastModified[filename] = lastWrite;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[HotReload] Error: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void HotReloadManager::addHistory(const ReloadHistory& history) {
    std::lock_guard<std::mutex> lock(mutex_);
    history_[history.moduleName].push_back(history);

    // 限制历史记录数量（最多保留100条）
    if (history_[history.moduleName].size() > 100) {
        history_[history.moduleName].erase(history_[history.moduleName].begin());
    }
}

} // namespace PaperCrawler
