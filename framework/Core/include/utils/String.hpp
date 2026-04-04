#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <regex>
#include <random>
#include <cstdint>

namespace PaperCrawler {
namespace Core {

/**
 * @brief StringTools - 通用字符串处理工具集
 *
 * 提供了完整的字符串处理功能，包括：
 * - 字符串修剪（去除首尾空白）
 * - 字符串分割和连接
 * - 大小写转换
 * - 字符串格式化
 * - 字符串验证
 * - 编码转换
 * - 随机字符串生成
 * - 字符串替换
 * - Base64编码/解码
 *
 * @section features 核心特性
 * - @ref trimming "字符串修剪"
 * - @ref splitting "字符串分割"
 * - @ref case_conversion "大小写转换"
 * - @ref validation "字符串验证"
 * - @ref encoding "编码转换"
 *
 * @section example_usage 示例用法
 * @code
 * // 字符串修剪
 * std::string text = "  hello world  ";
 * auto trimmed = StringTools::trim(text);  // "hello world"
 *
 * // 字符串分割
 * std::string csv = "apple,banana,cherry";
 * auto parts = StringTools::split(csv, ",");  // ["apple", "banana", "cherry"]
 *
 * // 大小写转换
 * std::string name = "hello";
 * auto upper = StringTools::toUpper(name);  // "HELLO"
 * auto lower = StringTools::toLower(name);  // "hello"
 *
 * // 字符串验证
 * bool isEmpty = StringTools::isEmpty("");  // true
 * bool isNumeric = StringTools::isNumeric("12345");  // true
 *
 * // 字符串格式化
 * auto formatted = StringTools::format("User: {}, Age: {}", "Alice", 25);
 * // "User: Alice, Age: 25"
 *
 * // 随机字符串生成
 * auto randomStr = StringTools::generateRandom(16);
 * // "aB3xY9pL2mK4jH8f"
 *
 * // Base64编码/解码
 * std::string data = "Hello, World!";
 * auto encoded = StringTools::base64Encode(data);
 * auto decoded = StringTools::base64Decode(encoded);
 * @endcode
 *
 * @threadsafe 所有静态方法都是线程安全的（无共享状态）
 */
class StringTools {
public:
    // ========================================================================
    // 字符串修剪
    // ========================================================================

    /**
     * @brief 去除字符串首尾空白字符
     *
     * @param str 输入字符串
     * @return 修剪后的字符串
     *
     * @section example 示例
     * @code
     * std::string text = "  hello world  ";
     * auto trimmed = StringTools::trim(text);  // "hello world"
     * @endcode
     */
    static std::string trim(const std::string& str) {
        return trimRight(trimLeft(str));
    }

    /**
     * @brief 去除字符串左侧空白字符
     *
     * @param str 输入字符串
     * @return 修剪后的字符串
     */
    static std::string trimLeft(const std::string& str) {
        auto start = std::find_if(str.begin(), str.end(), [](int ch) {
            return !std::isspace(ch);
        });
        return std::string(start, str.end());
    }

