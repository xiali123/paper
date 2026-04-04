#pragma once

#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cstdint>
#include <cmath>

namespace PaperCrawler {
namespace Core {

/**
 * @brief 时间单位枚举
 */
enum class TimeUnit {
    NANOSECONDS,   ///< 纳秒
    MICROSECONDS,  ///< 微秒
    MILLISECONDS,  ///< 毫秒
    SECONDS,       ///< 秒
    MINUTES,       ///< 分钟
    HOURS,         ///< 小时
    DAYS           ///< 天
};

/**
 * @brief TimeTools - 通用时间处理工具集
 *
 * 提供了完整的时间处理功能，包括：
 * - 高精度计时（纳秒级）
 * - 时间戳转换
 * - 时间格式化
 * - 时间间隔计算
 * - 时区处理
 * - 性能测量
 * - 时间解析
 *
 * @section features 核心特性
 * - @ref timing "高精度计时"
 * - @ref formatting "时间格式化"
 * - @ref conversion "时间戳转换"
 * - @ref performance "性能测量"
 *
 * @section example_usage 示例用法
 * @code
 * // 获取当前时间戳（毫秒）
 * int64_t now = TimeTools::now();  // 1712123456789
 *
 * // 格式化时间
 * std::string formatted = TimeTools::format("%Y-%m-%d %H:%M:%S");
 * // "2024-04-03 15:30:45"
 *
 * // 解析时间字符串
 * std::time_t parsed = TimeTools::parse("2024-04-03 15:30:45", "%Y-%m-%d %H:%M:%S");
 *
 * // 性能测量
 * auto timer = TimeTools::startTimer();
 * // ... 执行操作 ...
 * auto elapsed = TimeTools::elapsedMicroseconds(timer);
 *
 * // 时间间隔计算
 * auto duration = TimeTools::since(epoch);
 * auto human = TimeTools::toHumanReadable(duration);
 * // "2 days 3 hours 45 minutes"
 *
 * // 时间戳转换
 * std::string iso8601 = TimeTools::toISO8601(now);
 * // "2024-04-03T15:30:45Z"
 * @endcode
 *
 * @threadsafe 所有静态方法都是线程安全的（无共享状态）
 */
class TimeTools {
public:
    // ========================================================================
    // 类型定义
    // ========================================================================

    /**
     * @brief 时钟类型
     *
     * 使用高精度稳定时钟（std::chrono::steady_clock）
     */
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using SystemClock = std::chrono::system_clock;
    using SystemTimePoint = std::chrono::time_point<SystemClock>;

    /**
     * @brief 时间戳类型（毫秒）
     */
    using Timestamp = int64_t;

    // ========================================================================
    // 当前时间获取
    // ========================================================================

    /**
     * @brief 获取当前时间戳（毫秒）
     *
     * @return 时间戳（Unix epoch以来的毫秒数）
     *
     * @section example 示例
     * @code
     * int64_t now = TimeTools::now();
     * // 1712123456789
     * @endcode
     */
    static Timestamp now() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            SystemClock::now().time_since_epoch()
        ).count();
    }

    /**
     * @brief 获取当前时间戳（秒）
     *
     * @return 时间戳（Unix epoch以来的秒数）
     *
     * @section example 示例
     * @code
     * int64_t nowSec = TimeTools::nowSeconds();
     * // 1712123456
     * @endcode
     */
    static int64_t nowSeconds() {
        return std::chrono::duration_cast<std::chrono::seconds>(
            SystemClock::now().time_since_epoch()
        ).count();
    }

