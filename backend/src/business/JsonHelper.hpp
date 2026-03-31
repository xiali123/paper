/**
 * @file JsonHelper.hpp
 * @brief 简单的JSON构建辅助函数（替代ResponseHandlerModule）
 */

#pragma once

#include <string>
#include <map>
#include <sstream>
#include <iomanip>

namespace PaperCrawler {

/**
 * @brief 简单的JSON构建辅助类
 */
class JsonHelper {
public:
    /**
     * @brief 构建JSON响应
     * @param data 数据键值对
     * @param statusCode HTTP状态码（默认200）
     * @return JSON字符串
     */
    static std::string buildJsonResponse(
        const std::map<std::string, std::string>& data,
        int statusCode = 200
    ) {
        std::ostringstream json;
        json << "{";

        bool first = true;
        for (const auto& [key, value] : data) {
            if (!first) json << ",";
            json << "\n  \"" << key << "\": \"" << escapeJson(value) << "\"";
            first = false;
        }

        json << "\n}";
        return json.str();
    }

    /**
     * @brief 构建带数组的JSON响应（用于论文列表）
     */
    static std::string buildPapersJsonResponse(
        const std::string& papersJson,
        int count,
        int page,
        int limit
    ) {
        std::ostringstream json;
        json << "{\n";
        json << "  \"papers\": " << papersJson << ",\n";
        json << "  \"total\": " << count << ",\n";
        json << "  \"page\": " << page << ",\n";
        json << "  \"limit\": " << limit << "\n";
        json << "}";
        return json.str();
    }

    /**
     * @brief 构建统计信息JSON响应
     */
    static std::string buildStatsJsonResponse(
        const std::map<std::string, std::string>& stats
    ) {
        std::ostringstream json;
        json << "{";

        bool first = true;
        for (const auto& [key, value] : stats) {
            if (!first) json << ",";
            json << "\n  \"" << key << "\": " << value; // 数字值不需要引号
            first = false;
        }

        json << "\n}";
        return json.str();
    }

private:
    /**
     * @brief 转义JSON字符串中的特殊字符
     */
    static std::string escapeJson(const std::string& str) {
        std::string escaped;
        escaped.reserve(str.size() * 2);

        for (char c : str) {
            switch (c) {
                case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (c < ' ') {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned char>(c));
                        escaped += buf;
                    } else {
                        escaped += c;
                    }
            }
        }
        return escaped;
    }
};

} // namespace PaperCrawler