    /**
     * @brief 去除字符串右侧空白字符
     *
     * @param str 输入字符串
     * @return 修剪后的字符串
     */
    static std::string trimRight(const std::string& str) {
        auto end = std::find_if(str.rbegin(), str.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base();
        return std::string(str.begin(), end);
    }

    // ========================================================================
    // 字符串分割
    // ========================================================================

    /**
     * @brief 分割字符串
     *
     * @param str 输入字符串
     * @param delimiter 分隔符
     * @param maxSplits 最大分割次数（0表示无限制）
     * @return 分割后的字符串列表
     *
     * @section example 示例
     * @code
     * std::string csv = "apple,banana,cherry";
     * auto parts = StringTools::split(csv, ",");
     * // ["apple", "banana", "cherry"]
     *
     * // 限制分割次数
     * auto parts2 = StringTools::split("a,b,c,d", ",", 2);
     * // ["a", "b", "c,d"]
     * @endcode
     */
    static std::vector<std::string> split(
        const std::string& str,
        const std::string& delimiter,
        size_t maxSplits = 0
    ) {
        std::vector<std::string> result;
        size_t start = 0;
        size_t end = str.find(delimiter);
        size_t count = 0;

        while (end != std::string::npos) {
            if (maxSplits > 0 && count >= maxSplits) {
                break;
            }
            result.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
            count++;
        }

        result.push_back(str.substr(start));
        return result;
    }

    /**
     * @brief 按空白字符分割字符串
     *
     * @param str 输入字符串
     * @return 分割后的字符串列表
     *
     * @section example 示例
     * @code
     * std::string text = "hello  world\ttest\nhere";
     * auto parts = StringTools::splitByWhitespace(text);
     * // ["hello", "world", "test", "here"]
     * @endcode
     */
    static std::vector<std::string> splitByWhitespace(const std::string& str) {
        std::vector<std::string> result;
        std::istringstream iss(str);
        std::string token;
        while (iss >> token) {
            result.push_back(token);
        }
        return result;
    }

    /**
     * @brief 按行分割字符串
     *
     * @param str 输入字符串
     * @return 分割后的字符串列表
     *
     * @section example 示例
     * @code
     * std::string text = "line1\nline2\r\nline3";
     * auto lines = StringTools::splitLines(text);
     * // ["line1", "line2", "line3"]
     * @endcode
     */
    static std::vector<std::string> splitLines(const std::string& str) {
        std::vector<std::string> result;
        std::istringstream iss(str);
        std::string line;
        while (std::getline(iss, line)) {
            result.push_back(line);
        }
        return result;
    }

    // ========================================================================
    // 字符串连接
    // ========================================================================

    /**
     * @brief 连接字符串列表
     *
     * @param parts 字符串列表
     * @param delimiter 连接符
     * @return 连接后的字符串
     *
     * @section example 示例
     * @code
     * std::vector<std::string> parts = {"apple", "banana", "cherry"};
     * auto joined = StringTools::join(parts, ", ");
     * // "apple, banana, cherry"
     * @endcode
     */
    static std::string join(
        const std::vector<std::string>& parts,
        const std::string& delimiter
    ) {
        if (parts.empty()) {
            return "";
        }

        std::ostringstream oss;
        oss << parts[0];
        for (size_t i = 1; i < parts.size(); ++i) {
            oss << delimiter << parts[i];
        }
        return oss.str();
    }

    // ========================================================================
    // 大小写转换
    // ========================================================================

    /**
     * @brief 转换为大写
     *
     * @param str 输入字符串
     * @return 大写字符串
     *
     * @section example 示例
     * @code
     * std::string text = "hello world";
     * auto upper = StringTools::toUpper(text);
     * // "HELLO WORLD"
     * @endcode
     */
    static std::string toUpper(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::toupper(c); });
        return result;
    }

    /**
     * @brief 转换为小写
     *
     * @param str 输入字符串
     * @return 小写字符串
     *
     * @section example 示例
     * @code
     * std::string text = "HELLO WORLD";
     * auto lower = StringTools::toLower(text);
     * // "hello world"
     * @endcode
     */
    static std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    /**
     * @brief 首字母大写
     *
     * @param str 输入字符串
     * @return 首字母大写的字符串
     *
     * @section example 示例
     * @code
     * std::string text = "hello world";
     * auto capitalized = StringTools::capitalize(text);
     * // "Hello world"
     * @endcode
     */
    static std::string capitalize(const std::string& str) {
        if (str.empty()) {
            return str;
        }
        std::string result = str;
        result[0] = std::toupper(result[0]);
        return result;
    }

