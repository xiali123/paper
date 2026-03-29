#include "core/ModuleRegistry.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace PaperCrawler {

ModuleRegistry& ModuleRegistry::getInstance() {
    static ModuleRegistry instance;
    return instance;
}

bool ModuleRegistry::loadFromConfig(const std::string& configPath) {
    // TODO: 实现配置文件解析
    // 暂时返回 true，使用硬编码的模块信息
    spdlog::info("ModuleRegistry: config loading not yet implemented, using hardcoded modules");
    spdlog::info("Config path: {}", configPath);
    return true;
}

void ModuleRegistry::registerModule(const ModuleInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);

    modules_[info.name] = info;
    spdlog::info("Registered module: {} (version: {}, type: {})",
                 info.name, info.version,
                 info.type == ModuleType::SERVER ? "SERVER" : "BUSINESS");
}

ModuleInfo* ModuleRegistry::getModuleInfo(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = modules_.find(name);
    if (it != modules_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<ModuleInfo> ModuleRegistry::getAllModules() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ModuleInfo> result;
    for (const auto& pair : modules_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<ModuleInfo> ModuleRegistry::getModulesByType(ModuleType type) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ModuleInfo> result;
    for (const auto& pair : modules_) {
        if (pair.second.type == type) {
            result.push_back(pair.second);
        }
    }
    return result;
}

bool ModuleRegistry::checkDependencies(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = modules_.find(moduleName);
    if (it == modules_.end()) {
        return false;
    }

    // 检查所有依赖是否都已加载
    for (const auto& dep : it->second.dependencies) {
        if (modules_.find(dep) == modules_.end()) {
            spdlog::warn("Module {} depends on {} which is not loaded",
                         moduleName, dep);
            return false;
        }
    }

    return true;
}

std::vector<std::string> ModuleRegistry::getDependents(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> dependents;
    for (const auto& pair : modules_) {
        const auto& module = pair.second;
        if (std::find(module.dependencies.begin(), module.dependencies.end(),
                     moduleName) != module.dependencies.end()) {
            dependents.push_back(pair.first);
        }
    }

    return dependents;
}

void ModuleRegistry::incrementRefCount(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        it->second.referenceCount++;
        it->second.lastUsed = std::chrono::system_clock::now();
    }
}

void ModuleRegistry::decrementRefCount(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        if (it->second.referenceCount > 0) {
            it->second.referenceCount--;
        }
    }
}

int ModuleRegistry::getRefCount(const std::string& moduleName) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        return it->second.referenceCount;
    }
    return 0;
}

bool ModuleRegistry::canReload(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = modules_.find(moduleName);
    if (it == modules_.end()) {
        return false;
    }

    // 检查引用计数
    if (it->second.referenceCount > 0) {
        return false;
    }

    // 检查依赖者
    auto dependents = getDependents(moduleName);
    if (!dependents.empty()) {
        return false;
    }

    return true;
}

void ModuleRegistry::updateLastUsed(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        it->second.lastUsed = std::chrono::system_clock::now();
    }
}

} // namespace PaperCrawler
