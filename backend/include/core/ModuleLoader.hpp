#pragma once

#include "ModuleMetadata.hpp"
#include "IModule.hpp"
#include "Router.hpp"
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <mutex>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace PaperCrawler {

/**
 * @brief 模块加载事件回调类型
 */
using ModuleEventCallback = std::function<void(const std::string& moduleName, const std::string& message)>;

/**
 * @brief 模块加载器
 *
 * 负责自动加载、卸载、路由注册和健康检查
 */
class ModuleLoader {
public:
    /**
     * @brief 获取单例
     */
    static ModuleLoader& getInstance();

    /**
     * @brief 初始化模块加载器
     */
    bool initialize(const std::string& modulesConfigPath = "config/modules.json");

    /**
     * @brief 从配置文件加载所有模块
     */
    bool loadAllModules();

    /**
     * @brief 加载单个模块
     * @param metadata 模块元数据
     */
    bool loadModule(const ModuleMetadata& metadata);

    /**
     * @brief 卸载模块
     */
    bool unloadModule(const std::string& moduleName);

    /**
     * @brief 重新加载模块（热重载）
     */
    bool reloadModule(const std::string& moduleName);

    /**
     * @brief 获取模块元数据
     */
    ModuleMetadata* getModuleMetadata(const std::string& moduleName);

    /**
     * @brief 获取所有模块元数据
     */
    std::vector<ModuleMetadata> getAllModulesMetadata() const;

    /**
     * @brief 获取已加载的模块实例
     */
    IModule* getModule(const std::string& moduleName);

    /**
     * @brief 启动所有模块
     */
    bool startAllModules();

    /**
     * @brief 停止所有模块
     */
    bool stopAllModules();

    /**
     * @brief 启动健康检查线程
     */
    void startHealthCheckThread(int intervalSeconds = 30);

    /**
     * @brief 停止健康检查线程
     */
    void stopHealthCheckThread();

    /**
     * @brief 手动触发健康检查
     */
    void performHealthCheck();

    /**
     * @brief 注册模块事件监听器
     */
    void registerEventListener(const std::string& eventType, ModuleEventCallback callback);

    /**
     * @brief 获取模块统计信息
     */
    std::map<std::string, ModuleMetadata> getModuleStats() const;

    /**
     * @brief 扫描目录并自动发现模块
     */
    std::vector<ModuleMetadata> scanDirectory(const std::string& directory);

    /**
     * @brief 保存模块配置到JSON文件
     */
    bool saveConfig(const std::string& path);

    /**
     * @brief 清理所有模块
     */
    void cleanup();

private:
    ModuleLoader() = default;
    ~ModuleLoader();

    // 禁止拷贝
    ModuleLoader(const ModuleLoader&) = delete;
    ModuleLoader& operator=(const ModuleLoader&) = delete;

    // 内部辅助方法

    /**
     * @brief 从DLL读取模块元数据
     */
    bool readModuleMetadata(const std::string& libraryPath, ModuleMetadata& metadata);

    /**
     * @brief 注册模块路由
     */
    bool registerModuleRoutes(IModule* module, const ModuleMetadata& metadata);

    /**
     * @brief 检查模块依赖
     */
    bool checkDependencies(const ModuleMetadata& metadata);

    /**
     * @brief 按优先级排序模块
     */
    std::vector<ModuleMetadata> sortModulesByPriority(std::vector<ModuleMetadata> modules);

    /**
     * @brief 触发事件回调
     */
    void triggerEvent(const std::string& eventType, const std::string& moduleName, const std::string& message);

    /**
     * @brief 健康检查线程函数
     */
    void healthCheckThreadFunc();

    /**
     * @brief 检查单个模块健康状态
     */
    void checkModuleHealth(const std::string& moduleName);

    /**
     * @brief 从JSON文件加载配置
     */
    bool loadConfigFromJson(const std::string& path);

    /**
     * @brief 模块名到路由前缀的自动映射
     */
    std::string inferRoutePrefix(const std::string& moduleName);

private:
    // 模块存储（使用自定义deleter以支持DLL模块的destroyFunc）
    std::map<std::string, ModuleMetadata> modulesMetadata_;
    std::map<std::string, std::unique_ptr<IModule, std::function<void(IModule*)>>> modules_;

    // 配置
    std::string configPath_;
    std::string modulesDirectory_;

    // 线程同步
    mutable std::recursive_mutex mutex_;
    std::atomic<bool> healthCheckRunning_{false};
    std::thread healthCheckThread_;
    std::condition_variable_any healthCheckCV_;

    // 健康检查配置
    int healthCheckInterval_{30};  // 秒

    // 事件监听器
    std::map<std::string, std::vector<ModuleEventCallback>> eventListeners_;
};

} // namespace PaperCrawler