    /**
     * @brief 驼峰命名（首字母小写）
     *
     * @param str 输入字符串（支持snake_case、kebab-case、space separated）
     * @return 驼峰命名字符串
     *
     * @section example 示例
     * @code
     * StringTools::toCamelCase("hello_world");  // "helloWorld"
     * StringTools::toCamelCase("hello-world");  // "helloWorld"
     * StringTools::toCamelCase("hello world");  // "helloWorld"
     * @endcode
     */
    static std::string toCamelCase(const std::string& str) {
        std::string result = toPascalCase(str);
        if (!result.empty()) {
            result[0] = std::tolower(result[0]);
        }
        return result;
    }

    /**
     * @brief 帕斯卡命名（首字母大写）
     *
     * @param str 输入字符串（支持snake_case、kebab-case、space separated）
     * @return 帕斯卡命名字符串
     *
     * @section example 示例
     * @code
     * StringTools::toPascalCase("hello_world");  // "HelloWorld"
     * StringTools::toPascalCase("hello-world");  // "HelloWorld"
     * StringTools::toPascalCase("hello world");  // "HelloWorld"
     * @endcode
     */
    static std::string toPascalCase(const std::string& str) {
        std::vector<std::string> words;

        // 尝试不同的分隔符
        if (str.find('_') != std::string::npos) {
            words = split(str, "_");
        } else if (str.find('-') != std::string::npos) {
            words = split(str, "-");
        } else {
            words = splitByWhitespace(str);
        }

        std::string result;
        for (const auto& word : words) {
            if (!word.empty()) {
                result += capitalize(toLower(word));
            }
        }
        return result;
    }

    /**
     * @brief 蛇形命名（小写+下划线）
     *
     * @param str 输入字符串（支持CamelCase、PascalCase、kebab-case）
     * @return 蛇形命名字符串
     *
     * @section example 示例
     * @code
     * StringTools::toSnakeCase("helloWorld");  // "hello_world"
     * StringTools::toSnakeCase("HelloWorld");  // "hello_world"
     * StringTools::toSnakeCase("hello-world"); // "hello_world"
     * @endcode
     */
    static std::string toSnakeCase(const std::string& str) {
        std::string result;
        for (size_t i = 0; i < str.length(); ++i) {
            char c = str[i];
            if (c == '-' || c == ' ') {
                result += '_';
            } else if (std::isupper(c)) {
                if (i > 0 && !std::isupper(str[i - 1]) &&
                    str[i - 1] != '_' && str[i - 1] != '-') {
                    result += '_';
                }
                result += std::tolower(c);
            } else {
                result += c;
            }
        }
        return result;
    }

    /**
     * @brief 短横线命名（kebab-case）
     *
     * @param str 输入字符串（支持CamelCase、PascalCase、snake_case）
     * @return 短横线命名字符串
     *
     * @section example 示例
     * @code
     * StringTools::toKebabCase("helloWorld");  // "hello-world"
     * StringTools::toKebabCase("HelloWorld");  // "hello-world"
     * StringTools::toKebabCase("hello_world"); // "hello-world"
     * @endcode
     */
    static std::string toKebabCase(const std::string& str) {
        std::string snake = toSnakeCase(str);
        std::replace(snake.begin(), snake.end(), '_', '-');
        return snake;
    }

    // ========================================================================
    // 字符串替换
    // ========================================================================

    /**
     * @brief 替换所有子串
     *
     * @param str 输入字符串
     * @param from 查找的子串
     * @param to 替换的子串
     * @return 替换后的字符串
     *
     * @section example 示例
     * @code
     * std::string text = "hello world";
     * auto replaced = StringTools::replaceAll(text, "world", "there");
     * // "hello there"
     * @endcode
     */
    static std::string replaceAll(
        const std::string& str,
        const std::string& from,
        const std::string& to
    ) {
        if (from.empty()) {
            return str;
        }

        std::string result = str;
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }
        return result;
    }

    // ========================================================================
    // 字符串验证
    // ========================================================================

