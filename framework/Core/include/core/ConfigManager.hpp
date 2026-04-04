#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <functional>
#include <mutex>
#include <vector>
#include <chrono>
#include <memory>
#include <atomic>

namespace PaperCrawler {
namespace Core {

/**
 * @brief 配置文件格式枚举
 */
enum class ConfigFormat {
    JSON,   ///< JSON格式
    YAML,   ///< YAML格式
    TOML,   ///< TOML格式
    ENV,    ///< 环境变量格式
    AUTO    ///< 自动检测格式
};

/**
 * @brief 配置验证结果
 */
struct ValidationResult {
    bool valid{true};              ///< 是否有效
    std::vector<std::string> errors; ///< 错误列表
    std::vector<std::string> warnings; ///< 警告列表

    /**
     * @brief 添加错误
     */
    void addError(const std::string& error) {
        errors.push_back(error);
        valid = false;
    }

    /**
     * @brief 添加警告
     */
    void addWarning(const std::string& warning) {
        warnings.push_back(warning);
    }

    /**
     * @brief 是否有错误或警告
     */
    bool hasIssues() const {
        return !errors.empty() || !warnings.empty();
    }
};

/**
 * @brief 配置验证器函数类型
 *
 * @param config 配置数据
 * @return 验证结果
 */
using ConfigValidator = std::function<ValidationResult(const std::map<std::string, std::string>&)>;

/**
 * @brief 配置变更回调函数类型
 *
 * @param key 配置键
 * @param oldValue 旧值
 * @param newValue 新值
 */
using ConfigChangeCallback = std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)>;

/**
 * @brief ConfigManager - 通用配置管理器
 *
 * 提供了完整的配置管理功能，包括：
 * - 多种格式支持（JSON/YAML/TOML/ENV）
 * - 配置验证
 * - 热重载
 * - 配置变更监听
 * - 环境变量替换
 * - 类型安全访问
 *
 * @section features 核心特性
 * - @ref format_support "多格式支持"
 * - @ref validation "配置验证"
 * - @ref hot_reload "热重载"
 * - @ref change_listener "变更监听"
 *
 * @section example_usage 示例用法
 * @code
 * auto& config = ConfigManager::getInstance();
 *
 * // 加载配置
 * config.loadFromFile("config.json", ConfigFormat::JSON);
 * config.loadFromEnvironment();
 *
 * // 注册验证器
 * config.registerValidator([](auto& cfg) {
 *     ValidationResult result;
 *     if (cfg.find("database.host") == cfg.end()) {
 *         result.addError("database.host is required");
 *     }
 *     return result;
 * });
 *
 * // 验证配置
 * auto result = config.validate();
 *
 * // 监听配置变更
 * config.watch("database.password", [](auto key, auto oldVal, auto newVal) {
 *     // 配置变更处理
 * });
 *
 * // 获取配置
 * std::string dbHost = config.getString("database.host");
 * int dbPort = config.getInt("database.port", 3306);
 * bool enableCache = config.getBool("cache.enabled");
 * @endcode
 *
 * @threadsafe 所有公共方法都是线程安全的
 */
class ConfigManager {
public:
    /**
     * @brief 获取单例实例
     *
     * @return ConfigManager引用
     */
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    // ========================================================================
    // 配置加载
    // ========================================================================

    /**
     * @brief 从文件加载配置
     *
     * @param path 配置文件路径
     * @param format 配置格式（AUTO表示自动检测）
     * @return 是否成功
     *
     * @section example 示例
     * @code
     * // 自动检测格式
     * config.loadFromFile("config.json");
     *
     * // 指定格式
     * config.loadFromFile("config.yaml", ConfigFormat::YAML);
     * @endcode
     *
     * @threadsafe 线程安全
     */
    bool loadFromFile(const std::string& path, ConfigFormat format = ConfigFormat::AUTO);

    /**
     * @brief 从环境变量加载配置
     *
     * 环境变量格式：
     * - 支持嵌套键：DATABASE_HOST=localhost
     * - 支持环境变量替换：${VAR}
     *
     * @section example 示例
     * @code
     * // 加载所有环境变量
     * config.loadFromEnvironment();
     *
     * // 加载特定前缀的环境变量
     * config.loadFromEnvironment("APP_");
     * @endcode
     *
     * @threadsafe 线程安全
     */
    void loadFromEnvironment(const std::string& prefix = "");

