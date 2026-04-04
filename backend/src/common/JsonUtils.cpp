#include "common/JsonUtils.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace PaperCrawler {

std::optional<json> JsonUtils::parse(const std::string& jsonString) {
    try {
        return json::parse(jsonString);
    } catch (const json::parse_error& e) {
        return std::nullopt;
    }
}

std::string JsonUtils::serialize(const json& j, int indent) {
    try {
        if (indent > 0) {
            return j.dump(indent);
        } else {
            return j.dump();
        }
    } catch (const json::type_error& e) {
        return "{}";
    }
}

std::optional<json> JsonUtils::fromFile(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return std::nullopt;
        }

        json j;
        file >> j;
        return j;
    } catch (...) {
        return std::nullopt;
    }
}

bool JsonUtils::toFile(const std::string& filePath, const json& j, int indent) {
    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            return false;
        }

        file << j.dump(indent);
        return true;
    } catch (...) {
        return false;
    }
}

json JsonUtils::merge(const json& a, const json& b) {
    json result = a;

    if (!b.is_object()) {
        return result;
    }

    for (auto& [key, value] : b.items()) {
        if (result.contains(key) && result[key].is_object() && value.is_object()) {
            result[key] = merge(result[key], value);
        } else {
            result[key] = value;
        }
    }

    return result;
}

std::string JsonUtils::escape(const std::string& str) {
    json j = str;
    return j.dump();
}

std::string JsonUtils::urlEncode(const json& j) {
    // 简化实现，生产环境应使用curl_escape
    std::string jsonString = j.dump();
    // TODO: 实现URL编码
    return jsonString;
}

} // namespace PaperCrawler
