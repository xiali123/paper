#pragma once

#include "IModule.hpp"
#include "ModuleExports.hpp"
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief 模块信息
 */
struct ModuleInfo {
    std::string name;
    std::string libraryPath;
    std::string version;
    ModuleType type;
    std::string routePrefix;
    std::vector<std::string> dependencies;
    std::map<std::string, std::string> endpoints;

    // 运行时状态
    ModuleState state{ModuleState::UNLOADED};
    ModuleHandle handle{nullptr};
    IModule* instance{nullptr};

    // 引用计数和卸载控制
    std::atomic<int> referenceCount{0};
    std::chrono::system_clock::time_point lastUsed;
    std::chrono::system_clock::time_point loadedAt;
    int reloadCount{0};

    // 自定义拷贝操作（因为atomic不可拷贝）
    ModuleInfo() = default;
    ModuleInfo(const ModuleInfo& other)
        : name(other.name),
          libraryPath(other.libraryPath),
          version(other.version),
          type(other.type),
          routePrefix(other.routePrefix),
          dependencies(other.dependencies),
          endpoints(other.endpoints),
          state(other.state),
          handle(other.handle),
          instance(other.instance),
          referenceCount(other.referenceCount.load()),
          lastUsed(other.lastUsed),
          loadedAt(other.loadedAt),
          reloadCount(other.reloadCount) {}

    ModuleInfo& operator=(const ModuleInfo& other) {
        if (this != &other) {
            name = other.name;
            libraryPath = other.libraryPath;
            version = other.version;
            type = other.type;
            routePrefix = other.routePrefix;
            dependencies = other.dependencies;
            endpoints = other.endpoints;
            state = other.state;
            handle = other.handle;
            instance = other.instance;
            referenceCount.store(other.referenceCount.load());
            lastUsed = other.lastUsed;
            loadedAt = other.loadedAt;
            reloadCount = other.reloadCount;
        }
        return *this;
    }
};

/**
 * @brief 模块注册表
 *
 * 管理所有模块的元数据、依赖关系、状态等
 */
class ModuleRegistry {
public:
    /**
     * @brief 获取单例
     */
    static ModuleRegistry& getInstance();

    /**
     * @brief 从配置文件加载模块元数据
     * @param configPath 配置文件路径（如：./config/modules.json）
     * @return 成功返回true
     */
    bool loadFromConfig(const std::string& configPath);

    /**
     * @brief 注册模块信息
     */
    void registerModule(const ModuleInfo& info);

    /**
     * @brief 获取模块信息
     */
    ModuleInfo* getModuleInfo(const std::string& name);

    /**
     * @brief 获取所有模块
     */
    std::vector<ModuleInfo> getAllModules() const;

    /**
     * @brief 按类型获取模块
     */
    std::vector<ModuleInfo> getModulesByType(ModuleType type) const;

    /**
     * @brief 检查依赖关系
     * @return 依赖满足返回true
     */
    bool checkDependencies(const std::string& moduleName);

    /**
     * @brief 获取依赖该模块的其他模块
     */
    std::vector<std::string> getDependents(const std::string& moduleName);

    /**
     * @brief 增加引用计数
     */
    void incrementRefCount(const std::string& moduleName);

    /**
     * @brief 减少引用计数
     */
    void decrementRefCount(const std::string& moduleName);

    /**
     * @brief 获取引用计数
     */
    int getRefCount(const std::string& moduleName) const;

    /**
     * @brief 检查是否可以重载
     */
    bool canReload(const std::string& moduleName);

    /**
     * @brief 更新最后使用时间
     */
    void updateLastUsed(const std::string& moduleName);

    /**
     * @brief 获取已加载的模块列表
     */
    std::vector<std::string> getLoadedModules() const;

private:
    ModuleRegistry() = default;
    ~ModuleRegistry() = default;

    // 禁止拷贝
    ModuleRegistry(const ModuleRegistry&) = delete;
    ModuleRegistry& operator=(const ModuleRegistry&) = delete;

    std::map<std::string, ModuleInfo> modules_;
    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
