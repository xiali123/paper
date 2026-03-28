#pragma once

#include "ModuleRegistry.hpp"
#include <string>
#include <chrono>
#include <functional>

namespace PaperCrawler {

/**
 * @brief 卸载策略
 */
enum class UnloadStrategy {
    IMMEDIATE,      // 立即卸载（危险）
    GRACEFUL,       // 优雅卸载（等待请求完成）
    IDLE_TIMEOUT,   // 空闲超时卸载
    DEPENDENCY_SAFE // 依赖安全卸载（等待依赖者释放）
};

/**
 * @brief 卸载策略配置
 */
struct UnloadPolicy {
    UnloadStrategy strategy{UnloadStrategy::GRACEFUL};
    int idleTimeoutSeconds{300};
    int forceUnloadAfterSeconds{60};
    bool checkDependencies{true};
    bool gracefulShutdown{true};
};

/**
 * @brief 智能卸载策略
 *
 * 根据引用计数、依赖关系、活跃请求等智能决定是否可以卸载模块
 */
class SmartUnloadStrategy {
public:
    explicit SmartUnloadStrategy(const UnloadPolicy& policy);

    /**
     * @brief 检查模块是否可以安全卸载
     * @return {canUnload, reason}
     */
    std::pair<bool, std::string> canUnload(const std::string& moduleName);

    /**
     * @brief 执行智能卸载
     * @return 成功返回true
     */
    bool unload(const std::string& moduleName);

    /**
     * @brief 等待模块空闲
     * @param timeout 超时时间
     * @return 成功返回true
     */
    bool waitForIdle(const std::string& moduleName, std::chrono::seconds timeout);

    /**
     * @brief 强制卸载（危险）
     */
    bool forceUnload(const std::string& moduleName);

    /**
     * @brief 获取策略配置
     */
    const UnloadPolicy& getPolicy() const { return policy_; }

    /**
     * @brief 设置策略配置
     */
    void setPolicy(const UnloadPolicy& policy) { policy_ = policy; }

private:
    UnloadPolicy policy_;

    /**
     * @brief 检查引用计数
     */
    bool checkRefCount(const std::string& moduleName);

    /**
     * @brief 检查依赖者
     */
    bool checkDependents(const std::string& moduleName);

    /**
     * @brief 检查活跃请求
     */
    bool checkActiveRequests(const std::string& moduleName);
};

} // namespace PaperCrawler
