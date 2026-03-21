#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <mutex>
#include "core/Exception.hpp"

namespace PaperCrawler {

/**
 * @brief Configuration manager class
 *
 * Implements singleton pattern for managing application configuration
 * Thread-safe configuration access
 */
class Config {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to config instance
     */
    static Config& getInstance();

    /**
     * @brief Load configuration from JSON file
     * @param path Path to configuration file
     * @throws ConfigException if file cannot be loaded or parsed
     */
    void load(const std::string& path);

    /**
     * @brief Get string value by key (supports nested keys with dot notation)
     * @param key Configuration key (e.g., "database.host")
     * @param defaultValue Default value if key not found
     * @return String value
     */
    std::string get(const std::string& key, const std::string& defaultValue = "");

    /**
     * @brief Get integer value by key
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Integer value
     */
    int getInt(const std::string& key, int defaultValue = 0);

    /**
     * @brief Get boolean value by key
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Boolean value
     */
    bool getBool(const std::string& key, bool defaultValue = false);

    /**
     * @brief Get double value by key
     * @param key Configuration key
     * @param defaultValue Default value if key not found
     * @return Double value
     */
    double getDouble(const std::string& key, double defaultValue = 0.0);

    /**
     * @brief Check if key exists
     * @param key Configuration key
     * @return True if key exists
     */
    bool has(const std::string& key) const;

    /**
     * @brief Set configuration value
     * @param key Configuration key
     * @param value Value to set
     */
    void set(const std::string& key, const std::string& value);

    /**
     * @brief Save configuration to file
     * @param path Path to save configuration
     */
    void save(const std::string& path);

private:
    Config() = default;
    ~Config() = default;

    // Prevent copying
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    /**
     * @brief Get JSON object by key (supports dot notation)
     * @param key Configuration key
     * @return Pointer to JSON object, or nullptr if not found
     */
    nlohmann::json* getByPath(const std::string& key);

    nlohmann::json config_;
    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
