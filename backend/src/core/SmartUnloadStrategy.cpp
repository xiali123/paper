#include "framework/SmartUnloadStrategy.hpp"
#include "framework/PluginManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace PaperCrawler {

SmartUnloadStrategy::SmartUnloadStrategy(const UnloadPolicy& policy)
    : policy_(policy) {}

std::pair<bool, std::string> SmartUnloadStrategy::canUnload(const std::string& moduleName) {
    auto& registry = ModuleRegistry::getInstance();
    auto* moduleInfo = registry.getModuleInfo(moduleName);

    if (!moduleInfo) {
        return {false, "Module not found: " + moduleName};
    }

    // 检查引用计数
    if (!checkRefCount(moduleName)) {
        return {false, "Module has active references: " + std::to_string(moduleInfo->referenceCount.load())};
    }

    // 检查依赖者
    if (policy_.checkDependencies && !checkDependents(moduleName)) {
        auto dependents = registry.getDependents(moduleName);
        std::string depList;
        for (const auto& dep : dependents) {
            depList += dep + ", ";
        }
        return {false, "Module has active dependents: " + depList};
    }

    // 检查活跃请求
    if (!checkActiveRequests(moduleName)) {
        return {false, "Module has active requests"};
    }

    return {true, "Module can be safely unloaded"};
}

bool SmartUnloadStrategy::unload(const std::string& moduleName) {
    auto& pluginMgr = PluginManager::getInstance();

    // 根据策略执行卸载
    switch (policy_.strategy) {
        case UnloadStrategy::IMMEDIATE:
            return forceUnload(moduleName);

        case UnloadStrategy::GRACEFUL:
        case UnloadStrategy::DEPENDENCY_SAFE: {
            auto [canUnload, reason] = this->canUnload(moduleName);
            if (!canUnload) {
                std::cerr << "Cannot unload module: " << reason << std::endl;

                if (policy_.gracefulShutdown) {
                    std::cout << "Waiting for graceful shutdown..." << std::endl;
                    return waitForIdle(moduleName, std::chrono::seconds(policy_.idleTimeoutSeconds));
                }
                return false;
            }
            return pluginMgr.unloadModule(moduleName);
        }

        case UnloadStrategy::IDLE_TIMEOUT: {
            auto* moduleInfo = ModuleRegistry::getInstance().getModuleInfo(moduleName);
            if (!moduleInfo) return false;

            auto now = std::chrono::system_clock::now();
            auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(
                now - moduleInfo->lastUsed
            );

            if (idleTime.count() >= policy_.idleTimeoutSeconds) {
                return pluginMgr.unloadModule(moduleName);
            }
            return false;
        }

        default:
            return false;
    }
}

bool SmartUnloadStrategy::waitForIdle(const std::string& moduleName, std::chrono::seconds timeout) {
    auto startTime = std::chrono::steady_clock::now();
    auto& registry = ModuleRegistry::getInstance();

    while (std::chrono::steady_clock::now() - startTime < timeout) {
        auto [canUnload, reason] = this->canUnload(moduleName);
        if (canUnload) {
            return PluginManager::getInstance().unloadModule(moduleName);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cerr << "Timeout waiting for module to idle: " << moduleName << std::endl;
    return false;
}

bool SmartUnloadStrategy::forceUnload(const std::string& moduleName) {
    std::cout << "Force unloading module: " << moduleName << " (dangerous)" << std::endl;

    auto& registry = ModuleRegistry::getInstance();
    auto* moduleInfo = registry.getModuleInfo(moduleName);

    if (moduleInfo) {
        int interruptedRequests = moduleInfo->referenceCount.load();
        if (interruptedRequests > 0) {
            std::cout << "Warning: Interrupting " << interruptedRequests << " active requests" << std::endl;
        }
    }

    return PluginManager::getInstance().unloadModule(moduleName);
}

bool SmartUnloadStrategy::checkRefCount(const std::string& moduleName) {
    auto& registry = ModuleRegistry::getInstance();
    int refCount = registry.getRefCount(moduleName);
    return refCount == 0;
}

bool SmartUnloadStrategy::checkDependents(const std::string& moduleName) {
    auto& registry = ModuleRegistry::getInstance();
    auto dependents = registry.getDependents(moduleName);
    return dependents.empty();
}

bool SmartUnloadStrategy::checkActiveRequests(const std::string& moduleName) {
    // 这里需要实际的请求计数逻辑
    // 暂时返回true
    return true;
}

} // namespace PaperCrawler