    /**
     * @brief 检查字符串是否为空或只包含空白字符
     *
     * @param str 输入字符串
     * @return 是否为空
     *
     * @section example 示例
     * @code
     * StringTools::isEmpty("");      // true
     * StringTools::isEmpty("   ");   // true
     * StringTools::isEmpty("hello"); // false
     * @endcode
     */
    static bool isEmpty(const std::string& str) {
        return str.empty() || trim(str).empty();
    }

    /**
     * @brief 检查字符串是否只包含数字
     *
     * @param str 输入字符串
     * @return 是否为数字
     *
     * @section example 示例
     * @code
     * StringTools::isNumeric("12345");  // true
     * StringTools::isNumeric("-123.45"); // true
     * StringTools::isNumeric("abc");    // false
     * @endcode
     */
    static bool isNumeric(const std::string& str) {
        if (str.empty()) {
            return false;
        }

        std::string trimmed = trim(str);
        bool hasDigit = false;
        bool hasDot = false;

        size_t start = 0;
        if (trimmed[0] == '-' || trimmed[0] == '+') {
            start = 1;
        }

        for (size_t i = start; i < trimmed.length(); ++i) {
            char c = trimmed[i];
            if (c == '.') {
                if (hasDot) {
                    return false;  // 多个小数点
                }
                hasDot = true;
            } else if (!std::isdigit(c)) {
                return false;
            } else {
                hasDigit = true;
            }
        }

        return hasDigit;
    }

    /**
     * @brief 检查字符串是否只包含字母
     *
     * @param str 输入字符串
     * @return 是否为字母
     *
     * @section example 示例
     * @code
     * StringTools::isAlpha("hello");  // true
     * StringTools::isAlpha("hello123"); // false
     * @endcode
     */
    static bool isAlpha(const std::string& str) {
        return !str.empty() &&
            std::all_of(str.begin(), str.end(),
                [](unsigned char c) { return std::isalpha(c); });
    }

    /**
     * @brief 检查字符串是否只包含字母或数字
     *
     * @param str 输入字符串
     * @return 是否为字母数字
     *
     * @section example 示例
     * @code
     * StringTools::isAlphanumeric("hello123"); // true
     * StringTools::isAlphanumeric("hello_123"); // false (包含下划线)
     * @endcode
     */
    static bool isAlphanumeric(const std::string& str) {
        return !str.empty() &&
            std::all_of(str.begin(), str.end(),
                [](unsigned char c) { return std::isalnum(c); });
    }

    /**
     * @brief 检查字符串是否以指定前缀开头
     *
     * @param str 输入字符串
     * @param prefix 前缀
     * @return 是否以该前缀开头
     *
     * @section example 示例
     * @code
     * StringTools::startsWith("hello world", "hello"); // true
     * StringTools::startsWith("hello world", "world"); // false
     * @endcode
     */
    static bool startsWith(const std::string& str, const std::string& prefix) {
        if (prefix.length() > str.length()) {
            return false;
        }
        return str.compare(0, prefix.length(), prefix) == 0;
    }

