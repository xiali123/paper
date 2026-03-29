#pragma once

#include "IModule.hpp"
#include "ModuleExports.hpp"
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>

namespace PaperCrawler {

/**
 * @brief 插件管理器
 *
 * 负责动态加载、卸载模块
 */
class PluginManager {
public:
    /**
     * @brief 获取单例
     */
    static PluginManager& getInstance();

    /**
     * @brief 初始化插件管理器
     */
    bool initialize();

    /**
     * @brief 加载模块
     * @param moduleName 模块名称
     * @param modulePath 模块路径（.so/.dll文件）
     */
    bool loadModule(const std::string& moduleName, const std::string& modulePath);

    /**
     * @brief 卸载模块
     */
    bool unloadModule(const std::string& moduleName);

    /**
     * @brief 获取模块
     */
    IModule* getModule(const std::string& moduleName);

    /**
     * @brief 获取所有模块
     */
    std::vector<IModule*> getAllModules();

    /**
     * @brief 获取所有BUSINESS类型模块
     */
    std::vector<IModule*> getBusinessModules();

    /**
     * @brief 获取所有SERVER类型模块
     */
    std::vector<IModule*> getServerModules();

    /**
     * @brief 启动所有模块
     */
    bool startAllModules();

    /**
     * @brief 停止所有模块
     */
    bool stopAllModules();

    /**
     * @brief 获取已加载的模块列表
     */
    std::vector<std::string> getLoadedModules() const;

    /**
     * @brief 扫描目录并自动加载所有模块
     * @param modulesDir 模块目录路径
     * @return 成功返回true，失败返回false
     */
    bool scanAndLoadModules(const std::string& modulesDir);

private:
    PluginManager() = default;
    ~PluginManager();

    // 禁止拷贝
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    std::map<std::string, std::unique_ptr<IModule>> modules_;
    std::map<std::string, ModuleHandle> handles_;
    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
