#include "framework/ModuleRegistry.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace PaperCrawler {

using json = nlohmann::json;

ModuleRegistry& ModuleRegistry::getInstance() {
    static ModuleRegistry instance;
    return instance;
}

bool ModuleRegistry::loadFromConfig(const std::string& configPath) {
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        std::cerr << "Failed to open config file: " << configPath << std::endl;
        return false;
    }

    try {
        json config;
        configFile >> config;

        // 解析系统模块
        if (config.contains("modules") && config["modules"].contains("system")) {
            for (const auto& item : config["modules"]["system"].items()) {
                const auto& moduleConfig = item.value();

                ModuleInfo info;
                info.name = moduleConfig["name"];
                info.libraryPath = moduleConfig["library"];
                info.version = moduleConfig.contains("version") ? moduleConfig["version"] : "1.0.0";
                info.type = ModuleType::SERVER;
                info.routePrefix = moduleConfig.contains("route_prefix") ? moduleConfig["route_prefix"] : "";

                // 解析依赖
                if (moduleConfig.contains("dependencies")) {
                    for (const auto& dep : moduleConfig["dependencies"]) {
                        info.dependencies.push_back(dep.get<std::string>());
                    }
                }

                // 解析端点
                if (moduleConfig.contains("endpoints")) {
                    for (const auto& endpoint : moduleConfig["endpoints"].items()) {
                        info.endpoints[endpoint.key()] = endpoint.value();
                    }
                }

                info.state = ModuleState::UNLOADED;
                info.loadedAt = std::chrono::system_clock::now();
                info.lastUsed = std::chrono::system_clock::now();

                registerModule(info);
            }
        }

        // 解析业务模块
        if (config.contains("modules") && config["modules"].contains("business")) {
            for (const auto& item : config["modules"]["business"].items()) {
                const auto& moduleConfig = item.value();

                ModuleInfo info;
                info.name = moduleConfig["name"];
                info.libraryPath = moduleConfig["library"];
                info.version = moduleConfig.contains("version") ? moduleConfig["version"] : "1.0.0";
                info.type = ModuleType::BUSINESS;
                info.routePrefix = moduleConfig.contains("route_prefix") ? moduleConfig["route_prefix"] : "";

                // 解析依赖
                if (moduleConfig.contains("dependencies")) {
                    for (const auto& dep : moduleConfig["dependencies"]) {
                        info.dependencies.push_back(dep.get<std::string>());
                    }
                }

                // 解析端点
                if (moduleConfig.contains("endpoints")) {
                    for (const auto& endpoint : moduleConfig["endpoints"].items()) {
                        info.endpoints[endpoint.key()] = endpoint.value();
                    }
                }

                info.state = ModuleState::UNLOADED;
                info.loadedAt = std::chrono::system_clock::now();
                info.lastUsed = std::chrono::system_clock::now();

                registerModule(info);
            }
        }

        std::cout << "Loaded " << modules_.size() << " modules from config" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing config file: " << e.what() << std::endl;
        return false;
    }
}

void ModuleRegistry::registerModule(const ModuleInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    modules_[info.name] = info;
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

    const ModuleInfo& info = it->second;
    for (const auto& dep : info.dependencies) {
        auto depIt = modules_.find(dep);
        if (depIt == modules_.end()) {
            std::cerr << "Missing dependency: " << dep << " for module " << moduleName << std::endl;
            return false;
        }
        if (depIt->second.state != ModuleState::STARTED) {
            std::cerr << "Dependency not started: " << dep << " for module " << moduleName << std::endl;
            return false;
        }
    }

    return true;
}

std::vector<std::string> ModuleRegistry::getDependents(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;

    for (const auto& pair : modules_) {
        const ModuleInfo& info = pair.second;
        for (const auto& dep : info.dependencies) {
            if (dep == moduleName && info.state == ModuleState::STARTED) {
                result.push_back(info.name);
            }
        }
    }

    return result;
}

void ModuleRegistry::incrementRefCount(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        it->second.referenceCount++;
        updateLastUsed(moduleName);
    }
}

void ModuleRegistry::decrementRefCount(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        it->second.referenceCount--;
    }
}

int ModuleRegistry::getRefCount(const std::string& moduleName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        return it->second.referenceCount.load();
    }
    return 0;
}

bool ModuleRegistry::canReload(const std::string& moduleName) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = modules_.find(moduleName);
    if (it == modules_.end()) {
        return false;
    }

    const ModuleInfo& info = it->second;
    // 业务模块可以热重载，系统模块通常不行
    return info.type == ModuleType::BUSINESS;
}

void ModuleRegistry::updateLastUsed(const std::string& moduleName) {
    auto it = modules_.find(moduleName);
    if (it != modules_.end()) {
        it->second.lastUsed = std::chrono::system_clock::now();
    }
}

std::vector<std::string> ModuleRegistry::getLoadedModules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;

    for (const auto& pair : modules_) {
        if (pair.second.state == ModuleState::STARTED) {
            result.push_back(pair.first);
        }
    }

    return result;
}

} // namespace PaperCrawler
