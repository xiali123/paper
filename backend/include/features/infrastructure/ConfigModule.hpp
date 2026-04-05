#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <any>
#include <map>
#include <vector>
#include <functional>
#include <mutex>
#include <optional>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief 配置变更回调
 */
using ConfigChangeCallback = std::function<void(const std::string& key, const std::any& oldValue, const std::any& newValue)>;

/**
 * @brief 配置验证结果
 */
struct ValidationResult {
    bool valid{true};
    std::string errorMessage;
};

/**
 * @brief 配置验证器
 */
using ConfigValidator = std::function<ValidationResult(const std::string& key, const std::any& value)>;

/**
 * @brief 配置管理模块
 *
 * 功能：
 * 1. 热加载配置（无需重启）
 * 2. 配置验证（JSON Schema）
 * 3. 配置监听（文件变化自动重载）
 * 4. 配置变更通知
 * 5. 多环境配置（dev/test/prod）
 *
 * 特性：
 * - 类型安全：模板获取
 * - 原子更新：无锁读取
 * - 回滚支持：变更失败自动回滚
 * - 配置继承：基础配置 + 环境配置
 */
class ConfigModule : public IModule {
public:
    ConfigModule();
    ~ConfigModule() override;

    std::string getName() const override { return "Config"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Configuration management with hot-reload and validation";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 获取配置值（类型安全）
     */
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const {
        auto value = getAny(key);
        if (value.has_value()) {
            try {
                return std::any_cast<T>(value.value());
            } catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    /**
     * @brief 设置配置值
     */
    bool set(const std::string& key, const std::any& value);

    /**
     * @brief 删除配置项
     */
    bool remove(const std::string& key);

    /**
     * @brief 检查配置项是否存在
     */
    bool has(const std::string& key) const;

    /**
     * @brief 监听配置变更
     */
    size_t watch(const std::string& key, ConfigChangeCallback callback);

    /**
     * @brief 取消监听
     */
    bool unwatch(const std::string& key, size_t watcherId);

    /**
     * @brief 重新加载配置文件
     */
    bool reload();

    /**
     * @brief 保存配置到文件
     */
    bool save();

    /**
     * @brief 从文件加载配置
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * @brief 保存到文件
     */
    bool saveToFile(const std::string& filepath);

    /**
     * @brief 添加配置验证器
     */
    void addValidator(const std::string& key, ConfigValidator validator);

    /**
     * @brief 移除配置验证器
     */
    void removeValidator(const std::string& key);

    /**
     * @brief 验证所有配置
     */
    ValidationResult validateAll();

    /**
     * @brief 获取所有配置（JSON格式）
     */
    std::string exportJSON() const;

    /**
     * @brief 从JSON导入配置
     */
    bool importJSON(const std::string& json);

    /**
     * @brief 设置环境
     */
    void setEnvironment(const std::string& env);

    /**
     * @brief 获取环境
     */
    std::string getEnvironment() const;

    /**
     * @brief 配置统计
     */
    struct ConfigStats {
        size_t totalKeys;
        size_t watcherCount;
        std::chrono::system_clock::time_point lastReload;
        size_t reloadCount;
        std::string currentEnvironment;
    };
    ConfigStats getStats() const;

private:
    std::optional<std::any> getAny(const std::string& key) const;
    void notifyWatchers(const std::string& key, const std::any& oldValue, const std::any& newValue);

    class Impl;
    std::unique_ptr<Impl> impl_;

    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