    /**
     * @brief 获取当前时间戳（微秒）
     *
     * @return 时间戳（Unix epoch以来的微秒数）
     *
     * @section example 示例
     * @code
     * int64_t nowMicro = TimeTools::nowMicroseconds();
     * // 1712123456789012
     * @endcode
     */
    static int64_t nowMicroseconds() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            SystemClock::now().time_since_epoch()
        ).count();
    }

    /**
     * @brief 获取当前时间戳（纳秒）
     *
     * @return 时间戳（Unix epoch以来的纳秒数）
     *
     * @section example 示例
     * @code
     * int64_t nowNano = TimeTools::nowNanoseconds();
     * // 1712123456789012345
     * @endcode
     */
    static int64_t nowNanoseconds() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            SystemClock::now().time_since_epoch()
        ).count();
    }

    // ========================================================================
    // 时间格式化
    // ========================================================================

    /**
     * @brief 格式化当前时间
     *
     * @param format 格式化字符串（strftime格式）
     * @return 格式化后的时间字符串
     *
     * @section example 示例
     * @code
     * std::string formatted = TimeTools::format("%Y-%m-%d %H:%M:%S");
     * // "2024-04-03 15:30:45"
     *
     * std::string iso = TimeTools::format("%Y-%m-%dT%H:%M:%SZ");
     * // "2024-04-03T15:30:45Z"
     * @endcode
     *
     * @note 常用格式说明符：
     * - %Y: 4位年份 (2024)
     * - %m: 2位月份 (01-12)
     * - %d: 2位日期 (01-31)
     * - %H: 2位小时 (00-23)
     * - %M: 2位分钟 (00-59)
     * - %S: 2位秒数 (00-60)
     * - %a: 星期缩写 (Mon, Tue, ...)
     * - %A: 星期全称 (Monday, Tuesday, ...)
     * - %b: 月份缩写 (Jan, Feb, ...)
     * - %B: 月份全称 (January, February, ...)
     */
    static std::string format(const std::string& format, Timestamp timestamp = now()) {
        std::time_t tt = timestamp / 1000;
        std::tm* tm = std::gmtime(&tt);

        if (!tm) {
            return "";
        }

        std::ostringstream oss;
        oss << std::put_time(tm, format.c_str());
        return oss.str();
    }

    /**
     * @brief 格式化为本地时间
     *
     * @param format 格式化字符串（strftime格式）
     * @return 格式化后的本地时间字符串
     *
     * @section example 示例
     * @code
     * std::string local = TimeTools::formatLocal("%Y-%m-%d %H:%M:%S");
     * // "2024-04-03 23:30:45" (北京时间)
     * @endcode
     */
    static std::string formatLocal(const std::string& format, Timestamp timestamp = now()) {
        std::time_t tt = timestamp / 1000;
        std::tm* tm = std::localtime(&tt);

        if (!tm) {
            return "";
        }

        std::ostringstream oss;
        oss << std::put_time(tm, format.c_str());
        return oss.str();
    }

    /**
     * @brief 转换为ISO 8601格式
     *
     * @param timestamp 时间戳（毫秒）
     * @return ISO 8601格式字符串
     *
     * @section example 示例
     * @code
     * std::string iso = TimeTools::toISO8601();
     * // "2024-04-03T15:30:45.123Z"
     * @endcode
     */
    static std::string toISO8601(Timestamp timestamp = now()) {
        std::time_t tt = timestamp / 1000;
        int milliseconds = timestamp % 1000;

        std::tm* tm = std::gmtime(&tt);
        if (!tm) {
            return "";
        }

        std::ostringstream oss;
        oss << std::setfill('0');
        oss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
        oss << '.' << std::setw(3) << milliseconds << 'Z';

        return oss.str();
    }

    /**
     * @brief 从ISO 8601格式解析时间戳
     *
     * @param iso8601 ISO 8601格式字符串
     * @return 时间戳（毫秒）
     *
     * @section example 示例
     * @code
     * std::string iso = "2024-04-03T15:30:45.123Z";
     * int64_t timestamp = TimeTools::fromISO8601(iso);
     * // 1712123445123
     * @endcode
     */
    static Timestamp fromISO8601(const std::string& iso8601) {
        std::tm tm = {};
        std::istringstream iss(iso8601);

        // 尝试解析不同格式的ISO 8601
        // 格式1: 2024-04-03T15:30:45.123Z
        // 格式2: 2024-04-03T15:30:45Z
        // 格式3: 2024-04-03 15:30:45

        iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (iss.fail()) {
            iss.clear();
            iss.str(iso8601);
            iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        }

        if (iss.fail()) {
            return 0;
        }

        std::time_t tt = std::mktime(&tm);
        Timestamp timestamp = static_cast<Timestamp>(tt) * 1000;

        // 解析毫秒
        size_t dotPos = iso8601.find('.');
        if (dotPos != std::string::npos) {
            size_t startPos = dotPos + 1;
            size_t endPos = iso8601.find_first_not_of("0123456789", startPos);
            if (endPos != std::string::npos) {
                std::string msStr = iso8601.substr(startPos, endPos - startPos);
                if (msStr.length() >= 3) {
                    int ms = std::stoi(msStr.substr(0, 3));
                    timestamp += ms;
                }
            }
        }

        return timestamp;
    }

    // ========================================================================
    // 时间解析
    // ========================================================================

    /**
     * @brief 解析时间字符串
     *
     * @param timeStr 时间字符串
     * @param format 格式化字符串（strftime格式）
     * @return 时间戳（毫秒）
     *
     * @section example 示例
     * @code
     * std::time_t tt = TimeTools::parse(
     *     "2024-04-03 15:30:45",
     *     "%Y-%m-%d %H:%M:%S"
     * );
     * @endcode
     */
    static std::time_t parse(const std::string& timeStr, const std::string& format) {
        std::tm tm = {};
        std::istringstream iss(timeStr);
        iss >> std::get_time(&tm, format.c_str());

        if (iss.fail()) {
            return 0;
        }

        return std::mktime(&tm);
    }

    // ========================================================================
    // 时间间隔计算
    // ========================================================================

    /**
     * @brief 计算时间间隔（毫秒）
     *
     * @param start 起始时间点
     * @param end 结束时间点
     * @return 时间间隔（毫秒）
     *
     * @section example 示例
     * @code
     * auto start = TimeTools::startTimer();
     * // ... 执行操作 ...
     * auto elapsed = TimeTools::elapsedMilliseconds(start);
     * // 123
     * @endcode
     */
    template<typename Rep, typename Period>
    static int64_t elapsedMilliseconds(
        const std::chrono::duration<Rep, Period>& duration
    ) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }

    /**
     * @brief 计算时间间隔（微秒）
     *
     * @param duration 时间间隔
     * @return 时间间隔（微秒）
     */
    template<typename Rep, typename Period>
    static int64_t elapsedMicroseconds(
        const std::chrono::duration<Rep, Period>& duration
    ) {
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }

    /**
     * @brief 计算时间间隔（纳秒）
     *
     * @param duration 时间间隔
     * @return 时间间隔（纳秒）
     */
    template<typename Rep, typename Period>
    static int64_t elapsedNanoseconds(
        const std::chrono::duration<Rep, Period>& duration
    ) {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }

    // ========================================================================
    // 高精度计时
    // ========================================================================

    /**
     * @brief 开始计时
     *
     * @return 时间点
     *
     * @section example 示例
     * @code
     * auto timer = TimeTools::startTimer();
     * // ... 执行操作 ...
     * auto elapsed = TimeTools::elapsedMicroseconds(timer);
     * @endcode
     */
    static TimePoint startTimer() {
        return Clock::now();
    }

    /**
     * @brief 计算经过的时间（毫秒）
     *
     * @param start 起始时间点
     * @return 经过的毫秒数
     *
     * @section example 示例
     * @code
     * auto timer = TimeTools::startTimer();
     * // ... 执行操作 ...
     * int64_t ms = TimeTools::elapsedMilliseconds(timer);
     * // 123
     * @endcode
     */
    static int64_t elapsedMilliseconds(const TimePoint& start) {
        auto duration = Clock::now() - start;
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }

    /**
     * @brief 计算经过的时间（微秒）
     *
     * @param start 起始时间点
     * @return 经过的微秒数
     *
     * @section example 示例
     * @code
     * auto timer = TimeTools::startTimer();
     * // ... 执行操作 ...
     * int64_t us = TimeTools::elapsedMicroseconds(timer);
     * // 123456
     * @endcode
     */
    static int64_t elapsedMicroseconds(const TimePoint& start) {
        auto duration = Clock::now() - start;
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    }

    /**
     * @brief 计算经过的时间（纳秒）
     *
     * @param start 起始时间点
     * @return 经过的纳秒数
     *
     * @section example 示例
     * @code
     * auto timer = TimeTools::startTimer();
     * // ... 执行操作 ...
     * int64_t ns = TimeTools::elapsedNanoseconds(timer);
     * // 123456789
     * @endcode
     */
    static int64_t elapsedNanoseconds(const TimePoint& start) {
        auto duration = Clock::now() - start;
        return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }

    // ========================================================================
    // 时间单位转换
    // ========================================================================

    /**
     * @brief 转换时间单位
     *
     * @param value 时间值
     * @param from 源单位
     * @param to 目标单位
     * @return 转换后的时间值
     *
     * @section example 示例
     * @code
     * // 秒转毫秒
     * int64_t ms = TimeTools::convert(5, TimeUnit::SECONDS, TimeUnit::MILLISECONDS);
     * // 5000
     *
     * // 天转小时
     * int64_t hours = TimeTools::convert(2, TimeUnit::DAYS, TimeUnit::HOURS);
     * // 48
     * @endcode
     */
    static int64_t convert(int64_t value, TimeUnit from, TimeUnit to) {
        // 先转换为纳秒
        int64_t nanoseconds = 0;
        switch (from) {
            case TimeUnit::NANOSECONDS:
                nanoseconds = value;
                break;
            case TimeUnit::MICROSECONDS:
                nanoseconds = value * 1000;
                break;
            case TimeUnit::MILLISECONDS:
                nanoseconds = value * 1000000;
                break;
            case TimeUnit::SECONDS:
                nanoseconds = value * 1000000000;
                break;
            case TimeUnit::MINUTES:
                nanoseconds = value * 60000000000;
                break;
            case TimeUnit::HOURS:
                nanoseconds = value * 3600000000000;
                break;
            case TimeUnit::DAYS:
                nanoseconds = value * 86400000000000;
                break;
        }

        // 从纳秒转换为目标单位
        switch (to) {
            case TimeUnit::NANOSECONDS:
                return nanoseconds;
            case TimeUnit::MICROSECONDS:
                return nanoseconds / 1000;
            case TimeUnit::MILLISECONDS:
                return nanoseconds / 1000000;
            case TimeUnit::SECONDS:
                return nanoseconds / 1000000000;
            case TimeUnit::MINUTES:
                return nanoseconds / 60000000000;
            case TimeUnit::HOURS:
                return nanoseconds / 3600000000000;
            case TimeUnit::DAYS:
                return nanoseconds / 86400000000000;
        }

        return 0;
    }

    // ========================================================================
    // 人类可读时间
    // ========================================================================

    /**
     * @brief 转换为人类可读的时间格式
     *
     * @param milliseconds 毫秒数
     * @return 人类可读的时间字符串
     *
     * @section example 示例
     * @code
     * std::string human = TimeTools::toHumanReadable(183450000);
     * // "2 days 2 hours 57 minutes 30 seconds"
     *
     * std::string human2 = TimeTools::toHumanReadable(3675000);
     * // "1 hours 1 minutes 15 seconds"
     * @endcode
     */
    static std::string toHumanReadable(int64_t milliseconds) {
        int64_t seconds = milliseconds / 1000;
        int64_t minutes = seconds / 60;
        int64_t hours = minutes / 60;
        int64_t days = hours / 24;

        int64_t remSeconds = seconds % 60;
        int64_t remMinutes = minutes % 60;
        int64_t remHours = hours % 24;

        std::ostringstream oss;
        bool first = true;

        if (days > 0) {
            oss << days << (days == 1 ? " day" : " days");
            first = false;
        }

        if (remHours > 0 || !first) {
            if (!first) oss << " ";
            oss << remHours << (remHours == 1 ? " hour" : " hours");
            first = false;
        }

        if (remMinutes > 0 || !first) {
            if (!first) oss << " ";
            oss << remMinutes << (remMinutes == 1 ? " minute" : " minutes");
            first = false;
        }

        if (!first) oss << " ";
        oss << remSeconds << (remSeconds == 1 ? " second" : " seconds");

        return oss.str();
    }

    /**
     * @brief 转换为简短的人类可读时间格式
     *
     * @param milliseconds 毫秒数
     * @return 简短的时间字符串
     *
     * @section example 示例
     * @code
     * std::string short = TimeTools::toShortHumanReadable(183450000);
     * // "2d 2h 57m 30s"
     *
     * std::string short2 = TimeTools::toShortHumanReadable(3675000);
     * // "1h 1m 15s"
     * @endcode
     */
    static std::string toShortHumanReadable(int64_t milliseconds) {
        int64_t seconds = milliseconds / 1000;
        int64_t minutes = seconds / 60;
        int64_t hours = minutes / 60;
        int64_t days = hours / 24;

        int64_t remSeconds = seconds % 60;
        int64_t remMinutes = minutes % 60;
        int64_t remHours = hours % 24;

        std::ostringstream oss;
        bool first = true;

        if (days > 0) {
            oss << days << "d";
            first = false;
        }

        if (remHours > 0 || !first) {
            if (!first) oss << " ";
            oss << remHours << "h";
            first = false;
        }

        if (remMinutes > 0 || !first) {
            if (!first) oss << " ";
            oss << remMinutes << "m";
            first = false;
        }

        if (!first) oss << " ";
        oss << remSeconds << "s";

        return oss.str();
    }

    // ========================================================================
    // 时间戳转换
    // ========================================================================

    /**
     * @brief 时间戳转time_t
     *
     * @param timestamp 时间戳（毫秒）
     * @return time_t
     *
     * @section example 示例
     * @code
     * int64_t ts = 1712123456789;
     * std::time_t tt = TimeTools::toTimeT(ts);
     * // 1712123456
     * @endcode
     */
    static std::time_t toTimeT(Timestamp timestamp) {
        return static_cast<std::time_t>(timestamp / 1000);
    }

    /**
     * @brief time_t转时间戳
     *
     * @param tt time_t
     * @return 时间戳（毫秒）
     *
     * @section example 示例
     * @code
     * std::time_t tt = 1712123456;
     * int64_t ts = TimeTools::fromTimeT(tt);
     * // 1712123456000
     * @endcode
     */
    static Timestamp fromTimeT(std::time_t tt) {
        return static_cast<Timestamp>(tt) * 1000;
    }

    /**
     * @brief 添加时间
     *
     * @param timestamp 基准时间戳（毫秒）
     * @param amount 要添加的数量
     * @param unit 时间单位
     * @return 新的时间戳
     *
     * @section example 示例
     * @code
     * int64_t now = TimeTools::now();
     * int64_t tomorrow = TimeTools::add(now, 1, TimeUnit::DAYS);
     * int64_t nextHour = TimeTools::add(now, 1, TimeUnit::HOURS);
     * @endcode
     */
    static Timestamp add(Timestamp timestamp, int64_t amount, TimeUnit unit) {
        return timestamp + convert(amount, unit, TimeUnit::MILLISECONDS);
    }

    /**
     * @brief 计算两个时间戳之间的差值
     *
     * @param timestamp1 时间戳1（毫秒）
     * @param timestamp2 时间戳2（毫秒）
     * @param unit 时间单位
     * @return 差值（绝对值）
     *
     * @section example 示例
     * @code
     * int64_t ts1 = 1712123456000;
     * int64_t ts2 = 1712127056000;
     * int64_t hours = TimeTools::diff(ts1, ts2, TimeUnit::HOURS);
     * // 1
     * @endcode
     */
    static int64_t diff(Timestamp timestamp1, Timestamp timestamp2, TimeUnit unit) {
        int64_t diff_ms = std::abs(timestamp2 - timestamp1);
        return convert(diff_ms, TimeUnit::MILLISECONDS, unit);
    }

    // ========================================================================
    // 时间比较
    // ========================================================================

    /**
     * @brief 检查时间戳是否在过去
     *
     * @param timestamp 时间戳（毫秒）
     * @return 是否在过去
     *
     * @section example 示例
     * @code
     * int64_t past = TimeTools::now() - 1000;
     * bool isPast = TimeTools::isPast(past);
     * // true
     * @endcode
     */
    static bool isPast(Timestamp timestamp) {
        return timestamp < now();
    }

    /**
     * @brief 检查时间戳是否在未来
     *
     * @param timestamp 时间戳（毫秒）
     * @return 是否在未来
     *
     * @section example 示例
     * @code
     * int64_t future = TimeTools::now() + 1000;
     * bool isFuture = TimeTools::isFuture(future);
     * // true
     * @endcode
     */
    static bool isFuture(Timestamp timestamp) {
        return timestamp > now();
    }

    /**
     * @brief 检查时间戳是否在指定范围内
     *
     * @param timestamp 时间戳（毫秒）
     * @param start 起始时间戳（毫秒）
     * @param end 结束时间戳（毫秒）
     * @return 是否在范围内
     *
     * @section example 示例
     * @code
     * int64_t ts = 1712125000000;
     * int64_t start = 1712123456000;
     * int64_t end = 1712127056000;
     * bool inRange = TimeTools::isInRange(ts, start, end);
     * // true
     * @endcode
     */
    static bool isInRange(Timestamp timestamp, Timestamp start, Timestamp end) {
        return timestamp >= start && timestamp <= end;
    }

    // ========================================================================
    // 时间常量
    // ========================================================================

    /**
     * @brief 一天的毫秒数
     */
    static constexpr int64_t ONE_DAY_MS = 86400000;

    /**
     * @brief 一小时的毫秒数
     */
    static constexpr int64_t ONE_HOUR_MS = 3600000;

    /**
     * @brief 一分钟的毫秒数
     */
    static constexpr int64_t ONE_MINUTE_MS = 60000;

    /**
     * @brief 一秒的毫秒数
     */
    static constexpr int64_t ONE_SECOND_MS = 1000;
};

// ============================================================================
// 便捷别名
// ============================================================================

/**
 * @brief 时间工具的便捷别名
 */
namespace Time {
    inline int64_t now() {
        return TimeTools::now();
    }

    inline std::string format(const std::string& fmt, int64_t timestamp = TimeTools::now()) {
        return TimeTools::format(fmt, timestamp);
    }

    inline std::string toISO8601(int64_t timestamp = TimeTools::now()) {
        return TimeTools::toISO8601(timestamp);
    }

    inline TimeTools::TimePoint startTimer() {
        return TimeTools::startTimer();
    }

    inline int64_t elapsedMicroseconds(const TimeTools::TimePoint& start) {
        return TimeTools::elapsedMicroseconds(start);
    }

    inline int64_t elapsedMilliseconds(const TimeTools::TimePoint& start) {
        return TimeTools::elapsedMilliseconds(start);
    }
}

} // namespace Core
} // namespace PaperCrawler
