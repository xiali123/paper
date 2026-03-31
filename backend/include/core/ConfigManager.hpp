/**
 * @file ConfigManager.hpp
 * @brief 配置管理器 - 支持环境变量和配置文件
 */

#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>

namespace PaperCrawler {

/**
 * @brief 配置管理器单例
 *
 * 支持从配置文件和环境变量加载配置
 * 环境变量优先级高于配置文件
 *
 * @code
 * auto& config = ConfigManager::getInstance();
 * config.loadFromFile("./config/config.json");
 * config.loadFromEnvironment();
 *
 * std::string dbPassword = config.getString("database.password");
 * int dbPort = config.getInt("database.port", 3306);
 * @endcode
 */
class ConfigManager {
public:
    /**
     * @brief 获取单例实例
     */
    static ConfigManager& getInstance();

    /**
     * @brief 从JSON文件加载配置
     * @param path 配置文件路径
     * @return 是否成功
     */
    bool loadFromFile(const std::string& path);

    /**
     * @brief 从环境变量加载配置
     *
     * 环境变量格式：支持 ${VAR} 替换
     * 例如：DB_PASSWORD=secret123
     */
    void loadFromEnvironment();

    /**
     * @brief 获取字符串配置
     * @param key 配置键（支持点号分隔的嵌套键，如 "database.host"）
     * @param defaultValue 默认值
     * @return 配置值
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "");

    /**
     * @brief 获取整数配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值
     */
    int getInt(const std::string& key, int defaultValue = 0);

    /**
     * @brief 获取布尔配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值
     */
    bool getBool(const std::string& key, bool defaultValue = false);

    /**
     * @brief 设置配置值
     * @param key 配置键
     * @param value 配置值
     */
    void set(const std::string& key, const std::string& value);

    /**
     * @brief 检查配置是否存在
     * @param key 配置键
     * @return 是否存在
     */
    bool has(const std::string& key) const;

    /**
     * @brief 获取所有配置
     */
    const std::map<std::string, std::string>& getAll() const { return config_; }

    /**
     * @brief 清空所有配置
     */
    void clear() { config_.clear(); }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    // 禁止拷贝
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    /**
     * @brief 解析嵌套的配置键（如 "database.host"）
     * @param key 配置键
     * @return 配置值的引用
     */
    std::string& getNestedValue(const std::string& key);

    /**
     * @brief 处理环境变量替换
     * @param value 原始值
     * @return 处理后的值
     */
    std::string processEnvironmentVariables(const std::string& value);

    std::map<std::string, std::string> config_;
};

} // namespace PaperCrawler
