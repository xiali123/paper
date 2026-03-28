#pragma once

#include "ModuleRegistry.hpp"
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

namespace PaperCrawler {

/**
 * @brief 重载结果
 */
struct ReloadResult {
    bool success{false};
    std::string message;
    std::string oldVersion;
    std::string newVersion;
    std::chrono::milliseconds reloadTime{0};
};

/**
 * @brief 重载历史记录
 */
struct ReloadHistory {
    std::string moduleName;
    std::chrono::system_clock::time_point timestamp;
    std::string fromVersion;
    std::string toVersion;
    bool success{false};
};

/**
 * @brief 热重载管理器
 *
 * 负责模块的零停机热重载、文件监控、回滚等
 */
class HotReloadManager {
public:
    /**
     * @brief 获取单例
     */
    static HotReloadManager& getInstance();

    /**
     * @brief 热重载模块（零停机）
     *
     * 步骤：
     * 1. 加载新版本模块到新内存
     * 2. 迁移状态（如果支持）
     * 3. 切换路由到新版本
     * 4. 等待旧版本请求完成
     * 5. 卸载旧版本
     *
     * @param moduleName 模块名称
     * @return 重载结果
     */
    ReloadResult reloadModule(const std::string& moduleName);

    /**
     * @brief 热重载模块（指定新路径）
     */
    ReloadResult reloadModule(const std::string& moduleName, const std::string& newModulePath);

    /**
     * @brief 监控模块文件变化（自动重载）
     * @param modulesDir 模块目录
     */
    void startFileWatcher(const std::string& modulesDir);

    /**
     * @brief 停止文件监控
     */
    void stopFileWatcher();

    /**
     * @brief 回滚到之前版本
     */
    bool rollbackModule(const std::string& moduleName);

    /**
     * @brief 获取重载历史
     */
    std::vector<ReloadHistory> getReloadHistory(const std::string& moduleName) const;

    /**
     * @brief 清理历史记录
     */
    void clearHistory(const std::string& moduleName);

    /**
     * @brief 获取所有历史记录
     */
    std::map<std::string, std::vector<ReloadHistory>> getAllHistory() const;

private:
    HotReloadManager() = default;
    ~HotReloadManager();

    // 禁止拷贝
    HotReloadManager(const HotReloadManager&) = delete;
    HotReloadManager& operator=(const HotReloadManager&) = delete;

    std::map<std::string, std::vector<ReloadHistory>> history_;
    std::mutex mutex_;

    // 文件监控
    std::thread watcherThread_;
    std::atomic<bool> watching_{false};
    std::string watchDirectory_;

    /**
     * @brief 文件监控线程
     */
    void fileWatcherLoop();

    /**
     * @brief 添加历史记录
     */
    void addHistory(const ReloadHistory& history);
};

} // namespace PaperCrawler