    /**
     * @brief 从字符串加载配置
     *
     * @param content 配置内容
     * @param format 配置格式
     * @return 是否成功
     *
     * @threadsafe 线程安全
     */
    bool loadFromString(const std::string& content, ConfigFormat format);

    /**
     * @brief 重新加载配置文件（热重载）
     *
     * @return 是否成功
     *
     * @note 会触发配置变更回调
     * @threadsafe 线程安全
     */
    bool reload();

    // ========================================================================
    // 配置访问
    // ========================================================================

    /**
     * @brief 获取字符串配置
     *
     * @param key 配置键（支持点号分隔的嵌套键，如 "database.host"）
     * @param defaultValue 默认值
     * @return 配置值
     *
     * @threadsafe 线程安全
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "");

    /**
     * @brief 获取整数配置
     *
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值
     *
     * @threadsafe 线程安全
     */
    int getInt(const std::string& key, int defaultValue = 0);

    /**
     * @brief 获取浮点数配置
     *
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值
     *
     * @threadsafe 线程安全
     */
    double getDouble(const std::string& key, double defaultValue = 0.0);

    /**
     * @brief 获取布尔配置
     *
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值
     *
     * @section example 示例
     * @code
     * bool enabled = config.getBool("feature.enabled");
     * // 支持的值: true, false, 1, 0, yes, no
     * @endcode
     *
     * @threadsafe 线程安全
     */
    bool getBool(const std::string& key, bool defaultValue = false);

    // ========================================================================
    // 配置修改
    // ========================================================================

    /**
     * @brief 设置配置值
     *
     * @param key 配置键
     * @param value 配置值
     *
     * @note 会触发配置变更回调
     * @threadsafe 线程安全
     */
    void set(const std::string& key, const std::string& value);

    /**
     * @brief 设置配置值（整数）
     *
     * @param key 配置键
     * @param value 配置值
     *
     * @threadsafe 线程安全
     */
    void setInt(const std::string& key, int value);

    /**
     * @brief 设置配置值（浮点数）
     *
     * @param key 配置键
     * @param value 配置值
     *
     * @threadsafe 线程安全
     */
    void setDouble(const std::string& key, double value);

    /**
     * @brief 设置配置值（布尔）
     *
     * @param key 配置键
     * @param value 配置值
     *
     * @threadsafe 线程安全
     */
    void setBool(const std::string& key, bool value);

    // ========================================================================
    // 配置验证
    // ========================================================================

    /**
     * @brief 注册配置验证器
     *
     * @param validator 验证器函数
     *
     * @section example 示例
     * @code
     * config.registerValidator([](auto& cfg) {
     *     ValidationResult result;
     *     if (cfg.find("database.host") == cfg.end()) {
     *         result.addError("database.host is required");
     *     }
     *     if (cfg.find("database.port") != cfg.end()) {
     *         int port = std::stoi(cfg.at("database.port"));
     *         if (port < 1 || port > 65535) {
     *             result.addError("database.port must be 1-65535");
     *         }
     *     }
     *     return result;
     * });
     * @endcode
     *
     * @threadsafe 线程安全
     */
    void registerValidator(ConfigValidator validator);

    /**
     * @brief 验证配置
     *
     * @return 验证结果
     *
     * @threadsafe 线程安全
     */
    ValidationResult validate();

    // ========================================================================
    // 配置监听
    // ========================================================================

    /**
     * @brief 监听配置变更
     *
     * @param key 配置键（支持通配符*）
     * @param callback 回调函数
     * @return 监听器ID
     *
     * @section example 示例
     * @code
     * // 监听特定键
     * config.watch("database.password", [](auto key, auto oldVal, auto newVal) {
     *     // 处理密码变更
     * });
     *
     * // 监听所有数据库配置
     * config.watch("database.*", [](auto key, auto oldVal, auto newVal) {
     *     // 处理数据库配置变更
     * });
     * @endcode
     *
     * @threadsafe 线程安全
     */
    std::string watch(const std::string& key, ConfigChangeCallback callback);

    /**
     * @brief 取消监听
     *
     * @param watchId 监听器ID
     *
     * @threadsafe 线程安全
     */
    void unwatch(const std::string& watchId);

    // ========================================================================
    // 查询方法
    // ========================================================================

    /**
     * @brief 检查配置是否存在
     *
     * @param key 配置键
     * @return 是否存在
     *
     * @threadsafe 线程安全
     */
    bool has(const std::string& key) const;

