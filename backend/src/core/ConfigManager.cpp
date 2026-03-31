#include "core/ConfigManager.hpp"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <stdlib.h>
#endif

namespace PaperCrawler {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::error("[Config] Failed to open config file: {}", path);
        return false;
    }

    spdlog::info("[Config] Loading config from: {}", path);

    try {
        // 简单的JSON解析（不依赖外部库）
        std::string line;
        std::string currentSection;

        while (std::getline(file, line)) {
            // 跳过空行和注释
            line.erase(0, line.find_last_not_of(" \t\r\n") + 1);
            if (line.empty() || line[0] == '#' || line[0] == '/') {
                continue;
            }

            // 简化解析：只处理 "key": "value" 格式
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);

                // 去除引号和空格
                key.erase(0, key.find_last_not_of(" \t") + 1);
                key.erase(0, key.find_first_not_of(" \t\""));
                key.erase(key.find_last_not_of(" \t\"") + 1);

                value.erase(0, value.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t\""));
                value.erase(value.find_last_not_of(" \t\"") + 1);

                // 处理环境变量替换
                value = processEnvironmentVariables(value);

                config_[key] = value;
            }
        }

        file.close();
        spdlog::info("[Config] Loaded {} configuration items", config_.size());
        return true;

    } catch (const std::exception& e) {
        spdlog::error("[Config] Failed to parse config file: {}", e.what());
        return false;
    }
}

void ConfigManager::loadFromEnvironment() {
    spdlog::info("[Config] Loading configuration from environment variables");

    // 数据库配置
    const char* envVars[] = {
        "DB_HOST",
        "DB_PORT",
        "DB_NAME",
        "DB_USER",
        "DB_PASSWORD",
        "DB_POOL_SIZE",
        "JWT_SECRET",
        "BCRYPT_COST",
        "SERVER_PORT",
        nullptr
    };

    for (int i = 0; envVars[i] != nullptr; i++) {
        #ifdef _WIN32
            size_t len = 0;
            char* value = nullptr;
            errno_t err = _dupenv_s(&value, &len, envVars[i]);
            if (err == 0 && value && len > 0) {
                config_[envVars[i]] = std::string(value, len);
                free(value);
            }
        #else
            char* value = std::getenv(envVars[i]);
            if (value) {
                config_[envVars[i]] = std::string(value);
            }
        #endif
    }

    // 将环境变量名转换为配置键
    std::map<std::string, std::string> envMapping = {
        {"DB_HOST", "database.host"},
        {"DB_PORT", "database.port"},
        {"DB_NAME", "database.name"},
        {"DB_USER", "database.user"},
        {"DB_PASSWORD", "database.password"},
        {"DB_POOL_SIZE", "database.pool_size"},
        {"JWT_SECRET", "security.jwt_secret"},
        {"BCRYPT_COST", "security.bcrypt_cost"},
        {"SERVER_PORT", "server.port"}
    };

    for (const auto& [envKey, configKey] : envMapping) {
        auto it = config_.find(envKey);
        if (it != config_.end()) {
            config_[configKey] = it->second;
            config_.erase(it);
        }
    }

    spdlog::info("[Config] Loaded {} environment variables", config_.size());
}

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) {
    auto it = config_.find(key);
    if (it != config_.end()) {
        return it->second;
    }
    return defaultValue;
}

int ConfigManager::getInt(const std::string& key, int defaultValue) {
    auto it = config_.find(key);
    if (it != config_.end()) {
        try {
            return std::stoi(it->second);
        } catch (const std::exception&) {
            spdlog::warn("[Config] Invalid integer value for key '{}', using default: {}", key, defaultValue);
            return defaultValue;
        }
    }
    return defaultValue;
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) {
    auto it = config_.find(key);
    if (it != config_.end()) {
        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return (value == "true" || value == "1" || value == "yes");
    }
    return defaultValue;
}

void ConfigManager::set(const std::string& key, const std::string& value) {
    config_[key] = value;
}

bool ConfigManager::has(const std::string& key) const {
    return config_.find(key) != config_.end();
}

std::string& ConfigManager::getNestedValue(const std::string& key) {
    return config_[key];
}

std::string ConfigManager::processEnvironmentVariables(const std::string& value) {
    std::string result = value;
    size_t pos = 0;

    // 处理 ${VAR} 格式的环境变量
    while ((pos = result.find("${", pos)) != std::string::npos) {
        size_t endPos = result.find("}", pos);
        if (endPos == std::string::npos) {
            break;
        }

        std::string envVar = result.substr(pos + 2, endPos - pos - 2);

        #ifdef _WIN32
            size_t len = 0;
            char* envValue = nullptr;
            _dupenv_s(&envValue, &len, envVar.c_str());
            std::string valueStr = (envValue && len > 0) ? std::string(envValue) : "";
            if (envValue) free(envValue);
        #else
            char* envValue = std::getenv(envVar.c_str());
            std::string valueStr = envValue ? std::string(envValue) : "";
        #endif

        result.replace(pos, endPos - pos + 1, valueStr);
        pos += valueStr.length();
    }

    return result;
}

} // namespace PaperCrawler