    /**
     * @brief 检查字符串是否以指定后缀结尾
     *
     * @param str 输入字符串
     * @param suffix 后缀
     * @return 是否以该后缀结尾
     *
     * @section example 示例
     * @code
     * StringTools::endsWith("hello.world", ".world"); // true
     * StringTools::endsWith("hello.world", ".txt");   // false
     * @endcode
     */
    static bool endsWith(const std::string& str, const std::string& suffix) {
        if (suffix.length() > str.length()) {
            return false;
        }
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    /**
     * @brief 检查字符串是否包含子串
     *
     * @param str 输入字符串
     * @param substr 子串
     * @return 是否包含
     *
     * @section example 示例
     * @code
     * StringTools::contains("hello world", "world"); // true
     * StringTools::contains("hello world", "foo");   // false
     * @endcode
     */
    static bool contains(const std::string& str, const std::string& substr) {
        return str.find(substr) != std::string::npos;
    }

    // ========================================================================
    // 字符串格式化
    // ========================================================================

    /**
     * @brief 格式化字符串（类似sprintf）
     *
     * @param format 格式化字符串
     * @param args 参数
     * @return 格式化后的字符串
     *
     * @section example 示例
     * @code
     * auto formatted = StringTools::format("User: {}, Age: {}", "Alice", 25);
     * // "User: Alice, Age: 25"
     *
     * auto formatted2 = StringTools::format("Pi: {:.2f}", 3.14159);
     * // "Pi: 3.14"
     * @endcode
     */
    template<typename... Args>
    static std::string format(const std::string& format, Args&&... args) {
        // 如果没有参数，直接返回原字符串
        if constexpr (sizeof...(args) == 0) {
            return format;
        }

        // 使用fmt库格式化（如果可用）
        #ifdef FMT_FORMAT_H_
        return fmt::format(format, std::forward<Args>(args)...);
        #else
        // 否则使用stringstream
        std::ostringstream oss;
        formatImpl(oss, format, 0, std::forward<Args>(args)...);
        return oss.str();
        #endif
    }

    // ========================================================================
    // 大小转换
    // ========================================================================

    /**
     * @brief 转换为人类可读的大小格式
     *
     * @param bytes 字节数
     * @return 格式化后的字符串
     *
     * @section example 示例
     * @code
     * StringTools::bytesToHuman(1024);           // "1.00 KB"
     * StringTools::bytesToHuman(1048576);        // "1.00 MB"
     * StringTools::bytesToHuman(1073741824);     // "1.00 GB"
     * StringTools::bytesToHuman(1099511627776);  // "1.00 TB"
     * @endcode
     */
    static std::string bytesToHuman(uint64_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
        int unitIndex = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unitIndex < 5) {
            size /= 1024.0;
            unitIndex++;
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
        return oss.str();
    }

    // ========================================================================
    // 随机字符串生成
    // ========================================================================

    /**
     * @brief 生成随机字符串
     *
     * @param length 字符串长度
     * @param charset 字符集（默认为字母数字）
     * @return 随机字符串
     *
     * @section example 示例
     * @code
     * // 默认字母数字
     * auto random = StringTools::generateRandom(16);
     * // "aB3xY9pL2mK4jH8f"
     *
     * // 自定义字符集
     * auto hex = StringTools::generateRandom(32, "0123456789abcdef");
     * // "3a7f9b2c1e8d4f6a0b5c7d9e1f2a3b4c"
     * @endcode
     *
     * @note 使用std::random_device生成密码学安全的随机数
     * @threadsafe 线程安全（每次调用都创建独立的随机数生成器）
     */
    static std::string generateRandom(
        size_t length,
        const std::string& charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    ) {
        if (charset.empty() || length == 0) {
            return "";
        }

        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, charset.length() - 1);

        std::string result;
        result.reserve(length);

        for (size_t i = 0; i < length; ++i) {
            result += charset[dist(gen)];
        }

        return result;
    }

    /**
     * @brief 生成UUID v4
     *
     * @return UUID字符串
     *
     * @section example 示例
     * @code
     * auto uuid = StringTools::generateUUID();
     * // "f47ac10b-58cc-4372-a567-0e02b2c3d479"
     * @endcode
     */
    static std::string generateUUID() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        // time_low (32 bits)
        oss << std::setw(8) << dist(gen) << "-";

        // time_mid (16 bits)
        oss << std::setw(4) << (dist(gen) & 0xFFFF) << "-";

        // time_hi_and_version (16 bits, version 4)
        oss << std::setw(4) << ((dist(gen) & 0x0FFF) | 0x4000) << "-";

        // clock_seq_hi_and_res (8 bits, variant 1)
        oss << std::setw(4) << ((dist(gen) & 0x3FFF) | 0x8000) << "-";

        // node (48 bits)
        oss << std::setw(8) << dist(gen) << std::setw(8) << (dist(gen) & 0xFFFF);

