#pragma once

#include "../../core/external/nlohmann/json.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace PaperCrawler {

using json = nlohmann::json;

/**
 * @brief JSON工具类
 *
 * 提供常用的JSON操作方法
 */
class JsonUtils {
public:
    /**
     * @brief 解析JSON字符串
     */
    static std::optional<json> parse(const std::string& jsonString);

    /**
     * @brief 序列化为JSON字符串
     */
    static std::string serialize(const json& j, int indent = 0);

    /**
     * @brief 从文件读取JSON
     */
    static std::optional<json> fromFile(const std::string& filePath);

    /**
     * @brief 保存JSON到文件
     */
    static bool toFile(const std::string& filePath, const json& j, int indent = 4);

    /**
     * @brief 安全获取JSON值
     */
    template<typename T>
    static std::optional<T> getValue(const json& j, const std::string& key);

    /**
     * @brief 安全获取嵌套JSON值
     */
    template<typename T>
    static std::optional<T> getNestedValue(const json& j, const std::string& path);

    /**
     * @brief 合并两个JSON对象
     */
    static json merge(const json& a, const json& b);

    /**
     * @brief 转义JSON字符串
     */
    static std::string escape(const std::string& str);

    /**
     * @brief URL编码JSON
     */
    static std::string urlEncode(const json& j);
};

// ============================================================================
// 模板实现
// ============================================================================

template<typename T>
std::optional<T> JsonUtils::getValue(const json& j, const std::string& key) {
    try {
        if (j.contains(key)) {
            return j[key].get<T>();
        }
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

template<typename T>
std::optional<T> JsonUtils::getNestedValue(const json& j, const std::string& path) {
    try {
        std::vector<std::string> keys;
        std::stringstream ss(path);
        std::string key;

        while (std::getline(ss, key, '.')) {
            keys.push_back(key);
        }

        json current = j;
        for (const auto& k : keys) {
            if (current.contains(k)) {
                current = current[k];
            } else {
                return std::nullopt;
            }
        }

        return current.get<T>();
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace PaperCrawler
