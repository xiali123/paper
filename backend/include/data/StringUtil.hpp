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

// Row helper: safely get a string field from a DB row map with NULL/empty handling
inline std::string getRowStr(const std::map<std::string, std::string>& row,
                              const std::string& field,
                              const std::string& defaultVal = "") {
    auto it = row.find(field);
    if (it == row.end()) return defaultVal;
    return cleanDbString(it->second);
}

// Row helper: safely get an int field from a DB row map
inline int getRowInt(const std::map<std::string, std::string>& row,
                      const std::string& field,
                      int defaultVal = 0) {
    auto it = row.find(field);
    if (it == row.end()) return defaultVal;
    try { return std::stoi(cleanDbString(it->second)); }
    catch (...) { return defaultVal; }
}

// Row helper: safely get a int64_t field from a DB row map
inline int64_t getRowInt64(const std::map<std::string, std::string>& row,
                            const std::string& field,
                            int64_t defaultVal = 0) {
    auto it = row.find(field);
    if (it == row.end()) return defaultVal;
    try { return std::stoll(cleanDbString(it->second)); }
    catch (...) { return defaultVal; }
}

// Row helper: safely get a double field from a DB row map
inline double getRowDouble(const std::map<std::string, std::string>& row,
                            const std::string& field,
                            double defaultVal = 0.0) {
    auto it = row.find(field);
    if (it == row.end()) return defaultVal;
    try { return std::stod(cleanDbString(it->second)); }
    catch (...) { return defaultVal; }
}

} // namespace StringUtil
} // namespace PaperCrawler