    /**
     * @brief 获取所有配置
     *
     * @return 配置映射
     *
     * @threadsafe 线程安全
     */
    const std::map<std::string, std::string>& getAll() const;

    /**
     * @brief 清空所有配置
     *
     * @threadsafe 线程安全
     */
    void clear();

    /**
     * @brief 保存配置到文件
     *
     * @param path 文件路径
     * @param format 配置格式
     * @return 是否成功
     *
     * @threadsafe 线程安全
     */
    bool saveToFile(const std::string& path, ConfigFormat format = ConfigFormat::JSON);

private:
    /**
     * @brief 构造函数（私有）
     */
    ConfigManager() = default;

    /**
     * @brief 析构函数（私有）
     */
    ~ConfigManager() = default;

    // 禁止拷贝和移动
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    /**
     * @brief 解析嵌套的配置键
     *
     * @param key 配置键（如 "database.host"）
     * @return 配置值的引用
     */
    std::string& getNestedValue(const std::string& key);

    /**
     * @brief 处理环境变量替换
     *
     * @param value 原始值
     * @return 处理后的值
     */
    std::string processEnvironmentVariables(const std::string& value);

    /**
     * @brief 检测配置文件格式
     *
     * @param path 文件路径
     * @return 配置格式
     */
    ConfigFormat detectFormat(const std::string& path);

    /**
     * @brief 解析JSON配置
     *
     * @param content JSON内容
     * @return 是否成功
     */
    bool parseJSON(const std::string& content);

    /**
     * @brief 解析YAML配置
     *
     * @param content YAML内容
     * @return 是否成功
     */
    bool parseYAML(const std::string& content);

    /**
     * @brief 解析TOML配置
     *
     * @param content TOML内容
     * @return 是否成功
     */
    bool parseTOML(const std::string& content);

    /**
     * @brief 生成JSON配置
     *
     * @param content 输出JSON内容
     * @return 是否成功
     */
    bool generateJSON(std::string& content);

    /**
     * @brief 触发配置变更回调
     *
     * @param key 配置键
     * @param oldValue 旧值
     * @param newValue 新值
     */
    void notifyWatchers(const std::string& key, const std::string& oldValue, const std::string& newValue);

    /**
     * @brief 生成监听器ID
     *
     * @return 唯一的监听器ID
     */
    std::string generateWatchId();

    // 成员变量
    std::map<std::string, std::string> config_;           ///< 配置数据
    std::string currentFilePath_;                         ///< 当前配置文件路径
    ConfigFormat currentFormat_;                          ///< 当前配置格式
    mutable std::mutex mutex_;                            ///< 互斥锁

    std::vector<ConfigValidator> validators_;             ///< 配置验证器列表

    struct Watcher {
        std::string watchId;                ///< 监听器ID
        std::string keyPattern;             ///< 键模式（支持通配符）
        ConfigChangeCallback callback;     ///< 回调函数
    };
    std::vector<Watcher> watchers_;                     ///< 监听器列表
};

/**
 * @brief 全局配置便捷访问
 *
 * @section example 示例用法
 * @code
 * // 加载配置
 * Config::loadFromFile("config.json");
 *
 * // 获取配置
 * auto dbHost = Config::getString("database.host");
 * auto dbPort = Config::getInt("database.port", 3306);
 * @endcode
 */
class Config {
public:
    static bool loadFromFile(const std::string& path, ConfigFormat format = ConfigFormat::AUTO) {
        return ConfigManager::getInstance().loadFromFile(path, format);
    }

    static void loadFromEnvironment(const std::string& prefix = "") {
        ConfigManager::getInstance().loadFromEnvironment(prefix);
    }

    static std::string getString(const std::string& key, const std::string& defaultValue = "") {
        return ConfigManager::getInstance().getString(key, defaultValue);
    }

    static int getInt(const std::string& key, int defaultValue = 0) {
        return ConfigManager::getInstance().getInt(key, defaultValue);
    }

    static double getDouble(const std::string& key, double defaultValue = 0.0) {
        return ConfigManager::getInstance().getDouble(key, defaultValue);
    }

    static bool getBool(const std::string& key, bool defaultValue = false) {
        return ConfigManager::getInstance().getBool(key, defaultValue);
    }

    static void set(const std::string& key, const std::string& value) {
        ConfigManager::getInstance().set(key, value);
    }

    static bool has(const std::string& key) {
        return ConfigManager::getInstance().has(key);
    }
};

} // namespace Core
} // namespace PaperCrawler
