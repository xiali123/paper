#pragma once

#include <string>
#include <sstream>
#include <map>

namespace PaperCrawler {
namespace StringUtil {

inline std::string escapeJson(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            case '\b': result += "\\b";  break;
            case '\f': result += "\\f";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
                break;
        }
    }
    return result;
}

inline std::string escapeSql(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '\'': result += "''";    break;
            case '\\': result += "\\\\";  break;
            case '\0': result += "\\0";   break;
            case '\n': result += "\\n";   break;
            case '\r': result += "\\r";   break;
            case '\x1a': result += "\\Z"; break;
            default:   result += c;       break;
        }
    }
    return result;
}

inline std::string cleanDbString(const std::string& str) {
    if (str.empty() || str == "NULL" || str == "null") return "";
    return str;
}

inline std::string buildJsonResponse(int statusCode, bool success,
                                      const std::string& message,
                                      const std::string& data = "") {
    std::ostringstream json;
    json << "{\n  \"statusCode\": " << statusCode << ","
         << "\n  \"success\": " << (success ? "true" : "false") << ","
         << "\n  \"message\": \"" << escapeJson(message) << "\"";
    if (!data.empty()) {
        json << ",\n  \"data\": " << data;
    }
    json << "\n}";
    return json.str();
}

inline std::string buildJsonResponse(bool success, const std::string& message,
                                      const std::string& data = "") {
    return buildJsonResponse(success ? 200 : 400, success, message, data);
}

} // namespace StringUtil
} // namespace PaperCrawler