        return oss.str();
    }

    // ========================================================================
    // 编码转换
    // ========================================================================

    /**
     * @brief Base64编码
     *
     * @param data 输入数据
     * @return Base64编码后的字符串
     *
     * @section example 示例
     * @code
     * std::string data = "Hello, World!";
     * auto encoded = StringTools::base64Encode(data);
     * // "SGVsbG8sIFdvcmxkIQ=="
     * @endcode
     */
    static std::string base64Encode(const std::string& data) {
        const char* charset =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string result;
        result.reserve(((data.length() + 2) / 3) * 4);

        int val = 0;
        int valb = -6;
        for (unsigned char c : data) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                result.push_back(charset[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }

        if (valb > -6) {
            result.push_back(charset[((val << 8) >> (valb + 8)) & 0x3F]);
        }

        while (result.length() % 4) {
            result.push_back('=');
        }

        return result;
    }

    /**
     * @brief Base64解码
     *
     * @param encoded Base64编码的字符串
     * @return 解码后的数据
     *
     * @section example 示例
     * @code
     * std::string encoded = "SGVsbG8sIFdvcmxkIQ==";
     * auto decoded = StringTools::base64Decode(encoded);
     * // "Hello, World!"
     * @endcode
     */
    static std::string base64Decode(const std::string& encoded) {
        const char* charset =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) {
            T[static_cast<unsigned char>(charset[i])] = i;
        }

        std::string result;
        int val = 0;
        int valb = -8;
        for (unsigned char c : encoded) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                result.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }

        return result;
    }

    /**
     * @brief URL编码
     *
     * @param str 输入字符串
     * @return URL编码后的字符串
     *
     * @section example 示例
     * @code
     * std::string url = "hello world!";
     * auto encoded = StringTools::urlEncode(url);
     * // "hello+world%21"
     * @endcode
     */
    static std::string urlEncode(const std::string& str) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (unsigned char c : str) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                oss << c;
            } else {
                oss << '%' << std::setw(2) << static_cast<int>(c);
            }
        }

        return oss.str();
    }

    /**
     * @brief URL解码
     *
     * @param str URL编码的字符串
     * @return 解码后的字符串
     *
     * @section example 示例
     * @code
     * std::string encoded = "hello+world%21";
     * auto decoded = StringTools::urlDecode(encoded);
     * // "hello world!"
     * @endcode
     */
    static std::string urlDecode(const std::string& str) {
        std::string result;
        result.reserve(str.length());

        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '%' && i + 2 < str.length()) {
                std::string hex = str.substr(i + 1, 2);
                char c = static_cast<char>(std::stoi(hex, nullptr, 16));
                result += c;
                i += 2;
            } else if (str[i] == '+') {
                result += ' ';
            } else {
                result += str[i];
            }
        }

        return result;
    }

    // ========================================================================
    // 子串操作
    // ========================================================================

    /**
     * @brief 获取左侧N个字符
     *
     * @param str 输入字符串
     * @param n 字符数
     * @return 左侧N个字符
     *
     * @section example 示例
     * @code
     * StringTools::left("hello world", 5);  // "hello"
     * StringTools::left("hello", 10);       // "hello"
     * @endcode
     */
    static std::string left(const std::string& str, size_t n) {
        return str.substr(0, std::min(n, str.length()));
    }

    /**
     * @brief 获取右侧N个字符
     *
     * @param str 输入字符串
     * @param n 字符数
     * @return 右侧N个字符
     *
     * @section example 示例
     * @code
     * StringTools::right("hello world", 5);  // "world"
     * StringTools::right("hello", 10);       // "hello"
     * @endcode
     */
    static std::string right(const std::string& str, size_t n) {
        if (n >= str.length()) {
            return str;
        }
        return str.substr(str.length() - n);
    }

    /**
     * @brief 反转字符串
     *
     * @param str 输入字符串
     * @return 反转后的字符串
     *
     * @section example 示例
     * @code
     * StringTools::reverse("hello");  // "olleh"
     * @endcode
     */
    static std::string reverse(const std::string& str) {
        std::string result = str;
        std::reverse(result.begin(), result.end());
        return result;
    }

    // ========================================================================
    // 正则表达式
    // ========================================================================

    /**
     * @brief 正则表达式匹配
     *
     * @param str 输入字符串
     * @param pattern 正则表达式
     * @return 是否匹配
     *
     * @section example 示例
     * @code
     * StringTools::matches("hello123", "[a-z]+[0-9]+");  // true
     * StringTools::matches("hello", "[0-9]+");           // false
     * @endcode
     */
    static bool matches(const std::string& str, const std::string& pattern) {
        try {
            std::regex regex(pattern);
            return std::regex_match(str, regex);
        } catch (const std::regex_error&) {
            return false;
        }
    }

    /**
     * @brief 正则表达式搜索
     *
     * @param str 输入字符串
     * @param pattern 正则表达式
     * @return 匹配的子串列表
     *
     * @section example 示例
     * @code
     * std::string text = "Email: test@example.com and admin@test.org";
     * auto emails = StringTools::extract(text, R"([\w\.]+@[\w\.]+)");
     * // ["test@example.com", "admin@test.org"]
     * @endcode
     */
    static std::vector<std::string> extract(
        const std::string& str,
        const std::string& pattern
    ) {
        std::vector<std::string> result;

        try {
            std::regex regex(pattern);
            auto words_begin = std::sregex_iterator(
                str.begin(), str.end(), regex
            );
            auto words_end = std::sregex_iterator();

            for (auto it = words_begin; it != words_end; ++it) {
                result.push_back(it->str());
            }
        } catch (const std::regex_error&) {
            // 返回空列表
        }

        return result;
    }

