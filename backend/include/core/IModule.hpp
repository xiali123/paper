#pragma once

#include "ModuleExports.hpp"
#include <string>
#include <map>
#include <memory>

namespace PaperCrawler {

/**
 * @brief 模块接口
 *
 * 所有模块必须实现此接口
 */
class IModule {
public:
    virtual ~IModule() = default;

    /**
     * @brief 获取模块名称
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 获取模块版本
     */
    virtual std::string getVersion() const = 0;

    /**
     * @brief 获取模块描述
     */
    virtual std::string getDescription() const = 0;

    /**
     * @brief 获取模块类型
     * @return ModuleType::SERVER 或 ModuleType::BUSINESS
     */
    virtual ModuleType getModuleType() const {
        return ModuleType::SERVER;
    }

    /**
     * @brief 获取路由前缀（仅BUSINESS模块有效）
     * @return 路由前缀，如 "/api/papers"
     */
    virtual std::string getRoutePrefix() const {
        return "";
    }

    /**
     * @brief 初始化模块
     */
    virtual bool initialize() = 0;

    /**
     * @brief 启动模块
     */
    virtual bool start() = 0;

    /**
     * @brief 停止模块
     */
    virtual bool stop() = 0;

    /**
     * @brief 清理模块资源
     */
    virtual void cleanup() = 0;

    /**
     * @brief 获取模块状态
     */
    virtual ModuleState getState() const {
        return state_;
    }

protected:
    ModuleState state_ = ModuleState::UNLOADED;
};

} // namespace PaperCrawler