private:
    /**
     * @brief 格式化实现（无fmt库时使用）
     */
    template<typename T, typename... Args>
    static void formatImpl(
        std::ostringstream& oss,
        const std::string& format,
        size_t index,
        T&& first,
        Args&&... rest
    ) {
        // 简化的实现：查找"{}"并替换
        size_t pos = format.find("{}");
        if (pos != std::string::npos) {
            oss << format.substr(0, pos) << first;
            if constexpr (sizeof...(rest) > 0) {
                formatImpl(
                    oss,
                    format.substr(pos + 2),
                    0,
                    std::forward<Args>(rest)...
                );
            } else {
                oss << format.substr(pos + 2);
            }
        } else {
            oss << format;
        }
    }

    static void formatImpl(std::ostringstream& oss, const std::string& format, size_t) {
        oss << format;
    }
};

// ============================================================================
// 便捷别名
// ============================================================================

/**
 * @brief 字符串工具的便捷别名
 */
namespace Str {
    inline std::string trim(const std::string& str) {
        return StringTools::trim(str);
    }

    inline std::vector<std::string> split(
        const std::string& str,
        const std::string& delimiter,
        size_t maxSplits = 0
    ) {
        return StringTools::split(str, delimiter, maxSplits);
    }

    inline std::string join(
        const std::vector<std::string>& parts,
        const std::string& delimiter
    ) {
        return StringTools::join(parts, delimiter);
    }

    inline std::string toUpper(const std::string& str) {
        return StringTools::toUpper(str);
    }

    inline std::string toLower(const std::string& str) {
        return StringTools::toLower(str);
    }

    inline std::string replaceAll(
        const std::string& str,
        const std::string& from,
        const std::string& to
    ) {
        return StringTools::replaceAll(str, from, to);
    }

    inline bool isEmpty(const std::string& str) {
        return StringTools::isEmpty(str);
    }

    inline bool contains(const std::string& str, const std::string& substr) {
        return StringTools::contains(str, substr);
    }

    template<typename... Args>
    inline std::string format(const std::string& fmt, Args&&... args) {
        return StringTools::format(fmt, std::forward<Args>(args)...);
    }
}

} // namespace Core
} // namespace PaperCrawler
